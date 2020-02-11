#include <Arduino.h>
#include "SoftwareSerial.h"





SoftwareSerial co2_serial(10, 11); //Rx, Tx
float get_co2_uart();
char checksum(char *packet);
void calibrate_zero();
void off_self_calibration();



#define RX_BUFFER_LEN 128

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("Estabilizando o sensor. Aguarde 20 segundos");
  delay(20000);
  co2_serial.begin(9600);
  calibrate_zero();
  delay(1000);
  off_self_calibration();
  delay(1000);
  
}

void loop() {
  // put your main code here, to run repeatedly:
  float co2 = get_co2_uart();
  Serial.print("co2 = ");
  Serial.println(co2);
  
  delay(1000);


}


float get_co2_uart()
{
    char rx_buffer[RX_BUFFER_LEN] = {0};
    char command[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};

    co2_serial.write(command, (sizeof(command)/sizeof(char)));
    co2_serial.readBytes(rx_buffer, RX_BUFFER_LEN);

    if(checksum(rx_buffer) == rx_buffer[8])
    {
        float co2value=(rx_buffer[2])*256 + (rx_buffer[3]);
        return co2value;
    }
    else
    {
        Serial.println("Erro. Verifique a conexão!!!");
        return 333.333;
    }
}

void calibrate_zero()
{
    char rx_buffer[RX_BUFFER_LEN] = {0};
    char command[] = {0xFF, 0x01, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78};
    co2_serial.write(command, (sizeof(command) / sizeof(char)));
    Serial.println("Calibrado com sucesso!!\n");
}

void off_self_calibration()
{
    char rx_buffer[RX_BUFFER_LEN] = {0};
    char command[] = {0xFF, 0x01, 0x79, 0x00, 0x00, 0x00, 0x00, 0x00, 0x86};
    co2_serial.write(command, (sizeof(command) / sizeof(char)));
}


char checksum(char *packet)
{
    char check = 0;
    for(int i = 1; i < 8; i++)
    {
        check += packet[i];
    }
    check = 0xff - check;
    check += 1;
    return check;
}
