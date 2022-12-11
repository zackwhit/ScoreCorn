#pragma once

#include <DW1000Ng.hpp>
#include <DW1000NgUtils.hpp>
#include <DW1000NgRanging.hpp>

#include "globalTasks.h"

// "Do Serial" - debug switch
#define DS false

// connection pins -
#ifdef SEEED_XIAO_M0
// Seeduino XIAO
const uint8_t PIN_RST = 2; // reset pin
const uint8_t PIN_IRQ = 3; // iq pin
const uint8_t PIN_SS =  7; // spi select pin
#elif defined ARDUINO_AVR_UNO
// Uno/Nano
const uint8_t PIN_RST = 9; // reset pin
const uint8_t PIN_IRQ = 2; // irq pin
const uint8_t PIN_SS = SS; // spi select pin
#else
// Default
const uint8_t PIN_RST = 9; // reset pin
const uint8_t PIN_IRQ = 2; // irq pin
const uint8_t PIN_SS =  SS; // spi select pin
#endif


const uint8_t devAddr = 0x02;
const uint16_t devPairDelay = 100*devAddr+50; //ms to wait before replying to PAIR
const uint16_t deviceRetryTimeout = 500;

// messages used in the ranging protocol
// TODO replace by enum
#define POLL 0
#define POLL_ACK 1
#define RANGE 2
#define RANGE_REPORT 3
#define PAIR 4
#define PAIR_ACK 5
#define PAIR_ALL 6
#define RANGE_FAILED 255

#define PAIR_LED 12
#define SEND_TIMEOUT 10

extern bool isPaired;

//#ifdef SAMD21
// Different pins
//#endif

#define LEN_DATA 16

void InitAnchor();

void AnchorLoop();
bool AnchorPairLoop();
void AnchorReceiveHandler();
void AnchorSentHandler();
void AnchorPair();
void pollResponse();
void rangeResponse();
void pairResponse();
void checkSPI();

