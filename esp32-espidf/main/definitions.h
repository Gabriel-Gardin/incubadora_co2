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
#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "freertos/event_groups.h"
#include "esp_intr_alloc.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "tcpip_adapter.h"
#include "lwip/sockets.h"
#include "esp_http_client.h"


//WIFI*********************************************************************************************
/* The event group allows multiple bits for each event, but we only care about one event 
 * - are we connected to the AP with an IP? */
const int WIFI_CONNECTED_BIT = BIT0;

static const char *TAG = "wifi station";
//SSID DO WIFI
#define WIFI_SSID "Gardin"
#define WIFI_PASSWD "gardin12a"

//Número de vezes que tentará reconectar
#define RECONECT_MAX_RETRY 1000000

//PINOS
#define INT_PIN 32
#define D1_PIN 33
#define VALV_PIN 26
#define FAN1 14
#define FAN2 12
#define SCL 22
#define SDA 21
#define TEMP_DATA 19
#define CO2_ANALOG_PIN ADC_CHANNEL_6

#define GPIO_OUTPUT_PIN_SEL ((1ULL<<VALV_PIN) | (1ULL<<FAN1)| (1ULL<<FAN2)| (1ULL<<D1_PIN))
#define GPIO_INPUT_PIN_SEL ((1ULL<<TEMP_DATA) | (1ULL<<INT_PIN))
#define ESP_INTR_FLAG_DEFAULT 0

#define TIMER_GROUP TIMER_GROUP_0
#define TIMER TIMER_0

#define end_point "https://api.thingspeak.com/update?api_key=MMFBRWP4DEAR7FVR"
static int s_retry_num = 0;

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

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

//Lê e retorna a concentração de CO2 em % pela porta analógica do arduino.
float get_co2_level();


//Conecta ao wifi.
void wifi_init(void);

//Envia os dados para o Thinspeak. < https://thingspeak.com/channels/731711 >
void send_data(float temp, float humity, float co2);

//Wifi event handler.
static void event_handler(void* arg, esp_event_base_t event_base, 
                                int32_t event_id, void* event_data);

#endif