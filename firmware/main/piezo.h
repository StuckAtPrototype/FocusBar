/**
 * @file piezo.h
 * @brief Piezo buzzer driver header
 * 
 * This header file defines the interface for controlling a piezo buzzer
 * on GPIO 22. It provides functions for generating tones and melodies.
 * 
 * @author StuckAtPrototype, LLC
 * @version 1.0
 */

#ifndef PIEZO_H
#define PIEZO_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize piezo buzzer driver
 * 
 * This function initializes the PWM peripheral for controlling the piezo
 * buzzer on GPIO 22. Call this function once during system initialization.
 * 
 * @return 0 on success, negative error code on failure
 */
int piezo_init(void);

/**
 * @brief Play a tone on the piezo buzzer
 * 
 * This function plays a tone at the specified frequency for the given duration.
 * If duration is 0, the tone will play continuously until stopped.
 * 
 * @param frequency Frequency in Hz (e.g., 440 for A4, 880 for A5)
 * @param duration_ms Duration in milliseconds (0 = continuous)
 * @return 0 on success, negative error code on failure
 */
int piezo_play_tone(uint32_t frequency, uint32_t duration_ms);

/**
 * @brief Stop the piezo buzzer
 * 
 * This function immediately stops any currently playing tone.
 * 
 * @return 0 on success, negative error code on failure
 */
int piezo_stop(void);

/**
 * @brief Check if piezo is currently playing
 * 
 * @return true if piezo is playing, false otherwise
 */
bool piezo_is_playing(void);

/**
 * @brief Play a notification sound (timer completion)
 * 
 * Plays a pleasant notification sound suitable for timer completion.
 * 
 * @return 0 on success, negative error code on failure
 */
int piezo_play_notification(void);

/**
 * @brief Play an alert sound (timer missed)
 * 
 * Plays an alert sound suitable for when timer was not reset in time.
 * 
 * @return 0 on success, negative error code on failure
 */
int piezo_play_alert(void);

/**
 * @brief Play a startup jingle
 * 
 * Plays a short, pleasant sequence of notes to indicate device startup.
 * 
 * @return 0 on success, negative error code on failure
 */
int piezo_play_startup_jingle(void);

#ifdef __cplusplus
}
#endif

#endif // PIEZO_H
