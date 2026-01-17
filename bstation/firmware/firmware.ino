/**
 * @file receiver.ino
 * @brief LoRa receiver for environmental sensor packets
 *
 * Receives packets from sensor nodes and displays data on OLED + Serial
 */

#include "display.h"
#include "lora_config.h"
#include "packet.h"
#include <Arduino.h>
#include <LoRaWan_APP.h>

#define BAUD 115200

static OledDisplay oledDisplay;
static RadioEvents_t radioEvents;

static LoRaPacket rxPacket;
static uint8_t rxBuffer[PACKET_SIZE + 10]; // Extra buffer space
static uint16_t rxSize = 0;
static bool packetReceived = false;

static int16_t lastRssi = 0;
static int8_t lastSnr = 0;
static uint32_t packetsReceived = 0;
static uint32_t packetsError = 0;

// Radio event callbacks
static void onTxDone() {
  // Not used in receiver mode
}

static void onTxTimeout() {
  // Not used in receiver mode
}

static void onRxDone(uint8_t *payload, uint16_t size, int16_t rssi,
                     int8_t snr) {
  lastRssi = rssi;
  lastSnr = snr;
  rxSize = size;

  if (size <= sizeof(rxBuffer)) {
    memcpy(rxBuffer, payload, size);
    packetReceived = true;
  }

  // Immediately go back to receive mode
  Radio.Rx(0);
}

static void onRxTimeout() {
  // Continue listening
  Radio.Rx(0);
}

static void onRxError() {
  packetsError++;
  Serial.println("[RX] CRC Error detected");
  Radio.Rx(0);
}

void setup() {
  Serial.begin(BAUD);

  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

  Serial.println("LoRa Receiver");
  Serial.println("Initializing...");

  // Initialize display
  Serial.print("Initializing OLED...");
  oledDisplay.init();
  Serial.println("(done)");

  // Configure radio callbacks
  radioEvents.TxDone = onTxDone;
  radioEvents.TxTimeout = onTxTimeout;
  radioEvents.RxDone = onRxDone;
  radioEvents.RxTimeout = onRxTimeout;
  radioEvents.RxError = onRxError;

  /* Initialize radio*/
  Serial.print("Configuring LoRa radio...");
  Radio.Init(&radioEvents);
  Radio.SetChannel(RF_FREQUENCY);

  Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                    LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                    LORA_FIX_LENGTH_PAYLOAD_ON, 0, true, 0,
                    0, LORA_IQ_INVERSION_ON, true, RX_TIMEOUT_VALUE);

  Serial.println("(done)");

  oledDisplay.clear();
  oledDisplay.write(0, 0, " LORA RECEIVER ");
  oledDisplay.write(0, 2, "Waiting for data...");
  oledDisplay.commit();

  /*start receiving*/
  Radio.Rx(0);
  Serial.println("Listening for packets...\n");
}

static char buffer[64];

static void drawReceiverDashboard() {
  oledDisplay.clear();

  // Header with packet count
  sprintf(buffer, "RX #%lu", packetsReceived);
  oledDisplay.write(0, 0, buffer);

  // Device info
  sprintf(buffer, "Dev:0x%04X Seq:%u", rxPacket.deviceId, rxPacket.sequence);
  oledDisplay.write(0, 1, buffer);

  // Temperature & Humidity
  sprintf(buffer, "T:%d.%02dC H:%u.%02u%%", rxPacket.temperature / 100,
          abs(rxPacket.temperature % 100), rxPacket.humidity / 100,
          rxPacket.humidity % 100);
  oledDisplay.write(0, 2, buffer);

  // Pressure
  sprintf(buffer, "P:%u.%ukPa", rxPacket.pressure / 10000,
          (rxPacket.pressure / 10) % 1000);
  oledDisplay.write(0, 3, buffer);

  // Signal quality
  sprintf(buffer, "RSSI:%d SNR:%d", lastRssi, lastSnr);
  oledDisplay.write(0, 4, buffer);

  oledDisplay.commit();
}

static void drawWaitingScreen() {
  static uint8_t dots = 0;
  oledDisplay.clear();

  oledDisplay.write(0, 0, " LORA RECEIVER ");

  sprintf(buffer, "Waiting%.*s", (dots % 4) + 1, "....");
  oledDisplay.write(0, 2, buffer);

  sprintf(buffer, "Pkts:%lu Err:%lu", packetsReceived, packetsError);
  oledDisplay.write(0, 4, buffer);

  oledDisplay.commit();
  dots++;
}

void loop() {
  Radio.IrqProcess();

  if (packetReceived) {
    packetReceived = false;

    Serial.printf("\n[RX] Received %u bytes | RSSI: %d | SNR: %d\n", rxSize,
                  lastRssi, lastSnr);

    // Try to decode the packet
    if (decodePacket(rxBuffer, rxSize, rxPacket)) {
      packetsReceived++;

      Serial.println("[OK] Packet decoded successfully:");
      printPacket(rxPacket);

      // Update display with received data
      drawReceiverDashboard();
    } else {
      packetsError++;
      Serial.println("[ERR] Failed to decode packet (invalid CRC or format)");

      // Print raw bytes for debugging
      Serial.print("Raw bytes: ");
      for (uint16_t i = 0; i < rxSize; i++) {
        Serial.printf("%02X ", rxBuffer[i]);
      }
      Serial.println();
    }
  } else {
    // Show waiting animation if no recent packets
    static uint32_t lastUpdate = 0;
    if (millis() - lastUpdate > 500) {
      lastUpdate = millis();
      if (packetsReceived == 0) {
        drawWaitingScreen();
      }
    }
  }

  delay(10); //
}
