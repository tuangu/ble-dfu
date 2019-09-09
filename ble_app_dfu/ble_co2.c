#include <string.h>
#include "sdk_common.h"
#include "nrf_log.h"

#include "ble_co2.h"
#include "ble_srv_common.h"

static bool notification_enabled = false;

static void on_connect(ble_co2_t *p_co2, ble_evt_t const *p_ble_evt)
{
    p_co2->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
}


static void on_disconnect(ble_co2_t *p_co2, ble_evt_t const *p_ble_evt)
{
    UNUSED_PARAMETER(p_ble_evt);
    p_co2->conn_handle = BLE_CONN_HANDLE_INVALID;
}


static void on_co2_level_cccd_write(
    ble_co2_t *p_co2, ble_gatts_evt_write_t const *p_evt_write)
{
    if (p_evt_write->len == 2) {
        ble_co2_evt_t evt;

        if (ble_srv_is_notification_enabled(p_evt_write->data)) {
            evt.evt_type = BLE_CO2_EVT_NOTI_ENABLED;
        } else {
            evt.evt_type = BLE_CO2_EVT_NOTI_DISABLED;
        }

        p_co2->evt_handler(p_co2, &evt);
    }
}

static void on_co2_alert_char_write(
    ble_co2_t *p_co2, ble_gatts_evt_write_t const *p_evt_write)
{
    if (p_evt_write->len > 0) {
        ble_co2_evt_t evt;
        uint16_t value;

        value = uint16_decode(p_evt_write->data);
        if (value > 0) {
            p_co2->alert_threshold = value;
            
            evt.evt_type = BLE_CO2_EVT_ALERT_NEW_VALUE;
            evt.value = value;

            p_co2->evt_handler(p_co2, &evt);
        }
    }
}


static void on_write(ble_co2_t *p_co2, ble_evt_t const *p_ble_evt)
{
    ble_gatts_evt_write_t const *p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

    if (p_evt_write->handle == p_co2->co2_level_char_handles.cccd_handle) {
        on_co2_level_cccd_write(p_co2, p_evt_write);
    }

    if (p_evt_write->handle == p_co2->co2_alert_char_handles.value_handle) {
        on_co2_alert_char_write(p_co2, p_evt_write);
    }
}


void ble_co2_on_ble_evt(ble_evt_t const *p_ble_evt, void *p_context)
{
    ble_co2_t *p_co2 = (ble_co2_t *) p_context;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            on_connect(p_co2, p_ble_evt);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            on_disconnect(p_co2, p_ble_evt);
            break;

        case BLE_GATTS_EVT_WRITE:
            on_write(p_co2, p_ble_evt);
            break;
        
        default:
            break;
    }
}


uint32_t ble_co2_init(ble_co2_t *p_co2, ble_co2_init_t *p_co2_init)
{
    uint32_t err_code;
    ble_uuid_t uuid;
    ble_add_char_params_t char_params;
    uint16_t init_value;

    // Initialize the service structure
    p_co2->conn_handle = BLE_CONN_HANDLE_INVALID;
    p_co2->evt_handler = p_co2_init->evt_handler;
    p_co2->alert_threshold = p_co2_init->alert_threshold;

    // Add service UUID
    ble_uuid128_t co2_base_uuid = {BLE_CO2_SERVICE_BASE_UUID};
    err_code = sd_ble_uuid_vs_add(&co2_base_uuid, &p_co2->uuid_type);
    if (err_code != NRF_SUCCESS) {
        return err_code;
    }

    // Set up the UUID for the service
    uuid.type = p_co2->uuid_type;
    uuid.uuid = BLE_CO2_SERVICE_UUID;

    // Add CO2 service
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                        &uuid,
                                        &p_co2->service_handle);
    if (err_code != NRF_SUCCESS) {
        return err_code;
    }

    // Add CO2 level characteristic
    memset(&char_params, 0, sizeof(char_params));
    init_value = 0;

    char_params.uuid = BLE_CO2_LEVEL_CHAR_UUID;
    char_params.uuid_type = p_co2->uuid_type;
    char_params.max_len = sizeof(uint16_t);
    char_params.init_len = sizeof(uint16_t);
    char_params.p_init_value = (uint8_t *) &init_value;
    char_params.char_props.notify = 1;
    char_params.char_props.read = 1;
    char_params.read_access = SEC_OPEN;
    char_params.cccd_write_access = SEC_OPEN;
    
    err_code = characteristic_add(p_co2->service_handle,
                                  &char_params,
                                  &(p_co2->co2_level_char_handles));
    if (err_code != NRF_SUCCESS) {
        return err_code;
    }

    // Add CO2 alert threshold characteristic
    memset(&char_params, 0, sizeof(char_params));
    init_value = p_co2->alert_threshold;

    char_params.uuid = BLE_CO2_ALERT_CHAR_UUID;
    char_params.uuid_type = p_co2->uuid_type;
    char_params.max_len = sizeof(uint16_t);
    char_params.init_len = sizeof(uint16_t);
    char_params.p_init_value = (uint8_t *) &init_value;
    char_params.char_props.read = 1;
    char_params.char_props.write = 1;
    char_params.write_access = SEC_OPEN;

    err_code = characteristic_add(p_co2->service_handle,
                                  &char_params,
                                  &(p_co2->co2_alert_char_handles));
    if (err_code != NRF_SUCCESS) {
        return err_code;
    }

    return NRF_SUCCESS;
}


uint32_t ble_co2_level_update(ble_co2_t *p_co2, uint16_t *co2_value)
{
    uint32_t err_code;

    if (p_co2->conn_handle != BLE_CONN_HANDLE_INVALID) {
        // Update the database
        ble_gatts_value_t gatts_value;

        memset(&gatts_value, 0, sizeof(ble_gatts_value_t));
        gatts_value.len = sizeof(uint16_t);
        gatts_value.offset = 0;
        gatts_value.p_value = (uint8_t *) co2_value;

        err_code = sd_ble_gatts_value_set(p_co2->conn_handle,
                                          p_co2->co2_level_char_handles.value_handle,
                                          &gatts_value);
        if (err_code != NRF_SUCCESS) {
            return err_code;
        }

        if (notification_enabled) {
            uint16_t len = sizeof(uint8_t);
            uint16_t hvx_len = len;
            ble_gatts_hvx_params_t hvx_params;

            memset(&hvx_params, 0, sizeof(ble_gatts_hvx_params_t));

            hvx_params.handle = p_co2->co2_level_char_handles.value_handle;
            hvx_params.type = BLE_GATT_HVX_NOTIFICATION;
            hvx_params.p_len = &hvx_len;
            hvx_params.offset = 0;
            hvx_params.p_data = (uint8_t *) co2_value;

            err_code = sd_ble_gatts_hvx(p_co2->conn_handle, &hvx_params);

            if ((err_code == NRF_SUCCESS) && (hvx_len != len)) {
                err_code = NRF_ERROR_DATA_SIZE;
            }
        }
    } else {
        err_code = NRF_ERROR_INVALID_STATE;
    }

    return err_code;
}