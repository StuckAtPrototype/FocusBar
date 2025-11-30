/**
 * @file piezo.c
 * @brief Piezo buzzer driver implementation
 * 
 * This file implements the piezo buzzer driver using PWM to generate
 * tones on GPIO 22. It provides easy-to-use functions for generating
 * different sounds and melodies.
 * 
 * @author StuckAtPrototype, LLC
 * @version 1.0
 */

#include "piezo.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>

static const char *TAG = "piezo";

// Piezo GPIO configuration
#define PIEZO_GPIO 22
#define PIEZO_LEDC_TIMER LEDC_TIMER_0
#define PIEZO_LEDC_MODE LEDC_LOW_SPEED_MODE
#define PIEZO_LEDC_CHANNEL LEDC_CHANNEL_0

// Note frequencies
#define NOTE_C6 1047
#define NOTE_E6 1319
#define NOTE_G6 1568
#define NOTE_C7 2093
#define NOTE_D7 2349
#define NOTE_E7 2637
#define NOTE_G7 3136
#define NOTE_C8 4186

// Static variables
static bool piezo_initialized = false;
static bool piezo_playing = false;
static TaskHandle_t piezo_task_handle = NULL;

/**
 * @brief Piezo task for timed tone playback
 * 
 * This task handles playing tones for a specific duration.
 */
static void piezo_task(void *pvParameters)
{
    uint32_t duration_ms = (uint32_t)pvParameters;
    
    // Wait for the specified duration
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    
    // Stop the tone
    ledc_set_duty(PIEZO_LEDC_MODE, PIEZO_LEDC_CHANNEL, 0);
    ledc_update_duty(PIEZO_LEDC_MODE, PIEZO_LEDC_CHANNEL);
    
    piezo_playing = false;
    piezo_task_handle = NULL;
    
    vTaskDelete(NULL);
}

int piezo_init(void)
{
    if (piezo_initialized) {
        ESP_LOGW(TAG, "Piezo already initialized");
        return 0;
    }

    // Configure LEDC timer
    ledc_timer_config_t ledc_timer = {
        .speed_mode = PIEZO_LEDC_MODE,
        .timer_num = PIEZO_LEDC_TIMER,
        .duty_resolution = LEDC_TIMER_8_BIT,  // 8-bit resolution (0-255)
        .freq_hz = 1000,  // Default frequency, will be changed per tone
        .clk_cfg = LEDC_AUTO_CLK,
    };
    esp_err_t ret = ledc_timer_config(&ledc_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LEDC timer: %s", esp_err_to_name(ret));
        return -1;
    }

    // Configure LEDC channel
    ledc_channel_config_t ledc_channel = {
        .gpio_num = PIEZO_GPIO,
        .speed_mode = PIEZO_LEDC_MODE,
        .channel = PIEZO_LEDC_CHANNEL,
        .timer_sel = PIEZO_LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .duty = 0,  // Start with 0% duty (silent)
        .hpoint = 0,
    };
    ret = ledc_channel_config(&ledc_channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LEDC channel: %s", esp_err_to_name(ret));
        return -1;
    }

    piezo_initialized = true;
    piezo_playing = false;
    ESP_LOGI(TAG, "Piezo initialized on GPIO %d", PIEZO_GPIO);
    return 0;
}

int piezo_play_tone(uint32_t frequency, uint32_t duration_ms)
{
    if (!piezo_initialized) {
        ESP_LOGE(TAG, "Piezo not initialized");
        return -1;
    }

    // Stop any currently playing tone
    piezo_stop();

    // Update timer frequency
    esp_err_t ret = ledc_set_freq(PIEZO_LEDC_MODE, PIEZO_LEDC_TIMER, frequency);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set frequency: %s", esp_err_to_name(ret));
        return -1;
    }

    // Set duty cycle to 50% for clean tone (128 out of 255)
    ledc_set_duty(PIEZO_LEDC_MODE, PIEZO_LEDC_CHANNEL, 128);
    ledc_update_duty(PIEZO_LEDC_MODE, PIEZO_LEDC_CHANNEL);

    piezo_playing = true;

    // If duration is specified, create a task to stop after duration
    if (duration_ms > 0) {
        // Delete any existing task
        if (piezo_task_handle != NULL) {
            vTaskDelete(piezo_task_handle);
            piezo_task_handle = NULL;
        }

        // Create task to stop after duration
        BaseType_t task_ret = xTaskCreate(piezo_task, "piezo_task", 2048, 
                                         (void*)(uintptr_t)duration_ms, 5, &piezo_task_handle);
        if (task_ret != pdPASS) {
            ESP_LOGE(TAG, "Failed to create piezo task");
            piezo_stop();
            return -1;
        }
    }

    return 0;
}

int piezo_stop(void)
{
    if (!piezo_initialized) {
        return 0;  // Not initialized, nothing to stop
    }

    // Delete the task if it exists
    if (piezo_task_handle != NULL) {
        vTaskDelete(piezo_task_handle);
        piezo_task_handle = NULL;
    }

    // Set duty to 0 to stop the tone
    ledc_set_duty(PIEZO_LEDC_MODE, PIEZO_LEDC_CHANNEL, 0);
    ledc_update_duty(PIEZO_LEDC_MODE, PIEZO_LEDC_CHANNEL);

    piezo_playing = false;
    return 0;
}

bool piezo_is_playing(void)
{
    return piezo_playing;
}

int piezo_play_notification(void)
{
    // Play a pleasant two-tone notification: A4 (440Hz) then A5 (880Hz)
    piezo_play_tone(440, 200);
    vTaskDelay(pdMS_TO_TICKS(250));
    piezo_play_tone(880, 300);
    return 0;
}

int piezo_play_alert(void)
{
    // Play a more urgent alert sound: repeated beeps at safe high frequency
    for (int i = 0; i < 3; i++) {
        piezo_play_tone(2093, 200); // C7 (2093 Hz) - known to be audible
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    return 0;
}

int piezo_play_startup_jingle(void)
{
    ESP_LOGI(TAG, "Playing startup jingle");
    // Reverting to C6-C7 range which was confirmed audible
    uint32_t notes[] = {NOTE_C6, NOTE_E6, NOTE_G6, NOTE_C7}; 
    uint32_t durations[] = {100, 100, 100, 300};
    
    for (int i = 0; i < 4; i++) {
        int ret = piezo_play_tone(notes[i], durations[i]);
        if (ret != 0) {
            ESP_LOGE(TAG, "Failed to play note %d", i);
        }
        vTaskDelay(pdMS_TO_TICKS(durations[i] + 50)); // Wait for note duration + small gap
    }
    
    return 0;
}
