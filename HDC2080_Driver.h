/*
    HDC2080_Driver.h

    Minimal HDC2080 driver for the SQMD thesis project.

    This implementation was developed using the following repository as reference:

    https://github.com/lime-labs/HDC2080-Arduino

    Modified and simplified for the requirements of this project.
*/

#ifndef HDC2080_Driver_h
#define HDC2080_Driver_h

#include <Arduino.h>

class HDC2080
{
public:
    HDC2080(uint8_t addr);            // Initialize the HDC2080
    bool isConnected();               // Returns true if connected
    bool readTemperature(float &t_c); // Reads the temperature in degrees C
    bool readHumidity(float &rh);     // Reads the relative humidity
    bool triggerMeasurement();        // Triggers a manual temperature/humidity reading
    bool reset();                     // Triggers a soft reset and reconfigures DRDY interrupt output

    // Temperature & Humidity Resolution, default - 14 bit
    // Measurement mode,                  default - Temperature & Humidity
    // Reading rate,                      default - Manual
    // Interrupt polarity,                default - Active Low
    // Interrupt mode,                    default - Level sensitive
private:
    uint8_t _addr;                            // Address of sensor
    bool enableInterruptPin();                // Enables the interrupt/DRDY pin
    bool enableDataReadyInterrupt();          // Enables data ready interrupt generator
    bool readReg(uint8_t reg, uint8_t &data); // Reads a given register
    bool writeReg(uint8_t reg, uint8_t data); // Writes a byte of data to one register
};

#endif