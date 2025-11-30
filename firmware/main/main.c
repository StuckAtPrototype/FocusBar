#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_pm.h"
#include "nvs_flash.h"
#include "led.h"
#include "button.h"
#include "timer.h"
#include "piezo.h"

static const char *TAG = "main";
static TaskHandle_t main_task_handle = NULL;

// Button event callback
static void button_event_handler(uint8_t button_id, button_press_type_t press_type)
{
    bool is_long_press = (press_type == BUTTON_PRESS_LONG);
    timer_handle_button(button_id, is_long_press);
    
    // Notify main task to update immediately
    if (main_task_handle != NULL) {
        xTaskNotifyGive(main_task_handle);
    }
}

void app_main(void)
{
    // Capture main task handle first
    main_task_handle = xTaskGetCurrentTaskHandle();

    ESP_LOGI(TAG, "Pomodoro Timer Starting");

    // Configure power management
    esp_pm_config_esp32c2_t pm_config = {
        .max_freq_mhz = 10,
        .min_freq_mhz = 10,
        .light_sleep_enable = false
    };
    
    esp_err_t ret = esp_pm_configure(&pm_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure power management: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Power management configured");
    }

    // Initialize NVS (Non-Volatile Storage)
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS initialized");

    // Initialize LED system
    led_init();
    ESP_LOGI(TAG, "LED system initialized");
    
    // Initialize piezo buzzer
    if (piezo_init() != 0) {
        ESP_LOGE(TAG, "Failed to initialize piezo");
    } else {
        ESP_LOGI(TAG, "Piezo initialized");
        piezo_play_startup_jingle();
    }
    
    // Initialize button system
    button_init();
    button_register_callback(button_event_handler);
    ESP_LOGI(TAG, "Button system initialized");
    
    // Initialize timer system
    timer_init();
    ESP_LOGI(TAG, "Timer system initialized");
    
    // Clear all LEDs initially
    led_clear_all();
    
    ESP_LOGI(TAG, "Pomodoro Timer Ready");

    // Main loop
    timer_state_t last_state = TIMER_STATE_IDLE;
    
    while (1) {
        // Wait for notification or timeout (100ms)
        // This allows immediate response to button events while maintaining periodic updates
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100));
        
        // Update timer
        timer_update();
        
        // Get current timer state
        timer_state_t current_state = timer_get_state();
        // timer_state_t current_state = 4 // for testing
        
        // Handle state changes
        if (current_state != last_state) {
            ESP_LOGI(TAG, "Timer state changed: %d -> %d", last_state, current_state);
            last_state = current_state;
        }
        
        // Handle timer states
        switch (current_state) {
            case TIMER_STATE_IDLE:
                // All LEDs off
                led_clear_all();
                if (piezo_is_playing()) {
                    piezo_stop();
                }
                break;
                
            case TIMER_STATE_RUNNING: {
                // Show progress bar
                float progress = timer_get_progress();
                led_set_progress(progress, LED_COLOR_GREEN);
                if (piezo_is_playing()) {
                    piezo_stop();
                }
                break;
            }
            
            case TIMER_STATE_COMPLETED:
                // Pulse LEDs to notify completion
                led_set_pulsing(LED_COLOR_GREEN, true);
                if (piezo_is_playing()) {
                    piezo_stop();
                }
                break;
                
            case TIMER_STATE_GRACE_PERIOD:
                // Keep pulsing during grace period
                led_set_pulsing(LED_COLOR_GREEN, true);
                if (piezo_is_playing()) {
                    piezo_stop();
                }
                break;
                
            case TIMER_STATE_ALERTING: {
                // Pulse LEDs
                led_set_pulsing(LED_COLOR_RED, true);
                
                // Play jingle every 2 seconds
                static TickType_t last_jingle_time = 0;
                TickType_t now = xTaskGetTickCount();
                
                // Play jingle immediately if first time or 2 seconds passed
                if (last_jingle_time == 0 || (now - last_jingle_time > pdMS_TO_TICKS(2000))) {
                    if (!piezo_is_playing()) {
                        piezo_play_startup_jingle();
                        last_jingle_time = xTaskGetTickCount(); // Update time after playing (it blocks)
                    }
                }
                break;
            }
        }
    }
}
