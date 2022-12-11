#include "GameFunctions.hpp"


bool all_is_still(const int32_t *new_distances, const int32_t *old_distances, bool checkOld) {
  bool still = true;
  for (uint8_t i = 0; i < NUM_BAGS; i++) {
    if (/* if bag is thrown and bag has moved */
        new_distances[i] < THROWN_DISTANCE && abs(new_distances[i] - old_distances[i]) > NOISE_DISTANCE) {
      if (checkOld || old_distances[i] > THROWN_DISTANCE) {
        still = false;
        //Serial.println(new_distances[i]);
        //Serial.println(old_distances[i]);
        //Serial.println(abs(new_distances[i] - old_distances[i]));
        //if (checkOld) Serial.println("T");
        break;
      }
    }
  }
  return still;
}


uint8_t calc_bags_on_board(int32_t board_weight) {
  for (uint8_t i = 0; i <= NUM_BAGS; i++) {
    if (abs(board_weight - BAG_WEIGHT * i) < (BAG_WEIGHT / 2 + 1)) {
      return i;
    }
  }
  return 255;  // 255 indicates error
}

void print_state(const uint8_t *ids, const bag_state_t *bag_states) {
  for (int i = 0; i < NUM_BAGS; i++) {
    //printf("%d: ", ids[i]);
    Serial.print(ids[i]);
    Serial.print(": ");
    Serial.print(" ");
    switch (bag_states[i]) {
      case UNTHROWN:
        Serial.println("UNTHROWN");
        //printf("UNTHROWN\n");
        break;
      case ON_BOARD:
        Serial.println("ON BOARD");
        //printf("ON BOARD\n");
        break;
      case OFF_BOARD:
        Serial.println("OFF BOARD");
        //printf("OFF BOARD\n");
        break;
      case IN_HOLE:
        Serial.println("IN HOLE");
        //printf("IN HOLE\n");
        break;
      default:
        Serial.println("Error: Invalid state.");
        //printf("ERROR: Invalid state.\n");
    }
  }
  //printf("\n");
}