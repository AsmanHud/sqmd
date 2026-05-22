#include "accel_test.h"
#include "FXLS8971CF_Driver.h"
#include "config.h"
#include <Wire.h>

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

    // Configure sensor for normal operation
    if (!accel.configure())
    {
        Serial.println("FXLS8971CF: Sensor configuration failed!");
        return;
    }
    Serial.println("FXLS8971CF: Ready for normal operation.");
}

void accel_test_loop()
{
    delay(1000);

    // Read one measurement
    float x_g = -1, y_g = -1, z_g = -1;
    accel.readAcceleration(x_g, y_g, z_g);

    Serial.print("X = ");
    Serial.print(x_g);
    Serial.print("\tY = ");
    Serial.print(y_g);
    Serial.print("\tZ = ");
    Serial.println(z_g);
}