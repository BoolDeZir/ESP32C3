#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>               // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306Wire.h"        // legacy: #include "SSD1306.h"

int led1 = 12;
int led2 = 13;
// GPIO where the DS18B20 is connected to
const int oneWireBus = 8;
 // Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);
// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

SSD1306Wire display(0x3c, 4, 5);

float tempSensor1, tempSensor2;

uint8_t sensor1[8] = { 0x28, 0xEE, 0x48, 0x67, 0x25, 0x16, 0x02, 0xBE };
uint8_t sensor2[8] = { 0x28, 0xEE, 0x15, 0x57, 0x28, 0x16, 0x01, 0x7D};

void setup() {
 Serial.begin(115200);
 pinMode(led1,OUTPUT);
 pinMode(led2,OUTPUT);
 sensors.begin();
 
 display.init();
 display.flipScreenVertically();
 display.setFont(ArialMT_Plain_10);
}

void loop() {
  sensors.requestTemperatures();
  digitalWrite(led1,HIGH);digitalWrite(led2,LOW);
  delay(1000);
  tempSensor1 = sensors.getTempC(sensor1); // Получить значение температуры
  Serial.print("Температур сенсор 1: ");
  Serial.print(tempSensor1);
  Serial.println("");
  tempSensor2 = sensors.getTempC(sensor2); // Получить значение температуры
  Serial.print("Температур сенсор 2: ");
  Serial.print(tempSensor2);
  Serial.println(""); 
  digitalWrite(led1,LOW);digitalWrite(led2,HIGH);
  delay(1000);

}