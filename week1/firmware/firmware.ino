#include "SensorInterface.h"
#include "display.h"
#include "lora_config.h"
#include <Arduino.h>
#include <LoRaWan_APP.h>

#define BAUD 115200

static OledDisplay oledDisplay;
static RadioEvents_t radioEvents;
static SensorInterface sensorInterface;

static void on_tx_done() { Radio.Rx(0); }
static void on_tx_timeout() { Radio.Rx(0); }
static void on_rx_done(uint8_t *payload, uint16_t size, int16_t rssi,
                       int8_t snr) {
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
}

static uint8_t counter = 0;
static char buffer[64];

static void showTime() {
  uint32_t uptime = millis() / 1000; // seconds since boot
  uint32_t hours = uptime / 3600;
  uint32_t minutes = (uptime % 3600) / 60;
  uint32_t seconds = uptime % 60;

  sprintf(buffer, "U: %02d:%02d:%02d", hours, minutes, seconds);
  oledDisplay.write(2, 0, buffer);
}

void loop() {
  static int32_t temp, humidity, pressure, gas;
  sensorInterface.getSensorData(temp, humidity, pressure, gas);
  oledDisplay.clear();
  showTime(); // show the uptime on the display
  sprintf(buffer, "T: %d.%d", temp / 100, temp % 100);
  oledDisplay.write(0, 1, buffer);
  sprintf(buffer, "H: %d.%d", humidity / 1000, humidity % 1000);
  oledDisplay.write(0, 2, buffer);
  sprintf(buffer, "P: %d.%d", pressure / 1000, pressure % 1000);
  oledDisplay.write(0, 3, buffer);
  sprintf(buffer, "G: %d.%d", gas / 100, gas % 100);
  oledDisplay.write(0, 4, buffer);
  oledDisplay.commit();
  Radio.IrqProcess();
  delay(1000);
}
