#include "blink_test.h"
#include "hdc2080_test.h"
#include "accel_test.h"

void setup()
{
    // blink_test_setup();
    // hdc2080_test_setup();
    accel_test_setup();
}

void loop()
{
    // blink_test_loop();
    // hdc2080_test_loop();
    accel_test_loop();
}