#include "blink_test.h"
#include "config.h"
#include <Arduino.h>

void blink_test_setup()
{
    pinMode(USER_LED, OUTPUT);
}

void blink_test_loop()
{
    digitalWrite(USER_LED, LOW);
    delay(500);
    digitalWrite(USER_LED, HIGH);
    delay(5000);
}