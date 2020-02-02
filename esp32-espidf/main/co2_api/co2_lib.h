#ifndef CO2_UART_H
#define CO2_UART_H

#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "string.h"

#define TXD_PIN 17
#define RXD_PIN 16
#define RX_BUF_SIZE 128

//Publics...
esp_err_t co2_init();
float get_co2_uart();
void calibrate_zero();
void set_detection_range();
void off_self_calibration();
void on_self_calibration();


#endif