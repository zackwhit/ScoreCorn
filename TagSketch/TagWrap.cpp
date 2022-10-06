#include "TagWrap.hpp"

#define _TASK_STATUS_REQUEST
#define _TASK_TIMEOUT
#include <TaskScheduler.h>

// Task scheduler variables
StatusRequest sr;

Task tagWatchdog(&tagWatchdogCallback, &ts);


// timestamps to remember
uint64_t timePollSent;
uint64_t timePollReceived;
uint64_t timePollAckSent;
uint64_t timePollAckReceived;
uint64_t timeRangeSent;
uint64_t timeRangeReceived;

// Helps with message error handling
uint8_t nextExpectedMsg;

// reply times (same on both sides for symm. ranging)
uint16_t replyDelayTimeUS = 3000;

bool tagInitialized = false;

// Volatile variables
byte data[LEN_DATA];
volatile bool receivedMsg = false;

device_configuration_t DEFAULT_CONFIG = {
    false,
    true,
    true,
    true,
    false,
    SFDMode::STANDARD_SFD,
    Channel::CHANNEL_5,
    DataRate::RATE_850KBPS,
    PulseFrequency::FREQ_16MHZ,
    PreambleLength::LEN_256,
    PreambleCode::CODE_3
};

interrupt_configuration_t DEFAULT_INTERRUPT_CONFIG = {
    true,
    true,
    true,
    false,
    true
};

// Namespace alias
namespace DW = DW1000Ng;

void TagInit() {
  DW::initialize(PIN_SS, PIN_IRQ, PIN_RST);
  DW::applyConfiguration(DEFAULT_CONFIG);
  DW::applyInterruptConfiguration(DEFAULT_INTERRUPT_CONFIG);
  DW::setNetworkId(10);
  DW::setAntennaDelay(16436);

  // Set handlers
  DW::attachSentHandler(TagSentHandler);
  DW::attachReceivedHandler(TagReceiveHandler);

  // Check SPI communication
  // get device ID register and check it matches expected value
  if (DW::checkDeviceId()){
    Serial.println("SPI seems to work");
    tagInitialized = true;
  } else {
    Serial.println("SPI error - unable to talk to DW1000");
    tagInitialized = false;
  }
}

void TagLoop(){

  // Check for timeout & reset if so
  if (sr.getStatus() == TASK_SR_TIMEOUT && nextExpectedMsg != 10) {
    Serial.println("Experienced timeout");
    nextExpectedMsg = 10;
    DW::forceTRxOff();
  }


  // Check for received message
  if (receivedMsg) {
    // Get message type
    receivedMsg = false;
    DW::getReceivedData(data, LEN_DATA);
    byte msgId = data[0];

    // Handle POLL_ACK - send range
    if (nextExpectedMsg == msgId) {
      if (msgId == POLL_ACK) {
        Serial.println("Got poll ACK");
        timePollSent = DW::getTransmitTimestamp();
        timePollAckReceived = DW::getReceiveTimestamp();
        TagTransmitRange();
      } else if (msgId == RANGE_REPORT) {
        Serial.print("Got Range Report: ");
        float curRange = 0;
        memcpy(&curRange, data + 1, 4);
        Serial.print(curRange, 3); Serial.println("m");
        nextExpectedMsg = 10; // undefined msg id
        sr.signalComplete();
      }
    } else {
      Serial.println("unexpected message");
    }
  }
}

void TagTransmitPoll(uint8_t destAddr) {
  if (nextExpectedMsg == POLL_ACK)
    return;
  nextExpectedMsg = POLL_ACK;
  Serial.println("Sending poll message");

  data[0] = POLL;
  data[5] = destAddr;
  DW1000Ng::setTransmitData(data, LEN_DATA);
  DW1000Ng::startTransmit();

  // Start StatusRequest
  //    - setWaiting();
  // Timeout will reset everything and wait for new poll
  //    - setTimeout(_time_)
  // Each message stage will reset timeout
  //    - resetTimeout()
  // Final message will mark as complete
  //    - signalComplete()
  sr.setWaiting();
  sr.setTimeout(6);
  tagWatchdog.waitFor(&sr);
}

void TagTransmitRange() {
  nextExpectedMsg = RANGE_REPORT;
  sr.resetTimeout();
  Serial.println("Sending range message");

  data[0] = RANGE;

  /* Calculation of future time */
  byte futureTimeBytes[LENGTH_TIMESTAMP];

  timeRangeSent = DW1000Ng::getSystemTimestamp();
  timeRangeSent += DW1000NgTime::microsecondsToUWBTime(replyDelayTimeUS);
  DW1000NgUtils::writeValueToBytes(futureTimeBytes, timeRangeSent, LENGTH_TIMESTAMP);
  DW::setDelayedTRX(futureTimeBytes);
  timeRangeSent += DW1000Ng::getTxAntennaDelay();

  DW1000NgUtils::writeValueToBytes(data + 1, timePollSent, LENGTH_TIMESTAMP);
  DW1000NgUtils::writeValueToBytes(data + 6, timePollAckReceived, LENGTH_TIMESTAMP);
  DW1000NgUtils::writeValueToBytes(data + 11, timeRangeSent, LENGTH_TIMESTAMP);
  DW::setTransmitData(data, LEN_DATA);
  DW::startTransmit(TransmitMode::DELAYED);
  //Serial.print("Expect RANGE to be sent @ "); Serial.println(timeRangeSent.getAsFloat());

}

void TagReceiveHandler(){
  receivedMsg = true;
}

void TagSentHandler(){
  Serial.println("Done transmitting");

  //if (data[0] == POLL_ACK) {
  //  timePollAckSent = DW::getTransmitTimestamp();
  //}
  DW::forceTRxOff();
  DW::startReceive();
}

void tagWatchdogCallback() {
  return;
}
