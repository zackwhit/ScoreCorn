#include <HX711_ADC.h>
#if defined(ESP8266)|| defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#endif

extern float cellSummation;

//pins:
const int HX711_dout_1 = 23; //mcu > HX711 no 1 dout pin
const int HX711_sck_1 = 22; //mcu > HX711 no 1 sck pin
const int HX711_dout_2 = 25; //mcu > HX711 no 2 dout pin
const int HX711_sck_2 = 24; //mcu > HX711 no 2 sck pin
const int HX711_dout_3 = 26; //mcu > HX711 no 3 dout pin
const int HX711_sck_3 = 27; //mcu > HX711 no 3 sck pin
const int HX711_dout_4 = 28; //mcu > HX711 no 4 dout pin
const int HX711_sck_4 = 29; //mcu > HX711 no 4 sck pin

const int calVal_eepromAdress_1 = 0; // eeprom adress for calibration value load cell 1 (4 bytes)
const int calVal_eepromAdress_2 = 4; // eeprom adress for calibration value load cell 2 (4 bytes)
const int calVal_eepromAdress_3 = 8; // eeprom adress for calibration value load cell 3 (4 bytes)
const int calVal_eepromAdress_4 = 12; // eeprom adress for calibration value load cell 4 (4 bytes)

void setupCells();
void tickCells_Callback();
void tareCells();
