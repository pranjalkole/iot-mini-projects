/**
 * @file packet.h
 * @brief LoRaWAN packet encoding/decoding for environmental sensor data
 * All multi-byte fields are Big Endian (network byte order)
 */

#ifndef PACKET_H_
#define PACKET_H_

#include <Arduino.h>

#define PACKET_VERSION 0x01
#define PACKET_SIZE 21       // Total packet size in bytes
#define PACKET_HEADER_SIZE 5 // Version + DeviceID + Sequence

/**
 * @brief Structure holding all sensor data for transmission
 */
struct LoRaPacket {
  uint8_t version;     ///< Protocol version (PACKET_VERSION)
  uint16_t deviceId;   ///< Unique device identifier
  uint16_t sequence;   ///< Packet sequence number (0-65535, wraps)
  uint32_t uptime;     ///< Seconds since boot
  int16_t temperature; ///< Temperature in °C × 100 (e.g., 2543 = 25.43°C)
  uint16_t humidity;   ///< Humidity in % × 100 (e.g., 6500 = 65.00%)
  uint32_t
      pressure; ///< Pressure in Pa × 10 (24-bit, e.g., 1013250 = 101325.0 Pa)
  uint32_t gas; ///< Gas resistance in Ohms (24-bit)
  uint16_t crc; ///< CRC-16/CCITT checksum
};

/**
 * @brief Initialize a packet with default values
 * @param pkt Pointer to packet structure
 * @param deviceId Device identifier to use
 */
void initPacket(LoRaPacket *pkt, uint16_t deviceId);

/**
 * @brief Populate packet with current sensor readings
 * @param pkt Pointer to packet structure
 * @param sequence Current sequence number
 * @param uptimeSec Uptime in seconds
 * @param temp Temperature (raw from BME680, ÷100 for °C)
 * @param humidity Humidity (raw from BME680, ÷1000 for %)
 * @param pressure Pressure (raw from BME680, ÷1000 for kPa)
 * @param gas Gas resistance (raw from BME680, ÷100)
 */
void populatePacket(LoRaPacket *pkt, uint16_t sequence, uint32_t uptimeSec,
                    int32_t temp, int32_t humidity, int32_t pressure,
                    int32_t gas);

/**
 * @brief Encode packet to byte buffer for transmission
 * @param pkt Packet structure to encode
 * @param buffer Output buffer (must be at least PACKET_SIZE bytes)
 * @return Number of bytes written (PACKET_SIZE on success)
 */
uint8_t encodePacket(const LoRaPacket &pkt, uint8_t *buffer);

/**
 * @brief Decode byte buffer to packet structure
 * @param buffer Input buffer containing encoded packet
 * @param size Size of input buffer
 * @param pkt Output packet structure
 * @return true if decode successful and CRC valid, false otherwise
 */
bool decodePacket(const uint8_t *buffer, uint8_t size, LoRaPacket &pkt);

/**
 * @brief Calculate CRC-16/CCITT checksum
 * @param data Data buffer
 * @param length Length of data
 * @return 16-bit CRC value
 */
uint16_t calculateCRC16(const uint8_t *data, uint8_t length);

/**
 * @brief Print packet contents to Serial for debugging
 * @param pkt Packet to print
 */
void printPacket(const LoRaPacket &pkt);

#endif // PACKET_H_
