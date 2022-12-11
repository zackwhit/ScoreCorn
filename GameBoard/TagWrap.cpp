#include "DW1000Ng.hpp"
#include "TagWrap.hpp"
#include <SPI.h>

// Task scheduler variables
StatusRequest sr;

Task *tagWatchdog;

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
uint16_t curRange = 0;

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
    //DataRate::RATE_6800KBPS,
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

  tagWatchdog = new Task(&tagWatchdogCallback, &ts);

  // Pair with anchors
  // Send a ping packet
  // Each time we hear back from an anchor, assign it an ID (and give it a timing period?)
  // All anchors are in receive mode until pairing is done?
  // Range to first anchor, delay, and then send to next
  // Restart after reaching last anchor

}

int TagLoop(){

  // Check for timeout & reset if so
  if (sr.getStatus() == TASK_SR_TIMEOUT && nextExpectedMsg != 10) {
    //Serial.println("Experienced timeout");
    nextExpectedMsg = 10;
    DW::forceTRxOff();
  }


  // Check for received message
  if (receivedMsg) {
    // Get message type
    receivedMsg = false;
    DW::getReceivedData(data, LEN_DATA);
    byte msgId = data[0];
    uint8_t devAddr = data[5];

    // Handle POLL_ACK - send range
    if (nextExpectedMsg == msgId) {
      if (msgId == POLL_ACK) {
        //Serial.println("Got poll ACK");
        timePollSent = DW::getTransmitTimestamp();
        timePollAckReceived = DW::getReceiveTimestamp();
        TagTransmitRange();
      } else if (msgId == RANGE_REPORT) {
        Serial.print("R:");
        Serial.print(devAddr);
        Serial.print(":");
        memcpy(&curRange, data + 1, 2);
        double printableRange = (double)(curRange/1000.0);
        Serial.print(printableRange); Serial.println(":");
        nextExpectedMsg = 10; // undefined msg id
        sr.signalComplete();
        return devAddr+100;
      } else if (msgId == PAIR_ACK) {
        Serial.println("Got pair ACK");
        sr.signalComplete();
        DW::startReceive();
        return devAddr+1;
      } else {
        Serial.println("Unexpected expected error?");
      }
    } else {
      Serial.println("unexpected message");
    }
  }

  return 0; // Normal return
}

void TagTransmitPair() {
  nextExpectedMsg = PAIR_ACK;
  //Serial.println("Sending PAIR message");
  data[0] = PAIR;

  DW::setTransmitData(data, LEN_DATA);
  DW::startTransmit();
  
  // TODO:
  // - Add code to the loop to handle responses   
  // (if msgId == PAIR_ACK) ^

  // Timeout stuff?
  sr.setWaiting();
  sr.setTimeout(1200);
  tagWatchdog->waitFor(&sr);
}

void TagTransmitPairAll() {
  nextExpectedMsg = PAIR_ACK;
  //Serial.println("Sending PAIR message");
  data[0] = PAIR_ALL;

  DW::setTransmitData(data, LEN_DATA);
  DW::startTransmit();
  
  // TODO:
  // - Add code to the loop to handle responses   
  // (if msgId == PAIR_ACK) ^

  // Timeout stuff?
  sr.setWaiting();
  sr.setTimeout(1200);
  tagWatchdog->waitFor(&sr);
}

void TagTransmitPoll(uint8_t destAddr) {
  if (nextExpectedMsg == POLL_ACK)
    return;
  nextExpectedMsg = POLL_ACK;
  //Serial.println("Sending poll message");

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
  sr.setTimeout(SEND_TIMEOUT);
  tagWatchdog->waitFor(&sr);
}

void TagTransmitRange() {
  nextExpectedMsg = RANGE_REPORT;
  sr.resetTimeout();

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
}

void TagReceiveHandler(){
  receivedMsg = true;
}

void TagSentHandler(){
  //Serial.println("Done transmitting");

  //if (data[0] == POLL_ACK) {
  //  timePollAckSent = DW::getTransmitTimestamp();
  //}
  DW::forceTRxOff();
  DW::startReceive();
}

void tagWatchdogCallback() {
  //Serial.println("got timeout");
  return;
}
