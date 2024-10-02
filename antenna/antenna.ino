/*
Принимает запрос от бункера, отправялет в ответ состояние пинов азимутов и активирует пуск ракет.
Отвечает: 0 - никакой азиимут не выставлен, 1-1, 2-2, 3-3
В режиме ожидания обычно принимает false, для подрыва отправить true.
*/

#include "SPI.h"
#include "RF24.h"

// D2-D4 контакты азимутов
#define azimuth_1  2
#define azimuth_2  3
#define azimuth_3  4

// D5 контакт электроподжига
#define fire_pin 5

// D6, D7 светодиоды
#define led_red 6
#define led_green 7

// D9-D13 заняты радиомодулем
# define CE 9
# define CSN 10

RF24 radio(CE, CSN);  // "создать" модуль на пинах 9 и 10

byte pipeNo;
byte address[][6] = {"1Node", "2Node"}; //возможные номера труб
int message;
long switch_off;

void setup() {
  Serial.begin(9600);

  pinMode(led_red, OUTPUT);
  pinMode(led_green, OUTPUT);
  pinMode(fire_pin, OUTPUT);
  pinMode(azimuth_1, INPUT_PULLUP);
  pinMode(azimuth_2, INPUT_PULLUP);
  pinMode(azimuth_3, INPUT_PULLUP);

  while (!radio.begin()) {// ждем инициализацию радио
    Serial.println("radio hardware is not responding!");
    delay(50);
    digitalWrite(led_red, 0);
    delay(50);
    digitalWrite(led_red, 1);
  }

  radio.setAutoAck(1);
  radio.setRetries(0, 15);
  radio.enableAckPayload();
  radio.enableDynamicPayloads();
  radio.setPayloadSize(2); // int размером 2 байта
  radio.setChannel(0x60);
  radio.setDataRate(RF24_1MBPS);
  radio.setPALevel (RF24_PA_LOW);
  radio.openReadingPipe(1, address[0]);
  radio.startListening();
  Serial.println("setup done");

}

void loop() {
  int report = wich_azimuth();

  if (radio.available(&pipeNo)) { // если что-то пришло на радиоканал
    radio.read(&message, sizeof(message));  // чиатем входящий сигнал
    Serial.print("Got: ");
    Serial.println(message);
    radio.writeAckPayload(pipeNo, &report, sizeof(report));
    Serial.print("Sent: ");
    Serial.println(report);

    if (message) { // если получили true - взрываем
      digitalWrite(fire_pin, 1);
      switch_off = millis();

    }
  }

  if (millis() - switch_off > 1000) {
    digitalWrite(fire_pin, 0);
  }

  if (wich_azimuth() != 0) { // индикация светодиодами наведения на азимут
    digitalWrite(led_red, 0);
    digitalWrite(led_green, 1);
  }
  else {
    digitalWrite(led_green, 0);
    digitalWrite(led_red, 1);
  }
}

int wich_azimuth(){ // возвращает номер активированного азимута
  if (digitalRead(azimuth_1) == LOW) {
    return 1;
  }
  else if (digitalRead(azimuth_2) == LOW) {
    return 2;
  }
  else if (digitalRead(azimuth_3) == LOW) {
    return 3;
  }
  else {
    return 0;
  }
}
