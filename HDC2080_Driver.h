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
	HDC2080(uint8_t addr);					          // Initialize the HDC2080

	void begin();						                  // Join I2C bus
	bool isConnected();						            // Returns true if connected

  float readTemp();					                // Returns the temperature in degrees C
  float readHumidity();				              // Returns the relative humidity

  void triggerMeasurement();			          // Triggers a manual temperature/humidity reading
  void reset();						                  // Triggers a software reset

  void enableInterrupt();				            // Enables the interrupt/DRDY pin
  void enableDRDYInterrupt();			          // Enables data ready interrupt generator

  // these defaults are used
  // Temperature & Humidity Resolution, default - 14 bit
  // Measurement mode,                  default - Temperature & Humidity
  // Reading rate,                      default - Manual
  // Interrupt polarity,                default - Active Low
  // Interrupt mode,                    default - Level sensitive
private:
  uint8_t _addr;								            // Address of sensor
  
	void openReg(uint8_t reg);				        // Points to a given register
	uint8_t readReg(uint8_t reg);			        // Reads a given register, returns 1 byte
	void writeReg(uint8_t reg, uint8_t data); // Writes a byte of data to one register
};

#endif