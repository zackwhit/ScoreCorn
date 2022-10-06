#include "AnchorWrap.hpp"

byte data[LEN_DATA];

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

void InitAnchor() {
  // Initialize driver
  DW::initialize(PIN_SS, PIN_IRQ, PIN_RST);

  // Apply configuration
  DW::applyConfiguration(DEFAULT_CONFIG);
  DW::applyInterruptConfiguration(DEFAULT_INTERRUPT_CONFIG);

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

  // Check SPI communication
  // get device ID register and check it matches expected value
  if (DW::checkDeviceId()){
    Serial.println("SPI seems to work");
  } else {
    Serial.println("SPI error - unable to talk to DW1000");
  }

}

void AnchorPair() {
  // Note: Pairing is apperently not entirely necessary!
  // If we do want to pair, however, wait a random amount of
  // time after receiving the blink message, and then respond.

}

void AnchorLoop() {
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
    receivedMsg = false;
    DW::getReceivedData(data, LEN_DATA);
    byte msgId = data[0];
    if (msgId == POLL && data[5] == devAddr) {
      Serial.println("Received POLL message");
      pollResponse();
    } else if (msgId == RANGE) {
      Serial.println("Received RANGE message.");
      rangeResponse();
    } else {
      Serial.println("Got unknown message.");
    }
  }

}

void AnchorReceiveHandler() {
  // Distguinish between packet types
  // This should work with the TwoWayRangingInitiator example!
  receivedMsg = true;
}

void AnchorSentHandler() {
  Serial.println("Done transmitting");
  if (data[0] == POLL_ACK) {
    timePollAckSent = DW::getTransmitTimestamp();
  }

  DW::forceTRxOff();
  DW::startReceive();
}


// Responsible for handling a POLL
void pollResponse(){
  timePollReceived = DW::getReceiveTimestamp(); // retrieve 64 bit timestamp
  nextExpectedMsg = RANGE;
  data[0] = POLL_ACK;
  DW::setTransmitData(data, LEN_DATA);
  DW::startTransmit();
  Serial.println("Sent POLL ack");
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

  String rangeString = "Range: "; rangeString += distance; rangeString += " m";
  rangeString += "\t RX power: "; rangeString += DW1000Ng::getReceivePower(); rangeString += " dBm";
  Serial.println(rangeString);

  distance = distance * DISTANCE_OF_RADIO_INV;
  data[0] = RANGE_REPORT;
  // write final ranging result
  memcpy(data + 1, &distance, 4);
  DW1000Ng::setTransmitData(data, LEN_DATA);
  DW1000Ng::startTransmit();

}


// Wait for a message from the tag
// Timeout after a bit and report status
