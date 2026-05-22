#include "hdc2080_test.h"
#include "../sensors/HDC2080_Driver.h"
#include "../config.h"
#include <Wire.h>

volatile bool hdcDataReady = false;

void IRAM_ATTR hdcISR()
{
    hdcDataReady = true;
}

HDC2080 th_sensor(HDC2080_ADDR);

void hdc2080_test_setup()
{
    Serial.begin(SERIAL_BAUD);
    delay(1000);
    Serial.println("Serial initialized.");

    if (!Wire.begin(SDA_PIN, SCL_PIN, SCL_FREQ))
    {
        Serial.println("I2C setup failed!");
        return;
    }
    Serial.println("I2C initialized.");

    if (!th_sensor.isConnected())
    {
        Serial.println("HDC2080 not detected!");
        return;
    }
    Serial.println("HDC2080 detected.");

    // Begin with a device reset
    if (!th_sensor.reset())
    {
        Serial.println("HDC2080: Failed reset!");
        return;
    }
    Serial.println("HDC2080: Reset successful.");

    // Enable data ready interrupt generation
    if (!th_sensor.enableInterruptPin())
    {
        Serial.println("HDC2080: DRDY pin enable failed!");
        return;
    }
    if (!th_sensor.enableDataReadyInterrupt())
    {
        Serial.println("HDC2080: Data Ready Generator enable failed!");
        return;
    }
    Serial.println("HDC2080: Sensor is ready for DRDY generation.");

    // HDC2080 DRDY/INT is push-pull.
    // With INT_POL = 0, DRDY event = active LOW.
    pinMode(HDC2080_DRDY_PIN, INPUT);

    attachInterrupt(
        digitalPinToInterrupt(HDC2080_DRDY_PIN),
        hdcISR,
        FALLING);

    // Start first measurement
    hdcDataReady = false;
    if (!th_sensor.triggerMeasurement())
    {
        Serial.println("HDC2080: Trigger measurement failed!");
        return;
    }
}

void hdc2080_test_loop()
{
    if (hdcDataReady)
    {
        hdcDataReady = false;

        float temperature = -1;
        float humidity = -1;

        if (!th_sensor.readTemperature(temperature))
        {
            Serial.println("HDC2080: Temperature read failed!");
        }
        if (!th_sensor.readHumidity(humidity))
        {
            Serial.println("HDC2080: Humidity read failed!");
        }

        Serial.print("Temperature (C): ");
        Serial.print(temperature);

        Serial.print("\tHumidity (%RH): ");
        Serial.println(humidity);

        delay(1000);

        // Start next measurement
        if (!th_sensor.triggerMeasurement())
        {
            Serial.println("HDC2080: Trigger measurement failed!");
        }
    }
}