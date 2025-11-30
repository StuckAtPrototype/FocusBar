idf_component_register(SRCS "main.c"
                                "led.c"
                                "led_color_lib.c"
                                "ws2812_control.c"
                                "button.c"
                                "timer.c"
                                "piezo.c"
                                "serial_protocol.c"
                       PRIV_REQUIRES spi_flash esp_driver_rmt esp_driver_ledc esp_pm esp_driver_gpio esp_driver_uart nvs_flash
                       INCLUDE_DIRS "")
