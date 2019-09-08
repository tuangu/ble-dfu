#include "fake_co2_sensor.h"

#define CO2_SENSOR_MAX_PPM      10000
#define CO2_SENSOR_MIN_PPM      100

static uint16_t last_value = CO2_SENSOR_MIN_PPM;
static const uint16_t inc = 50;

void fake_co2_sensor_init()
{

}

uint16_t fake_co2_sensor_get()
{
    last_value += inc;

    if (last_value > CO2_SENSOR_MAX_PPM)
        last_value = CO2_SENSOR_MIN_PPM;

    return last_value;
}