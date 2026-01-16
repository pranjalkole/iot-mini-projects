#ifndef CONFIG_H_
#define CONFIG_H_

// ================= OLED PINS (HELTEC V3) =================
#define OLED_SDA 17
#define OLED_SCL 18
#define OLED_RST 21
#define Vext 36

// ================= BME680 CONFIG ===============
#define BME_SDA 41
#define BME_SCL 42

// ================= LORA CONFIG =================
#define RF_FREQUENCY 915000000 // change to 865000000 for India
#define TX_OUTPUT_POWER 14
#define LORA_BANDWIDTH 0
#define LORA_SPREADING_FACTOR 7
#define LORA_CODINGRATE 1
#define LORA_PREAMBLE_LENGTH 8
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false
#define RX_TIMEOUT_VALUE 1000
#define BUFFER_SIZE 64

#endif // CONFIG_H_
