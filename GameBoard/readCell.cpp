#include "readCells.h"

// These are the constructors as defined in the HX711_ADC header file
HX711_ADC TopRight(HX711_dout_1, HX711_sck_1);     //HX711 1
HX711_ADC BottomRight(HX711_dout_2, HX711_sck_2);  //HX711 2
HX711_ADC TopLeft(HX711_dout_3, HX711_sck_3);      //HX711 3
HX711_ADC BottomLeft(HX711_dout_4, HX711_sck_4);   //HX711 4

unsigned long t = 0;
float cellSummation = 0;

void setupCells() {

  float calibrationValue_1;  // calibration value load cell 1
  float calibrationValue_2;  // calibration value load cell 2
  float calibrationValue_3;  // calibration value load cell 3
  float calibrationValue_4;  // calibration value load cell 4

  calibrationValue_1 = -300;
  calibrationValue_2 = 404.17;
  calibrationValue_3 = -261.92;
  calibrationValue_4 = 271.21;

  // Begin is what powers up the HX711, sets the pinMode and HX711 gain
  TopRight.begin();
  BottomRight.begin();
  TopLeft.begin();
  BottomLeft.begin();

  // Waiting for tare to stabilize
  unsigned long stabilizingtime = 100;


  boolean _tare = true;  //set this to false if you don't want tare to be performed in the next step
  byte TopRight_rdy = 0;
  byte BottomRight_rdy = 0;
  byte TopLeft_rdy = 0;
  byte BottomLeft_rdy = 0;

  while ((TopRight_rdy + BottomRight_rdy + TopLeft_rdy + BottomLeft_rdy) < 4) {  //run startup, stabilization and tare, both modules simultaniously
    if (!TopRight_rdy) TopRight_rdy = TopRight.startMultiple(stabilizingtime, _tare);
    if (!BottomRight_rdy) BottomRight_rdy = BottomRight.startMultiple(stabilizingtime, _tare);
    if (!TopLeft_rdy) TopLeft_rdy = TopLeft.startMultiple(stabilizingtime, _tare);
    if (!BottomLeft_rdy) BottomLeft_rdy = BottomLeft.startMultiple(stabilizingtime, _tare);
  }
  if (TopRight.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 no.1 wiring and pin designations");
  }
  if (BottomRight.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 no.2 wiring and pin designations");
  }

  // user set calibration values
  TopRight.setCalFactor(calibrationValue_1);
  BottomRight.setCalFactor(calibrationValue_2);
  TopLeft.setCalFactor(calibrationValue_3);
  BottomLeft.setCalFactor(calibrationValue_4);
  Serial.println("Startup is complete");
}


void tickCells_Callback() {
  static boolean newDataReady = 0;
  const int serialPrintInterval = 500;  //increase value to slow down serial print activity

  // check for new data/start next conversion:
  if (TopRight.update()) newDataReady = true;
  BottomRight.update();
  TopLeft.update();
  BottomLeft.update();

  //get smoothed value from data set
  if ((newDataReady)) {
    float a = TopRight.getData();
    float b = BottomRight.getData();
    float c = TopLeft.getData();
    float d = BottomLeft.getData();
    cellSummation = (a + b + c + d);
    if (millis() > t + serialPrintInterval) {
      //Serial.print("Top Right: ");
      //Serial.print(a);
      //Serial.print(" Bottom Right: ");
      //Serial.print(b);
      //Serial.print(" Top Left: ");
      //Serial.print(c);
      //Serial.print(" Bottom Left: ");
      //Serial.println(d);
      Serial.print("Sum:");
      Serial.println(cellSummation);

      //Serial.println(x, DEC);

      newDataReady = 0;
      t = millis();
    }
  }

  //check if last tare operation is complete
  if (TopRight.getTareStatus() == true) {
    Serial.println("Tare Top Right complete");
  }
  if (BottomRight.getTareStatus() == true) {
    Serial.println("Tare Bottom Right complete");
  }
  if (TopLeft.getTareStatus() == true) {
    Serial.println("Tare Top Left complete");
  }
  if (BottomLeft.getTareStatus() == true) {
    Serial.println("Tare Bottom Left complete");
  }
}

void tareCells() {
  TopRight.tareNoDelay();
  BottomRight.tareNoDelay();
  TopLeft.tareNoDelay();
  BottomLeft.tareNoDelay();  
}