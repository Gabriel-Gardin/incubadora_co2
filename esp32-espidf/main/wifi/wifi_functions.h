#ifndef WIFI_FUNCTIONS_H
#define WIFI_FUNCTIONS_H

#include "tcpip_adapter.h"
#include "lwip/sockets.h"
#include "esp_http_client.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_wpa2.h"
#include "esp_netif.h"
#include "freertos/event_groups.h"
#include "esp_log.h"


//SSID DO WIFI
#define USERNAME "cesar.tischer"
#define PASSWORD "68Eu7490"
#define SSID "wifiuel"

//Número de vezes que tentará reconectar
#define RECONECT_MAX_RETRY 10000000

#define end_point "https://api.thingspeak.com/update?api_key=MMFBRWP4DEAR7FVR"

//Conecta ao wifi.
void wifi_init(void);

//Envia os dados para o Thinspeak. < https://thingspeak.com/channels/731711 >
void send_data(float temp, float humity, float co2, int delay_us);

//Wifi event handler.
static void event_handler(void* arg, esp_event_base_t event_base, 
                                int32_t event_id, void* event_data);

#endif