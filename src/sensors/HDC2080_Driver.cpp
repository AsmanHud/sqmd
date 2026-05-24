/*
    HDC2080_Driver.cpp

    Minimal HDC2080 driver for the SQMD thesis project.

    This implementation was developed using the following repository as reference:

    https://github.com/lime-labs/HDC2080-Arduino

    Modified and simplified for the requirements of this project.

    This driver relies on the Arduino Wire.h library for I2C communication.
*/

#include "HDC2080_Driver.h"
#include <Wire.h>

// register map from the datasheet
#define TEMP_LOW 0x00
#define TEMP_HIGH 0x01
#define HUMID_LOW 0x02
#define HUMID_HIGH 0x03
#define INT_DRDY 0x04
#define INTERRUPT_CONFIG 0x07
#define CONFIG 0x0E
#define MEASUREMENT_CONFIG 0x0F

// public implementation

HDC2080::HDC2080(uint8_t addr)
{
    _addr = addr;
}

bool HDC2080::isConnected()
{
    if ((_addr == 0x40) || (_addr == 0x41))
    {
        Wire.beginTransmission(_addr);
        return (Wire.endTransmission() == 0);
    }

    return false;
}

/*  Writing 0x04 to the CONFIG register:
    - enables the DRDY/INT output pin
    - keeps manual measurement mode
    - keeps heater disabled
    - keeps active-low interrupt polarity
    - keeps level-sensitive interrupt mode
*/
bool HDC2080::enableInterruptPin()
{
    return writeReg(CONFIG, 0x04);
}

/*  Writing 0x80 to the INTERRUPT_CONFIG register:
    - enables only the DataReady interrupt generator
    - keeps all temperature/humidity threshold interrupt generators disabled
    - keeps reserved bits at 0
*/
bool HDC2080::enableDataReadyInterrupt()
{
    return writeReg(INTERRUPT_CONFIG, 0x80);
}

bool HDC2080::readDRDYFlag(bool &dataReady)
{
    uint8_t data;
    if (!readReg(INT_DRDY, data))
    {
        return false;
    }

    dataReady = (data & 0x80) != 0;
    return true;
}

bool HDC2080::readTemperature(float &t_c)
{
    uint8_t byte[2];
    uint16_t temp;

    if (!readReg(TEMP_LOW, byte[0]))
    {
        return false;
    }
    if (!readReg(TEMP_HIGH, byte[1]))
    {
        return false;
    }

    temp = byte[1];
    temp = (temp << 8) | byte[0];
    t_c = temp;
    t_c = ((t_c * 165.0f) / 65536.0f) - (40.5f + 0.08f * (3.3f - 1.8f));

    return true;
}

bool HDC2080::readHumidity(float &rh)
{
    uint8_t byte[2];
    uint16_t humidity;

    if (!readReg(HUMID_LOW, byte[0]))
    {
        return false;
    }
    if (!readReg(HUMID_HIGH, byte[1]))
    {
        return false;
    }

    humidity = byte[1];
    humidity = (humidity << 8) | byte[0];
    rh = humidity;
    rh = (rh / 65536.0f) * 100.0f;

    return true;
}

/*  Writing 0x01 to the MEASUREMENT_CONFIG register:
    - triggers a measurement (bit 0 set to 1)
    - uses 14-bit temperature resolution
    - uses 14-bit humidity resolution
    - measures both temperature and humidity

    MEAS_TRIG (bit 0) is self-clearing after conversion completion.
*/
bool HDC2080::triggerMeasurement()
{
    return writeReg(MEASUREMENT_CONFIG, 0x01);
}

/*  Writing 0x80 to the CONFIG register triggers a soft reset.
    The SOFT_RES bit (bit 7) is self-clearing after reset completion. */
bool HDC2080::reset()
{
    bool res = writeReg(CONFIG, 0x80);
    delay(50); // Lime Labs recommendation for soft reset duration
    return res;
}

// private implementation

bool HDC2080::readReg(uint8_t reg, uint8_t &data)
{
    Wire.beginTransmission(_addr);

    if (Wire.write(reg) != 1)
    {
        return false;
    }

    // passing false here marks a pending repeated-start transaction
    if (Wire.endTransmission(false) != 0)
    {
        return false;
    }

    // actual i2c transaction is made here
    if (Wire.requestFrom(_addr, (uint8_t)1) != 1)
    {
        return false;
    }

    data = Wire.read();
    return true;
}

bool HDC2080::writeReg(uint8_t reg, uint8_t data)
{
    Wire.beginTransmission(_addr);

    if (Wire.write(reg) != 1)
    {
        return false;
    }

    if (Wire.write(data) != 1)
    {
        return false;
    }

    return (Wire.endTransmission() == 0);
}