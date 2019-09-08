#ifndef __CO2_SENSOR_H__
#define __CO2_SENSOR_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the fake CO2 sensor
void fake_co2_sensor_init();


// Get the sensor value
uint16_t fake_co2_sensor_get();


#ifdef __cplusplus
}
#endif

#endif // define __CO2_SENSOR_H__