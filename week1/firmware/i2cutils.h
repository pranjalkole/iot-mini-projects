#ifndef I2CUTILS_H_
#define I2CUTILS_H_

#include <Wire.h>

void listI2Cdevices();
void listI2Cdevices(TwoWire &wire);

#endif // I2CUTILS_H_
