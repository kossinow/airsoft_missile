/*
Принимает запрос от бункера, отправялет в ответ состояние пинов азимутов и активирует пуск ракет.
Отвечает: 0 - никакой азиимут не выставлен, 1-1, 2-2, 3-3
В режиме ожидания обычно принимает false, для подрыва отправить true.
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
# define CE 9
# define CSN 10

RF24 radio(CE, CSN);  // "создать" модуль на пинах 9 и 10

byte pipeNo;
byte address[][6] = {"1Node", "2Node"}; //возможные номера труб
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

  radio.setAutoAck(1);
  radio.setRetries(0, 15);
  radio.enableAckPayload();
  radio.enableDynamicPayloads();
  radio.setPayloadSize(2);
  radio.setChannel(0x60);
  radio.setDataRate(RF24_1MBPS);
  radio.setPALevel (RF24_PA_LOW);
  radio.openReadingPipe(1, address[0]);
  radio.startListening();

}

void loop() {

  if (radio.available(&pipeNo)) { // если что-то пришло на радиоканал
    radio.read(&message, sizeof(message));  // чиатем входящий сигнал
    int report = wich_azimuth();
    radio.writeAckPayload(pipeNo, &report, sizeof(report));

    if (message) { // если получили true - взрываем
      digitalWrite(fire_pin, 1);
      delay(1000);
      digitalWrite(fire_pin, 0);
    }
  }
}

int wich_azimuth(){ // возвращает номер активированного азимута
  if (digitalRead(azimuth_1) == HIGH) {
    return 1;
  }
  else if (digitalRead(azimuth_2) == HIGH) {
    return 2;
  }
  else if (digitalRead(azimuth_3) == HIGH) {
    return 3;
  }
  else {
    return 0;
  }
}
