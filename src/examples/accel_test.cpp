#include "accel_test.h"
#include "../sensors/FXLS8971CF_Driver.h"
#include "../config.h"
#include <Wire.h>

static uint32_t lastEventEndMs = 0;
const uint32_t SDCD_EVENT_COOLDOWN_MS = 200;
volatile bool sdcdOtInt = false;

void IRAM_ATTR fxlISR()
{
    sdcdOtInt = true;
}

FXLS8971CF accel(FXLS8971CF_ADDR);

void accel_test_setup()
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

    if (!accel.isConnected())
    {
        Serial.println("FXLS8971CF not detected!");
        return;
    }
    Serial.println("FXLS8971CF detected.");

    // Begin with a device reset
    if (!accel.reset())
    {
        Serial.println("FXLS8971CF: Failed reset!");
        return;
    }
    Serial.println("FXLS8971CF: Reset successful.");

    if (!accel.configure())
    {
        Serial.println("FXLS8971CF: Configuration failed!");
        return;
    }
    Serial.println("FXLS8971CF: Configuration successful, sensor active.");

    // FXLS8971CF INTx is push-pull.
    // With INT_POL = 1, INT asserted = 1
    pinMode(FXLS8971CF_INT1_PIN, INPUT);

    attachInterrupt(
        digitalPinToInterrupt(FXLS8971CF_INT1_PIN),
        fxlISR,
        RISING);

    sdcdOtInt = false;
}

void accel_test_loop()
{
    if (sdcdOtInt)
    {
        sdcdOtInt = false;
        if ((millis() - lastEventEndMs) < SDCD_EVENT_COOLDOWN_MS)
        {
            Serial.println("FXLS8971CF: SDCD event detected during cooldown, ignoring.");
            return;
        }
        Serial.println("FXLS8971CF: SDCD event detected.");

        uint8_t bufCount;
        float xSamples[32], ySamples[32], zSamples[32];

        bool firstBufferRead = true;
        uint8_t quietBatches = 0;
        uint32_t lastPoll = millis();
        uint32_t eventStart = millis();

        while (millis() - eventStart < 100) // 100 ms event timeout
        {
            if (firstBufferRead || (millis() - lastPoll >= 10))
            {
                lastPoll = millis();

                if (!accel.readBufCount(bufCount))
                {
                    Serial.println("FXLS8971CF: Failed to read buffer count!");
                    return;
                }
                Serial.print("Buffer count: ");
                Serial.println(bufCount);

                if ((bufCount == 0) || (bufCount < 16 && !firstBufferRead))
                {
                    Serial.println("FXLS8971CF: Buffer empty or below SDCD watermark.");
                    firstBufferRead = false;
                    continue;
                }
                firstBufferRead = false;

                if (!accel.readBuffer(xSamples, ySamples, zSamples, bufCount))
                {
                    Serial.println("FXLS8971CF: Failed to read buffer data!");
                    return;
                }
                Serial.println("FXLS8971CF: Buffer data read successfully.");

                float deltaPeak = 0.0f;
                float prevMag = sqrt(xSamples[0] * xSamples[0] + ySamples[0] * ySamples[0] + zSamples[0] * zSamples[0]);

                for (int i = 1; i < bufCount; i++)
                {
                    float mag = sqrt(xSamples[i] * xSamples[i] + ySamples[i] * ySamples[i] + zSamples[i] * zSamples[i]);
                    float delta = fabs(mag - prevMag);

                    if (delta > deltaPeak)
                    {
                        deltaPeak = delta;
                    }

                    prevMag = mag;
                }
                Serial.print("FXLS8971CF: Peak delta magnitude in buffer: ");
                Serial.println(deltaPeak);

                if (deltaPeak < SDCD_EVENT_THRESHOLD_G)
                {
                    quietBatches++;
                    if (quietBatches >= 2)
                    {
                        break;
                    }
                }
                else
                {
                    quietBatches = 0;
                }
            }
        }

        lastEventEndMs = millis();
    }
}
