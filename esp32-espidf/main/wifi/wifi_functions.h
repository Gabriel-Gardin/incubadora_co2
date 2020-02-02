#ifndef WIFI_FUNCTIONS_H
#define WIFI_FUNCTIONS_H

#include "tcpip_adapter.h"
#include "lwip/sockets.h"
#include "esp_http_client.h"
#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "freertos/event_groups.h"
#include "esp_log.h"

//SSID DO WIFI
#define WIFI_SSID "Gardin"
#define WIFI_PASSWD "gardin12a"

//Número de vezes que tentará reconectar
#define RECONECT_MAX_RETRY 1000000

#define end_point "https://api.thingspeak.com/update?api_key=MMFBRWP4DEAR7FVR"

//Conecta ao wifi.
void wifi_init(void);

//Envia os dados para o Thinspeak. < https://thingspeak.com/channels/731711 >
void send_data(float temp, float humity, float co2);

//Wifi event handler.
static void event_handler(void* arg, esp_event_base_t event_base, 
                                int32_t event_id, void* event_data);

#endif