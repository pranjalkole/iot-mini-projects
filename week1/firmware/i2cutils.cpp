#include "i2cutils.h"
#include <Arduino.h>

void listI2Cdevices() {
    unsigned char error, address;
    int devices = 0;

    for (address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0) {
            Serial.print("I2C device found at address 0x");
            if (address < 16)
                Serial.print("0");
            Serial.println(address, HEX);
            devices++;
        }
    }

    if (devices == 0)
        Serial.println("No I2C devices found.\n");
    else
        Serial.println("Scan complete.\n");
}

void listI2Cdevices(TwoWire &wire) {
    unsigned char error, address;
    int devices = 0;

    for (address = 1; address < 127; address++) {
        wire.beginTransmission(address);
        error = wire.endTransmission();

        if (error == 0) {
            Serial.print("I2C device found at address 0x");
            if (address < 16)
                Serial.print("0");
            Serial.println(address, HEX);
            devices++;
        }
    }

    if (devices == 0)
        Serial.println("No I2C devices found.\n");
    else
        Serial.println("Scan complete.\n");
}
