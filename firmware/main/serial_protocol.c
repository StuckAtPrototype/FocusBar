/**
 * @file serial_protocol.c
 * @brief Serial protocol implementation
 * 
 * This file implements the serial communication protocol.
 * Functionality to be implemented later.
 * 
 * @author StuckAtPrototype, LLC
 * @version 1.0
 */

#include "serial_protocol.h"
#include "esp_log.h"
#include "driver/uart.h"
#include <string.h>

static const char *TAG = "serial_protocol";

// UART configuration
#define UART_NUM UART_NUM_0
#define UART_BUF_SIZE 1024

static bool serial_initialized = false;

void serial_protocol_init(void)
{
    if (serial_initialized) {
        ESP_LOGW(TAG, "Serial protocol already initialized");
        return;
    }

    // Configure UART parameters
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // Install UART driver
    esp_err_t ret = uart_driver_install(UART_NUM, UART_BUF_SIZE, 0, 0, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install UART driver: %s", esp_err_to_name(ret));
        return;
    }

    // Configure UART parameters
    ret = uart_param_config(UART_NUM, &uart_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure UART parameters: %s", esp_err_to_name(ret));
        return;
    }

    // Set UART pins (default pins for UART_NUM_0)
    ret = uart_set_pin(UART_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, 
                       UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set UART pins: %s", esp_err_to_name(ret));
        return;
    }

    serial_initialized = true;
    ESP_LOGI(TAG, "Serial protocol initialized");
}

void serial_process_commands(void)
{
    if (!serial_initialized) {
        return;
    }

    // TODO: Implement command processing
    // Read from UART and process commands
}

void serial_send_sensor_data(uint8_t ens210_status, float temp_c, float humidity,
                             const char* ens16x_status_str, int etvoc, int eco2, int aqi)
{
    if (!serial_initialized) {
        return;
    }

    // TODO: Implement JSON serialization and transmission
    // Format: {"ens210":{"status":...,"temp":...,"humidity":...},"ens16x":{"status":...,"etvoc":...,"eco2":...,"aqi":...}}
    
    char json_buffer[512];
    int len = snprintf(json_buffer, sizeof(json_buffer),
        "{\"ens210\":{\"status\":%u,\"temp\":%.2f,\"humidity\":%.2f},"
        "\"ens16x\":{\"status\":\"%s\",\"etvoc\":%d,\"eco2\":%d,\"aqi\":%d}}\n",
        ens210_status, temp_c, humidity, ens16x_status_str, etvoc, eco2, aqi);
    
    if (len > 0 && len < sizeof(json_buffer)) {
        uart_write_bytes(UART_NUM, json_buffer, len);
    }
}

