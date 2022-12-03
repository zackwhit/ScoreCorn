#define _TASK_STATUS_REQUEST
#define _TASK_TIMEOUT
#include <TaskScheduler.h>

#include "TagWrap.hpp"

Scheduler ts;


void poll_1_Callback1();
void poll_2_Callback1();
void poll_3_Callback1();

Task poll1(1000, TASK_ONCE, &poll_1_Callback1, &ts, true);
Task poll2(1000, TASK_ONCE, &poll_2_Callback1, &ts, false);
Task poll3(1000, TASK_ONCE, &poll_3_Callback1, &ts, false);


void setup() {
  Serial.begin(115200);
  while(!Serial)
    delay(100);

  //ts = new Scheduler();

  // Start up the DW1000 & tag code
  TagInit();

  // Run task scheduler (ts)
  ts.startNow();
  Serial.print("Address of ts: "); Serial.println((uint32_t)&ts, HEX);
}

void loop() {

  TagLoop();
  ts.execute();
}

void poll_1_Callback1() {
  Serial.println("Ranging anchor 1...");
  TagTransmitPoll(0x01);
  poll1.disable();
  poll2.restartDelayed();
}

void poll_2_Callback1() {
  Serial.println("Ranging anchor 2...");
  TagTransmitPoll(0x02);
  poll2.disable();
  poll3.restartDelayed();
}

void poll_3_Callback1() {
  Serial.println("Ranging anchor 3...");
  TagTransmitPoll(0x03);
  poll3.disable();
  poll1.restartDelayed();
  
}


// Ranging setup process
// Range with first anchor

