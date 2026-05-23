/*
    FXLS8971CF_Driver.cpp

    Minimal FXLS8971CF driver for the SQMD thesis project.

    This driver relies on the Arduino Wire.h library for I2C communication.
*/

#include "FXLS8971CF_Driver.h"
#include <Wire.h>

// register map from the datasheet
#define INT_STATUS 0x00
#define OUT_X_LSB 0x04 // -> 0x09
#define BUF_STATUS 0x0B
#define BUF_X_LSB 0x0C // -> 0x11
#define WHO_AM_I 0x13
#define SENS_CONFIG1 0x15
#define SENS_CONFIG3 0x17

// public implementation

FXLS8971CF::FXLS8971CF(uint8_t addr)
{
    _addr = addr;
}

bool FXLS8971CF::isConnected()
{
    if ((_addr != 0x18) && (_addr != 0x19))
    {
        return false;
    }

    Wire.beginTransmission(_addr);

    if (Wire.endTransmission() != 0)
    {
        return false;
    }

    uint8_t id = 0;

    if (!readReg(WHO_AM_I, id))
    {
        return false;
    }

    return (id == 0x83);
}

bool FXLS8971CF::configure()
{
    /*  Writing 0x07 to the SENS_CONFIG1 register:
        - sets FSR[1:0] = 11b; +-16 g; 7.81 mg/LSB (128 LSB/g) nominal sensitivity
        - sets ACTIVE = 1, placing the device into Active mode
        - keeps self-test and SPI mode bits disabled  */
    if (!writeReg(SENS_CONFIG1, 0x07))
    {
        return false;
    }

    /*  Writing 0x22 to the SENS_CONFIG3 register:
        - sets ODR = 800 Hz  */
    if (!writeReg(SENS_CONFIG3, 0x22))
    {
        return false;
    }
}

bool FXLS8971CF::readAcceleration(float &x_g, float &y_g, float &z_g)
{
    uint8_t data[6];

    if (!readRegs(OUT_X_LSB, data, 6))
    {
        return false;
    }

    int16_t rawX = ((int16_t)data[1] << 8) | data[0];
    int16_t rawY = ((int16_t)data[3] << 8) | data[2];
    int16_t rawZ = ((int16_t)data[5] << 8) | data[4];

    // assuming 7.81 mg/LSB (128 LSB/g) nominal sensitivity is chosen
    x_g = rawX / 128.0f;
    y_g = rawY / 128.0f;
    z_g = rawZ / 128.0f;

    return true;
}

/*  Writing 0x80 to the SENS_CONFIG1 register triggers a soft reset.
    The RST bit (bit 7) is self-clearing after reset completion. */
bool FXLS8971CF::reset()
{
    bool res = writeReg(SENS_CONFIG1, 0x80);
    delay(50);
    return res;
}

// private implementation

bool FXLS8971CF::readRegs(uint8_t startReg, uint8_t *buffer, uint8_t length)
{
    if (buffer == NULL)
    {
        return false;
    }

    Wire.beginTransmission(_addr);

    if (Wire.write(startReg) != 1)
    {
        return false;
    }

    // passing false here marks a pending repeated-start transaction
    if (Wire.endTransmission(false) != 0)
    {
        return false;
    }

    // actual i2c transaction is made here
    if (Wire.requestFrom(_addr, (uint8_t)length) != length)
    {
        return false;
    }

    for (uint8_t i = 0; i < length; i++)
    {
        buffer[i] = Wire.read();
    }

    return true;
}

bool FXLS8971CF::readReg(uint8_t reg, uint8_t &data)
{
    return readRegs(reg, &data, 1);
}

bool FXLS8971CF::writeReg(uint8_t reg, uint8_t data)
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