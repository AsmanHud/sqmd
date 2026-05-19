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
#define INTERRUPT_CONFIG 0x07
#define CONFIG 0x0E
#define MEASUREMENT_CONFIG 0x0F

// public implementation

HDC2080::HDC2080(uint8_t addr)
{
	_addr = addr;
}

void HDC2080::begin()
{
	Wire.begin();
}

bool HDC2080::isConnected()
{
	if ((_addr == 0x40) || (_addr == 0x41))
	{
		Wire.beginTransmission(_addr);
		return (Wire.endTransmission() == 0);
	}
	else
	{
		return false;
	}
}

float HDC2080::readTemp()
{
	uint8_t byte[2];
	uint16_t temp;
	byte[0] = readReg(TEMP_LOW);
	byte[1] = readReg(TEMP_HIGH);
	temp = byte[1];
	temp = (temp << 8) | byte[0];
	float f = temp;
	f = ((f * 165.0f) / 65536.0f) - (40.5f + 0.08f * (3.3f - 1.8f));

	return f;
}

float HDC2080::readHumidity()
{
	uint8_t byte[2];
	uint16_t humidity;
	byte[0] = readReg(HUMID_LOW);
	byte[1] = readReg(HUMID_HIGH);
	humidity = byte[1];
	humidity = (humidity << 8) | byte[0];
	float f = humidity;
	f = (f / 65536.0f) * 100.0f;

	return f;
}

/*  Bit 0 of the MEASUREMENT_CONFIG register can be used
	to trigger measurements  */
void HDC2080::triggerMeasurement()
{
	uint8_t configContents;
	configContents = readReg(MEASUREMENT_CONFIG);

	configContents = (configContents | 0x01);
	writeReg(MEASUREMENT_CONFIG, configContents);
}

/*  Bit 7 of the CONFIG register can be used to trigger a
	soft reset  */
void HDC2080::reset()
{
	uint8_t configContents;
	configContents = readReg(CONFIG);

	configContents = (configContents | 0x80);
	writeReg(CONFIG, configContents);
	delay(50);
}

/*  Bit 2 of the CONFIG register can be used to enable/disable
	the interrupt pin  */
void HDC2080::enableInterrupt()
{
	uint8_t configContents;
	configContents = readReg(CONFIG);

	configContents = (configContents | 0x04);
	writeReg(CONFIG, configContents);
}

// enables the interrupt pin generator for DRDY operation
void HDC2080::enableDRDYInterrupt()
{
	uint8_t regContents;
	regContents = readReg(INTERRUPT_CONFIG);

	regContents = (regContents | 0x80);

	writeReg(INTERRUPT_CONFIG, regContents);
}

// private implementation

void HDC2080::openReg(uint8_t reg)
{
	Wire.beginTransmission(_addr); // Connect to HDC2080
	Wire.write(reg);			         // point to specified register
	Wire.endTransmission();		     // Relinquish bus control
}

uint8_t HDC2080::readReg(uint8_t reg)
{
	openReg(reg);
	uint8_t reading;			      // holds byte of read data
	Wire.requestFrom(_addr, 1); // Request 1 byte from open register
	if (Wire.available() == 0)
	{
		reading = 0;
	}
	else
	{
		reading = Wire.read();
	}

	return reading;
}

void HDC2080::writeReg(uint8_t reg, uint8_t data)
{

	Wire.beginTransmission(_addr); // Open Device
	Wire.write(reg);			         // Point to register
	Wire.write(data);			         // Write data to register
	Wire.endTransmission();		     // Relinquish bus control
}