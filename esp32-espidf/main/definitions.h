#ifndef DEFINITIONS
#define DEFINITIONS

#include <stdlib.h>
#include <assert.h>
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_now.h"
#include "driver/timer.h"
#include "esp_crc.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_intr_alloc.h"
#include "driver/gpio.h"
#include "driver/adc.h"


//PINOS
#define INT_PIN 32
#define D1_PIN 33
#define VALV_PIN 12
#define FAN1 26
#define FAN2 14
#define SCL 22
#define SDA 21
#define TEMP_DATA 19
#define CO2_ANALOG_PIN ADC_CHANNEL_6

#define GPIO_OUTPUT_PIN_SEL ((1ULL<<VALV_PIN) | (1ULL<<FAN1)| (1ULL<<FAN2)| (1ULL<<D1_PIN))
#define GPIO_INPUT_PIN_SEL ((1ULL<<TEMP_DATA) | (1ULL<<INT_PIN))
#define ESP_INTR_FLAG_DEFAULT 0

#define TIMER_GROUP TIMER_GROUP_0
#define TIMER TIMER_0

#define POINT_TEMP 37.0
#define TEMP_FACTOR 2

#define CO2_LEVEL 5.0
#define CO2_CONST 1700

#define T_BUFFER_SIZE 3
#define DELAY_GOOD_VALUE 6100
#define T_DELAY 30000
//*******************************************************************************************************

timer_config_t config;

//Configura os pinos
void config_pins(void);

//Configura o timer do dimmer.
void set_timer(void);

//Interrupção zero cross. Mantém a sincronia entre o timer do dimmer e a frequência da rede elétrica.
static void IRAM_ATTR zero_cros(void *arg);

//Aciona o GPIO após atingir o intervalo de tempo do timer, acionando o triac do dimmer.
static void IRAM_ATTR dimmer_timer_callback(void* arg);

// task que controla a temperatura.
void temp_task(void *pvParameters);

//Task que monitor o nível de co2
void co2_task(void *pvParameters);

//Task principal
void main_task(void *pvParameters);

//Abre a válvula de CO2 controlando o GPIO 27
esp_err_t open_co2_valv(void);

//Fecha a válvula de CO2 controlando oGPIO 27
esp_err_t close_co2_valv(void);

//Liga as ventoinhas internas. GPIO 12 e 14
esp_err_t fans_on(void);

//Desliga as ventoinhas internas. GPIO 12 e 14
esp_err_t fans_off(void);

#endif