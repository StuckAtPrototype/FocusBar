/**
 * @file button.h
 * @brief Button control header for Pomodoro timer
 * 
 * This header file defines the interface for button functionality with
 * 5 buttons supporting short and long press detection.
 * 
 * @author StuckAtPrototype, LLC
 * @version 2.0
 */

#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Button GPIO pins
#define BUTTON_SW0_GPIO 10
#define BUTTON_SW1_GPIO 5
#define BUTTON_SW2_GPIO 4
#define BUTTON_SW3_GPIO 14
#define BUTTON_SW4_GPIO 13

// Button indices
#define BUTTON_SW0 0
#define BUTTON_SW1 1
#define BUTTON_SW2 2
#define BUTTON_SW3 3
#define BUTTON_SW4 4
#define NUM_BUTTONS 5

// Button press types
typedef enum {
    BUTTON_PRESS_SHORT = 0,
    BUTTON_PRESS_LONG = 1
} button_press_type_t;

// Button event callback type
typedef void (*button_event_callback_t)(uint8_t button_id, button_press_type_t press_type);

/**
 * @brief Initialize button functionality
 * 
 * This function configures all 5 buttons as inputs with pull-down resistors
 * and sets up interrupts for both rising and falling edges to detect
 * short and long presses.
 */
void button_init(void);

/**
 * @brief Register a callback for button events
 * 
 * @param callback Function to call when a button event occurs
 */
void button_register_callback(button_event_callback_t callback);

#ifdef __cplusplus
}
#endif

#endif // BUTTON_H
