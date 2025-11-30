/**
 * @file timer.h
 * @brief Pomodoro timer state machine header
 * 
 * This header file defines the interface for the Pomodoro timer system
 * with state machine, progress tracking, and button integration.
 * 
 * @author StuckAtPrototype, LLC
 * @version 1.0
 */

#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Timer durations in minutes (mapped to button indices)
#define TIMER_DURATION_5MIN  1
#define TIMER_DURATION_15MIN 15
#define TIMER_DURATION_30MIN 30
#define TIMER_DURATION_45MIN 45
#define TIMER_DURATION_60MIN 60

// Timer states
typedef enum {
    TIMER_STATE_IDLE = 0,
    TIMER_STATE_RUNNING,
    TIMER_STATE_COMPLETED,
    TIMER_STATE_GRACE_PERIOD,
    TIMER_STATE_ALERTING
} timer_state_t;

// Grace period duration (1 minute = 60 seconds)
#define GRACE_PERIOD_SECONDS 60

// Alert duration (1 minute = 60 seconds)
#define ALERT_DURATION_SECONDS 60

/**
 * @brief Initialize the timer system
 */
void timer_init(void);

/**
 * @brief Start timer with specified duration
 * 
 * @param duration_minutes Duration in minutes (5, 15, 30, 45, or 60)
 * @return true on success, false on failure
 */
bool timer_start(uint32_t duration_minutes);

/**
 * @brief Stop and reset the timer
 */
void timer_stop(void);

/**
 * @brief Reset the timer (stop and clear)
 */
void timer_reset(void);

/**
 * @brief Get current timer state
 * 
 * @return Current timer state
 */
timer_state_t timer_get_state(void);

/**
 * @brief Get current timer progress (0.0 to 1.0)
 * 
 * @return Progress value from 0.0 (start) to 1.0 (complete)
 */
float timer_get_progress(void);

/**
 * @brief Get remaining time in seconds
 * 
 * @return Remaining time in seconds, or 0 if not running
 */
uint32_t timer_get_remaining_seconds(void);

/**
 * @brief Update timer (call periodically, e.g., every 100ms)
 * 
 * This function updates the timer state machine and should be called
 * regularly from the main loop.
 */
void timer_update(void);

/**
 * @brief Handle button press event
 * 
 * @param button_id Button ID (0-4)
 * @param is_long_press true if long press, false if short press
 */
void timer_handle_button(uint8_t button_id, bool is_long_press);

#ifdef __cplusplus
}
#endif

#endif // TIMER_H

