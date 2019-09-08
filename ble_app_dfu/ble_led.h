#ifndef __BLE_LED_H__
#define __BLE_LED_H__

#include <stdint.h>
#include <stdbool.h>
#include "ble.h"
#include "ble_srv_common.h"
#include "nrf_sdh_ble.h"
#include "nrf_ble_gatt.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BLE_LED_BLE_OBSERVER_PRIO   2   //! Priority with which BLE events are dispatched to the LED Service

/**@brief   Macro for defining a ble_led instance.
 *
 * @param   _name   Name of the instance.
 * @hideinitializer
 */
#define BLE_LED_SERVICE_DEF(_name)                         \
static ble_led_t _name;                                    \
NRF_SDH_BLE_OBSERVER(_name ## _obs,                        \
                     BLE_LED_BLE_OBSERVER_PRIO,            \
                     ble_led_on_ble_evt, &_name)

// LED Service:                709E0001-C6D8-45CE-BA5A-406667428FCE
//  LED state Characteristic:  709E0002-C6D8-45CE-BA5A-406667428FCE

// The bytes are stored in little-endian format, meaning the
// Least Significant Byte is stored first
// (reversed from the order they're displayed as)

// Base UUID: 709E0000-C6D8-45CE-BA5A-406667428FCE
#define BLE_LED_SERVICE_BASE_UUID   {0xce, 0x8f, 0x42, 0x67, 0x66, 0x40, 0x5a, 0xba, 0xce, 0x45, 0xd8, 0xc6, 0x00, 0x00, 0x9e, 0x70}

// Service & characteristics UUIDs
#define BLE_LED_SERVICE_UUID        0x0001
#define BLE_LED_STATE_CHAR_UUID     0x0002

// Forward declaration of the ble_led_t type
typedef struct ble_led_s ble_led_t;

// LED Service event type
typedef enum
{
    BLE_LED_STATE_EVT_NOTIFICATION_ENABLED,
    BLE_LED_STATE_EVT_NOTIFICATION_DISABLED,
} ble_led_evt_type_t;

// LED Service event
typedef struct
{
    ble_led_evt_type_t evt_type;
} ble_led_evt_t;

// LED Service event handler type
typedef void (*ble_led_evt_handler_t) (ble_led_t *p_led, ble_led_evt_t *p_evt);

// LED Service init structure
typedef struct ble_led_init_s
{
    ble_led_evt_handler_t evt_handler;
} ble_led_init_t;

// LED Service structure
struct ble_led_s 
{
    uint16_t conn_handle;
    uint16_t service_handle;
    uint8_t uuid_type;
    ble_led_evt_handler_t evt_handler;
    ble_gatts_char_handles_t led_state_char_handles;
};


// Function for initializing the LED Service
uint32_t ble_led_init(ble_led_t *p_led, ble_led_init_t *p_led_init);


// Function for handling the Application's BLE Stack events
void ble_led_on_ble_evt(ble_evt_t const *p_ble_evt, void *p_context);


// Function for updating the LED state Characteristic
uint32_t ble_led_state_characteristic_update(ble_led_t *p_led, uint8_t *led_state);


#ifdef __cplusplus
}
#endif

#endif // define __BLE_LED_H__