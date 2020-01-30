/*
/ Código fonte da incubadora de CO2 compilado pelo ESP-IDF versão  v4.1-dev-1086-g93a8603c5.
/ O código consiste em três tasks, uma geral chamada de main_task, outra para controle do co2 e outra para o controle da temepratura.
/ 
*/
#include "dht22.h"
#include "liquidCrystalI2C.h"
#include "definitions.h"
#include "wifi_functions.h"

int dimmer_delay_us = 6000;

QueueHandle_t temp_queue;

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    wifi_init();
    xTaskCreatePinnedToCore(main_task, "main_task", 5120, NULL, 0, NULL, 1); //Cria a task no APP CPU
}


void main_task(void *pvParameters)
{
    begin_display();
    config_pins();
    set_timer();
    begin_display();
    static int loop_counter = 0;
    for(;;)
    {
        loop_counter ++;
        static char msg1[9];
        static char msg2[16];
        dht_data data = get_temp_humity();
        float co2_lev = get_co2_level();

        printf("co2: %.1f temp: %.1f hum: %.1f\n", co2_lev, data.temperature, data.humity);
        
        sprintf(msg1, "CO2: %.1f", co2_lev);
        sprintf(msg2, "T: %.1f H: %.1f", data.temperature, data.humity);
        uint8_t msg1_size = (sizeof(msg1)/sizeof(char));
        uint8_t msg2_size = (sizeof(msg2)/sizeof(char));
        D_set_cursor(0, 0);
        D_write_str(&msg1, msg1_size);
        vTaskDelay(pdMS_TO_TICKS(500));
        D_set_cursor(0, 1);
        D_write_str(&msg2, msg2_size);
        vTaskDelay(pdMS_TO_TICKS(500));
        //D_clear();
        
        if(loop_counter > 120)
        {
            send_data(data.temperature, data.humity, co2_lev);
            loop_counter = 0;
        }
    }
}


float get_co2_level()
{
    int val = 0;
    float voltage;
    float co2_conc;

    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(CO2_ANALOG_PIN, ADC_ATTEN_DB_6); //Atenuação de 6dB. Leituras de 0 até 2.2V; Equivale a 0 - 4096.
    for(int i = 0; i < 15; i++)
    {
        val += adc1_get_raw(CO2_ANALOG_PIN);
        ets_delay_us(1);
    }
    
    val = val/15; //Tira a média de 15 leituras do ADC.
    voltage = ((val * 2.2) / 4095) - 0.130;  //Subtrai um valor empirico, próximo ao erro do ADC.
    //printf("Read voltage is = %f\n", voltage);
    
    co2_conc =  ((voltage - 0.4) * 10) / 1.6;
    return co2_conc;
}


esp_err_t open_co2_valv(void)
{
    return gpio_set_level(VALV_PIN, 1);
}

esp_err_t close_co2_valv(void)
{
    return gpio_set_level(VALV_PIN, 0);
}

esp_err_t fans_on(void)
{
    esp_err_t err1 = gpio_set_level(FAN1, 1);
    esp_err_t err2 = gpio_set_level(FAN2, 1);
    return err1 ? err1 : err2;
}

esp_err_t fans_off(void)
{
    esp_err_t err1 = gpio_set_level(FAN1, 0);
    esp_err_t err2 = gpio_set_level(FAN2, 0);
    return err1 ? err1 : err2;
}

//********************************************CONFIGURA OS GPIOS*******************************************
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