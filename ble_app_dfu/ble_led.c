#include <string.h>
#include "sdk_common.h"
#include "nrf_log.h"

#include "ble_led.h"
#include "ble_srv_common.h"

static const uint8_t LedStateCharName[]   = "LEDState";
static bool notifications_enabled = false;


// Function for handling the Connect event
static void on_connect(ble_led_t *p_led, ble_evt_t const *p_ble_evt)
{
    p_led->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
}


// Function for handling the Disconnect event
static void on_disconnect(ble_led_t *p_led, ble_evt_t const *p_ble_evt)
{
    UNUSED_PARAMETER(p_ble_evt);
    p_led->conn_handle = BLE_CONN_HANDLE_INVALID;
}


// Function for handling the Write event
static void on_write(ble_led_t *p_led, ble_evt_t const *p_ble_evt)
{
    ble_gatts_evt_write_t const *p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

    // Write to CCCD
    if ((p_evt_write->handle == p_led->led_state_char_handles.cccd_handle) &&
        (p_evt_write->len == 2)) {
        ble_led_evt_t evt;

        if (ble_srv_is_notification_enabled(p_evt_write->data)) {
            evt.evt_type = BLE_LED_STATE_EVT_NOTIFICATION_ENABLED;
            notifications_enabled = true;
        } else {

            evt.evt_type = BLE_LED_STATE_EVT_NOTIFICATION_DISABLED;
            notifications_enabled = false;
        }

        if (p_led->evt_handler != NULL) {
            p_led->evt_handler(p_led, &evt);
        }
    }
}


// Function for adding LED state characteristic to LED service
uint32_t led_state_char_add(ble_led_t *p_led)
{
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_md_t cccd_md;
    ble_gatts_attr_t attr_char_value;
    ble_uuid_t ble_uuid;
    ble_gatts_attr_md_t attr_md;
    uint8_t init_value = 0;

    memset(&char_md, 0, sizeof(char_md));
    memset(&cccd_md, 0, sizeof(cccd_md));
    memset(&attr_md, 0, sizeof(attr_md));
    memset(&attr_char_value, 0, sizeof(attr_char_value));

    // Set permissions on the CCCD and Characteristic value
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);

    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&attr_md.write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);

    // CCCD settings (needed for notifications and/or indications)
    cccd_md.vloc = BLE_GATTS_VLOC_STACK;

    // Characteristic Metadata
    char_md.char_props.read          = 1;
    char_md.char_props.notify        = 1;
    char_md.p_char_user_desc         = LedStateCharName;
    char_md.char_user_desc_size      = sizeof(LedStateCharName);
    char_md.char_user_desc_max_size  = sizeof(LedStateCharName);
    char_md.p_char_pf                = NULL;
    char_md.p_user_desc_md           = NULL;
    char_md.p_cccd_md                = &cccd_md;
    char_md.p_sccd_md                = NULL;

    // Define the Led state Characteristic UUID
    ble_uuid.type = p_led->uuid_type;
    ble_uuid.uuid = BLE_LED_STATE_CHAR_UUID;

    // Attribute Metadata settings
    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth    = 0;
    attr_md.wr_auth    = 0;
    attr_md.vlen       = 0;

    // Attribute Value settings
    attr_char_value.p_uuid       = &ble_uuid;
    attr_char_value.p_attr_md    = &attr_md;
    attr_char_value.init_len     = sizeof(uint8_t);
    attr_char_value.init_offs    = 0;
    attr_char_value.max_len      = sizeof(uint8_t);
    attr_char_value.p_value      = &init_value;

    // Adding characteristic
    return sd_ble_gatts_characteristic_add(p_led->service_handle,
                                           &char_md,
                                           &attr_char_value,
                                           &p_led->led_state_char_handles);
}


uint32_t ble_led_init(ble_led_t *p_led, ble_led_init_t *p_led_init)
{
    uint32_t err_code;
    ble_uuid_t uuid;

    // Initialize the service structure
    p_led->conn_handle = BLE_CONN_HANDLE_INVALID;
    p_led->evt_handler = p_led_init->evt_handler;

    // Add service UUID
    ble_uuid128_t led_base_uuid = {BLE_LED_SERVICE_BASE_UUID};
    err_code = sd_ble_uuid_vs_add(&led_base_uuid, &p_led->uuid_type);
    if (err_code != NRF_SUCCESS) {
        return err_code;
    }

    // Set up the UUID for the service (base + service-specific)
    uuid.type = p_led->uuid_type;
    uuid.uuid = BLE_LED_SERVICE_UUID;

    // Add LED Service
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                        &uuid,
                                        &p_led->service_handle);
    if (err_code != NRF_SUCCESS) {
        return err_code;
    }

    // Add LED state characteristic
    err_code = led_state_char_add(p_led);
    if (err_code != NRF_SUCCESS) {
        return err_code;
    }

    return NRF_SUCCESS;
}


void ble_led_on_ble_evt(ble_evt_t const *p_ble_evt, void *p_context)
{
    ble_led_t *p_led = (ble_led_t *) p_context;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            on_connect(p_led, p_ble_evt);
            break;
    
        case BLE_GAP_EVT_DISCONNECTED:
            on_disconnect(p_led, p_ble_evt);
            break;

        case BLE_GATTS_EVT_WRITE:
            on_write(p_led, p_ble_evt);
            break; 

        default:
            break;
    }
}


uint32_t ble_led_state_characteristic_update(ble_led_t *p_led, uint8_t *led_state)
{
    uint32_t err_code;

    if (p_led->conn_handle != BLE_CONN_HANDLE_INVALID) {
        // Update the database
        ble_gatts_value_t gatts_value;
        
        memset(&gatts_value, 0, sizeof(ble_gatts_value_t));
        gatts_value.len = sizeof(uint8_t);
        gatts_value.offset = 0;
        gatts_value.p_value = led_state;
        
        err_code = sd_ble_gatts_value_set(p_led->conn_handle,
                                          p_led->led_state_char_handles.value_handle,
                                          &gatts_value);
        if (err_code != NRF_SUCCESS) {
            return err_code;
        }
        
        if (notifications_enabled) {
            uint16_t len = sizeof(uint8_t);
            uint16_t hvx_len = len;
            ble_gatts_hvx_params_t hvx_params;
            
            memset(&hvx_params, 0, sizeof(ble_gatts_hvx_params_t));
            
            hvx_params.handle = p_led->led_state_char_handles.value_handle;
            hvx_params.type = BLE_GATT_HVX_NOTIFICATION;
            hvx_params.p_len = &hvx_len;
            hvx_params.offset = 0;
            hvx_params.p_data = (uint8_t *) led_state;
            
            err_code = sd_ble_gatts_hvx(p_led->conn_handle, &hvx_params);
            if ((err_code == NRF_SUCCESS) && (hvx_len != len)) {
                err_code = NRF_ERROR_DATA_SIZE;
            }
        }
    } else {
        err_code = NRF_ERROR_INVALID_STATE;
    }

    return err_code;
}