/*
    FXLS8971CF_Driver.h

    Minimal FXLS8971CF driver for the SQMD thesis project.
*/

#ifndef FXLS8971CF_Driver_h
#define FXLS8971CF_Driver_h

#include <Arduino.h>

constexpr float SDCD_EVENT_THRESHOLD_G = 4.0f; // must be positive, max = FSR

class FXLS8971CF
{
public:
    FXLS8971CF(uint8_t addr);          // Initialize the FXLS8971CF
    bool isConnected();                // Returns true if connected
    bool configure();                  // Configures the accelerometer for normal operation
    bool readBufCount(uint8_t &count); // Reads the number of samples currently in the buffer
    // Reads multiple samples from the buffer
    bool readBuffer(float *xSamples,
                    float *ySamples,
                    float *zSamples,
                    uint8_t numSamples);
    // Reads SDCD_INT_SRC1. If the SDCD event is latched,
    // this read clears the latched SDCD source and allows INT1 to deassert.
    bool readSDCDEventFlag(bool &eventActiveFlag,
                           bool &xAxisEventFlag,
                           bool &xAxisPolFlag);
    bool reset(); // Triggers a soft reset

    // FSR = +-16 g; 7.81 mg/LSB (128 LSB/g) nominal sensitivity
    // ODR = 800 Hz; fixed
    // Event detection threshold = SDCD_EVENT_THRESHOLD_G
private:
    uint8_t _addr;
    bool readRegs(uint8_t startReg, uint8_t *buffer, uint8_t length); // Reads multiple registers
    bool readReg(uint8_t reg, uint8_t &data);                         // Reads a given register
    bool writeReg(uint8_t reg, uint8_t data);                         // Writes a byte of data to one register
    uint16_t gToPositiveS12(float g, float sensitivity = 128.0f);
    uint16_t gToNegativeS12(float g, float sensitivity = 128.0f);
};

#endif