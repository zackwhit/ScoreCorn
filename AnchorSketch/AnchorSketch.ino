#include "AnchorWrap.hpp"
#include "globalTasks.h"
#include <TaskScheduler.h>

void setup() {
  // Startup serial
  Serial.begin(115200);
  while(!Serial);

  // Startup anchor code
  InitAnchor();

  // Wait to pair
  delay(0);
  // Pair with tag
  AnchorPair();
}

void loop() {

  // OR
  // Skip window, try waiting till the next on

  // Anchor loop processes pairing and such
  AnchorLoop();
}
