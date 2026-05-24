/*
    FXLS8971CF_Driver.cpp

    Minimal FXLS8971CF driver for the SQMD thesis project.

    This driver relies on the Arduino Wire.h library for I2C communication.
*/

#include "FXLS8971CF_Driver.h"
#include <Wire.h>

// register map from the datasheet
#define BUF_STATUS 0x0B
#define BUF_X_LSB 0x0C // -> 0x11
#define WHO_AM_I 0x13
#define SENS_CONFIG1 0x15
#define SENS_CONFIG3 0x17
#define SENS_CONFIG5 0x19
#define INT_EN 0x20
#define BUF_CONFIG1 0x26
#define SDCD_CONFIG1 0x2F
#define SDCD_CONFIG2 0x30
#define SDCD_LTHS_LSB 0x33
#define SDCD_LTHS_MSB 0x34
#define SDCD_UTHS_LSB 0x35
#define SDCD_UTHS_MSB 0x36
/*  Other registers are left at their defaults (after reset):
SENS_CONFIG2
    - wake and sleep power mode: low-power
    - output mode: little-endian
    - read mode: normal
SENS_CONFIG4
    - auto-wake/sleep transition sources disabled
    - INTx output driver: push-pull
    - interrupt logic polarity: active high (asserted = 1)
WAKE/SLEEP_IDLE
    = 0; irrelevant since flexible performance mode is not selected
ASLP_COUNT
    = 0; this essentially disables the SLEEP mode
INT_PIN_SEL
    - all interrupts are routed to INT1 by default
OFF_X/Y/Z
    = 0; no offset correction applied to the raw acceleration data
BUF_CONFIG2
    - buffer flush functionality is unused
    - wake-to-sleep transition is irrelevant since SLEEP mode is effectively disabled
    - BUF_WMRK = 0, watermark event generation is effectively disabled
ORIENT_{ANY}
    - orientation detection registers are irrelevant since
      orientation detection is not used
SDCD_OT/WT_DBCNT
    = 0; debounce is not used in this implementation -
         SDCD events are triggered as soon as the
         vector magnitude crosses the threshold
SELF_TEST_CONFIG{X}
    - self-test is not used in this implementation;
      all self-test bits are left at their default values
*/

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
    /*  Writing 0x22 to the SENS_CONFIG3 register:
        - sets ODR = 800 Hz  */
    if (!writeReg(SENS_CONFIG3, 0x22))
    {
        return false;
    }

    /*  Writing 0x10 to the SENS_CONFIG5 register:
        - sets VECM_EN = 1, enabling vector magnitude calculation
        - keeps X/Y/Z-axis auto-increment enabled (for buffered mode as well)
        - keeps Hibernate mode disabled  */
    if (!writeReg(SENS_CONFIG5, 0x10))
    {
        return false;
    }

    /*  Writing 0x22 to the INT_EN register:
        - enables SDCD outside of thresholds interrupting
        - disables BOOT interrupt output, since it is not used in this implementation
        - keeps other interrupt outputs disabled by default  */
    if (!writeReg(INT_EN, 0x22))
    {
        return false;
    }

    /*  Writing 0x20 to the BUF_CONFIG1 register:
        - sets the buffer data collection mode to stream
        - keeps the FIFO data read out order
        - keeps trigger sources disabled (irrelevant for stream buffer)  */
    if (!writeReg(BUF_CONFIG1, 0x20))
    {
        return false;
    }

    /*  Writing 0x20 to the SDCD_CONFIG1 register:
        - sets X_OT_EN = 1; X-axis channel is required for vector magnitude comparison (SDCD_OT function)
        - keeps the Y/Z_OT_EN bits disabled, since they are not needed for vector magnitude comparison
        - keeps all X/Y/Z_WT_EN bits disabled, so SDCD_WT function is effectively disabled
        - keeps event flag latching disabled  */
    if (!writeReg(SDCD_CONFIG1, 0x20))
    {
        return false;
    }

    /*  Writing 0xC2 to the SDCD_CONFIG2 register:
        - enables the SDCD function
        - sets the SDCD reference value upd mode to previous sample
        - uses vector magnitude data for the SDCD function
        - keeps the default debounce counter behavior (irrelevant - debounce is not used)  */
    if (!writeReg(SDCD_CONFIG2, 0xC2))
    {
        return false;
    }

    /*  Writing 12-bit signed values to the SDCD threshold registers  */
    uint16_t lower_threshold = gToNegativeS12(SDCD_EVENT_THRESHOLD_G);
    uint16_t upper_threshold = gToPositiveS12(SDCD_EVENT_THRESHOLD_G);
    if (!writeReg(SDCD_LTHS_LSB, lower_threshold & 0xFF))
    {
        return false;
    }
    if (!writeReg(SDCD_LTHS_MSB, (lower_threshold >> 8) & 0x0F))
    {
        return false;
    }
    if (!writeReg(SDCD_UTHS_LSB, upper_threshold & 0xFF))
    {
        return false;
    }
    if (!writeReg(SDCD_UTHS_MSB, (upper_threshold >> 8) & 0x0F))
    {
        return false;
    }

    /*  Writing 0x07 to the SENS_CONFIG1 register:
        - sets FSR[1:0] = 11b; +-16 g; 7.81 mg/LSB (128 LSB/g) nominal sensitivity
        - sets ACTIVE = 1, placing the device into Active mode
        - keeps self-test and SPI mode bits disabled  */
    return writeReg(SENS_CONFIG1, 0x07);
}

bool FXLS8971CF::readBufCount(uint8_t &count)
{
    uint8_t status;

    if (!readReg(BUF_STATUS, status))
    {
        return false;
    }

    count = status & 0x3F;
    return true;
}

bool FXLS8971CF::readBuffer(float *xSamples, float *ySamples, float *zSamples, uint8_t numSamples)
{
    if ((xSamples == NULL) || (ySamples == NULL) || (zSamples == NULL) || (numSamples == 0) || (numSamples > 32))
    {
        return false;
    }

    uint8_t byteBuffer[6 * numSamples];

    if (!readRegs(BUF_X_LSB, byteBuffer, 6 * numSamples))
    {
        return false;
    }

    for (int i = 0; i < numSamples; i++)
    {
        int16_t rawX = (byteBuffer[6 * i + 1] << 8) | byteBuffer[6 * i];
        int16_t rawY = (byteBuffer[6 * i + 3] << 8) | byteBuffer[6 * i + 2];
        int16_t rawZ = (byteBuffer[6 * i + 5] << 8) | byteBuffer[6 * i + 4];

        xSamples[i] = ((float)rawX) / 128.0f;
        ySamples[i] = ((float)rawY) / 128.0f;
        zSamples[i] = ((float)rawZ) / 128.0f;
    }

    return true;
}

/*  Writing 0x80 to the SENS_CONFIG1 register triggers a soft reset.
    The RST bit (bit 7) is self-clearing after reset completion. */
bool FXLS8971CF::reset()
{
    bool res = writeReg(SENS_CONFIG1, 0x80);
    delay(1); // TBOOT1 max
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

uint16_t FXLS8971CF::gToPositiveS12(float g, float sensitivity)
{
    float scaled = g * sensitivity;

    if (scaled <= 0.0f)
    {
        return 0x0000;
    }
    if (scaled >= 2047.0f)
    {
        return 0x07FF;
    }

    return (uint16_t)scaled;
}

uint16_t FXLS8971CF::gToNegativeS12(float g, float sensitivity)
{
    uint16_t positive = gToPositiveS12(g, sensitivity);

    return ((~positive) + 1) & 0x0FFF;
}