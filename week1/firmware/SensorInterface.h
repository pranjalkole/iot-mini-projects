#ifndef SENSORINTERFACE_H_
#define SENSORINTERFACE_H_

#include "BME680.h"

/* default pins for I2C interface */
#define DEFAULT_BME680_SDA 41
#define DEFAULT_BME680_SCL 42

class SensorInterface {
  private:
    uint8_t sda;
    uint8_t scl;
    BME680_Class bme680;

  public:
    SensorInterface() {}
    SensorInterface(uint8_t sda, uint8_t scl) : sda(sda), scl(scl) {}
    ~SensorInterface() {}
    bool init();
    int8_t getSensorData(int32_t &temp, int32_t &humidity, int32_t &pressure,
                         int32_t &gas);
};

#endif // SENSORINTERFACE_H_
