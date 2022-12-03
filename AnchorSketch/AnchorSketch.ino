
#define _TASK_STATUS_REQUEST
#define _TASK_TIMEOUT
#include <TaskScheduler.h>

#include "AnchorWrap.hpp"


Scheduler ts;

bool isPaired = false;

void setup() {
  // Startup serial
  if (DS) Serial.begin(115200);
  while (!Serial && DS)
    ;
  delay(100);

  // Startup anchor code
  InitAnchor();

  // Wait to pair
  delay(0);
  // Pair with tag
  AnchorPair();

  ts.startNow();

  if (DS) Serial.println("Done anchor init");
  pinMode(PAIR_LED, OUTPUT);
  digitalWrite(PAIR_LED, HIGH);
}

void loop() {
  // Handle serial commands
  if (Serial.available() > 0 && DS) {
    char inByte = Serial.read();
    if (inByte == 'p') { // pair
      if (DS) Serial.println(F("Re-pairing..."));
      isPaired = false;
    } else if (inByte == 't') {  // t for test
      checkSPI();
    } else if (inByte == 'r') {  // reset DW
      InitAnchor();
    }
  }

  // OR
  // Skip window, try waiting till the next on

  // Anchor loop processes pairing and such
  if (isPaired)
    AnchorLoop();
  else {
    isPaired = AnchorPairLoop();
    if (isPaired) {
      if (DS) Serial.println("Done pairing.");
      // Turn on PAIR LED
      digitalWrite(PAIR_LED, LOW);
    }
  }


  ts.execute();
}
