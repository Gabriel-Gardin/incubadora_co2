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
#include "math.h"

IRAM_ATTR int dimmer_delay_us = 3000; //Faixa de tempo de 0 até 7500 uS;

float var_temp = 0;

TaskHandle_t temp_task_handle;
TaskHandle_t co2_task_handle;
QueueHandle_t temp_queue;
QueueHandle_t co2_queue;

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
    xTaskCreatePinnedToCore(main_task, "main_task", 4096, NULL, 1, NULL, 1); //Cria a task no APP CPU
    xTaskCreatePinnedToCore(temp_task, "temp_task", 4096, NULL, 0, temp_task_handle, 1); //Cria a task no APP CPU
    xTaskCreatePinnedToCore(co2_task, "co2_task", 4096, NULL, 0, co2_task_handle, 1); //Cria a task no APP CPU
    temp_queue = xQueueCreate(1, sizeof(float));
    co2_queue = xQueueCreate(1, sizeof(float));
}

//Inicia todos os periféricos e mantém o dispal atualizado.
void main_task(void *pvParameters)
{
    begin_display();
    config_pins();
    set_timer();
    begin_display();
    co2_init();
    
    static int loop_counter = 0;
    float temperatura = 0;
    int temp_counter = 0;

    for(;;)
    {
        loop_counter ++;
        static char msg1[10];
        static char msg2[16];

        dht_data data = get_temp_humity();    
        float co2_lev = get_co2_uart()/10000; //Passa de parte por milhão para %.

        if((data.temperature != 333.333) && (co2_lev != 333.333))
        {
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
            
            xQueueOverwrite(co2_queue, &co2_lev);
            temp_counter ++;
            temperatura += (float)data.temperature;

            if(temp_counter == 3)
            {
                temperatura = temperatura / 3;
                xQueueOverwrite(temp_queue, &temperatura); //Envia os dados para a temp_queue. Evita muitos acessos ao modulo RMT do dht22.
                temperatura = 0;
                temp_counter = 0;
            
            }
            
            if(loop_counter > 40)
            {
                send_data(data.temperature, data.humity, co2_lev, dimmer_delay_us);
                loop_counter = 0;
            }
        }
            
    }
}

//Responsável pelo controle da temperatura.
//Recebe a temperatura por uma queue da main_task.
void temp_task(void *pvParameters)
{
    float erro = 0;
    float derivada = 0;

    float temp0_task = 0;
    float temp1_task = 0;
    
    float temp_set_point = POINT_TEMP;

    int delay_time = 180000; //Milisegundos. 

    bool first_run = 1;

    for(;;)
    {
        printf("derivada %.2f, proportional %.2f, dimmer_delay%i, temp0 %f\n",(derivada), (erro), dimmer_delay_us, temp0_task);
        //printf("dimmer_delay %i\n", dimmer_delay_us);

        BaseType_t ret = xQueueReceive(temp_queue, &temp0_task, pdMS_TO_TICKS(1000));       
        if(ret == pdPASS)
        {
            if(first_run) //Se acabou de ligar pré aquece a estufa.
            {
                if(temp0_task <= 33.0)
                {
                    dimmer_delay_us = 2000;
                    temp1_task = temp0_task;
                //    vTaskDelay(pdMS_TO_TICKS(delay_time/3));
                }
                else
                {
                    dimmer_delay_us = 6100;
                    vTaskDelay(pdMS_TO_TICKS(delay_time * 3));
                    first_run = 0;
                    erro = 0;
                    derivada = 0;
                    temp1_task = temp0_task;
                    var_temp = derivada;
                }
            }

            else if(temp0_task == temp_set_point) //Três condições distintas do PID. t > 0, t < 0 t == 0; 
            {
                int Kd = 700;
                int Kp = 30;

                erro = temp0_task - temp_set_point;
                derivada = temp0_task - temp1_task;
                temp1_task = temp0_task;

                dimmer_delay_us += (derivada * Kd) + (Kp * erro);
                
                var_temp = derivada;

                if(dimmer_delay_us > 7000){dimmer_delay_us = 7000;}
                else if(dimmer_delay_us < 3000){dimmer_delay_us = 3000;}

                vTaskDelay(pdMS_TO_TICKS(delay_time)); 
            }

            else if(temp0_task > temp_set_point)
            {
                int Kd = 800; 
                int Kp = 70;

                erro = temp0_task - temp_set_point;
                derivada = temp0_task - temp1_task;
                temp1_task = temp0_task;

                dimmer_delay_us += (derivada * Kd) + (Kp * erro);
                
                var_temp = derivada;

                if(dimmer_delay_us > 7000){dimmer_delay_us = 7000;}
                else if(dimmer_delay_us < 3000){dimmer_delay_us = 3000;}

                vTaskDelay(pdMS_TO_TICKS(delay_time)); 
            }

            else if(temp0_task < temp_set_point)
            {
                int Kd = 550;
                int Kp = 50;

                erro = temp0_task - temp_set_point;
                derivada = temp0_task - temp1_task;
                temp1_task = temp0_task;

                dimmer_delay_us += (derivada * Kd) + (Kp * erro);
                
                var_temp = derivada;

                if(dimmer_delay_us > 7000){dimmer_delay_us = 7000;}
                else if(dimmer_delay_us < 3000){dimmer_delay_us = 3000;}

                vTaskDelay(pdMS_TO_TICKS(delay_time)); 
            }  
        }
    }
}

void co2_task(void *pvParameters)
{
    int co2_constant = CO2_CONST;
    float configured_co2 = CO2_LEVEL;
    float delta_co2 = 0;
    for(;;)
    {
        float co2_level = 0;
        BaseType_t ret = xQueueReceive(co2_queue, &co2_level, pdMS_TO_TICKS(1000));
        if(ret == pdPASS)
        {            
            delta_co2 = co2_level - configured_co2;

        if(delta_co2 > -0.02 && delta_co2 < 0.02)
        {
            vTaskDelay(pdMS_TO_TICKS(2000));
        }

        else if(delta_co2 > 0.03)
        {
            fans_on();
            vTaskDelay(pdMS_TO_TICKS(1500));
            fans_off();
        }

        else if(delta_co2 < -0.02)
        {
            int open_valv_time = abs(delta_co2 * co2_constant);
            open_co2_valv();
            vTaskDelay(pdMS_TO_TICKS(open_valv_time));
            close_co2_valv();
            fans_on();
            vTaskDelay(pdMS_TO_TICKS(400));
            fans_off();
            vTaskDelay(1000);
            }
        }
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