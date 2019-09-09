#ifndef __BLE_CO2_H__
#define __BLE_CO2_H__

#include <stdint.h>
#include <stdbool.h>
#include "ble.h"
#include "ble_srv_common.h"
#include "nrf_sdh_ble.h"
#include "nrf_ble_gatt.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BLE_CO2_BLE_OBSERVER_PRIO   2   //! Priority with which BLE events are dispatched to the LED Service

#define BLE_CO2_DEFAULT_THRESHOLD   200

/**@brief   Macro for defining a ble_co2 instance.
 *
 * @param   _name   Name of the instance.
 * @hideinitializer
 */
#define BLE_CO2_SERVICE_DEF(_name)                         \
static ble_co2_t _name;                                    \
NRF_SDH_BLE_OBSERVER(_name ## _obs,                        \
                     BLE_CO2_BLE_OBSERVER_PRIO,            \
                     ble_co2_on_ble_evt, &_name)

// CO2 Service:                             B4330001-8F44-42AB-94B9-FE4C80B0F7DF
//  CO2 Level Characteristic:               B4330002-8F44-42AB-94B9-FE4C80B0F7DF
//  C02 Alert threshold Characteristic:     B4330003-8F44-42AB-94B9-FE4C80B0F7DF

// Base UUID: B4330000-8F44-42AB-94B9-FE4C80B0F7DF
#define BLE_CO2_SERVICE_BASE_UUID {0xdf, 0xf7, 0xb0, 0x80, 0x4c, 0xfe, 0xb9, 0x94, 0xab, 0x42, 0x44, 0x8f, 0x00, 0x00, 0x33, 0xb4}

// Service and characteristics UUIDs
#define BLE_CO2_SERVICE_UUID        0x0001
#define BLE_CO2_LEVEL_CHAR_UUID     0x0002
#define BLE_CO2_ALERT_CHAR_UUID     0x0003

// Forward declaration of the ble_co2_t type
typedef struct ble_co2_s ble_co2_t;

// CO2 service event type
typedef enum
{
    BLE_CO2_EVT_NOTI_ENABLED,
    BLE_CO2_EVT_NOTI_DISABLED,
    BLE_CO2_EVT_ALERT_NEW_VALUE,
} ble_co2_evt_type_t;

// CO2 service event
typedef struct ble_co2_evt_s
{
    ble_co2_evt_type_t evt_type;
    uint16_t value;
} ble_co2_evt_t;

// CO2 service event handler type
typedef void (*ble_co2_evt_handler_t) (ble_co2_t *p_co2, ble_co2_evt_t *p_evt);

// CO2 service init structure
typedef struct ble_co2_init_s
{
    ble_co2_evt_handler_t evt_handler;
    uint16_t alert_threshold;
} ble_co2_init_t;

// CO2 service structure
struct ble_co2_s
{
    uint16_t conn_handle;
    uint16_t service_handle;
    uint8_t uuid_type;
    uint16_t alert_threshold;
    ble_co2_evt_handler_t evt_handler;
    ble_gatts_char_handles_t co2_level_char_handles;
    ble_gatts_char_handles_t co2_alert_char_handles;
};


// Function for initializing the CO2 service
uint32_t ble_co2_init(ble_co2_t *p_co2, ble_co2_init_t *p_co2_init);


// Function for handling the Application's BLE stack events
void ble_co2_on_ble_evt(ble_evt_t const *p_ble_evt, void *p_context);


// Function for updating the CO2 level characteristic
uint32_t ble_co2_level_update(ble_co2_t *p_co2, uint16_t *co2_value);


#ifdef __cplusplus
}
#endif

#endif // define __BLE_CO2_H__