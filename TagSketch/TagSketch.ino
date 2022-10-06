#include "TagWrap.hpp"

Scheduler ts;

void pollCallback1();

Task poll1(1000, TASK_FOREVER, &pollCallback1, &ts, true);

void setup() {
  Serial.begin(115200);
  while(!Serial)
    delay(100);

  //ts = new Scheduler();

  // Start up the DW1000 & tag code
  TagInit();

  // Run task scheduler (ts)
  ts.startNow();
}

void loop() {

  TagLoop();
  ts.execute();
}

void pollCallback1() {
  TagTransmitPoll(0x00);
}


// Ranging setup process
