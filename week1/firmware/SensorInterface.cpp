#include "SensorInterface.h"

bool SensorInterface::init() {
    if (!bme680.begin(I2C_STANDARD_MODE))
        return false;
    bme680.setOversampling(TemperatureSensor, Oversample16);
    bme680.setOversampling(PressureSensor, Oversample16);
    bme680.setOversampling(HumiditySensor, Oversample16);
    bme680.setIIRFilter(IIR4);
    bme680.setGas(320, 150);
    return true;
}

int8_t SensorInterface::getSensorData(int32_t &temp, int32_t &humidity,
                                      int32_t &pressure, int32_t &gas) {
    return bme680.getSensorData(temp, humidity, pressure, gas);
}
