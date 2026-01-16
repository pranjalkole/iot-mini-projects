#include "SensorInterface.h"
#include "display.h"
#include "lora_config.h"
#include "packet.h"
#include <Arduino.h>
#include <LoRaWan_APP.h>

#define BAUD 115200
#define DEVICE_ID 0xA109

static OledDisplay oledDisplay;
static RadioEvents_t radioEvents;
static SensorInterface sensorInterface;
static LoRaPacket packet;
static uint8_t txBuffer[PACKET_SIZE];

static int16_t lastRssi = 0;
static int8_t lastSNR = 0;
static uint16_t packetSequence = 0;

static void on_tx_done() { Radio.Rx(0); }
static void on_tx_timeout() { Radio.Rx(0); }

static void on_rx_done(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  lastRssi = rssi;
  lastSNR = snr;
  Serial.printf("A frame sized %d was received.\n", size);
}

static char buffer[64];

void setup() {
  Serial.begin(BAUD);
  delay(200);

  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

  // ✅ SENSOR INIT (NON-BLOCKING)
  Serial.print("Initializing sensors...");
  bool sensorOK = sensorInterface.init();

  if (sensorOK) {
    Serial.println("(done)");
  } else {
    Serial.println("\n[WARN] Sensor init returned false. Continuing anyway...");
    Serial.println("[WARN] MQ135 should still work even if BME680 is missing.");
  }

  // OLED init
  Serial.print("Initializing oledDisplay...");
  oledDisplay.init();
  Serial.println("(done)");
  oledDisplay.info();

  Serial.print("Setting radio configurations...");

  // set callbacks for radio events
  radioEvents.TxDone = on_tx_done;
  radioEvents.TxTimeout = on_tx_timeout;
  radioEvents.RxDone = on_rx_done;

  // configure the LoRa Radio Object
  Radio.Init(&radioEvents);
  Radio.SetChannel(RF_FREQUENCY);

  Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                    LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                    LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON, true, 0,
                    0, LORA_IQ_INVERSION_ON, 3000);

  Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                    LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                    LORA_FIX_LENGTH_PAYLOAD_ON, 0, true, 0, 0,
                    LORA_IQ_INVERSION_ON, true, RX_TIMEOUT_VALUE);

  Serial.println("(done)");

  Radio.Rx(0);

  delay(2000);
  oledDisplay.clear();
  oledDisplay.commit();

  initPacket(&packet, DEVICE_ID);
}

// ---- OLED Helpers ----

static void showTime() {
  uint32_t uptime = millis() / 1000;
  uint32_t hours = uptime / 3600;
  uint32_t minutes = (uptime % 3600) / 60;
  uint32_t seconds = uptime % 60;

  snprintf(buffer, sizeof(buffer), "U: %02lu:%02lu:%02lu",
           (unsigned long)hours, (unsigned long)minutes, (unsigned long)seconds);
  oledDisplay.write(5, 3, buffer);
}

static void drawBanner() {
  oledDisplay.write(0, 0, " ENV MONITOR ");
}

static void drawNetwork() {
  snprintf(buffer, sizeof(buffer), "RSSI:%d | SNR:%d", lastRssi, lastSNR);
  oledDisplay.write(0, 4, buffer);
}

// ✅ Updated sensor render for MQ135 raw gas
static void drawSensors(int32_t t, int32_t h, int32_t p, int32_t g) {
  // BME680 values (or 0 if missing)
  snprintf(buffer, sizeof(buffer), "T:%ld.%02ldC | H:%ld.%02ld%%",
           (long)(t / 100), (long)abs(t % 100),
           (long)(h / 100), (long)abs(h % 100));
  oledDisplay.write(0, 1, buffer);

  snprintf(buffer, sizeof(buffer), "P:%ld.%03ldkPa",
           (long)(p / 1000), (long)abs(p % 1000));
  oledDisplay.write(0, 2, buffer);

  // MQ135 raw
  snprintf(buffer, sizeof(buffer), "MQ135 Raw:%ld", (long)g);
  oledDisplay.write(0, 3, buffer);
}

static void renderDashboard(int32_t t, int32_t h, int32_t p, int32_t g) {
  oledDisplay.clear();
  drawBanner();
  drawSensors(t, h, p, g);
  drawNetwork();
  showTime();
  oledDisplay.commit();
}

// ---- Main Loop ----

void loop() {
  static int32_t temp = 0, humidity = 0, pressure = 0, gas = 0;

  // ✅ Read sensors (works even if BME680 missing)
  sensorInterface.getSensorData(temp, humidity, pressure, gas);

  // OLED display
  renderDashboard(temp, humidity, pressure, gas);

  // Packet send
  populatePacket(&packet, packetSequence++, millis() / 1000,
                 temp, humidity, pressure, gas);

  uint8_t len = encodePacket(packet, txBuffer);
  Radio.Send(txBuffer, len);

  // Debug output
  Serial.println("--------------- TX PACKET ---------------");
  printPacket(packet);

  Radio.IrqProcess();
  delay(1000);
}
