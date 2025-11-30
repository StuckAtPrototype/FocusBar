/**
 * @file timer.c
 * @brief Pomodoro timer state machine implementation
 * 
 * This file implements the Pomodoro timer with state machine,
 * progress tracking, and integration with buttons, LEDs, and piezo.
 * 
 * @author StuckAtPrototype, LLC
 * @version 1.0
 */

#include "timer.h"
#include "button.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>

static const char *TAG = "timer";

// Timer durations in minutes (mapped to button indices)
static const uint32_t timer_durations[NUM_BUTTONS] = {
    TIMER_DURATION_5MIN,   // Button 0: 5 minutes
    TIMER_DURATION_15MIN, // Button 1: 15 minutes
    TIMER_DURATION_30MIN, // Button 2: 30 minutes
    TIMER_DURATION_45MIN, // Button 3: 45 minutes
    TIMER_DURATION_60MIN  // Button 4: 60 minutes
};

// Timer state
static timer_state_t timer_state = TIMER_STATE_IDLE;
static uint32_t timer_duration_seconds = 0;
static uint32_t timer_start_time_ticks = 0;
static uint32_t grace_period_start_ticks = 0;
static uint32_t alert_start_ticks = 0;
static float current_progress = 0.0f;
static float target_progress = 0.0f;

// Smooth transition speed (same as LED transition algorithm)
#define TRANSITION_SPEED 0.02f

void timer_init(void)
{
    timer_state = TIMER_STATE_IDLE;
    timer_duration_seconds = 0;
    timer_start_time_ticks = 0;
    grace_period_start_ticks = 0;
    alert_start_ticks = 0;
    current_progress = 0.0f;
    target_progress = 0.0f;
    ESP_LOGI(TAG, "Timer system initialized");
}

bool timer_start(uint32_t duration_minutes)
{
    // Validate duration
    bool valid_duration = false;
    for (int i = 0; i < NUM_BUTTONS; i++) {
        if (timer_durations[i] == duration_minutes) {
            valid_duration = true;
            break;
        }
    }
    
    if (!valid_duration) {
        ESP_LOGE(TAG, "Invalid timer duration: %lu minutes", duration_minutes);
        return false;
    }
    
    // Stop any existing timer
    timer_stop();
    
    // Start new timer
    timer_duration_seconds = duration_minutes * 60;
    timer_start_time_ticks = xTaskGetTickCount();
    timer_state = TIMER_STATE_RUNNING;
    current_progress = 0.0f;
    target_progress = 0.0f;
    
    ESP_LOGI(TAG, "Timer started: %lu minutes (%lu seconds)", duration_minutes, timer_duration_seconds);
    return true;
}

void timer_stop(void)
{
    if (timer_state == TIMER_STATE_IDLE) {
        return;
    }
    
    timer_state = TIMER_STATE_IDLE;
    timer_duration_seconds = 0;
    timer_start_time_ticks = 0;
    grace_period_start_ticks = 0;
    alert_start_ticks = 0;
    current_progress = 0.0f;
    target_progress = 0.0f;
    
    ESP_LOGI(TAG, "Timer stopped");
}

void timer_reset(void)
{
    timer_stop();
    ESP_LOGI(TAG, "Timer reset");
}

timer_state_t timer_get_state(void)
{
    return timer_state;
}

float timer_get_progress(void)
{
    return current_progress;
}

uint32_t timer_get_remaining_seconds(void)
{
    if (timer_state != TIMER_STATE_RUNNING) {
        return 0;
    }
    
    TickType_t current_ticks = xTaskGetTickCount();
    TickType_t elapsed_ticks = current_ticks - timer_start_time_ticks;
    uint32_t elapsed_seconds = (elapsed_ticks * 1000) / configTICK_RATE_HZ / 1000;
    
    if (elapsed_seconds >= timer_duration_seconds) {
        return 0;
    }
    
    return timer_duration_seconds - elapsed_seconds;
}

void timer_update(void)
{
    TickType_t current_ticks = xTaskGetTickCount();
    
    switch (timer_state) {
        case TIMER_STATE_IDLE:
            // Nothing to do
            break;
            
        case TIMER_STATE_RUNNING: {
            // Calculate elapsed time in ticks for smooth progress
            TickType_t elapsed_ticks = current_ticks - timer_start_time_ticks;
            uint32_t total_duration_ticks = pdMS_TO_TICKS(timer_duration_seconds * 1000);
            
            // Calculate target progress (0.0 to 1.0)
            if (elapsed_ticks >= total_duration_ticks) {
                target_progress = 1.0f;
                // Timer completed
                timer_state = TIMER_STATE_COMPLETED;
                grace_period_start_ticks = current_ticks;
                ESP_LOGI(TAG, "Timer completed");
            } else {
                // Ensure we have at least a tiny bit of progress to trigger "on" state
                if (elapsed_ticks == 0) elapsed_ticks = 1; 
                target_progress = (float)elapsed_ticks / (float)total_duration_ticks;
            }
            
            // Smoothly transition current_progress towards target_progress
            float progress_diff = target_progress - current_progress;
            current_progress += progress_diff * TRANSITION_SPEED;
            
            // Clamp to valid range
            if (current_progress < 0.0f) current_progress = 0.0f;
            if (current_progress > 1.0f) current_progress = 1.0f;
            break;
        }
        
        case TIMER_STATE_COMPLETED:
            // Keep progress at 1.0
            current_progress = 1.0f;
            target_progress = 1.0f;
            
            // Check if grace period has expired
            TickType_t grace_elapsed_ticks = current_ticks - grace_period_start_ticks;
            uint32_t grace_elapsed_seconds = (grace_elapsed_ticks * 1000) / configTICK_RATE_HZ / 1000;
            
            if (grace_elapsed_seconds >= GRACE_PERIOD_SECONDS) {
                // Grace period expired, start alerting
                timer_state = TIMER_STATE_ALERTING;
                alert_start_ticks = current_ticks;
                ESP_LOGI(TAG, "Grace period expired, starting alert");
            }
            break;
            
        case TIMER_STATE_GRACE_PERIOD:
            // This state is handled by COMPLETED state
            // If we're here, it means button was pressed during grace period
            // and timer should be reset
            break;
            
        case TIMER_STATE_ALERTING: {
            // Check if alert duration has expired
            TickType_t alert_elapsed_ticks = current_ticks - alert_start_ticks;
            uint32_t alert_elapsed_seconds = (alert_elapsed_ticks * 1000) / configTICK_RATE_HZ / 1000;
            
            if (alert_elapsed_seconds >= ALERT_DURATION_SECONDS) {
                // Alert duration expired, stop alerting
                timer_state = TIMER_STATE_IDLE;
                current_progress = 0.0f;
                target_progress = 0.0f;
                ESP_LOGI(TAG, "Alert duration expired, timer idle");
            }
            break;
        }
    }
}

void timer_handle_button(uint8_t button_id, bool is_long_press)
{
    if (button_id >= NUM_BUTTONS) {
        ESP_LOGW(TAG, "Invalid button ID: %d", button_id);
        return;
    }
    
    switch (timer_state) {
        case TIMER_STATE_IDLE:
            // Short press: start timer with button's duration
            if (!is_long_press) {
                uint32_t duration = timer_durations[button_id];
                timer_start(duration);
            }
            // Long press: no action in idle state
            break;
            
        case TIMER_STATE_RUNNING:
            // Short press: stop/reset timer
            if (!is_long_press) {
                timer_stop();
            }
            // Long press: no action while running
            break;
            
        case TIMER_STATE_COMPLETED:
        case TIMER_STATE_GRACE_PERIOD:
            // Any button press during grace period: reset timer (no piezo)
            timer_reset();
            ESP_LOGI(TAG, "Timer reset during grace period (no alert)");
            break;
            
        case TIMER_STATE_ALERTING:
            // Any button press during alerting: stop alert and reset
            timer_reset();
            ESP_LOGI(TAG, "Timer reset during alert");
            break;
    }
}

