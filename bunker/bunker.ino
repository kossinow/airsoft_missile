/*
Пульт управления ракетами.
При правильном вводе кода активации и правильно установленной ракете начинается таймер.
По истечении тацмера, если ракеты стоят на правильном азимуте происходит их запуск.
Во время обратного отсчета можно ввести код деактивации таймера.
*/

#include <SPI.h>
#include "RF24.h"


// D9-D13 заняты радиомодулем
# define CE 9
# define CSN 10

RF24 radio(CE, CSN);  // "создать" модуль на пинах 9 и 10

byte address[][6] = {"1Node", "2Node"}; //возможные номера труб
int message;
byte mode = 0;
byte second_mode = 0;

int activation_pin = 1234;
int deactivation_pin = 4321;
byte azimuth_settings[3] = {true, false, false}

void setup() {
  Serial.begin(9600);

  while (!radio.begin()) {// ждем инициализацию радио
    Serial.println("radio hardware is not responding!!");
    delay(50);
    delay(50);
  }
  radio.setPALevel (RF24_PA_LOW); // уровень мощности передатчика. На выбор RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
  radio.setPayloadSize(sizeof(pin_condition)); // размер пакета, в байтах 4
  radio.openReadingPipe(1, address[0]);   // хотим слушать трубу 0
  radio.startListening();  // слушаем радиоэфир, мы приемник

}

void loop() {

  switch (mode) { // главное меню - переключение между состояниями
    case 0: standby(); break;
    case 1: setting(); break;
  }
}

void standby() {

  switch(second_mode) {
    case 0: waiting(); break;
    case 1; ticking(); break;
    case 2; exploding(); break;
  }
}

void waiting() { // состояние ожидания ввода кода активации
  // TODO отображение полосок на дисплее
  int entered_pin;// TODO ввод с клавиатуры
  if (entered_pin == activation_pin) { // проверка пина активации
    byte message;// TODO отправить запрос ракетам
    if (message == azimuth_settings) { // проверка азимута ракет
      second_mode = 1;
    }
    else {
      //TODO вывести Азимут на дисплей
      delay(1000);
    }
  }
  else {
    //TODO вывести на экран PASS
    delay(1000);
  }

}

void ticking() { // обратный отсчет

}