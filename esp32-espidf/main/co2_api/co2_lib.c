#include "co2_lib.h"

//Private
static void init_uart();
static char checksum(char *packet);

//Private
static void init_uart()
{
    const uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}


//Private

static char checksum(char *packet)
{
    char check = 0;
    for(int i = 1; i < 8; i++)
    {
        check += packet[i];
    }
    check = 0xff - check;
    check += 1;
    return check;
}

esp_err_t co2_init()
{
    init_uart();
    return ESP_OK;
}

//Public
float get_co2_uart()
{
    char rx_buffer[RX_BUF_SIZE] = {0};
    char command[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
    const int txBytes = uart_write_bytes(UART_NUM_1, command, 9);
    const int rxBytes = uart_read_bytes(UART_NUM_1, &rx_buffer, RX_BUF_SIZE, 1000 / portTICK_RATE_MS);
    if(checksum(&rx_buffer) == rx_buffer[8])
    {
        float co2value=(rx_buffer[2])*256 + (rx_buffer[3]);
        return co2value;
    }
    else
    {
        return 0;
    }
}

//Public
//Só chamar esta função após deixar o sensor por pelo menos 20 minutos em ambiente aberto e limpo!
//A calibragem é feita considerando que a [co2 ambiente] é igual a 400 ppm.
void calibrate_zero()
{
    vTaskDelay(pdMS_TO_TICKS(90000)); //Espera 1 minuto e meio para garantir que o modulo esteja "quente"?
    char rx_buffer[RX_BUF_SIZE] = {0};
    char command[] = {0xFF, 0x01, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78};
    const int txBytes = uart_write_bytes(UART_NUM_1, command, 9);
    printf("Calibrado com sucesso!!\n");
}

//Public
void set_detection_range()
{

}

void off_self_calibration()
{
    char rx_buffer[RX_BUF_SIZE] = {0};
    char command[] = {0xFF, 0x01, 0x79, 0x00, 0x00, 0x00, 0x00, 0x00, 0x86};
    const int txBytes = uart_write_bytes(UART_NUM_1, command, 9);

}

//Não usar. Habilita o modo de auto calibragem do modulo!!!
void on_self_calibration()
{
    char rx_buffer[RX_BUF_SIZE] = {0};
    char command[] = {0xFF, 0x01, 0x79, 0xA0, 0x00, 0x00, 0x00, 0x00, 0xE6};
    const int txBytes = uart_write_bytes(UART_NUM_1, command, 9);
}