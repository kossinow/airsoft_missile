/*
Пульт управления ракетами.
*/

#include <SPI.h>
#include "RF24.h"
#include <TM1637Display.h>
#include <Keypad.h>
#include <EEPROM.h>


// A0-A5, D4 заняты клавиатурой
const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}};
byte rowPins[ROWS] = {14, 15, 16, 17};
byte colPins[COLS] = {18, 19, 4};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS );

// D2, D3 заняты экраном
#define CLK 2
#define DIO 3

// D9-D13 заняты радиомодулем
# define CE 9
# define CSN 10

RF24 radio(CE, CSN);
TM1637Display disp(CLK, DIO);

byte address[][6] = {"1Node", "2Node"}; //каналы радиомодуля

byte mode = 0;
long entered_pin = 0;
int setting_step = 0;
int current_word = 0;

int activation_pin;
int deactivation_pin;
int azimuth_settings;
int min;
int sec = 0;

long t_blink = 0; // счетчик мигания
bool t_flag = false; // флаг мигания

const uint8_t seg_dash[] = {SEG_G, SEG_G, SEG_G, SEG_G}; /// ----

const uint8_t seg_azim[] = {
	SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G, // A
	SEG_A | SEG_B | SEG_D | SEG_E | SEG_G, // Z
	SEG_E | SEG_F, // I
  SEG_E | SEG_C // m
	};
const uint8_t seg_a_pin[] = {
	SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G, // A
	SEG_A | SEG_B | SEG_E | SEG_F | SEG_G, // P
	SEG_E | SEG_F, // I
	SEG_C | SEG_E | SEG_G // n
	};
const uint8_t seg_c_pin[] = {
	SEG_A | SEG_D | SEG_E | SEG_F, // C
	SEG_A | SEG_B | SEG_E | SEG_F | SEG_G, // P
	SEG_E | SEG_F, // I
	SEG_C | SEG_E | SEG_G // n
	};
const uint8_t seg_time[] = {
	SEG_D | SEG_E | SEG_F | SEG_G, // t
	SEG_E | SEG_F, // I
	SEG_E | SEG_C, // m
	SEG_A | SEG_D | SEG_E | SEG_F | SEG_G // E
	};
  

void setup() {
  Serial.begin(9600); // TODO 
  // настройки дисплея
  disp.clear();
  disp.setBrightness(7);
  disp.setSegments(seg_dash);

  keypad.addEventListener(keypadEvent);

  // запись данных в память при прошивке свежей ардуинки
  if (EEPROM.read(1023) != 40) { // первый запуск
    EEPROM.write(1023, 40);    // записали ключ
    EEPROM.put(2, 1234);
    EEPROM.put(4, 4321);
    EEPROM.put(6, 1);
    EEPROM.put(8, 5);
  }

  EEPROM.get(2, activation_pin);
  EEPROM.get(4, deactivation_pin);
  EEPROM.get(6, azimuth_settings);
  EEPROM.get(8, min);

  // настройки радиомодуля
  while (!radio.begin()) {// ждем инициализацию радио
    Serial.println("radio hardware is not responding!!"); //TODO
    delay(200);
  }

  radio.setAutoAck(1);
  radio.setRetries(0, 15);
  radio.enableAckPayload();
  radio.enableDynamicPayloads();
  radio.setPayloadSize(2);
  radio.setChannel(0x60);
  radio.setDataRate(RF24_1MBPS);
  radio.setPALevel (RF24_PA_MAX); // уровень мощности передатчика. На выбор RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
  radio.openWritingPipe(address[0]);
  radio.stopListening();   // не слушаем радиоэфир, мы передатчик
  
  Serial.println("setup done"); // TODO
  
}

void loop() {
  show(entered_pin, current_word);
  switch (mode) { // главное меню - переключение между состояниями
    case 0: standby(); break;
    case 1: ticking(); break;
    case 2: exploding(); break;
    case 3: canceling(); break;
    case 4: setting(); break;
  }
}

void standby() { // состояние ожидания ввода кода активации

  int key = get_key();
  if (key>=0){ // при получении нажатия собираем пин
    entered_pin = assemble_pin(key);
    current_word = 0; // при неудачной активации меняем слово азимут на прочерк
  }
  if (entered_pin == activation_pin) { // проверка пина активации
    int message = communicate_antenna(false);
    Serial.println(message); // TODO
    if (message == azimuth_settings) { // проверка азимута ракет
      mode = 1;
      entered_pin = 0;
      t_blink = millis();
    }
    else {
      entered_pin = 0;
      current_word = 3; // говорим что азмут не тот
    }
  }
  else if(entered_pin > 1000){
    entered_pin = 0;
  }
}

void ticking() { // обратный отсчет
  int key = get_key();
  if (key>=0){ // при получении нажатия собираем пин
    entered_pin = assemble_pin(key);
  }

  if(entered_pin == deactivation_pin){
    mode = 3; // canceling
  }
  else if(entered_pin > 1000){
    entered_pin = activation_pin;
  }
  else if(entered_pin == 0){ // дабы избежать скрывания обратного отсчета за прочерком
    entered_pin = activation_pin;
  }

  if (millis() - t_blink > 1000){ // подсчет секунд и обновления состояния точек-разделителей
    t_blink = millis();
    t_flag = !t_flag;
    sec = sec - 1;
  }

  if (sec < 0){
    sec = 59;
    min --;
  }

  if (min < 0){
    mode = 2; // exploding
  }
}

void exploding() { // состояние взрыва
  int message = communicate_antenna(false);
  if (message == azimuth_settings) { // проверка азимута ракет
    communicate_antenna(true); // отправить сигнал на запуск
    min = 0;
    sec = 0;
    mode = 3;
  }
  else {
    mode = 0;
    entered_pin = 0;
    current_word = 3;
    min = EEPROM.read(8);
  }
}

void canceling(){ // состояние при деактивации

}

void setting() { // настройки
  int key = get_key();
  if (key>=0){ // при получении нажатия собираем пин
    entered_pin = assemble_pin(key);
  }

  switch (setting_step) { // переключение между настройками
    case 0:
      if(entered_pin > 999){
        activation_pin = entered_pin;
        EEPROM.put(2, activation_pin);
        setting_step = 1;
        current_word = 2;
        entered_pin = 0;
      }
      break;
    case 1:
      if(entered_pin == activation_pin){ // пин активации не должен совпадать с деактивацией
        entered_pin = 0;
      }
      else if(entered_pin > 999){
        deactivation_pin = entered_pin;
        EEPROM.put(4, deactivation_pin);
        setting_step = 2;
        current_word = 3;
        entered_pin = 0;
      }
      break;
    case 2:
      if(entered_pin>=1 && entered_pin<=3){
        azimuth_settings = entered_pin;
        EEPROM.put(6, azimuth_settings);
        setting_step = 3;
        current_word = 4;
        entered_pin = 0;
      }
      else{
        entered_pin = 0;
      }
      break;
    case 3:
      if(entered_pin>9 && entered_pin<100){
        min = entered_pin;
        EEPROM.put(8, min);
        setting_step = 0;
        current_word = 0;
        entered_pin = 0;
        mode = 0;
      }
      break;
  }
}

int get_key(){ // возвращает положительную цифру если она нажата, во всех остальных случаях отрицательную
  int key = keypad.getKey();
  if (key){
    switch (key){
      case '1': return 1;
      case '2': return 2;
      case '3': return 3;
      case '4': return 4;
      case '5': return 5;
      case '6': return 6;
      case '7': return 7;
      case '8': return 8;
      case '9': return 9;
      case '*': return -1;
      case '0': return 0;
      case '#': return -2;
    }
  }
  else return -3;
}

void keypadEvent(KeypadEvent key){ // дополнительны функции проверки подключения антенны и меню настройки
  switch(keypad.getState()){
  case HOLD:
    if (key == '#'){ // если не в режиме обратного отсчета переходим в настройки
      if (mode != 1){
        mode = 4;
        setting_step = 0;
        current_word = 1;
        entered_pin = 0;
      }
    break;
    }
    else if (key == '*'){ // проверка связи с блоком запуска
      int message = communicate_antenna(false);
      if (message >= 0 && message <= 3){
        disp.setSegments(seg_azim);
        delay(500); // я художник, я так вижу)
      }
    }
  }
}

int assemble_pin(int key){ // собирает четырехзначное число, если больше возврящает последнюю цифру
  entered_pin = entered_pin * 10 + key;
  if(entered_pin < 10000) return entered_pin;
  else return key;
}

void show(int pin, int word_n){ // отображает информацию на экране в зависимости от пин кода или слова
  if(pin == 0){ // нулевой пин показывает выбранное слово
    switch (word_n) { // главное меню - переключение между состояниями
      case 0: disp.setSegments(seg_dash); break; // прочерк
      case 1: disp.setSegments(seg_a_pin); break; // pin a
      case 2: disp.setSegments(seg_c_pin); break; // pin c
      case 3: disp.setSegments(seg_azim); break; // azi
      case 4: disp.setSegments(seg_time); break; // t
    }
  }
  else if(pin == activation_pin){ // верный пин показвыает таймер
    if (t_flag == true){
      disp.showNumberDecEx(min*100+sec, 0b01000000, true);
    }
    else if (t_flag == false){
      disp.showNumberDec(min*100+sec, true);
    }
  }
  else if(pin == deactivation_pin){
    disp.showNumberDecEx(min*100+sec, 0b01000000, true);
  }
  else if(pin != 0){ // показывает процесс вода пина
    disp.showNumberDec(pin);
  }
}

int communicate_antenna(bool command){ // принимает булеву команду на запуск ракеты, возвращает цифру установленного азимута
  int message;
  for (int i = 0; i < 10; i++){  // костыль чтобы получить текущее значение азимута(почему-то отправляет предыдущее)
    radio.write(&command, sizeof(command));
    if (radio.available()) { // если получаем ответ
      while (radio.available() ) { // если в ответе что-то есть
        radio.read(&message, sizeof(message)); // читаем
      }
    }
  }
  return message;
}