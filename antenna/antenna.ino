/*
Смотрим на каком азимуте замкнут контакт, шлем эту информацию на бункер,
бункер присылает информацию об успешном введении кода, подаем питание на контакт запуска
*/

#include <SPI.h>
#include "RF24.h"
#include <Button.h>
#include <EEPROM.h>

// D2-D4 контакты азимутов
#define azimuth_1_pin  2
#define azimuth_2_pin  3
#define azimuth_3_pin  4

// D5 контакт электроподжига
#define fire_pin 5

// D6 светодиод
#define led 6

// D7 кнопка настройки
#define button_pin 7

// D9-D13 заняты радиомодулем
# define CLI 9
# define CLO 10

RF24 radio(9, 10);  // "создать" модуль на пинах 9 и 10
Button button(button_pin);

byte mode = 0; // флаг режимов
long last_tick; // отсчет веремени мигания
int target_azimuth; // азимут цели
int blinking_amount; // обратный счтечик для мигания настроек 
bool led_state = false;
bool released = false; // убеждаемся что кнопка отжата

byte address[][6] = {"1Node", "2Node"}; //возможные номера труб
bool payload = true;

void setup() {
  Serial.begin(9600);

  pinMode(led, OUTPUT);
  pinMode(fire_pin, OUTPUT);
  pinMode(azimuth_1_pin, INPUT_PULLUP);
  pinMode(azimuth_2_pin, INPUT_PULLUP);
  pinMode(azimuth_3_pin, INPUT_PULLUP);
  button.begin();

  target_azimuth = EEPROM.read(1); // считываем азимут цели

  if (digitalRead(button_pin)) { // вход в режим настройки
    mode = 1;
    digitalWrite(led, 1);
    Serial.println("settings");
    last_tick = millis();
  }
  else {
    mode = 0;
    while (!radio.begin()) {// ждем инициализацию радио
      Serial.println("radio hardware is not responding!!");
      delay(100);
    }
    radio.setPALevel (RF24_PA_LOW); // уровень мощности передатчика. На выбор RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
    radio.setPayloadSize(sizeof(payload)); // размер пакета, в байтах 4
    radio.openReadingPipe(1, address[0]);   // хотим слушать трубу 0
    radio.startListening();  // слушаем радиоэфир, мы приемник

  }

}

void loop() {

  switch (mode) { // главное меню - переключение между состояниями
    case 0: standby(); break;
    case 1: settings(); break;
  }
}

void standby() { // режим работы

}

void settings() { // режим настройки цели

  if (button.released() & released){ // изменение количества попаданий
    target_azimuth ++;
    if (target_azimuth > 3){
      target_azimuth = 1;
    }
    EEPROM.write(1, target_azimuth);
    last_tick = millis();
    blinking_amount = target_azimuth;
    led_state = false;
    Serial.println(target_azimuth);
  }

  if (blinking_amount > 0){ // мигание показывающее количество необходимых попаданий
    digitalWrite(led, led_state);
    if (millis() - last_tick > 400) {
      last_tick = millis();
      if (!led_state) {
        led_state = true;
      }
      else {
        led_state = false;
        blinking_amount --;
      } 
    }
  }
  else {
    led_state = false;
    if (millis() - last_tick > 1000){
      last_tick = millis();
      blinking_amount = target_azimuth;
      released = true;
    }
  }
}


