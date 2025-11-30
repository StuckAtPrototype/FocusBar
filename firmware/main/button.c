/**
 * @file button.c
 * @brief Button control for Pomodoro timer
 * 
 * This file implements button functionality for 5 buttons with short
 * and long press detection. Buttons are on GPIO 10, 5, 4, 14, and 13.
 * 
 * @author StuckAtPrototype, LLC
 * @version 2.0
 */

#include "button.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <stdbool.h>

static const char *TAG = "button";

// Button GPIO configuration
#define BUTTON_GPIO_PINS {BUTTON_SW0_GPIO, BUTTON_SW1_GPIO, BUTTON_SW2_GPIO, BUTTON_SW3_GPIO, BUTTON_SW4_GPIO}
static const uint8_t button_gpios[NUM_BUTTONS] = BUTTON_GPIO_PINS;

// Debounce timing (in milliseconds)
#define DEBOUNCE_MS 50

// Long press threshold (in milliseconds)
#define LONG_PRESS_MS 500

// GPIO interrupt queue
static QueueHandle_t gpio_evt_queue = NULL;

// Button state tracking
typedef struct {
    uint8_t gpio;
    bool pressed;
    TickType_t press_start_time;
    bool event_sent;  // Track if we've already sent an event for this press
} button_state_t;

static button_state_t button_states[NUM_BUTTONS] = {0};

// Event callback
static button_event_callback_t event_callback = NULL;

/**
 * @brief GPIO interrupt handler
 * 
 * This ISR is called when a GPIO interrupt occurs.
 * It sends the GPIO number and level to the queue for processing.
 */
typedef struct {
    uint32_t gpio_num;
    uint32_t level;
} gpio_event_t;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    uint32_t level = gpio_get_level(gpio_num);
    
    gpio_event_t event = {
        .gpio_num = gpio_num,
        .level = level
    };
    
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(gpio_evt_queue, &event, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/**
 * @brief Find button index by GPIO number
 * 
 * @param gpio GPIO number
 * @return Button index (0-4) or -1 if not found
 */
static int find_button_index(uint8_t gpio)
{
    for (int i = 0; i < NUM_BUTTONS; i++) {
        if (button_gpios[i] == gpio) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Button task to handle debouncing and press detection
 * 
 * This task processes button presses with debouncing and detects
 * short vs long presses.
 */
static void button_task(void *pvParameters)
{
    gpio_event_t event;
    TickType_t last_debounce_time[NUM_BUTTONS] = {0};
    
    ESP_LOGI(TAG, "Button task started");
    
    while (1) {
        // Wait for GPIO event from ISR
        if (xQueueReceive(gpio_evt_queue, &event, portMAX_DELAY)) {
            int button_idx = find_button_index(event.gpio_num);
            if (button_idx < 0) {
                continue;  // Unknown GPIO, ignore
            }
            
            TickType_t current_time = xTaskGetTickCount();
            button_state_t *btn = &button_states[button_idx];
            
            // Debounce: only process if enough time has passed since last event
            if (current_time - last_debounce_time[button_idx] > pdMS_TO_TICKS(DEBOUNCE_MS)) {
                last_debounce_time[button_idx] = current_time;
                
                if (event.level == 1) {
                    // Button pressed (rising edge)
                    btn->pressed = true;
                    btn->press_start_time = current_time;
                    btn->event_sent = false;
                    ESP_LOGD(TAG, "Button %d pressed", button_idx);
                } else {
                    // Button released (falling edge)
                    if (btn->pressed) {
                        TickType_t press_duration = current_time - btn->press_start_time;
                        uint32_t press_duration_ms = (press_duration * 1000) / configTICK_RATE_HZ;
                        
                        // Determine if it was a short or long press
                        button_press_type_t press_type = (press_duration_ms >= LONG_PRESS_MS) ? 
                                                          BUTTON_PRESS_LONG : BUTTON_PRESS_SHORT;
                        
                        // Only send event if we haven't already sent a long press event
                        if (!btn->event_sent) {
                            ESP_LOGI(TAG, "Button %d %s press (%lu ms)", 
                                    button_idx, 
                                    press_type == BUTTON_PRESS_LONG ? "long" : "short",
                                    press_duration_ms);
                            
                            if (event_callback != NULL) {
                                event_callback(button_idx, press_type);
                            }
                        }
                        
                        btn->pressed = false;
                        btn->event_sent = false;
                    }
                }
            }
        }
    }
}

/**
 * @brief Check for long presses (called periodically)
 * 
 * This function checks if any button has been held long enough
 * to trigger a long press event.
 */
static void button_long_press_check_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Button long press check task started");
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(50));  // Check every 50ms
        
        TickType_t current_time = xTaskGetTickCount();
        
        for (int i = 0; i < NUM_BUTTONS; i++) {
            button_state_t *btn = &button_states[i];
            
            if (btn->pressed && !btn->event_sent) {
                TickType_t press_duration = current_time - btn->press_start_time;
                uint32_t press_duration_ms = (press_duration * 1000) / configTICK_RATE_HZ;
                
                // If button has been held for long press threshold, send long press event
                if (press_duration_ms >= LONG_PRESS_MS) {
                    ESP_LOGI(TAG, "Button %d long press detected (%lu ms)", i, press_duration_ms);
                    
                    if (event_callback != NULL) {
                        event_callback(i, BUTTON_PRESS_LONG);
                    }
                    
                    btn->event_sent = true;  // Mark that we've sent the long press event
                }
            }
        }
    }
}

/**
 * @brief Initialize button functionality
 * 
 * This function configures all 5 buttons as inputs with pull-down resistors
 * and sets up interrupts for both rising and falling edges.
 */
void button_init(void)
{
    ESP_LOGI(TAG, "Initializing %d buttons", NUM_BUTTONS);
    
    // Create queue for GPIO events
    gpio_evt_queue = xQueueCreate(20, sizeof(gpio_event_t));
    if (gpio_evt_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create GPIO event queue");
        return;
    }
    
    // Initialize button states
    for (int i = 0; i < NUM_BUTTONS; i++) {
        button_states[i].gpio = button_gpios[i];
        button_states[i].pressed = false;
        button_states[i].press_start_time = 0;
        button_states[i].event_sent = false;
    }
    
    // Configure all button GPIOs
    uint64_t pin_bit_mask = 0;
    for (int i = 0; i < NUM_BUTTONS; i++) {
        pin_bit_mask |= (1ULL << button_gpios[i]);
    }
    
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_ANYEDGE,      // Interrupt on both rising and falling edges
        .mode = GPIO_MODE_INPUT,              // Input mode
        .pin_bit_mask = pin_bit_mask,
        .pull_down_en = GPIO_PULLDOWN_ENABLE, // Enable pull-down (button normally LOW)
        .pull_up_en = GPIO_PULLUP_DISABLE,    // Disable pull-up
    };
    
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO: %s", esp_err_to_name(ret));
        return;
    }
    
    // Install GPIO ISR service
    ret = gpio_install_isr_service(0);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        // ESP_ERR_INVALID_STATE means ISR service already installed (OK)
        ESP_LOGE(TAG, "Failed to install GPIO ISR service: %s", esp_err_to_name(ret));
        return;
    }
    
    // Hook ISR handler for each GPIO
    for (int i = 0; i < NUM_BUTTONS; i++) {
        ret = gpio_isr_handler_add(button_gpios[i], gpio_isr_handler, (void*)(uintptr_t)button_gpios[i]);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to add ISR handler for GPIO %d: %s", button_gpios[i], esp_err_to_name(ret));
            return;
        }
        ESP_LOGI(TAG, "Button %d configured on GPIO %d", i, button_gpios[i]);
    }
    
    // Create button task
    BaseType_t task_ret = xTaskCreate(button_task, "button_task", 4096, NULL, 5, NULL);
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create button task");
        return;
    }
    
    // Create long press check task
    task_ret = xTaskCreate(button_long_press_check_task, "button_long_check", 2048, NULL, 5, NULL);
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create button long press check task");
        return;
    }
    
    ESP_LOGI(TAG, "Button system initialized successfully");
}

void button_register_callback(button_event_callback_t callback)
{
    event_callback = callback;
    ESP_LOGI(TAG, "Button event callback registered");
}
