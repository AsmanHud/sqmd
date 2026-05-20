#include "HDC2080_Driver.h"
#include "config.h"
#include <Wire.h>

HDC2080 th_sensor(HDC2080_ADDR);

volatile bool hdcDataReady = false;

void IRAM_ATTR hdcISR()
{
    hdcDataReady = true;
}

void setup()
{
    Serial.begin(SERIAL_BAUD);
    delay(1000);

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
        Serial.println("HDC2080 failed reset!");
        return;
    }
    Serial.println("HDC2080 reset successful.");

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

void loop()
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
