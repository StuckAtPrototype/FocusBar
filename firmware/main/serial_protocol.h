/**
 * @file serial_protocol.h
 * @brief Serial protocol header
 * 
 * This header file defines the interface for serial communication protocol.
 * Functionality to be implemented later.
 * 
 * @author StuckAtPrototype, LLC
 * @version 1.0
 */

#ifndef SERIAL_PROTOCOL_H
#define SERIAL_PROTOCOL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize serial protocol
 * 
 * Initializes the serial communication protocol.
 */
void serial_protocol_init(void);

/**
 * @brief Process incoming serial commands
 * 
 * Processes any incoming commands from the serial interface.
 * Should be called periodically.
 */
void serial_process_commands(void);

/**
 * @brief Send sensor data as JSON over serial
 * 
 * @param ens210_status ENS210 sensor status
 * @param temp_c Temperature in Celsius
 * @param humidity Humidity percentage
 * @param ens16x_status_str ENS16X status string
 * @param etvoc eTVOC value
 * @param eco2 eCO2 value
 * @param aqi AQI value
 */
void serial_send_sensor_data(uint8_t ens210_status, float temp_c, float humidity,
                             const char* ens16x_status_str, int etvoc, int eco2, int aqi);

#ifdef __cplusplus
}
#endif

#endif // SERIAL_PROTOCOL_H

