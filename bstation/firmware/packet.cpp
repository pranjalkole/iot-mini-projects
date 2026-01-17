#include "packet.h"

// CRC-16/CCITT polynomial (0x1021) - commonly used in LoRa/LoRaWAN
static const uint16_t CRC16_POLY = 0x1021;
static const uint16_t CRC16_INIT = 0xFFFF;

uint16_t calculateCRC16(const uint8_t *data, uint8_t length) {
  uint16_t crc = CRC16_INIT;

  for (uint8_t i = 0; i < length; i++) {
    crc ^= ((uint16_t)data[i] << 8);
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x8000) {
        crc = (crc << 1) ^ CRC16_POLY;
      } else {
        crc <<= 1;
      }
    }
  }
  return crc;
}

void initPacket(LoRaPacket *pkt, uint16_t deviceId) {
  pkt->version = PACKET_VERSION;
  pkt->deviceId = deviceId;
  pkt->sequence = 0;
  pkt->uptime = 0;
  pkt->temperature = 0;
  pkt->humidity = 0;
  pkt->pressure = 0;
  pkt->gas = 0;
  pkt->crc = 0;
}

void populatePacket(LoRaPacket *pkt, uint16_t sequence, uint32_t uptimeSec,
                    int32_t temp, int32_t humidity, int32_t pressure,
                    int32_t gas) {
  pkt->sequence = sequence;
  pkt->uptime = uptimeSec;

  // Temperature: already in °C × 100, fits in int16_t
  pkt->temperature = (int16_t)temp;

  // Humidity: convert from ÷1000 to ÷100 (e.g., 65432 → 6543)
  pkt->humidity = (uint16_t)(humidity / 10);

  // Pressure: convert from Pa÷1000 (kPa) to Pa÷10
  // Input: 101325 (meaning 101.325 kPa = 101325 Pa)
  // Output: 1013250 (meaning 101325.0 Pa)
  pkt->pressure = (uint32_t)(pressure * 10) & 0xFFFFFF; // Mask to 24 bits

  // Gas: use raw value (already ÷100), mask to 24 bits
  pkt->gas = (uint32_t)gas & 0xFFFFFF;
}

uint8_t encodePacket(const LoRaPacket &pkt, uint8_t *buffer) {
  uint8_t idx = 0;

  // Version (1 byte)
  buffer[idx++] = pkt.version;

  // Device ID (2 bytes, Big Endian)
  buffer[idx++] = (pkt.deviceId >> 8) & 0xFF;
  buffer[idx++] = pkt.deviceId & 0xFF;

  // Sequence (2 bytes, Big Endian)
  buffer[idx++] = (pkt.sequence >> 8) & 0xFF;
  buffer[idx++] = pkt.sequence & 0xFF;

  // Uptime (4 bytes, Big Endian)
  buffer[idx++] = (pkt.uptime >> 24) & 0xFF;
  buffer[idx++] = (pkt.uptime >> 16) & 0xFF;
  buffer[idx++] = (pkt.uptime >> 8) & 0xFF;
  buffer[idx++] = pkt.uptime & 0xFF;

  // Temperature (2 bytes, Big Endian, signed)
  buffer[idx++] = (pkt.temperature >> 8) & 0xFF;
  buffer[idx++] = pkt.temperature & 0xFF;

  // Humidity (2 bytes, Big Endian)
  buffer[idx++] = (pkt.humidity >> 8) & 0xFF;
  buffer[idx++] = pkt.humidity & 0xFF;

  // Pressure (3 bytes, Big Endian)
  buffer[idx++] = (pkt.pressure >> 16) & 0xFF;
  buffer[idx++] = (pkt.pressure >> 8) & 0xFF;
  buffer[idx++] = pkt.pressure & 0xFF;

  // Gas (3 bytes, Big Endian)
  buffer[idx++] = (pkt.gas >> 16) & 0xFF;
  buffer[idx++] = (pkt.gas >> 8) & 0xFF;
  buffer[idx++] = pkt.gas & 0xFF;

  // Calculate CRC over all data bytes (excluding CRC field itself)
  uint16_t crc = calculateCRC16(buffer, idx);

  // CRC-16 (2 bytes, Big Endian)
  buffer[idx++] = (crc >> 8) & 0xFF;
  buffer[idx++] = crc & 0xFF;

  return idx; // Should be PACKET_SIZE (21)
}

/**
 * @brief Decode byte buffer to packet structure
 */
bool decodePacket(const uint8_t *buffer, uint8_t size, LoRaPacket &pkt) {
  if (size < PACKET_SIZE) {
    return false;
  }

  uint8_t idx = 0;

  // Version
  pkt.version = buffer[idx++];

  // Check version compatibility
  if (pkt.version != PACKET_VERSION) {
    return false;
  }

  // Device ID (Big Endian)
  pkt.deviceId = ((uint16_t)buffer[idx] << 8) | buffer[idx + 1];
  idx += 2;

  // Sequence (Big Endian)
  pkt.sequence = ((uint16_t)buffer[idx] << 8) | buffer[idx + 1];
  idx += 2;

  // Uptime (Big Endian)
  pkt.uptime = ((uint32_t)buffer[idx] << 24) |
               ((uint32_t)buffer[idx + 1] << 16) |
               ((uint32_t)buffer[idx + 2] << 8) | buffer[idx + 3];
  idx += 4;

  // Temperature (Big Endian, signed)
  pkt.temperature = (int16_t)(((uint16_t)buffer[idx] << 8) | buffer[idx + 1]);
  idx += 2;

  // Humidity (Big Endian)
  pkt.humidity = ((uint16_t)buffer[idx] << 8) | buffer[idx + 1];
  idx += 2;

  // Pressure (Big Endian, 3 bytes)
  pkt.pressure = ((uint32_t)buffer[idx] << 16) |
                 ((uint32_t)buffer[idx + 1] << 8) | buffer[idx + 2];
  idx += 3;

  // Gas (Big Endian, 3 bytes)
  pkt.gas = ((uint32_t)buffer[idx] << 16) | ((uint32_t)buffer[idx + 1] << 8) |
            buffer[idx + 2];
  idx += 3;

  // CRC (Big Endian)
  pkt.crc = ((uint16_t)buffer[idx] << 8) | buffer[idx + 1];

  // Verify CRC
  uint16_t calculatedCrc = calculateCRC16(buffer, PACKET_SIZE - 2);
  if (calculatedCrc != pkt.crc) {
    return false;
  }

  return true;
}

/**
 * @brief Print packet contents to Serial for debugging
 */
void printPacket(const LoRaPacket &pkt) {
  Serial.println("=== LoRa Packet ===");
  Serial.printf("  Version:     0x%02X\n", pkt.version);
  Serial.printf("  Device ID:   0x%04X (%u)\n", pkt.deviceId, pkt.deviceId);
  Serial.printf("  Sequence:    %u\n", pkt.sequence);
  Serial.printf("  Uptime:      %u sec (%02d:%02d:%02d)\n", pkt.uptime,
                pkt.uptime / 3600, (pkt.uptime % 3600) / 60, pkt.uptime % 60);
  Serial.printf("  Temperature: %d.%02d °C\n", pkt.temperature / 100,
                abs(pkt.temperature % 100));
  Serial.printf("  Humidity:    %u.%02u %%\n", pkt.humidity / 100,
                pkt.humidity % 100);
  Serial.printf("  Pressure:    %u.%u Pa (%u.%03u kPa)\n", pkt.pressure / 10,
                pkt.pressure % 10, pkt.pressure / 10000,
                (pkt.pressure / 10) % 1000);
  Serial.printf("  Gas:         %u ohms\n", pkt.gas);
  Serial.printf("  CRC:         0x%04X\n", pkt.crc);
  Serial.println("===================");
}
