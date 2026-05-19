#include "HDC2080_Driver.h"

#define HDC2080_ADDR 0x40
#define HDC2080_INT_PIN 4

HDC2080 th_sensor(HDC2080_ADDR);

volatile bool hdcDataReady = false;

void IRAM_ATTR hdcISR()
{
    hdcDataReady = true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialize I2C communication
  th_sensor.begin();

  if (!th_sensor.isConnected()) {
    Serial.println("HDC2080 not detected");
    return;
  }
  Serial.println("HDC2080 detected");

  // Begin with a device reset
  th_sensor.reset();

  // Enable HDC2080 DRDY/INT pin and Data Ready interrupt source
  th_sensor.enableInterrupt();
  th_sensor.enableDRDYInterrupt();

  // HDC2080 DRDY/INT is push-pull.
  // With INT_POL = 0, DRDY event = active LOW.
  pinMode(HDC2080_INT_PIN, INPUT);

  attachInterrupt(
    digitalPinToInterrupt(HDC2080_INT_PIN),
    hdcISR,
    FALLING
  );

  // Start first measurement
  hdcDataReady = false;
  th_sensor.triggerMeasurement();
}

void loop() {
    // if (hdcDataReady)
    // {
    //     hdcDataReady = false;

    //     float temperature = th_sensor.readTemp();
    //     float humidity = th_sensor.readHumidity();

    //     Serial.print("Temperature (C): ");
    //     Serial.print(temperature);

    //     Serial.print("\tHumidity (%RH): ");
    //     Serial.println(humidity);

    //     delay(1000);

    //     // Start next measurement
    //     th_sensor.triggerMeasurement();
    // }
}
