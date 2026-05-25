#include <math.h>
#include <Arduino.h>
#include <LittleFS.h>
#include <Wire.h>
#include "driver/uart.h"
#include "esp_sleep.h"
#include "src/sensors/HDC2080_Driver.h"
#include "src/sensors/FXLS8971CF_Driver.h"
#include "src/config.h"

static const char *LOG_FILE_PATH = "/log.csv";

static uint64_t lightSleepTimeUs = PERIODIC_MEASUREMENT_INTERVAL_S * 1000000ULL;

void fatalError(const char *msg);
void configureLightSleepTimer(uint64_t sleepTimeUs);
void createCsvFileIfNeeded();
void handleThSensorData();
void handleAccelData();
void handleUARTWake();

HDC2080 th_sensor(HDC2080_ADDR);
FXLS8971CF accel(FXLS8971CF_ADDR);

void setup()
{
    // Setup serial for debug output
    Serial.begin(SERIAL_BAUD);
    delay(SERIAL_STARTUP_DELAY_MS);
    Serial.println("Serial initialized.");

    // Enable user LED for status indication
    pinMode(USER_LED, OUTPUT);
    digitalWrite(USER_LED, HIGH); // LED is active LOW, so start with it off

    // Indicate power-up with short LED blinks
    for (int i = 0; i < POWERUP_LED_BLINKS; ++i)
    {
        digitalWrite(USER_LED, LOW);
        delay(SHORT_LED_BLINK_STATE_DURATION_MS);
        digitalWrite(USER_LED, HIGH);
        delay(SHORT_LED_BLINK_STATE_DURATION_MS);
    }

    // Setup the flash memory with LittleFS
    if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED))
    {
        fatalError("LittleFS Mount Failed!");
    }
    Serial.println("LittleFS mounted.");

    createCsvFileIfNeeded();

    // Enable light sleep wake up sources
    if (esp_sleep_enable_timer_wakeup(lightSleepTimeUs) != ESP_OK)
    {
        fatalError("Failed to enable light sleep timer wake up!");
    }
    if (gpio_wakeup_enable((gpio_num_t)FXLS8971CF_INT1_PIN, GPIO_INTR_HIGH_LEVEL) != ESP_OK)
    {
        fatalError("Failed to enable FXLS8971CF interrupt wake up!");
    }
    if (gpio_wakeup_enable((gpio_num_t)HDC2080_DRDY_PIN, GPIO_INTR_LOW_LEVEL) != ESP_OK)
    {
        fatalError("Failed to enable HDC2080 DRDY interrupt wake up!");
    }
    if (esp_sleep_enable_gpio_wakeup() != ESP_OK)
    {
        fatalError("Failed to enable GPIO wake up!");
    }
    if (uart_set_wakeup_threshold(UART_NUM_0, UART_WAKEUP_THRESHOLD) != ESP_OK)
    {
        fatalError("Failed to set UART wakeup threshold!");
    }
    if (esp_sleep_enable_uart_wakeup(UART_NUM_0) != ESP_OK)
    {
        fatalError("Failed to enable UART wakeup!");
    }
    Serial.println("Light sleep wake up sources configured.");

    // Initialize I2C
    if (!Wire.begin(SDA_PIN, SCL_PIN, SCL_FREQ))
    {
        fatalError("I2C setup failed!");
    }
    Serial.println("I2C initialized.");

    // Check sensor connectivity
    if (!th_sensor.isConnected())
    {
        fatalError("HDC2080 not detected!");
    }
    if (!accel.isConnected())
    {
        fatalError("FXLS8971CF not detected!");
    }
    Serial.println("HDC2080 and FXLS8971CF detected.");

    // Reset sensors to ensure clean state
    if (!th_sensor.reset())
    {
        fatalError("HDC2080: Failed reset!");
    }
    Serial.println("HDC2080: Reset successful.");
    if (!accel.reset())
    {
        fatalError("FXLS8971CF: Failed reset!");
    }
    Serial.println("FXLS8971CF: Reset successful.");

    // Configure sensors for operation
    if (!th_sensor.enableInterruptPin())
    {
        fatalError("HDC2080: DRDY pin enable failed!");
    }
    if (!th_sensor.enableDataReadyInterrupt())
    {
        fatalError("HDC2080: Data Ready Generator enable failed!");
    }
    Serial.println("HDC2080: Configuration successful, sensor active.");
    if (!accel.configure())
    {
        fatalError("FXLS8971CF: Configuration failed!");
    }
    Serial.println("FXLS8971CF: Configuration successful, sensor active.");

    // Trigger the first HDC2080 conversion
    if (!th_sensor.triggerMeasurement())
    {
        Serial.println("HDC2080: Trigger measurement failed! Continuing to the next measurement.");
    }
}

void loop()
{
    static int enterLightSleepAttempts = 0;

    uint32_t lastSleepEntryMs = millis();

    esp_err_t err = esp_light_sleep_start();

    if (err != ESP_OK)
    {
        if (enterLightSleepAttempts >= LIGHT_SLEEP_ENTER_ATTEMPT_RETRIES)
        {
            fatalError("Failed to enter light sleep after multiple attempts!");
        }

        Serial.print("Failed to enter light sleep! Retrying in ");
        Serial.print(LIGHT_SLEEP_ENTER_ATTEMPT_DELAY_MS);
        Serial.print(" ms. ");
        Serial.print(LIGHT_SLEEP_ENTER_ATTEMPT_RETRIES - enterLightSleepAttempts);
        Serial.println(" attempt(s) remaining.");

        enterLightSleepAttempts++;
        delay(LIGHT_SLEEP_ENTER_ATTEMPT_DELAY_MS);
        return;
    }
    enterLightSleepAttempts = 0; // reset attempts counter on successful sleep entry

    esp_sleep_wakeup_cause_t wakeUpCause = esp_sleep_get_wakeup_cause();

    // Light sleep interrupted by GPIO interrupt
    if (wakeUpCause == ESP_SLEEP_WAKEUP_GPIO)
    {
        Serial.println("Interrupt wake.");
        uint64_t periodUs = PERIODIC_MEASUREMENT_INTERVAL_S * 1000000ULL;
        uint64_t elapsedUs = (uint64_t)(millis() - lastSleepEntryMs) * 1000ULL;

        if (elapsedUs >= periodUs)
        {
            lightSleepTimeUs = 1;
        }
        else
        {
            lightSleepTimeUs = periodUs - elapsedUs;
        }

        configureLightSleepTimer(lightSleepTimeUs);

        bool hdc2080DRDYFlag = false;
        if (!th_sensor.readDRDYFlag(hdc2080DRDYFlag))
        {
            Serial.println("HDC2080: DRDY flag read failed!");
        }
        else if (hdc2080DRDYFlag)
        {
            handleThSensorData();
        }

        bool sdcdIntFlag = false;
        bool axisFlag = false;
        bool polFlag = false;
        if (!accel.readSDCDEventFlag(sdcdIntFlag, axisFlag, polFlag))
        {
            Serial.println("FXLS8971CF: SDCD event flag read failed!");
        }
        else if (sdcdIntFlag)
        {
            handleAccelData();
        }
    }
    // Light sleep timer wake up
    else if (wakeUpCause == ESP_SLEEP_WAKEUP_TIMER)
    {
        Serial.println("Periodic wake.");

        // reset timer
        lightSleepTimeUs = PERIODIC_MEASUREMENT_INTERVAL_S * 1000000ULL;
        configureLightSleepTimer(lightSleepTimeUs);

        if (!th_sensor.triggerMeasurement())
        {
            Serial.println("HDC2080: Failed trigger measurement! Skipping current measurement.");
        }
    }
    else if (wakeUpCause == ESP_SLEEP_WAKEUP_UART)
    {
        handleUARTWake();
        fatalError("Test case terminated.");
    }
    else
    {
        Serial.println("Unexpected light sleep wake up source!");
    }
}

void fatalError(const char *msg)
{
    Serial.println(msg);

    while (true)
    {
        digitalWrite(USER_LED, LOW);
        delay(HALTING_DELAY_MS);
        digitalWrite(USER_LED, HIGH);
        delay(HALTING_DELAY_MS);
    }
}

void configureLightSleepTimer(uint64_t sleepTimeUs)
{
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);

    if (esp_sleep_enable_timer_wakeup(sleepTimeUs) != ESP_OK)
    {
        fatalError("Failed to configure light sleep timer wake up!");
    }
}

void createCsvFileIfNeeded()
{
    if (LittleFS.exists(LOG_FILE_PATH))
    {
        if (!LittleFS.remove(LOG_FILE_PATH))
        {
            fatalError("Failed to remove old CSV file!");
        }

        Serial.println("Old CSV file removed.");
    }

    File file = LittleFS.open(LOG_FILE_PATH, FILE_WRITE);
    if (!file)
    {
        fatalError("Failed to create CSV file!");
    }

    file.println("timestamp_ms,type,value1,value2");
    file.close();

    Serial.println("CSV file created/cleared.");
}

// writes the periodic temp/rh measurement into flash
void handleThSensorData()
{
    uint32_t timestampMs = millis();
    float temp_c = 0;
    float rh_p = 0;

    if (!th_sensor.readTemperature(temp_c))
    {
        Serial.println("HDC2080: Read temperature failed!");
        return;
    }
    if (!th_sensor.readHumidity(rh_p))
    {
        Serial.println("HDC2080: Read humidity failed!");
        return;
    }

    File file = LittleFS.open(LOG_FILE_PATH, FILE_APPEND);
    if (!file)
    {
        Serial.println("Failed to open CSV file for TH append!");
        return;
    }

    file.print(timestampMs);
    file.print(",TH,");
    file.print(temp_c, 2);
    file.print(",");
    file.println(rh_p, 2);

    file.close();
}

// writes the shock event into flash
void handleAccelData()
{
    Serial.println("FXLS8971CF: Handling SDCD shock event.");

    uint8_t bufCount = 0;

    if (!accel.readBufCount(bufCount))
    {
        Serial.println("FXLS8971CF: Failed to read buffer count!");
        return;
    }

    Serial.print("FXLS8971CF: Buffer count = ");
    Serial.println(bufCount);

    if (bufCount < 2)
    {
        Serial.println("FXLS8971CF: Not enough samples in buffer.");
        return;
    }

    if (bufCount > 32)
    {
        bufCount = 32;
    }

    static float xSamples[32];
    static float ySamples[32];
    static float zSamples[32];

    if (!accel.readBuffer(xSamples, ySamples, zSamples, bufCount))
    {
        Serial.println("FXLS8971CF: Failed to read buffer data!");
        return;
    }

    float deltaPeak = 0.0f;

    float prevMag = sqrtf(
        xSamples[0] * xSamples[0] +
        ySamples[0] * ySamples[0] +
        zSamples[0] * zSamples[0]);

    for (uint8_t i = 1; i < bufCount; i++)
    {
        float mag = sqrtf(
            xSamples[i] * xSamples[i] +
            ySamples[i] * ySamples[i] +
            zSamples[i] * zSamples[i]);

        float delta = fabsf(mag - prevMag);

        if (delta > deltaPeak)
        {
            deltaPeak = delta;
        }

        prevMag = mag;
    }

    uint32_t timestampMs = millis();

    File file = LittleFS.open(LOG_FILE_PATH, FILE_APPEND);
    if (!file)
    {
        Serial.println("Failed to open CSV file for accel append!");
        return;
    }

    file.print(timestampMs);
    file.print(",ACCEL_PEAK,");
    file.print(deltaPeak, 3);
    file.print(",");
    file.println(bufCount);

    file.close();

    Serial.print("FXLS8971CF: Peak delta magnitude written: ");
    Serial.println(deltaPeak, 3);
    delay(250);
}

// dumps the recorded CSV file for further processing
void handleUARTWake()
{
    Serial.println();
    Serial.println("===== LOG DUMP START =====");

    File file = LittleFS.open(LOG_FILE_PATH, FILE_READ);
    if (!file)
    {
        Serial.println("Failed to open log file for reading!");
        Serial.println("===== LOG DUMP END =====");
        return;
    }

    if (file.size() == 0)
    {
        Serial.println("Log file is empty.");
        file.close();
        Serial.println("===== LOG DUMP END =====");
        return;
    }

    while (file.available())
    {
        Serial.write(file.read());
    }

    file.close();

    Serial.println();
    Serial.println("===== LOG DUMP END =====");
}