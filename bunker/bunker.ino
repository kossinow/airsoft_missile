/*
Пульт управления ракетами.
При правильном вводе кода активации и правильно установленной ракете начинается таймер.
По истечении тацмера, если ракеты стоят на правильном азимуте происходит их запуск.
Во время обратного отсчета можно ввести код деактивации таймера.
*/

#include <SPI.h>
#include "RF24.h"
#include <TM1637Display.h>
#include <Keypad.h>


// A0-A5, D4 заняты клавиатурой  _________
const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}};
byte rowPins[ROWS] = {14, 15, 16, 17};
byte colPins[COLS] = {18, 19, 4}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

// ______________________________________


// D2, D3 заняты экраном
#define CLK 2
#define DIO 3

// D9-D13 заняты радиомодулем
# define CE 9
# define CSN 10

RF24 radio(CE, CSN);
TM1637Display disp(CLK, DIO);

byte address[][6] = {"1Node", "2Node"}; //возможные номера труб
int message;
byte mode = 0;
byte second_mode = 0;

long entered_pin = 0;
long activation_pin = 1234;
long deactivation_pin = 4321;
int azimuth_settings = 1; // выбор азимута 1..3

int min = 0;
int sec = 5;
long t_blink = 0; // счетчик мигания
bool t_flag = false; // флаг мигания
bool show_timer = true; // флаг отключения цифр таймера во время дективации
long switch_off; // счетчик для отключения временной информации

const uint8_t seg_dash[] = {SEG_G, SEG_G, SEG_G, SEG_G};
const uint8_t seg_vzrv[] = {
	SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G, // B
	SEG_A | SEG_B | SEG_C | SEG_D | SEG_G, // З
	SEG_A | SEG_B | SEG_E | SEG_F | SEG_G, // Р
	SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G // B
	};
const uint8_t seg_azim[] = { //TODO придумать азимут
	SEG_A | SEG_B | SEG_D | SEG_E | SEG_F | SEG_G, // A
	SEG_C | SEG_D | SEG_G, // Z
	SEG_A | SEG_B | SEG_E | SEG_F | SEG_G, // I
	};

void setup() {
  Serial.begin(9600);
  // настройки дисплея
  disp.clear();
  disp.setBrightness(7);
  disp.setSegments(seg_dash);

  // настройки радиомодуля
  while (!radio.begin()) {// ждем инициализацию радио
    Serial.println("radio hardware is not responding!!");
    delay(200);
  }

  radio.setAutoAck(1);
  radio.setRetries(0, 15);
  radio.enableAckPayload();
  radio.enableDynamicPayloads();
  radio.setPayloadSize(2);
  radio.setChannel(0x60);
  radio.setDataRate(RF24_1MBPS);
  radio.setPALevel (RF24_PA_LOW); // уровень мощности передатчика. На выбор RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
  radio.openWritingPipe(address[0]);
  radio.stopListening();   // не слушаем радиоэфир, мы передатчик
  Serial.println("setup done");
  
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
    case 1: ticking(); break;
    case 2: exploding(); break;
  }
}

void setting() { // настройки

}

void waiting() { // состояние ожидания ввода кода активации

  keyboarding(); // обработка ввода с клавиатуры

  if (entered_pin == activation_pin) { // проверка пина активации

    bool command = false;
    byte i = 10; // костыль чтобы получить текущее значение азимута(почему-то отправляет предыдущее)
    while (i > 0) {
      radio.write(&command, sizeof(command));
      if (radio.available()) { // если получаем ответ
        while (radio.available() ) { // если в ответе что-то есть
          radio.read(&message, sizeof(message)); // читаем
        }
      }
      i --;
    }

    if (message == azimuth_settings) { // проверка азимута ракет
      t_blink = millis();
      second_mode = 1;
    }
    else {
      disp.setSegments(seg_azim);
      switch_off = millis();
    }
  entered_pin = 0;
  }
  if (millis() - switch_off > 3000){
    disp.setSegments(seg_dash);
    entered_pin = 0;
  }
}

void ticking() { // обратный отсчет

  keyboarding(); // TODO сделать чтобы функция возвращала значение для отсчета времени

  if (second_mode == 1){ // состояние отсчета времени
    if (millis() - t_blink > 1000){
      t_blink = millis();
      t_flag = !t_flag;
      sec = sec - 1;
    }

    if (sec < 0){
      sec = 59;
      min --;
    }

    if (min < 0){
      if (message == azimuth_settings) {
        second_mode = 2;
      }
      else {
        disp.setSegments(seg_azim);
        delay(2000);
        disp.setSegments(seg_dash);
        second_mode = 0;
      }
      t_blink = millis();
    }

    if (show_timer) {
      if (t_flag == true){
        disp.showNumberDecEx(min*100+sec, 0b01000000, true);
      }
      else if (t_flag == false){
        disp.showNumberDec(min*100+sec, true);
      }
    }
  }
}


void exploding() { // состояние взрыва
    bool command = false;
    byte i = 10; // костыль чтобы получить текущее значение азимута(почему-то отправляет предыдущее)
    while (i > 0) {
      radio.write(&command, sizeof(command));
      if (radio.available()) { // если получаем ответ
        while (radio.available() ) { // если в ответе что-то есть
          radio.read(&message, sizeof(message)); // читаем
        }
      }
      i --;
    }

    if (message == azimuth_settings) { // проверка азимута ракет
      command = true;
      byte i = 10; // костыль чтобы получить текущее значение азимута(почему-то отправляет предыдущее)
      while (i > 0) {
        radio.write(&command, sizeof(command));
          if (radio.available()) { // если получаем ответ
            while (radio.available() ) { // если в ответе что-то есть
              radio.read(&message, sizeof(message)); // читаем
            }
          }
      i --;
      }
      t_blink = millis();
      disp.setSegments(seg_vzrv);
      switch_off = millis();
    }
    else {
      disp.setSegments(seg_azim);
      switch_off = millis();
    }
second_mode = 0;
sec = 5; //TODO обновить время ожидания
}

void keyboarding() {
  char key = keypad.getKey();
  if (key){
    Serial.println(key);
    switch_off = millis();
    long n = key + '0' - 96;
    entered_pin = entered_pin * 10 + n;    
    if (entered_pin / 10000 >= 1 || entered_pin < 0) {
      entered_pin = 0;
      disp.setSegments(seg_dash);
    }
    else {
      disp.showNumberDec(entered_pin);
    }
  }
}