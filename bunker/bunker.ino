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


// A1-A5, D4 заняты клавиатурой  _________
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

int min = 30;
int sec = 0;
long t_blink = 0; // счетчик мигания
bool t_flag = false; // флаг мигания
bool show_timer = true; // флаг отключения цифр таймера во время дективации

const uint8_t seg_dash[] = {SEG_G, SEG_G, SEG_G, SEG_G};
const uint8_t seg_vzrv[] = {
	SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G, // B
	SEG_A | SEG_B | SEG_C | SEG_D | SEG_G, // З
	SEG_A | SEG_B | SEG_E | SEG_F | SEG_G, // Р
	SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G // B
	};
const uint8_t seg_azim[] = { //TODO придумать азимут
	SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G, // A
	SEG_A | SEG_B | SEG_C | SEG_D | SEG_G, // Z
	SEG_A | SEG_B | SEG_E | SEG_F | SEG_G, // I
	};

void setup() {
  Serial.begin(9600);
  // настройки дисплея
  disp.clear();
  disp.setBrightness(7);
  disp.setSegments(seg_dash);

  // настройки радиомодуля
  while (radio.begin()) {// ждем инициализацию радио TODO инвертировать значение при установке радио
    Serial.println("radio hardware is not responding!!");
    delay(200);
  }
  radio.setPALevel (RF24_PA_LOW); // уровень мощности передатчика. На выбор RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
  radio.setPayloadSize(2); // размер пакета, в байтах 4
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
    case 1: ticking(); break;
    case 2: exploding(); break;
  }
}

void setting() { // настройки

}

void waiting() { // состояние ожидания ввода кода активации

  keyboarding(); // обработка ввода с клавиатуры

  if (entered_pin == activation_pin) { // проверка пина активации
    int message = 1;// TODO отправить запрос ракетам
    if (message == azimuth_settings) { // проверка азимута ракет
      t_blink = millis();
      second_mode = 1;
    }
    else {
      disp.setSegments(seg_azim);
      delay(2000);
      second_mode = 0;
    }
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
      // TODO запрос азимута
      if (message == azimuth_settings) {
        second_mode = 2;
      }
      else {
        disp.setSegments(seg_azim);
        delay(2000);
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
  disp.setSegments(seg_vzrv);

}

void keyboarding() {
  char key = keypad.getKey();
  if (key){
    Serial.println(key);
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