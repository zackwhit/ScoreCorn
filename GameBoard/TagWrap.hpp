#pragma once

#include <DW1000Ng.hpp>
#include <DW1000NgUtils.hpp>
#include <DW1000NgRanging.hpp>
#include <DW1000NgTime.hpp>
#include <DW1000NgConstants.hpp>

#include "globalTasks.h"

// connection pins -
#ifdef SEEED_XIAO_M0
// Seeduino XIAO
const uint8_t PIN_RST = 2; // reset pin
const uint8_t PIN_IRQ = 3; // irq pin
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

#define LEN_DATA 16

void TagInit();
int TagLoop();
void TagTransmitPoll(uint8_t destAddr);
void TagTransmitPair();
void TagTransmitPairAll();
void TagTransmitRange();

void TagReceiveHandler();
void TagSentHandler();

void tagWatchdogCallback();


