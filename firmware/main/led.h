/**
 * @file led.h
 * @brief LED control system header for Pomodoro timer
 * 
 * This header file defines the interface for the LED control system.
 * It provides functions for controlling WS2812 addressable LEDs with
 * individual LED control and progress bar functionality.
 * 
 * @author StuckAtPrototype, LLC
 * @version 5.0
 */

#ifndef LED_H
#define LED_H

#include <stdbool.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ws2812_control.h"

// Predefined LED colors in GRB format (not RGB)
#define LED_COLOR_OFF    0x000000  // Black (LEDs off)
#define LED_COLOR_RED    0x00FF00  // Red
#define LED_COLOR_GREEN  0xFF0000  // Green
#define LED_COLOR_BLUE   0x0000FF  // Blue
#define LED_COLOR_YELLOW 0xFFFF00  // Yellow
#define LED_COLOR_CYAN   0xFF00FF  // Cyan (Green + Blue)

// LED system initialization and control functions
/**
 * @brief Initialize the LED control system
 * 
 * Initializes the LED driver, creates the LED task, and sets up the mutex
 * for thread-safe LED operations.
 */
void led_init(void);

/**
 * @brief Set all LEDs to the same color
 * 
 * @param color Color value in GRB format (0x00RRGGBB)
 */
void led_set_color(uint32_t color);

/**
 * @brief Set individual LED color
 * 
 * @param led_index LED index (0-9)
 * @param color Color value in GRB format
 */
void led_set_led_color(uint8_t led_index, uint32_t color);

/**
 * @brief Set LED intensity (brightness)
 * 
 * @param intensity Intensity value (0.0 to 1.0)
 */
void led_set_intensity(float intensity);

/**
 * @brief Set progress bar (1-10 LEDs based on progress)
 * 
 * This function sets the progress bar by lighting up LEDs 1-10
 * based on the progress value (0.0 to 1.0). Uses smooth transitions.
 * 
 * @param progress Progress value from 0.0 (no LEDs) to 1.0 (all 10 LEDs)
 * @param color Color for the progress LEDs
 */
void led_set_progress(float progress, uint32_t color);

/**
 * @brief Set pulsing effect on all LEDs
 * 
 * This function enables a pulsing effect on all LEDs with the specified color.
 * 
 * @param color Base color for pulsing
 * @param enabled true to enable pulsing, false to disable
 */
void led_set_pulsing(uint32_t color, bool enabled);

/**
 * @brief Clear all LEDs (turn off)
 */
void led_clear_all(void);

/**
 * @brief Get current LED intensity
 * 
 * @return Current intensity value (0.0 to 1.0)
 */
float led_get_intensity(void);

#endif // LED_H
