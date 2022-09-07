#include <SPI.h>
#include <DW1000.h>

// connection pins
const uint8_t PIN_RST = 8; // reset pin
const uint8_t PIN_IRQ = 9; // irq pin
const uint8_t PIN_SS = SS; // spi select pin


// DEBUG packet sent status and count
boolean sent = false;
volatile boolean sentAck = false;
volatile unsigned long delaySent = 0;
int16_t sentNum = 0; // todo check int type
DW1000Time sentTime;

void setup() {
  // DEBUG monitoring
  Serial.begin(9600);
  while(!Serial);
  Serial.println(F("### DW1000-arduino-sender-test ###"));
  // initialize the driver
  //DW1000._slowSPI = SPISettings(2000000L, MSBFIRST, SPI_MODE0);
  DW1000._currentSPI = &DW1000._slowSPI;
  DW1000.begin(PIN_IRQ, PIN_RST);
  DW1000.select(PIN_SS);
  Serial.println(F("DW1000 initialized ..."));
  // general configuration
  DW1000.newConfiguration();
  DW1000.setDefaults();
  DW1000.setDeviceAddress(5);
  DW1000.setNetworkId(10);
  DW1000.enableMode(DW1000.MODE_LONGDATA_RANGE_LOWPOWER);
  DW1000.commitConfiguration();
  Serial.println(F("Committed configuration ..."));
  // DEBUG chip info and registers pretty printed
  char msg[128];
  DW1000.getPrintableDeviceIdentifier(msg);
  Serial.print("Device ID: "); Serial.println(msg);
  DW1000.getPrintableExtendedUniqueIdentifier(msg);
  Serial.print("Unique ID: "); Serial.println(msg);
  DW1000.getPrintableNetworkIdAndShortAddress(msg);
  Serial.print("Network ID & Device Address: "); Serial.println(msg);
  DW1000.getPrintableDeviceMode(msg);
  Serial.print("Device mode: "); Serial.println(msg);
  // attach callback for (successfully) sent messages
  DW1000.attachSentHandler(handleSent);
  // start a transmission
  transmitter();
}

void loop() {
  // put your main code here, to run repeatedly:

}
