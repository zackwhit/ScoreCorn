#include "AnchorWrap.hpp"

byte data[LEN_DATA];

void anchorWatchdogCallback();

// Task scheduler variables
StatusRequest sr;

Task *anchorWatchdog;
Task *testTask;

// timestamps to remember
uint64_t timePollSent;
uint64_t timePollReceived;
uint64_t timePollAckSent;
uint64_t timePollAckReceived;
uint64_t timeRangeSent;
uint64_t timeRangeReceived;

// Helps with message error handling
uint8_t nextExpectedMsg;

// Volatile variables
volatile bool receivedMsg = false;

// Namespace alias
namespace DW = DW1000Ng;

device_configuration_t DEFAULT_CONFIG = {
  false,
  true,
  true,
  true,
  false,
  SFDMode::STANDARD_SFD,
  Channel::CHANNEL_5,
  //DataRate::RATE_850KBPS,
  DataRate::RATE_6800KBPS,
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

void InitAnchor() {
  // Initialize driver
  DW::initialize(PIN_SS, PIN_IRQ, PIN_RST);

  // Apply configuration
  DW::applyConfiguration(DEFAULT_CONFIG);
  DW::applyInterruptConfiguration(DEFAULT_INTERRUPT_CONFIG);

  DW::setNetworkId(10);

  // Set device address
  DW::setDeviceAddress(devAddr);

  // Set antenna delay
  DW::setAntennaDelay(16436);

  // Set handlers
  DW::attachSentHandler(AnchorSentHandler);
  DW::attachReceivedHandler(AnchorReceiveHandler);


  // Start in receive mode
  DW::forceTRxOff();
  DW::startReceive();

  anchorWatchdog = new Task(&anchorWatchdogCallback, &ts);

  // Check SPI communication
  // get device ID register and check it matches expected value
  checkSPI();
}

void AnchorPair() {
  // Note: Pairing is apperently not entirely necessary!
  // If we do want to pair, however, wait a random amount of
  // time after receiving the blink message, and then respond.
}

void AnchorLoop() {
  // Check for timeout & reset if so
  if (sr.getStatus() == TASK_SR_TIMEOUT && nextExpectedMsg != 10 && nextExpectedMsg != PAIR_ACK) {
    if (DS) Serial.println(F("Experienced timeout-----------------------------------"));
    if (nextExpectedMsg == POLL) {
      // This is a big problem, shouldn't have missed a POLL.
      // Try to re-init anchor in order to fix it.
      InitAnchor();
      sr.setWaiting();
      sr.setTimeout(100); // TODO: Make a retry-interval variable
      anchorWatchdog->waitFor(&sr);
    } else {
      nextExpectedMsg = 10;
    }
    DW::forceTRxOff();
    DW::startReceive();
    
  }

  // anchor needs to pair with a tag
  // pairing will consist of waiting a pre-determined
  // amount of time, then entering receive mode and waiting
  // for the pairing packet.

  // Simplest ranging would be
  // Tag: Poll
  // Anchor: Poll response
  // Tag: Final
  // Is the final message entirely necessary?

  // For testing, check if we get a message and print the type of it
  if (receivedMsg) {
    digitalWrite(PAIR_LED, LOW);
    receivedMsg = false;
    DW::getReceivedData(data, LEN_DATA);
    byte msgId = data[0];
    if (msgId == POLL && data[5] == devAddr) {
      if (DS) Serial.println("Received POLL message");
      pollResponse();
      sr.signalComplete(); 
    } else if (msgId == RANGE && nextExpectedMsg == RANGE) {
      if (DS) Serial.println("Received RANGE message.");
      rangeResponse();
      sr.signalComplete(); 
    } else if (msgId == PAIR_ALL) { // Restart pairing?
      isPaired = false;
      if (DS) Serial.println("Got restart pair request, pairing...");
      DW::forceTRxOff();
      DW::startReceive();
      sr.signalComplete();
    } else if (data[5] != devAddr) {
      //Serial.println("Message received for other anchor");
      DW::startReceive();
    } else {
      if (DS) Serial.println("Got unknown message.");
      //DW::forceTRxOff();
      DW::startReceive();
    }
  }
}


bool AnchorPairLoop() {
  if (receivedMsg) {
    receivedMsg = false;
    DW::getReceivedData(data, LEN_DATA);
    byte msgId = data[0];
    if (msgId == PAIR || msgId == PAIR_ALL) {
      if (DS) Serial.println("Received PAIR message");
      // Set the watchdog to count down and then send a PAIR_ACK.
      sr.setWaiting();
      sr.setTimeout(devPairDelay);
      anchorWatchdog->waitFor(&sr);
      nextExpectedMsg = PAIR_ACK;  // note: we don't actually expect to receive this
    } else {                       // We shouldn't actually ever get a reply to our PAIR_ACK
      if (DS) Serial.println("Got unknown message during pairing.");
      DW::forceTRxOff();
      DW::startReceive();
    }
  }
  // Wait until we are after our designated delay time to reply
  if (sr.getStatus() == TASK_SR_TIMEOUT && nextExpectedMsg == PAIR_ACK) {
    pairResponse();
    return true;
  }

  return false;
}

void AnchorReceiveHandler() {
  // Distguinish between packet types
  // This should work with the TwoWayRangingInitiator example!
  receivedMsg = true;
}

void AnchorSentHandler() {
  if (data[0] == POLL_ACK) {
    timePollAckSent = DW::getTransmitTimestamp();
  } 

  // Start a "deeper" reset timer if this is a range report
  // E,g. we expect another poll request eventually
  if (nextExpectedMsg == POLL) {
    sr.setWaiting();
    sr.setTimeout(deviceRetryTimeout);
    anchorWatchdog->waitFor(&sr);
  }
  // This should fix errors causes by a bad connection or a
  // stalled DW1000.


  DW::forceTRxOff();
  DW::startReceive();
  if (DS) Serial.println("Done transmitting");
}

void pairResponse() {
  nextExpectedMsg = 10;
  data[0] = PAIR_ACK;
  data[5] = devAddr;
  DW::setTransmitData(data, LEN_DATA);
  DW::startTransmit();
  if (DS) Serial.println("Sent PAIR ack");
}

// Responsible for handling a POLL
void pollResponse() {
  timePollReceived = DW::getReceiveTimestamp();  // retrieve 64 bit timestamp
  nextExpectedMsg = RANGE;
  data[0] = POLL_ACK;
  DW::setTransmitData(data, LEN_DATA);
  DW::startTransmit();
  if (DS) Serial.println("Sent POLL ack");

  sr.setWaiting();
  sr.setTimeout(7);
  anchorWatchdog->waitFor(&sr);
}

// Handles the response to a RANGE message from the tag
void rangeResponse() {
  timeRangeReceived = DW::getReceiveTimestamp();
  nextExpectedMsg = POLL;

  timePollSent = DW1000NgUtils::bytesAsValue(data + 1, LENGTH_TIMESTAMP);
  timePollAckReceived = DW1000NgUtils::bytesAsValue(data + 6, LENGTH_TIMESTAMP);
  timeRangeSent = DW1000NgUtils::bytesAsValue(data + 11, LENGTH_TIMESTAMP);
  // (re-)compute range as two-way ranging is done
  double distance = DW1000NgRanging::computeRangeAsymmetric(timePollSent,
                                                            timePollReceived,
                                                            timePollAckSent,
                                                            timePollAckReceived,
                                                            timeRangeSent,
                                                            timeRangeReceived);
  /* Apply simple bias correction */
  distance = DW1000NgRanging::correctRange(distance);
  uint16_t dist2 = distance * 1000.0;
  if (dist2 > 10000) {
    dist2 = 1234;
    distance = 1234;
  }

  String rangeString = "Range: ";
  rangeString += distance;
  rangeString += " m";
  rangeString += "\t RX power: ";
  rangeString += DW1000Ng::getReceivePower();
  rangeString += " dBm";
  if (DS) Serial.println(rangeString);

  distance = distance * DISTANCE_OF_RADIO_INV;
  data[0] = RANGE_REPORT;
  // write final ranging result
  //memcpy(data + 1, &distance, 4);
  memcpy(data + 1, &dist2, 2);
  data[5] = devAddr;
  DW1000Ng::setTransmitData(data, LEN_DATA);
  DW1000Ng::startTransmit();

  sr.setWaiting();
  sr.setTimeout(7);
  anchorWatchdog->waitFor(&sr);
  digitalWrite(PAIR_LED, HIGH);
}

void anchorWatchdogCallback() {
  //if (DS) Serial.println("Watchdog fired");
  return;
}


void checkSPI() {
  if (DW::checkDeviceId()) {
    if (DS) Serial.println("SPI seems to work");
  } else {
    if (DS) Serial.println("SPI error - unable to talk to DW1000");
  }
}
