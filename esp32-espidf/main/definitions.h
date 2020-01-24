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
#define VALV 27
#define FAN1 14
#define FAN2 12
#define SCL 22
#define SDA 21
#define TEMP_DATA 19

#define GPIO_OUTPUT_PIN_SEL ((1ULL<<VALV) | (1ULL<<FAN1)| (1ULL<<FAN2)| (1ULL<<D1_PIN))
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

//COnfigura o timer.
void set_timer(void);

//INterrupção zero cross
static void IRAM_ATTR zero_cros(void *arg);


static void IRAM_ATTR dimmer_timer_callback(void* arg);

// task que controla a temperatura.
void temp_task(void *pvParameters);

//Task que monitor o nível de co2
void co2_task(void *pvParameters);

//Task principal
void main_task(void *pvParameters);

//Conecta ao wifi.
void wifi_init(void);

void send_data(float temp, float humity, float co2);

static void event_handler(void* arg, esp_event_base_t event_base, 
                                int32_t event_id, void* event_data);

#endif