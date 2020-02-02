/*
/ Código fonte da incubadora de CO2 compilado pelo ESP-IDF versão  v4.1-dev-1086-g93a8603c5.
/ O código consiste em três tasks, uma geral chamada de main_task, outra para controle do co2 e outra para o controle da temepratura.
/ 
*/
#include "dht22/dht22.h"
#include "lcd_display/liquidCrystalI2C.h"
#include "definitions.h"
#include "wifi/wifi_functions.h"
#include "co2_api/co2_lib.h"

int dimmer_delay_us = 5000; //Faixa de tempo de 0 até 8333 uS;

QueueHandle_t temp_queue; 

void app_main(void)
{
    //vTaskDelay(pdMS_TO_TICKS(1000)); //Pausa 10 segundos para iniciar o sensor...
    esp_err_t ret = nvs_flash_init(); //Inicia a memória flash, usado pelo wifi...
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init();
    xTaskCreatePinnedToCore(main_task, "main_task", 4096, NULL, 0, NULL, 1); //Cria a task no APP CPU
    xTaskCreatePinnedToCore(temp_task, "temp_task", 4096, NULL, 0, NULL, 1); //Cria a task no APP CPU
    xTaskCreatePinnedToCore(co2_task, "co2_task", 4096, NULL, 0, NULL, 1); //Cria a task no APP CPU
}

//Inicia todos os periféricos e mantém o dispal atualizado.
void main_task(void *pvParameters)
{
    begin_display();
    config_pins();
    set_timer();
    begin_display();
    co2_init();
    
  //  calibrate_zero();
  //  off_self_calibration();
    
    static int loop_counter = 0;
    for(;;)
    {
        loop_counter ++;
        static char msg1[10];
        static char msg2[16];
        dht_data data = get_temp_humity();
        float co2_lev = get_co2_uart()/10000; //Passa de parte por milhão para %.

        printf("co2: %f temp: %.1f hum: %.1f\n", co2_lev*10000, data.temperature, data.humity);
        sprintf(msg1, "CO2: %.2f", co2_lev);
        sprintf(msg2, "T: %.1f H: %.1f", data.temperature, data.humity);
        uint8_t msg1_size = (sizeof(msg1)/sizeof(char));
        uint8_t msg2_size = (sizeof(msg2)/sizeof(char));
        D_set_cursor(0, 0);
        D_write_str(&msg1, msg1_size);
        vTaskDelay(pdMS_TO_TICKS(1000));
        D_set_cursor(0, 1);
        D_write_str(&msg2, msg2_size);
        vTaskDelay(pdMS_TO_TICKS(1000));
        //D_clear();
        
        if(loop_counter > 20)
        {
            send_data(data.temperature, data.humity, co2_lev);
            loop_counter = 0;
        }
    }
}

void temp_task(void *pvParameters)
{
    for(;;)
    {
        printf("Temp task\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void co2_task(void *pvParameters)
{
    for(;;)
    {
        fans_on();
        printf("co2_task\n");
        vTaskDelay(pdMS_TO_TICKS(600));
        fans_off();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
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