/**
 * @file led.c
 * @brief LED control system implementation for Pomodoro timer
 * 
 * This file implements the LED control system with individual LED control
 * and progress bar functionality using smooth transitions.
 * 
 * @author StuckAtPrototype, LLC
 * @version 5.0
 */

#include "led.h"
#include "esp_log.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "led_color_lib.h"
#include <math.h>

static const char *TAG = "led";

// Mutex to protect LED updates
static SemaphoreHandle_t led_mutex = NULL;

// LED state structure for WS2812 driver
static struct led_state led_state = {0};

// Individual LED colors (GRB format)
static uint32_t led_colors[NUM_LEDS] = {0};

// Global intensity
static float target_intensity = 1.0f;
static float current_intensity = 1.0f;

// Progress bar state
static float target_progress = 0.0f;  // Target progress (0.0 to 1.0)
static float current_progress = 0.0f;  // Current progress (smoothly transitioning)
static uint32_t progress_color = LED_COLOR_GREEN;  // Color for progress bar

// Pulsing/Solid state
static bool pulsing_enabled = false;
static bool solid_mode = false; 
static bool pending_solid_mode = false; // Pending switch to solid mode after windback
static uint32_t pending_solid_color = LED_COLOR_OFF;
static bool pending_start_transition = false; // Pending switch to progress mode after windback
static uint32_t pending_start_color = LED_COLOR_OFF;
static float pending_start_progress = 0.0f;
static uint32_t pulsing_color = LED_COLOR_OFF;
static uint32_t pulse_time_ms = 0;
#define PULSE_MS 4000  // Pulse period 

// Smooth transition speed (same as timer)
#define TRANSITION_SPEED 0.02f
#define INTENSITY_SPEED 0.05f // Slower speed for smooth brightness fades

/**
 * @brief Get pulsing color with intensity
 * 
 * @param red Red component (0-255)
 * @param green Green component (0-255)
 * @param blue Blue component (0-255)
 * @return 24-bit GRB color value with pulsing brightness
 */
static uint32_t get_pulsing_color_with_intensity(uint8_t red, uint8_t green, uint8_t blue) {
    // Calculate the phase of the pulse (0 to 2π) using modulo to wrap around
    float phase = ((pulse_time_ms % PULSE_MS) / (float)PULSE_MS) * 2 * M_PI;

    // Use a sine wave to create a smooth pulse (range: 0 to 1.0 for full brightness)
    float pulse_brightness = (sinf(phase) + 1.0f) / 2.0f;  // Range: 0.0 to 1.0

    // Apply the pulse brightness to the specified color
    float r = pulse_brightness * red;
    float g = pulse_brightness * green;
    float b = pulse_brightness * blue;

    // Convert to GRB format for WS2812 LEDs
    return ((uint32_t)(g + 0.5f) << 16) | ((uint32_t)(r + 0.5f) << 8) | (uint32_t)(b + 0.5f);
}

/**
 * @brief Extract RGB components from GRB color
 * 
 * @param grb_color Color in GRB format
 * @param r Pointer to store red component
 * @param g Pointer to store green component
 * @param b Pointer to store blue component
 */
static void extract_rgb_from_grb(uint32_t grb_color, uint8_t *r, uint8_t *g, uint8_t *b) {
    *g = (grb_color >> 16) & 0xFF;
    *r = (grb_color >> 8) & 0xFF;
    *b = grb_color & 0xFF;
}

/**
 * @brief LED control task
 * 
 * This task continuously updates the WS2812 LEDs with the current state.
 */
static void led_task(void *pvParameters)
{
    while (1) {
        // Take mutex to safely read LED state
        if (led_mutex != NULL && xSemaphoreTake(led_mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
            // Read pulsing/solid state
            bool is_pulsing = pulsing_enabled;
            bool is_solid = solid_mode;
            uint32_t pulse_color = pulsing_color;
            
            // Update progress bar with smooth transition (only if not pulsing and not solid)
            if (!is_pulsing && !is_solid) {
                float progress_diff = target_progress - current_progress;
                current_progress += progress_diff * TRANSITION_SPEED;
                
                // Clamp progress
                if (current_progress < 0.0f) current_progress = 0.0f;
                if (current_progress > 1.0f) current_progress = 1.0f;
                
                // Check if windback is complete and we need to switch to solid mode
                if (pending_solid_mode && current_progress <= 0.01f) {
                    solid_mode = true;
                    // Apply pending color to all LEDs
                    for (int k = 0; k < NUM_LEDS; k++) {
                        led_colors[k] = pending_solid_color;
                    }
                    pending_solid_mode = false;
                    
                    // Force intensity to 0 so it fades in to the target (e.g. 0.3)
                    current_intensity = 0.0f;
                }
                
                // Check if start transition windback is complete
                if (pending_start_transition && current_progress <= 0.01f) {
                    progress_color = pending_start_color;
                    target_progress = pending_start_progress;
                    pending_start_transition = false;
                    // Reset current progress to ensure it starts from 0 for the new color
                    current_progress = 0.0f; 
                }
            } else if (is_pulsing) {
                // When pulsing, ensure progress is at 1.0
                current_progress = 1.0f;
            }
            
            // Increment pulse time once per loop iteration
            if (is_pulsing) {
                pulse_time_ms += 10;
            } else {
                pulse_time_ms = 0; // Reset pulse time when not pulsing
            }
            
            // Smooth intensity transition
            float intensity_diff = target_intensity - current_intensity;
            if (fabs(intensity_diff) > 0.0001f) {
                current_intensity += intensity_diff * INTENSITY_SPEED;
            } else {
                current_intensity = target_intensity;
            }
            
            // Calculate how many LEDs should be on with smooth transitions
            float num_leds_float = current_progress * NUM_LEDS;
            
            // Apply intensity and update LED state
            for (int i = 0; i < NUM_LEDS; i++) {
                uint32_t color = led_colors[i];
                
                if (is_solid) {
                    // Solid mode: Use the set color directly (do not mask with progress)
                    // Just apply intensity below
                } else if (is_pulsing) {
                    // If pulsing is enabled, pulse all LEDs regardless of progress
                    uint8_t r, g, b;
                    extract_rgb_from_grb(pulse_color, &r, &g, &b);
                    color = get_pulsing_color_with_intensity(r, g, b);
                } else {
                    // Progress bar mode
                    // Calculate brightness for this LED (0.0 to 1.0) based on progress
                    float led_brightness = 0.0f;
                    float led_position = (float)(i + 1);  // Position of this LED (1 to NUM_LEDS)
                    
                    if (num_leds_float >= led_position) {
                        // LED is fully on
                        led_brightness = 1.0f;
                    } else if (num_leds_float > (led_position - 1.0f)) {
                        // LED is partially on (smooth fade-in)
                        led_brightness = num_leds_float - (led_position - 1.0f);
                        // Clamp to valid range
                        if (led_brightness < 0.0f) led_brightness = 0.0f;
                        if (led_brightness > 1.0f) led_brightness = 1.0f;
                    } else {
                        // LED is off
                        led_brightness = 0.0f;
                    }
                    
                    // Special case: First LED (i == 0) should turn on to at least 50% immediately when timer starts
                    // Check target_progress to ensure immediate feedback even if current_progress is still 0
                    if (i == 0 && target_progress > 0.0f && led_brightness < 0.5f) {
                        led_brightness = 0.5f;
                    }
                    
                    // Apply brightness to progress color
                    if (led_brightness > 0.0f) {
                        uint8_t r, g, b;
                        extract_rgb_from_grb(progress_color, &r, &g, &b);
                        r = (uint8_t)(r * led_brightness);
                        g = (uint8_t)(g * led_brightness);
                        b = (uint8_t)(b * led_brightness);
                        color = ((uint32_t)g << 16) | ((uint32_t)r << 8) | (uint32_t)b;
                    } else {
                        // LED off
                        color = LED_COLOR_OFF;
                    }
                }
                
                // Apply intensity (Gamma correction: square the intensity for smoother perceived fade)
                float gamma_corrected_intensity = current_intensity * current_intensity;
                color = apply_color_intensity(color, gamma_corrected_intensity);
                
                // Store in LED state
                led_state.leds[i] = color;
            }
            
            xSemaphoreGive(led_mutex);
            
            // Update WS2812 LEDs
            ws2812_write_leds(led_state);
        } else {
            ESP_LOGW(TAG, "Failed to take LED mutex - skipping update");
        }

        // Task delay for 20ms (50Hz update rate) - sufficient for smooth animations
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/**
 * @brief Initialize the LED control system
 */
void led_init(void) {
    // Initialize WS2812 LED driver
    ws2812_control_init();

    // Create mutex for thread-safe LED updates
    led_mutex = xSemaphoreCreateMutex();
    if (led_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create LED mutex - system will be unstable");
    }

    // Initialize LED state
    for (int i = 0; i < NUM_LEDS; i++) {
        led_colors[i] = LED_COLOR_OFF;
        led_state.leds[i] = LED_COLOR_OFF;
    }
    
    target_intensity = 1.0f;
    current_intensity = 1.0f;
    target_progress = 0.0f;
    current_progress = 0.0f;
    progress_color = LED_COLOR_GREEN;
    pulsing_enabled = false;
    pulse_time_ms = 0;

    // Create LED control task
    BaseType_t ret = xTaskCreate(led_task, "led_task", 4096, NULL, 10, NULL);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create LED task");
    }
    
    ESP_LOGI(TAG, "LED system initialized with %d LEDs", NUM_LEDS);
}

void led_set_color(uint32_t color) {
    if (led_mutex != NULL && xSemaphoreTake(led_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        // If we are in progress mode, always trigger the windback/fade-in sequence
        // even if progress is 0, to ensure the intensity fade-in happens.
        if (!solid_mode) {
            // If we were pulsing, capture the pulsing color so the windback matches what the user saw
            if (pulsing_enabled) {
                progress_color = pulsing_color;
            }
            
            target_progress = 0.0f;
            pending_solid_mode = true;
            pending_solid_color = color;
            pulsing_enabled = false;
        } else {
            // Already in solid mode, just update color immediately (no fade reset)
            for (int i = 0; i < NUM_LEDS; i++) {
                led_colors[i] = color;
            }
            solid_mode = true;
            target_progress = 0.0f;
            pulsing_enabled = false;
            pending_solid_mode = false;
        }
        xSemaphoreGive(led_mutex);
    } else if (led_mutex == NULL) {
        // Fallback (no mutex)
        if (!solid_mode) {
            if (pulsing_enabled) {
                progress_color = pulsing_color;
            }
            target_progress = 0.0f;
            pending_solid_mode = true;
            pending_solid_color = color;
            pulsing_enabled = false;
        } else {
            for (int i = 0; i < NUM_LEDS; i++) {
                led_colors[i] = color;
            }
            solid_mode = true;
            target_progress = 0.0f;
            pulsing_enabled = false;
            pending_solid_mode = false;
        }
    }
}

void led_set_led_color(uint8_t led_index, uint32_t color) {
    if (led_index >= NUM_LEDS) {
        return;
    }
    
    if (led_mutex != NULL && xSemaphoreTake(led_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        led_colors[led_index] = color;
        // Enable solid mode
        solid_mode = true;
        target_progress = 0.0f;
        pulsing_enabled = false;
        xSemaphoreGive(led_mutex);
    } else if (led_mutex == NULL) {
        led_colors[led_index] = color;
        solid_mode = true;
        target_progress = 0.0f;
        pulsing_enabled = false;
    }
}

void led_set_intensity(float intensity) {
    // Clamp intensity to valid range
    if (intensity < 0.0f) intensity = 0.0f;
    if (intensity > 1.0f) intensity = 1.0f;
    
    if (led_mutex != NULL && xSemaphoreTake(led_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        target_intensity = intensity;
        xSemaphoreGive(led_mutex);
    } else if (led_mutex == NULL) {
        target_intensity = intensity;
    }
}

void led_set_progress(float progress, uint32_t color) {
    // Clamp progress to valid range
    if (progress < 0.0f) progress = 0.0f;
    if (progress > 1.0f) progress = 1.0f;
    
    if (led_mutex != NULL && xSemaphoreTake(led_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        if (solid_mode) {
            // We are transitioning FROM solid mode TO progress mode.
            // Trigger a windback of the current solid color first.
            solid_mode = false;
            current_progress = 1.0f; // Start windback from full
            progress_color = led_colors[0]; // Use the current solid color
            target_progress = 0.0f; // Target empty
            
            pending_start_transition = true;
            pending_start_color = color; // The new color (Green)
            pending_start_progress = progress; // The initial target
        } else if (pending_start_transition) {
            // Update the pending target so we jump to the correct spot when windback finishes
            pending_start_progress = progress;
        } else {
            // Normal operation
            target_progress = progress;
            progress_color = color;
        }
        
        pulsing_enabled = false;
        pending_solid_mode = false; // Cancel any pending solid switch
        xSemaphoreGive(led_mutex);
    } else if (led_mutex == NULL) {
        if (solid_mode) {
            solid_mode = false;
            current_progress = 1.0f;
            progress_color = led_colors[0];
            target_progress = 0.0f;
            pending_start_transition = true;
            pending_start_color = color;
            pending_start_progress = progress;
        } else if (pending_start_transition) {
            pending_start_progress = progress;
        } else {
            target_progress = progress;
            progress_color = color;
        }
        pulsing_enabled = false;
        pending_solid_mode = false;
    }
}

void led_set_pulsing(uint32_t color, bool enabled) {
    if (led_mutex != NULL && xSemaphoreTake(led_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        pulsing_enabled = enabled;
        pulsing_color = color;
        solid_mode = false;
        if (enabled) {
            // Set progress to full immediately when pulsing (no transition delay)
            target_progress = 1.0f;
            current_progress = 1.0f;  // Immediately set to full so all LEDs pulse
        }
        xSemaphoreGive(led_mutex);
    } else if (led_mutex == NULL) {
        pulsing_enabled = enabled;
        pulsing_color = color;
        solid_mode = false;
        if (enabled) {
            target_progress = 1.0f;
            current_progress = 1.0f;  // Immediately set to full so all LEDs pulse
        }
    }
}

void led_clear_all(void) {
    if (led_mutex != NULL && xSemaphoreTake(led_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        for (int i = 0; i < NUM_LEDS; i++) {
            led_colors[i] = LED_COLOR_OFF;
        }
        solid_mode = true;
        target_progress = 0.0f;
        current_progress = 0.0f;
        pulsing_enabled = false;
        xSemaphoreGive(led_mutex);
    } else if (led_mutex == NULL) {
        for (int i = 0; i < NUM_LEDS; i++) {
            led_colors[i] = LED_COLOR_OFF;
        }
        solid_mode = true;
        target_progress = 0.0f;
        current_progress = 0.0f;
        pulsing_enabled = false;
    }
}

float led_get_intensity(void) {
    float intensity = 0.0f;
    if (led_mutex != NULL && xSemaphoreTake(led_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        intensity = target_intensity;
        xSemaphoreGive(led_mutex);
    } else if (led_mutex == NULL) {
        intensity = target_intensity;
    }
    return intensity;
}
