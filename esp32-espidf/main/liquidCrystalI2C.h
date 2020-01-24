#ifndef LIQUID_CRYSTAL_H
#define LIQUID_CRYSTAL_H

#include "esp_system.h"
#include "esp_now.h"
#include "esp_crc.h"
#include "driver/i2c.h"

//Definições I2C...
#define I2C_MASTER_TX_BUF_DISABLE 0          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_SCL_IO 22                 /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO 21                 /*!< gpio number for I2C master data  */
#define ACK_CHECK_EN 0x1                     /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0                    /*!< I2C master will not check ack from slave */
#define ACK_VAL 0x0                          /*!< I2C ack value */
#define NACK_VAL 0x1                         /*!< I2C nack value */
#define WRITE_BIT I2C_MASTER_WRITE              /*!< I2C master write */


//Definições do display
#define DISP_ADDR 0X27 // Endereço padrão do display ox27 em decimal...

#define En 0b00000100  // Enable bit
#define Rw 0b00000010  // Read/Write bit

#define FLAG_BACKLIGHT_ON    0b00001000      // backlight enabled (disabled if clear)
#define FLAG_BACKLIGHT_OFF   0b00000000      // backlight disabled
#define FLAG_ENABLE          0b00000100
#define FLAG_READ            0b00000010      // read (write if clear)
#define FLAG_WRITE           0b00000000      // write
#define FLAG_RS_DATA         0b00000001      // data (command if clear)
#define FLAG_RS_COMMAND      0b00000000      // command
#define FLAG_FUNCTION_SET_MODE_4BIT      0x00
#define FLAG_FUNCTION_SET_LINES_2        0x08
#define FLAG_FUNCTION_SET_DOTS_5X10      0x04
#define FLAG_FUNCTION_SET_DOTS_5X8       0x00

// COMMAND_ENTRY_MODE_SET flags
#define FLAG_ENTRY_MODE_SET_ENTRY_INCREMENT       0x02
#define COMMAND_CLEAR_DISPLAY       0x01
#define COMMAND_RETURN_HOME         0x02
#define COMMAND_FUNCTION_SET        0x20
#define COMMAND_DISPLAY_CONTROL     0x08
#define COMMAND_ENTRY_MODE_SET      0x04

#define DELAY_ENABLE_PULSE_WIDTH      1  // enable pulse must be at least 450ns wide
#define DELAY_ENABLE_PULSE_SETTLE    50  // command requires > 37us to settle (table 6 in datasheet)
#define DELAY_CLEAR_DISPLAY        2000
#define DELAY_RETURN_HOME          2000
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_SETDDRAMADDR 0x80

esp_err_t i2c_master_init(void);
esp_err_t i2c_master_write_slave(i2c_port_t i2c_num, uint8_t *data_wr, size_t size);
esp_err_t begin_display(void);
esp_err_t D_go_home(void);
void write_4bits(uint8_t value);
void display_write(uint8_t value, uint8_t mode);
void D_clear(void);
void pulse_en(uint8_t data);
void write_4bits(uint8_t value);
void i2c_write(uint8_t value);
void D_write_str(char * data, int str_length);
void D_set_cursor(uint8_t col, uint8_t row);


#endif