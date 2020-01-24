/*
/ Código fonte da incubadora de CO2 compilado pelo ESP-IDF versão  v4.1-dev-1086-g93a8603c5.
/ O código consiste em três tasks, uma geral chamada de main_task, outra para controle do co2 e outra para o controle da temepratura.
/ 
*/
#include "dht22.h"
#include "liquidCrystalI2C.h"
#include "definitions.h"

int dimmer_delay_us = 6000;

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    
    ESP_ERROR_CHECK(ret);
    wifi_init();

    xTaskCreatePinnedToCore(main_task, "main_task", 4096, NULL, 0, NULL, 1);
    xTaskCreatePinnedToCore(temp_task, "temp_task", 4096, NULL, 0, NULL, 1);
    xTaskCreatePinnedToCore(co2_task, "co2_task", 4096, NULL, 0, NULL, 1);

    config_pins();
    set_timer();
}

void main_task(void *pvParameters)
{
    begin_display();

    for(;;)
    {
        D_set_cursor(0, 0);
        static char msg[] = "daee parca";
        uint8_t msg_size = (sizeof(msg)/sizeof(char));
        D_write_str(&msg, msg_size);
        vTaskDelay(pdMS_TO_TICKS(1000));
        D_set_cursor(0, 1);
        D_write_str(&msg, msg_size);
        vTaskDelay(1000);
        D_clear();
    }
}

void temp_task(void *pvParameters)
{
    unsigned int t0;
    for(;;)
    {
        dht_data data = get_temp_humity();
        t0 = esp_timer_get_time();
        vTaskDelay(pdMS_TO_TICKS(10000));
        send_data(((float)(data.temperature)), ((float)(data.humity)), 35.0f);
        printf("Temp = %f, humity = %f\n", data.temperature, data.humity);
    }
}

void co2_task(void *pvParameters)
{
    unsigned int t0;
    for(;;)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
        printf("time passed3\n");
    }
}


void wifi_init(void)
{
    s_wifi_event_group = xEventGroupCreate();

    esp_netif_init();

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));


    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWD
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");
    ESP_LOGI(TAG, "connect to ap SSID:%s password:%s", WIFI_SSID, WIFI_PASSWD);
}


void config_pins(void)
{
    gpio_config_t io_conf;  //Configura os pinos OUTPUT(ventoinhas e valvula CO2)
    io_conf.intr_type    = GPIO_PIN_INTR_DISABLE;
    io_conf.mode         = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en   = 0;

    gpio_config(&io_conf);

    io_conf.intr_type    = GPIO_PIN_INTR_DISABLE;
    io_conf.mode         = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en   = 0;

    gpio_config(&io_conf);
    gpio_set_pull_mode(INT_PIN, GPIO_PULLUP_ONLY);

    //change gpio intrrupt type for one pin
    gpio_set_intr_type(INT_PIN, GPIO_INTR_NEGEDGE);
    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(INT_PIN, zero_cros, NULL);
}

void send_data(float temp, float humity, float co2)
{

    char url[110];
    sprintf(url, "https://api.thingspeak.com/update?api_key=MMFBRWP4DEAR7FVR&field1=%f&field2=%f&field3=%f", co2, temp, humity);

    esp_http_client_config_t config = {
        .url =  url,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    }
    esp_http_client_cleanup(client);
}



//****************************************DIMMER****************************************
void set_timer(void)
{
    config.alarm_en=TIMER_ALARM_EN;
    config.counter_en=TIMER_PAUSE;
    config.intr_type = TIMER_INTR_LEVEL;
    config.counter_dir = TIMER_COUNT_UP;
    config.auto_reload = true;
    config.divider = 80;

    timer_init(TIMER_GROUP, TIMER, &config);
    timer_set_counter_value(TIMER_GROUP, TIMER, 0x00000000ULL); 
    //timer_set_alarm_value(TIMER_GROUP, TIMER, 6000);
    //timer_enable_intr(TIMER_GROUP, TIMER);
    timer_isr_register(TIMER_GROUP, TIMER, dimmer_timer_callback, (void *) TIMER, ESP_INTR_FLAG_IRAM, NULL );
    timer_start(TIMER_GROUP, TIMER);
}

static void IRAM_ATTR zero_cros(void *arg)
{
    uint64_t counter = timer_group_get_counter_value_in_isr(TIMER_GROUP, TIMER); //Calcula o tempo para a interrupção a partir do valor atual do TIMER...
    uint64_t delta_t = counter + dimmer_delay_us;
    timer_group_set_alarm_value_in_isr(TIMER_GROUP, TIMER, delta_t);       //Seta o alarme 
    timer_group_enable_alarm_in_isr(TIMER_GROUP, TIMER);
}

static void IRAM_ATTR dimmer_timer_callback(void* arg)
{
    timer_group_intr_clr_in_isr(TIMER_GROUP, TIMER);      // Equaivalente a:     .int_clr_timers.t1 = 1;
    GPIO.out1_w1ts.data = (1 << (D1_PIN - 32)); //Coloca o GPIO33 HIGH através dos registradores...
    unsigned long t0 = timer_group_get_counter_value_in_isr(TIMER_GROUP, TIMER);
    while(timer_group_get_counter_value_in_isr(TIMER_GROUP, TIMER) - t0 < 5){}
    GPIO.out1_w1tc.data = (1 << (D1_PIN - 32));  //Coloca o GPIO33 LOW através dos registradores...
}

//**************************************FIM DIMMER **************************************************************




//Wifi event handler.
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < RECONECT_MAX_RETRY) {
            esp_wifi_connect();
            xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}