#include "liquidCrystalI2C.h"

/*

 *   - B7: data bit 3
 *   - B6: data bit 2
 *   - B5: data bit 1
 *   - B4: data bit 0
 *   - B3: backlight (BL): off = 0, on = 1
 *   - B2: enable (EN): change from 1 to 0 to clock data into controller
 *   - B1: read/write (RW): write = 0, read = 1
 *   - B0: register select (RS): command = 0, data = 1
*/

i2c_port_t i2c_port = I2C_NUM_0;

void D_write_str(char * data, int str_length)
{
    for(int i = 0; i < (str_length - 1); i++)
    {
        display_write(data[i], FLAG_RS_DATA);
    }
}

void D_set_cursor(uint8_t col, uint8_t row)
{
    int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
	if (row > 2)
    {
		row = 1;    // we count rows starting w/0
	}
	display_write((LCD_SETDDRAMADDR | (col + row_offsets[row])), FLAG_RS_COMMAND);
}

void D_clear(void)
{
    uint8_t command = COMMAND_CLEAR_DISPLAY;
    uint8_t flag = FLAG_RS_COMMAND;
    display_write(command, flag);
    ets_delay_us(DELAY_CLEAR_DISPLAY);
}

esp_err_t D_go_home(void)
{
    display_write(COMMAND_RETURN_HOME, FLAG_RS_COMMAND);
    ets_delay_us(DELAY_RETURN_HOME);
    return ESP_OK;
}

void display_write(uint8_t value, uint8_t mode)
{
    uint8_t highnib = value & 0xf0;
    uint8_t lownib = (( value & 0x0f) << 4 );
    write_4bits((highnib) | mode);
    write_4bits((lownib) | mode);
}

void write_4bits(uint8_t value)
{
    i2c_write(value);
    pulse_en(value);
}

void pulse_en(uint8_t data)
{
    uint8_t command = data | En;
    i2c_write(command);
    ets_delay_us(DELAY_ENABLE_PULSE_WIDTH);
    command = data & ~En;
    i2c_write(command);
    ets_delay_us(DELAY_ENABLE_PULSE_SETTLE);
}

void i2c_write(uint8_t value)
{
    uint8_t msg = value | FLAG_BACKLIGHT_ON;
    i2c_master_write_slave(i2c_port, &msg, sizeof(char)); 
}

esp_err_t begin_display(void)
{
    ets_delay_us(4800); //Garantir que o esp32 esteja energizado...
    esp_err_t ERR;

    ERR = i2c_master_init();
    uint8_t command = 0x03 << 4;
    uint8_t command2 = 0x02 << 4;
    uint8_t command3 = COMMAND_FUNCTION_SET | FLAG_FUNCTION_SET_MODE_4BIT | FLAG_FUNCTION_SET_LINES_2 | FLAG_FUNCTION_SET_DOTS_5X8;
    uint8_t command4 = COMMAND_DISPLAY_CONTROL | 0x04 | 0x00 | 0x00 | 0x04;


    uint8_t command5 = COMMAND_ENTRY_MODE_SET | FLAG_ENTRY_MODE_SET_ENTRY_INCREMENT;

    if(ERR == ESP_OK)
    {
        //FIGURA 24 PAGINA 46 DATASHEET HITACHI HD4478 inicialização.......
        ets_delay_us(20000);
        write_4bits(command);
        ets_delay_us(4500);
        write_4bits(command);
        ets_delay_us(4500);
        write_4bits(command);
        ets_delay_us(150);
        write_4bits(command2); //Inicia o disply para funcionar no modo de 4 bits...
        ets_delay_us(120);
        display_write(command3, FLAG_RS_COMMAND);
        display_write(command4, FLAG_RS_COMMAND);
        D_clear();
        display_write(command5, FLAG_RS_COMMAND);
        D_go_home();
  }
    return ERR;
}

esp_err_t i2c_master_write_slave(i2c_port_t i2c_num, uint8_t *data_wr, size_t size)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DISP_ADDR << 1) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write(cmd, data_wr, size, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

esp_err_t i2c_master_init(void)
{
    int i2c_master_port = i2c_port;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = 100000; //100KHz
    i2c_param_config(i2c_master_port, &conf);
    return i2c_driver_install(i2c_master_port, conf.mode,
                              I2C_MASTER_RX_BUF_DISABLE,
                              I2C_MASTER_TX_BUF_DISABLE, 0);
}