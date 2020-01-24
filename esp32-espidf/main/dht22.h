#ifndef DHT22_H
#define DHT22_H

#include "driver/rmt.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"

#define RMT_RX_CHANNEL    0     /*!< RMT channel for receiver */
#define RMT_RX_GPIO_NUM  19     /*!< GPIO number for receiver */
#define RMT_CLK_DIV      80    /*!< RMT counter clock divider */
#define RMT_TICK_10_US    (80000000/RMT_CLK_DIV/100000)   /*!< RMT counter value for 10 us.(Source clock is APB clock) */
#define rmt_item32_tIMEOUT_US  200   /*!< RMT receiver timeout value(us) */

typedef struct
{
    float temperature;
    float humity;
} dht_data;

dht_data get_temp_humity();
static void nec_rx_init(void);
void pulse_line(gpio_num_t gpio);
void set_read(gpio_num_t gpio);
int *get_bits(void);

#endif