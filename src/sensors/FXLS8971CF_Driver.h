/*
    FXLS8971CF_Driver.h

    Minimal FXLS8971CF driver for the SQMD thesis project.
*/

#ifndef FXLS8971CF_Driver_h
#define FXLS8971CF_Driver_h

#include <Arduino.h>

class FXLS8971CF
{
public:
    FXLS8971CF(uint8_t addr);                                  // Initialize the FXLS8971CF
    bool isConnected();                                        // Returns true if connected
    bool configure();                                          // Configures the accelerometer for normal operation
    bool readAcceleration(float &x_g, float &y_g, float &z_g); // Reads acceleration in g
    bool reset();                                              // Triggers a soft reset

    // Full-scale measurement range, default: +-4g; 1.95 mg/LSB (512 LSB/g) nominal sensitivity
private:
    uint8_t _addr;
    bool readRegs(uint8_t startReg, uint8_t *buffer, uint8_t length); // Reads multiple registers
    bool readReg(uint8_t reg, uint8_t &data);                         // Reads a given register
    bool writeReg(uint8_t reg, uint8_t data);                         // Writes a byte of data to one register
};

#endif