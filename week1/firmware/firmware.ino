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
static void on_rx_done(uint8_t *payload, uint16_t size, int16_t rssi,
                       int8_t snr) {
  lastRssi = rssi;
  lastSNR = snr;
  Serial.printf("A frame sized %d was received.\n", size);
}

void setup() {
  Serial.begin(BAUD);

  // delay(5000); // TODO: remove this in deployment (in place just to wait for
  //  serial monitor to connect)

  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

  Serial.print("Connecting to the sensor...");
  while (!sensorInterface.init()) {
    Serial.println("- Trying to connect to the Sensor in 5 minutes.");
    delay(5000);
  }
  Serial.println("(done)");

  Serial.print("Initializing oledDisplay...");
  oledDisplay.init(); // initialize the display
  Serial.println("(done)");
  oledDisplay.info(); // show the info of the display pins in the serial
  Serial.print("Setting radio configurations...");
  /* set callbacks for radio events */
  radioEvents.TxDone = on_tx_done;
  radioEvents.TxTimeout = on_tx_timeout;
  radioEvents.RxDone = on_rx_done;

  /* configure the LoRa Radio Object */
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

static uint8_t counter = 0;
static char buffer[64];

static void showTime() {
  uint32_t uptime = millis() / 1000; // seconds since boot
  uint32_t hours = uptime / 3600;
  uint32_t minutes = (uptime % 3600) / 60;
  uint32_t seconds = uptime % 60;

  sprintf(buffer, "U: %02d:%02d:%02d", hours, minutes, seconds);
  oledDisplay.write(5, 3, buffer);
}

/* Dashboard Network element */
static void drawBanner() { oledDisplay.write(0, 0, " ENV MONITOR "); }

static void drawNetwork() {
  sprintf(buffer, "RSSI:%d | SNR: %d", lastRssi, lastSNR);
  oledDisplay.write(0, 4, buffer);
}

static void drawSensors(int32_t t, int32_t h, int32_t p, int32_t g) {
  sprintf(buffer, "T:%d.%02dC | H:%d.%03d%%", t / 100, abs(t % 100), h / 1000,
          abs(h % 1000));
  oledDisplay.write(0, 1, buffer);

  sprintf(buffer, "P:%d.%03dkPa", p / 1000, abs(p % 1000));
  oledDisplay.write(0, 2, buffer);

  sprintf(buffer, "G:%d.%02d", g / 100, abs(g % 100));
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

void loop() {
  static int32_t temp, humidity, pressure, gas;

  sensorInterface.getSensorData(temp, humidity, pressure, gas);
  renderDashboard(temp, humidity, pressure, gas);

  populatePacket(&packet, packetSequence++, millis() / 1000, temp, humidity,
                 pressure, gas);

  uint8_t len = encodePacket(packet, txBuffer);
  Radio.Send(txBuffer, len);

  // Debug output
  printPacket(packet);

  Radio.IrqProcess();
  delay(1000);
}
