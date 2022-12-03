#define _TASK_STATUS_REQUEST
#define _TASK_TIMEOUT
#include <TaskScheduler.h>

#include "TagWrap.hpp"
#include "GameFunctions.hpp"

#define EXPECTED_ANCHORS 1
#define PAIR_TIMEOUT 500
#define VIRT_TOSS_PERIOD 30
#define VIRT_TOSS_STEPS 50

void(* resetFunc) (void) = 0;

Scheduler ts;
void simulateThrow(uint32_t endDist, uint8_t bagIndex);
void poll_1_Callback1();
void poll_2_Callback1();
void poll_3_Callback1();
void pair_Callback();
void gameLoop_CallBack();
void game_loop(
  const uint8_t *ids,
  const int32_t *new_distances,
  bag_state_t *bag_states,
  int32_t board_weight,
  bool line_break_is_tripped);
void simToss_Callback();

Task poll1(100, TASK_ONCE, &poll_1_Callback1, &ts, false);
Task poll2(100, TASK_ONCE, &poll_2_Callback1, &ts, false);
Task poll3(100, TASK_ONCE, &poll_3_Callback1, &ts, false);

// Task for pairing with anchors
Task pairTask(2500, TASK_FOREVER, &pair_Callback, &ts, false);
// Task for the game loop
Task gameLoop(500, TASK_FOREVER, &gameLoop_CallBack, &ts, false);

Task simToss(VIRT_TOSS_PERIOD, TASK_ONCE, &simToss_Callback, &ts, false);

/*
TODO: 
- Finish Pairing code on both anchor and tag devices

Game loop requires:
- Distance
- uint8_t Bag ids
- uint32_t board weight
*/

bool doTagLoop = true;
bool doPairLoop = true;
uint16_t pairTimeoutCount = 0;
uint8_t pairedAnchors = 0;

uint8_t bag_ids[NUM_BAGS];
int32_t new_distances[NUM_BAGS];
bag_state_t bag_states[NUM_BAGS];
bool line_break_is_tripped = false;
int32_t board_weight = 0;
bool line_break_was_tripped = false;

// Simulation/test variables
uint32_t tossDistSteps[VIRT_TOSS_STEPS];
uint16_t currentStep = 0;
uint16_t selectedIndex = 0;
uint8_t throwFlags = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(100);

  //ts = new Scheduler();

  // Start up the DW1000 & tag code
  TagInit();

  // Run task scheduler (ts)
  ts.startNow();

  // Start pairing
  pairTask.enable();

  // Seed the random number generator
  randomSeed(analogRead(0));

  // Setup the game variables
  for (int i = 0; i < NUM_BAGS; i++) {
    bag_ids[i] = i + 1;
    bag_states[i] = UNTHROWN;
    new_distances[i] = 8000;
  }
}

void loop() {
  /* receive command from serial terminal, 
  - send 't' to initiate tare operation
  - send 'wXXXXX' to set the weight on the board
  - send 'vXXXXiXX[n/y][n/y]' to simulate a virtual toss 
          (endDist, bag index, y/n on board, y/n thru hole)
    - Example: 'v3000i01nn' means throw bag 01 just 3m away from the board, not on it
    - Example: 'v0500i00ny' means throw bag 00 to 500mm away from board, but going thru hole
  - send 'p' to print bag states
  - send 'r' to reset arduino (uno only!)
  */
  if (Serial.available() > 0) {
    char inByte = Serial.read();
    if (inByte == 't') {
      //TopRight.tareNoDelay();
      //BottomRight.tareNoDelay();
      //TopLeft.tareNoDelay();
      //BottomLeft.tareNoDelay();
      Serial.println(F("Taring loadcells..."));
    } else if (inByte == 'w') {
      char serBuf[6];
      Serial.readBytes(serBuf, 6);
      uint32_t weightConverted = atoi(serBuf);
      Serial.print(F("Read "));
      Serial.println(weightConverted);
    } else if (inByte == 'v') {
      if (Serial.available() < 10) {
        delay(10);
      }

      char serBuf[5];
      Serial.readBytes(serBuf, 4); // 'XXXX'
      uint32_t distConverted = atoi(serBuf);
      inByte = Serial.read();
      if (inByte != 'i') { // 'i'
        Serial.print(F("bad ser data, "));
        Serial.println(inByte, DEC);
      }
      char serBuf2[3];
      if (Serial.readBytes(serBuf2, 2) != 2) // 'XX'
        Serial.println(F("outta ser data"));
      uint8_t idConverted = atoi(serBuf2);
      Serial.print(F("Read ")); Serial.print(distConverted);
      Serial.print(", "); Serial.println(idConverted);
      throwFlags = 0;

      if (Serial.read() == 'y') {  // on-board
        throwFlags |= 0x1;
      }

      if (Serial.read() == 'y') {  // thru-hole
        throwFlags |= 0x2;
      }
      Serial.println(throwFlags, HEX);

      simulateThrow(distConverted, idConverted);

    } else if (inByte == 'p') {
      print_state(bag_ids, bag_states);
    } else if (inByte == 'r') { // reset arduino
      resetFunc();
    } else if (inByte == 'r') { // reset game
      // ?
    }
  }

  if (doTagLoop) {
    int result = TagLoop();
    if (result > 0) {  // We got a sucessful pair
      pairedAnchors++;
      Serial.print(F("Paried with anchor: "));
      Serial.println(result-1);
      bag_ids[pairedAnchors-1] = result-1;
    }
  }

  ts.execute();
}

void poll_1_Callback1() {
  //Serial.println(F("Ranging anchor 1..."));
  TagTransmitPoll(bag_ids[0]);
  poll1.disable();
  poll2.restartDelayed();
}

void poll_2_Callback1() {
  //Serial.println(F("Ranging anchor 2..."));
  TagTransmitPoll(bag_ids[1]);
  poll2.disable();
  poll3.restartDelayed();
}

void poll_3_Callback1() {
  //Serial.println(F("Ranging anchor 3..."));
  TagTransmitPoll(bag_ids[2]);
  poll3.disable();
  poll1.restartDelayed();
}

void pair_Callback() {
  if (pairedAnchors == EXPECTED_ANCHORS) {
    pairTask.disable();
    poll1.restartDelayed();
    gameLoop.restartDelayed();
    Serial.println(F("Done pairing."));
    return;
  }
  Serial.println(F("Attempting to pair"));

  if (pairTimeoutCount == 0) {
    TagTransmitPairAll();
  } else {
    TagTransmitPair();
  }
  

  pairTimeoutCount++;  // Replace with a real timeout?
  if (pairTimeoutCount >= PAIR_TIMEOUT) {
    Serial.print(F("Pairing timeout, only found "));
    Serial.print(pairedAnchors);
    Serial.println(F(" anchors."));
    pairTask.disable();
    poll1.restartDelayed();
  }
}

void gameLoop_CallBack() {

  // Figure out if we want to call the game_loop function itself (and feed it its parameters!)
  // [Don't do this until we're paired and set up...]

  game_loop(bag_ids, new_distances, bag_states, board_weight, line_break_is_tripped);
}

/* Simulating a throw
Step through a series of states using the event system library, basically determine
a random but plausible outcome and follow the steps to get there with some noise added
to the distance readings to represent the real world.

So we'd have like, the distance to VirtBag1 (VB1) and it would be a variable we can update
in a callback. In the 'simToss' callback, we would first setup the throw in a series of 
steps (uint16_t VB1_steps[]) and then walk through them with some varience in the timing
(restartDelayed(unsigned long aDelay)). 
*/


void simulateThrow(uint32_t endDist, uint8_t bagIndex) {
  // Set up the number of steps we go through in order to simulate the throw
  // The default starting position is 8m
  // The highest indice represents the last step (when the bag has arrived)
  // Calculate the step size
  uint16_t stepSize = (8000 - endDist) / VIRT_TOSS_STEPS;
  for (uint16_t i = 0; i < VIRT_TOSS_STEPS; i++) {
    tossDistSteps[i] = 8000 - i * stepSize;
    // Add some random variation too
    tossDistSteps[i] += random(-50, 50);
  }

  tossDistSteps[VIRT_TOSS_STEPS - 1] = endDist;
  selectedIndex = bagIndex;

  // Turn on the simulate-throw task callback
  simToss.restartDelayed();

  // Optionally have it "move" another bag?
}

void simToss_Callback() {
  new_distances[selectedIndex] = tossDistSteps[currentStep];
  //erial.print(F("Fired simToss on step "));
  //Serial.print(currentStep);
  //Serial.print(F(", dist "));
  //Serial.println(tossDistSteps[currentStep]);
  currentStep++;
  if (currentStep >= VIRT_TOSS_STEPS) {
    currentStep = 0;
    if (throwFlags & 0x1) {
      board_weight += BAG_WEIGHT;
    }
    if (throwFlags & 0x2) {
      line_break_was_tripped = true;
    }
  } else {
    simToss.restartDelayed();
  }
}


void game_loop(
  const uint8_t *ids,
  const int32_t *new_distances,
  bag_state_t *bag_states,
  int32_t board_weight,
  bool line_break_is_tripped) {
  /* 
     * -- Explanation of Distances -- 
     * We have 3 sets of distances floating around in this function:
     * new_distances, last_distances, and old_distances. 
     * - The new_distances are the ones we just received. 
     * - The last_distances are the ones we received in the last call to this 
     *   function. 
     * - The old_distances is the last set of distances which we used to update
     *   bag states. 
     * We only update bag states if 2 things are true:
     *  1. The thrown bags aren't currently moving.
     *  2. The thrown bags have changed position since we last updated bag 
     *     states.
     */

  
  static int32_t last_distances[NUM_BAGS];
  static int32_t old_distances[NUM_BAGS];
  static uint8_t num_bags_through_hole = 0;

  // Testing print
  //Serial.println(F("Game loop ran"));

  /* make all the bags start as far away as possible */
  static bool first_call = true;
  if (first_call) {
    first_call = false;
    for (uint8_t i = 0; i < NUM_BAGS; i++) {
      last_distances[i] = INT32_MAX;
      old_distances[i] = INT32_MAX;
    }
  }

  //if (line_break_is_tripped) line_break_was_tripped = true;

  if (!all_is_still(new_distances, last_distances)) {
    /* clean up and return */
    for (uint8_t i = 0; i < NUM_BAGS; i++) {
      last_distances[i] = new_distances[i];
    }
    return;
  }

  if (all_is_still(new_distances, old_distances)) {
    /* clean up and return */
    for (uint8_t i = 0; i < NUM_BAGS; i++) {
      last_distances[i] = new_distances[i];
    }
    return;
  }

  Serial.println("All was not still, re-calculating");

  /* count bags */
  uint8_t num_bags_on_board = calc_bags_on_board(board_weight);
  if (line_break_was_tripped) num_bags_through_hole++;
  uint8_t num_bags_thrown = 0;
  for (uint8_t i = 0; i < NUM_BAGS; i++) {
    if (new_distances[i] < THROWN_DISTANCE) num_bags_thrown++;
  }

  /* order bag indices by distance from sensor */
  uint8_t ndx_ordered_by_distance_asc[NUM_BAGS];

  int32_t distance_copy[NUM_BAGS];
  for (uint8_t i = 0; i < NUM_BAGS; i++) {
    distance_copy[i] = new_distances[i];
  }

  for (uint8_t j = 0; j < NUM_BAGS; j++) {
    int32_t min_distance = INT32_MAX;
    uint8_t min_ndx = 0;
    for (uint8_t i = 0; i < NUM_BAGS; i++) {
      if (distance_copy[i] < min_distance) {
        min_distance = distance_copy[i];
        min_ndx = i;
      }
    }
    ndx_ordered_by_distance_asc[j] = min_ndx;
    distance_copy[min_ndx] = INT32_MAX;
  }

  /* update the bag states*/
  // closest bags are through the hole
  for (uint8_t i = 0; i < num_bags_through_hole; i++) {
    uint8_t state_ndx = ndx_ordered_by_distance_asc[i];
    bag_states[state_ndx] = IN_HOLE;
    Serial.println(F("New bag through hole"));
  }

  // next furthest bags are on the board
  for (uint8_t i = num_bags_through_hole; i < num_bags_through_hole + num_bags_on_board; i++) {
    uint8_t state_ndx = ndx_ordered_by_distance_asc[i];
    bag_states[state_ndx] = ON_BOARD;
    Serial.println(F("New bag on board"));
  }

  // next furthest bags are off board
  for (uint8_t i = num_bags_through_hole + num_bags_on_board; i < num_bags_thrown; i++) {
    uint8_t state_ndx = ndx_ordered_by_distance_asc[i];
    bag_states[state_ndx] = OFF_BOARD;
    Serial.println(F("New bag off board"));
  }

  // remaining bags are still unthrown
  for (uint8_t i = num_bags_thrown; i < NUM_BAGS; i++) {
    uint8_t state_ndx = ndx_ordered_by_distance_asc[i];
    bag_states[state_ndx] = UNTHROWN;
  }

  /* clean up and return */
  line_break_was_tripped = false;
  for (uint8_t i = 0; i < NUM_BAGS; i++) {
    last_distances[i] = new_distances[i];
    old_distances[i] = new_distances[i];
  }
}
