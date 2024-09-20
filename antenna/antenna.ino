/*
Смотрим на каком азимуте замкнут контакт, шлем эту информацию на бункер,
бункер присылает информацию об успешном введении кода, подаем питание на контакт запуска
*/

#include <SPI.h>
#include "RF24.h"

// D2-D4 контакты азимутов
#define azimuth_1  2
#define azimuth_2  3
#define azimuth_3  4

// D5 контакт электроподжига
#define fire_pin 5

// D6 светодиод
#define led 6

// D9-D13 заняты радиомодулем
# define CLI 9
# define CLO 10

RF24 radio(9, 10);  // "создать" модуль на пинах 9 и 10

byte address[][6] = {"1Node", "2Node"}; //возможные номера труб
int pin_condition[3];
int message;

void setup() {
  Serial.begin(9600);

  pinMode(led, OUTPUT);
  pinMode(fire_pin, OUTPUT);
  pinMode(azimuth_1, INPUT_PULLUP);
  pinMode(azimuth_2, INPUT_PULLUP);
  pinMode(azimuth_3, INPUT_PULLUP);

  while (!radio.begin()) {// ждем инициализацию радио
    Serial.println("radio hardware is not responding!!");
    delay(50);
    digitalWrite(led, 0);
    delay(50);
    digitalWrite(led, 1);
  }
  radio.setPALevel (RF24_PA_LOW); // уровень мощности передатчика. На выбор RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
  radio.setPayloadSize(sizeof(pin_condition)); // размер пакета, в байтах 4
  radio.openReadingPipe(1, address[0]);   // хотим слушать трубу 0
  radio.startListening();  // слушаем радиоэфир, мы приемник

}

void loop() {
  byte pipeNo;
  while (radio.available(&pipeNo)) {        // слушаем эфир со всех труб
    radio.read(&message, sizeof(message));  // чиатем входящий сигнал

    if (message == 1) { // если получили 1, отправляем состояние пинов азимутов
      digitalWrite(led, 0);
      bool pin_condition[3] = {digitalRead(azimuth_1), digitalRead(azimuth_2), digitalRead(azimuth_3)};
      // переходим в режим вещания
      radio.write(&pin_condition, sizeof(pin_condition));
      // слушаем
      digitalWrite(led, 1);
      message = 0;
    }
    else if (message == 2) { // если получили 2, запускаем ракеты
      digitalWrite(fire_pin, 1);
      delay(2000);
      digitalWrite(fire_pin, 0);
      message = 0;
    }
  }


}
