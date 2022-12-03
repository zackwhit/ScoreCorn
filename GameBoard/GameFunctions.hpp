#include "stdlib.h"
#include "stdint.h"
#include "stdbool.h"
#include "math.h"
#include "stdio.h"
#include <Arduino.h>

/* number of bags in the cornhole game */
#define NUM_BAGS 4

/* bags which move less than this distance probably didn't actually move */
#define NOISE_DISTANCE 10 

/* bags closer than this distance have been thrown */
#define THROWN_DISTANCE 5000

/* the weight of one bag */
#define BAG_WEIGHT 300

typedef enum {
    UNTHROWN,
    ON_BOARD,
    OFF_BOARD,
    IN_HOLE,
} bag_state_t;

bool all_is_still(const int32_t *new_distances, const int32_t *old_distances);
uint8_t calc_bags_on_board(int32_t board_weight);
void print_state(const uint8_t *ids, const bag_state_t *bag_states);