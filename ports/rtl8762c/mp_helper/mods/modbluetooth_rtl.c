/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2019-2021 Damien P. George
 * Copyright (c) 2019-2020 Jim Mussared
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <string.h>

#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/mphal.h"

#if MICROPY_PY_BLUETOOTH

// #include <bluetooth/bluetooth.h>
// #include <bluetooth/hci.h>
#include "extmod/modbluetooth.h"

#include <gap.h>
#include <gap_adv.h>
#include <gap_bond_le.h>
#include <gap_lib/gap_config.h>
#include <gap_msg.h>
#include <gap_privacy.h>
#include "os/os_sched.h"
#include "os/os_task.h"
#include "os/os_sync.h"
#include "os/os_msg.h"
#include "app/app_msg.h"
#include "bluetooth/gap/gap.h"
#include "bluetooth/gap/gap_adv.h"
#include "bluetooth/gap/gap_msg.h"
#include "bluetooth/gap/gap_storage_le.h"
#include "bluetooth/profile/server/simple_ble_service.h"

#include "BLE/BLE.h"
#include "BLE/Peripheral.h"
#include "BLE/Services/ServiceUtils.h"

#define DEBUG_printf(...) // printk("BLE: " __VA_ARGS__)

// #define BLE_HCI_SCAN_ITVL_MIN 0x10
// #define BLE_HCI_SCAN_ITVL_MAX 0xffff
// #define BLE_HCI_SCAN_WINDOW_MIN 0x10
// #define BLE_HCI_SCAN_WINDOW_MAX 0xffff

#define ERRNO_BLUETOOTH_NOT_ACTIVE MP_ENODEV

#define BLE_TASK_PRIORITY 2 //!< Task priorities
#define BLE_TASK_STACK_SIZE 256 * 8 //!<  Task stack size

#define BLE_TIME_FOREVER (0xffffffff)

#define MAX_NUMBER_OF_GAP_MESSAGE 0x20 //!<  GAP message queue size
#define MAX_NUMBER_OF_IO_MESSAGE 0x20 //!<  IO message queue size
#define MAX_NUMBER_OF_MESSAGES 1 //!<  Message queue sizes
#define MAX_NUMBER_OF_EVENT_MESSAGE \
    (MAX_NUMBER_OF_GAP_MESSAGE + MAX_NUMBER_OF_IO_MESSAGE) //!< Event message queue size

enum {
    MP_BLUETOOTH_ZEPHYR_BLE_STATE_OFF,
    MP_BLUETOOTH_ZEPHYR_BLE_STATE_ACTIVE,
    MP_BLUETOOTH_ZEPHYR_BLE_STATE_SUSPENDED,
};

enum {
    MP_BLUETOOTH_ZEPHYR_GAP_SCAN_STATE_INACTIVE,
    MP_BLUETOOTH_ZEPHYR_GAP_SCAN_STATE_DEACTIVATING,
    MP_BLUETOOTH_ZEPHYR_GAP_SCAN_STATE_ACTIVE,
};


#define MP_BLUETOOTH_MAX_SERVICES (8)

typedef struct _mp_bluetooth_zephyr_root_pointers_t {
    // Characteristic (and descriptor) value storage.
    mp_gatts_db_t gatts_db;

    // Pending service definitions.
    uint8_t n_services;
    void *semaphore_send;
    struct {
        T_SERVER_ID id;
        uint8_t n_attributes;
        Attribute_t *attributes;
    } services[MP_BLUETOOTH_MAX_SERVICES];
} mp_bluetooth_zephyr_root_pointers_t;

typedef union {
    uint16_t u16;
    struct {
        uint8_t attr_index, service_index;
    } __attribute__((packed)) indices;
} handle_t;

static volatile int mp_bluetooth_zephyr_ble_state = MP_BLUETOOTH_ZEPHYR_BLE_STATE_OFF;

// void* ble_task_handle; //!< APP Task handle
// void* evt_queue_handle; //!< Event queue handle
// void* io_queue_handle; //!< IO queue handlezs

const char foo[] = "foo\r\n";

static int bt_err_to_errno(int err) {
    // Zephyr uses errno codes directly, but they are negative.
    return -err;
}

/**
 * Magic stack config for setting number of bonds
 * @see: lib/src/sample/ble_scatternet/
 * @note: confirmed by RTL to be the correct way do to this.
 */
#ifdef BT_STACK_CONFIG_ENABLE
#include "app_section.h"
/**
 * Bt Stack Config Init
 * @returns nothing
 */
APP_FLASH_TEXT_SECTION void bt_stack_config_init(void)
{
    gap_config_max_le_paired_device(APP_MAX_BONDS);
}
#endif

int mp_bluetooth_init(void) {
    DEBUG_printf("mp_bluetooth_init\n");

    // os_task_create(&ble_task_handle, "ble", BLE_task, NULL, BLE_TASK_STACK_SIZE,
    //     BLE_TASK_PRIORITY);

    // while (mp_bluetooth_zephyr_ble_state != MP_BLUETOOTH_ZEPHYR_BLE_STATE_ACTIVE) {
    //     os_delay(100);
    // }

    // Clean up if necessary.
    mp_bluetooth_deinit();

    // Allocate memory for state.
    MP_STATE_PORT(bluetooth_zephyr_root_pointers) = m_new0(mp_bluetooth_zephyr_root_pointers_t, 1);
    mp_bluetooth_gatts_db_create(&MP_STATE_PORT(bluetooth_zephyr_root_pointers)->gatts_db);

    if (!os_sem_create(&(MP_STATE_PORT(bluetooth_zephyr_root_pointers)->semaphore_send), 1, 1))
    {
        // Semaphore not created successfully.
        mp_raise_OSError(ENOMEM);
    }

    init_BLE();

    le_gap_init(APP_MAX_LINKS);
    gap_lib_init();

    // // #if MICROPY_PY_BLUETOOTH_ENABLE_CENTRAL_MODE
    // // mp_bluetooth_zephyr_gap_scan_state = MP_BLUETOOTH_ZEPHYR_GAP_SCAN_STATE_INACTIVE;
    // // k_timer_init(&mp_bluetooth_zephyr_gap_scan_timer, gap_scan_cb_timeout, NULL);
    // // mp_bluetooth_zephyr_gap_scan_cb_struct.recv = gap_scan_cb_recv;
    // // mp_bluetooth_zephyr_gap_scan_cb_struct.timeout = NULL; // currently not implemented in Zephyr
    // // bt_le_scan_cb_register(&mp_bluetooth_zephyr_gap_scan_cb_struct);
    // // #endif

    // if (mp_bluetooth_zephyr_ble_state == MP_BLUETOOTH_ZEPHYR_BLE_STATE_OFF) {
    //     // bt_enable can only be called once.
    //     T_GAP_CAUSE ret = gap_write_airplan_mode(0x00);
    //     if (ret) {
    //         return bt_err_to_errno(ret);
    //     }
    // }

    mp_bluetooth_zephyr_ble_state = MP_BLUETOOTH_ZEPHYR_BLE_STATE_ACTIVE;

    // DEBUG_printf("mp_bluetooth_init: ready\n");

    return 0;
}

void mp_bluetooth_deinit(void) {
    DEBUG_printf("mp_bluetooth_deinit %d\n", mp_bluetooth_zephyr_ble_state);
    if (mp_bluetooth_zephyr_ble_state == MP_BLUETOOTH_ZEPHYR_BLE_STATE_OFF
        || mp_bluetooth_zephyr_ble_state == MP_BLUETOOTH_ZEPHYR_BLE_STATE_SUSPENDED) {
        return;
    }

    mp_bluetooth_gap_advertise_stop();

    // There is no way to turn off the BT stack in Zephyr, so just set the
    // state as suspended so it can be correctly reactivated later.
    mp_bluetooth_zephyr_ble_state = MP_BLUETOOTH_ZEPHYR_BLE_STATE_SUSPENDED;

    MP_STATE_PORT(bluetooth_zephyr_root_pointers) = NULL;
}

bool mp_bluetooth_is_active(void) {
    return mp_bluetooth_zephyr_ble_state == MP_BLUETOOTH_ZEPHYR_BLE_STATE_ACTIVE;
}

void mp_bluetooth_get_current_address(uint8_t *addr_type, uint8_t *addr) {
    // if (!mp_bluetooth_is_active()) {
    //     mp_raise_OSError(ERRNO_BLUETOOTH_NOT_ACTIVE);
    // }
    // bt_addr_le_t le_addr;
    // size_t count = 1;
    // bt_id_get(&le_addr, &count);
    // if (count == 0) {
    //     mp_raise_OSError(EIO);
    // }
    // reverse_addr_byte_order(addr, &le_addr);
    // *addr_type = le_addr.type;
}

void mp_bluetooth_set_address_mode(uint8_t addr_mode) {
    // TODO: implement
}

size_t mp_bluetooth_gap_get_device_name(const uint8_t **buf) {
    static T_LOCAL_NAME device_name;
    if (flash_load_local_name(&device_name) == 0) {
        if (device_name.local_name[0] != 0xff) {
            *buf = (uint8_t *)device_name.local_name;
        }
    }
    *buf = NULL;
    return strlen((const char *) device_name.local_name);
}

int mp_bluetooth_gap_set_device_name(const uint8_t *buf, size_t len) {
    T_LOCAL_NAME device_name;
    if (len + 1 > sizeof(device_name.local_name)) {
        return MP_EINVAL;
    }

    memcpy(device_name.local_name, buf, len);
    device_name.local_name[len] = '\0';
    return flash_save_local_name(&device_name);
}

/**
 * All the BT Profile service callback events are handled in this function
 * @note     Then the event handling function shall be called according to the
 *           service_id
 * @param    service_id  Profile service ID
 * @param    p_data      Pointer to callback data
 * @return   T_APP_RESULT, which indicates the function call is successful or
 * not
 * @retval   APP_RESULT_SUCCESS  Function run successfully
 * @retval   others              Function run failed, and return number
 * indicates the reason
 */
T_APP_RESULT
app_profile_callback(T_SERVER_ID service_id, void* p_data)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;

    if (service_id == SERVICE_PROFILE_GENERAL_ID)
    {
        T_SERVER_APP_CB_DATA* p_param = (T_SERVER_APP_CB_DATA*)p_data;
        switch (p_param->eventId) {
            case PROFILE_EVT_SRV_REG_COMPLETE: {
            } break;
            case PROFILE_EVT_SEND_DATA_COMPLETE: {
                os_sem_give(MP_STATE_PORT(bluetooth_zephyr_root_pointers)->semaphore_send);
            } break;
            default:
                break;
        }
    }

    return app_result;
}

int mp_bluetooth_gap_advertise_start(bool connectable, int32_t interval_us, const uint8_t *adv_data, size_t adv_data_len, const uint8_t *sr_data, size_t sr_data_len) {
    if (!mp_bluetooth_is_active()) {
        return ERRNO_BLUETOOTH_NOT_ACTIVE;
    }
    // mp_bluetooth_gap_advertise_stop();

    BLE_On();

    // /* Device name and device appearance */
    // uint8_t  device_name[GAP_DEVICE_NAME_LEN] = "BLE_PERIPHERAL";
    // uint16_t appearance = GAP_GATT_APPEARANCE_GENERIC_WATCH;
    uint8_t  slave_init_mtu_req = false;
    // le_set_gap_param(GAP_PARAM_DEVICE_NAME, GAP_DEVICE_NAME_LEN, device_name);
    // le_set_gap_param(GAP_PARAM_APPEARANCE, sizeof(appearance), &appearance);
    le_set_gap_param(GAP_PARAM_SLAVE_INIT_GATT_MTU_REQ, sizeof(slave_init_mtu_req),
                     &slave_init_mtu_req);

    /* Advertising parameters */
    uint8_t  adv_evt_type = GAP_ADTYPE_ADV_IND;
    uint8_t  adv_direct_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t  adv_direct_addr[GAP_BD_ADDR_LEN] = {0};
    uint8_t  adv_chann_map = GAP_ADVCHAN_ALL;
    uint8_t  adv_filter_policy = GAP_ADV_FILTER_ANY;
    uint16_t adv_int_min = interval_us/625;
    uint16_t adv_int_max = interval_us/625;

    le_adv_set_param(GAP_PARAM_ADV_EVENT_TYPE, sizeof(adv_evt_type), &adv_evt_type);
    le_adv_set_param(GAP_PARAM_ADV_DIRECT_ADDR_TYPE, sizeof(adv_direct_type), &adv_direct_type);
    le_adv_set_param(GAP_PARAM_ADV_DIRECT_ADDR, sizeof(adv_direct_addr), adv_direct_addr);
    le_adv_set_param(GAP_PARAM_ADV_CHANNEL_MAP, sizeof(adv_chann_map), &adv_chann_map);
    le_adv_set_param(GAP_PARAM_ADV_FILTER_POLICY, sizeof(adv_filter_policy), &adv_filter_policy);
    le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MIN, sizeof(adv_int_min), &adv_int_min);
    le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MAX, sizeof(adv_int_max), &adv_int_max);
    if (adv_data != NULL) {
        le_adv_set_param(GAP_PARAM_ADV_DATA, adv_data_len, (void *)adv_data);
    }
    if (sr_data != NULL) {
        le_adv_set_param(GAP_PARAM_SCAN_RSP_DATA, sr_data_len, (void *)sr_data);
    }

    /* GAP Bond Manager parameters */
    uint8_t  auth_pair_mode = GAP_PAIRING_MODE_PAIRABLE;
    uint16_t auth_flags = GAP_AUTHEN_BIT_BONDING_FLAG;
    uint8_t  auth_io_cap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
    uint8_t  auth_oob = false;
    uint8_t  auth_use_fix_passkey = false;
    uint32_t auth_fix_passkey = 0;
#if F_BT_ANCS_CLIENT_SUPPORT
    uint8_t  auth_sec_req_enable = true;
#else
    uint8_t  auth_sec_req_enable = false;
#endif
    uint16_t auth_sec_req_flags = GAP_AUTHEN_BIT_BONDING_FLAG;

    gap_set_param(GAP_PARAM_BOND_PAIRING_MODE, sizeof(auth_pair_mode), &auth_pair_mode);
    gap_set_param(GAP_PARAM_BOND_AUTHEN_REQUIREMENTS_FLAGS, sizeof(auth_flags), &auth_flags);
    gap_set_param(GAP_PARAM_BOND_IO_CAPABILITIES, sizeof(auth_io_cap), &auth_io_cap);
    gap_set_param(GAP_PARAM_BOND_OOB_ENABLED, sizeof(auth_oob), &auth_oob);
    le_bond_set_param(GAP_PARAM_BOND_FIXED_PASSKEY, sizeof(auth_fix_passkey), &auth_fix_passkey);
    le_bond_set_param(GAP_PARAM_BOND_FIXED_PASSKEY_ENABLE, sizeof(auth_use_fix_passkey),
                      &auth_use_fix_passkey);
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(auth_sec_req_enable), &auth_sec_req_enable);
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_REQUIREMENT, sizeof(auth_sec_req_flags),
                      &auth_sec_req_flags);

    le_register_app_cb(app_gap_callback);

    BLE_Peripheral_On(true);

    // return bt_err_to_errno(bt_le_adv_start(&param, bt_ad_data, bt_ad_len, bt_sd_data, bt_sd_len));
    return 0;
}

void mp_bluetooth_gap_advertise_stop(void) {
    // Note: bt_le_adv_stop returns 0 if adv is already stopped.
    T_GAP_CAUSE ret = le_adv_stop();
    // int ret = 1;
    if (ret != 0) {
        mp_raise_OSError(bt_err_to_errno(ret));
    }
}

int mp_bluetooth_gatts_register_service_begin(bool append) {
    if (!mp_bluetooth_is_active()) {
        return ERRNO_BLUETOOTH_NOT_ACTIVE;
    }

    if (append) {
        // Don't support append yet (modbluetooth.c doesn't support it yet anyway).
        return MP_EOPNOTSUPP;
    }

    // Reset the gatt characteristic value db.
    mp_bluetooth_gatts_db_reset(MP_STATE_PORT(bluetooth_zephyr_root_pointers)->gatts_db);

    // Unref any previous service definitions.
    for (size_t i = 0; i < MP_STATE_PORT(bluetooth_zephyr_root_pointers)->n_services; ++i) {
        MP_STATE_PORT(bluetooth_zephyr_root_pointers)->services[i].id = 0;
        MP_STATE_PORT(bluetooth_zephyr_root_pointers)->services[i].attributes = NULL;
        MP_STATE_PORT(bluetooth_zephyr_root_pointers)->services[i].n_attributes = 0;
    }
    MP_STATE_PORT(bluetooth_zephyr_root_pointers)->n_services = 0;

    return 0;
}

static inline int service_index_from_id(T_SERVER_ID service_id)
{
    for (size_t i = 0; i < MP_STATE_PORT(bluetooth_zephyr_root_pointers)->n_services; ++i) {
        if (service_id == MP_STATE_PORT(bluetooth_zephyr_root_pointers)->services[i].id) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Reads the characteristic data from the service.
 *
 * @param conn_id      The used connection ID.
 * @param service_id   ServiceID of characteristic data.
 * @param attrib_index Attribute index of getting characteristic data.
 * @param offset       Used for Blob Read.
 * @param p_length     Length of getting characteristic data.
 * @param pp_value     Data got from service.
 * @return Profile procedure result.
*/
T_APP_RESULT service_attr_read_cb(uint8_t conn_id, T_SERVER_ID service_id,
                                                  uint16_t attrib_index, uint16_t offset,
                                                  uint16_t *p_length, uint8_t **pp_value)
{
    size_t value_len = 0;
    handle_t handle = {
        .indices = {
            .service_index = service_index_from_id(service_id),
            .attr_index = attrib_index,
        },
    };

    if (0 != mp_bluetooth_gatts_on_read_request(conn_id, handle.u16)) {
        return APP_RESULT_APP_ERR;
    }

    if (0 != mp_bluetooth_gatts_db_read(MP_STATE_PORT(bluetooth_zephyr_root_pointers)->gatts_db, handle.u16, (const uint8_t **) pp_value, &value_len)) {
        return APP_RESULT_ATTR_NOT_FOUND;
    }
    *p_length = value_len;
    
    return APP_RESULT_SUCCESS;
}

/**
 * @brief Callback function after writing service information.
 *
 * @param conn_id Used connection ID.
 * @param service_id ID of the service.
 * @param attrib_index Index of the attribute.
 * @param length Lengt of the value.
 * @param p_value The value itself.
 */
void write_post_callback(uint8_t conn_id, T_SERVER_ID service_id,
    uint16_t attrib_index, uint16_t length,
    uint8_t* p_value)
{
}

/**
 * @brief Writes characteristic data from motor service.
 *
 * @param conn_id               The connection ID.
 * @param service_id            ServiceID to be written.
 * @param attrib_index          Attribute index of characteristic.
 * @param write_type            GATT write type.
 * @param length                Length of value to be written.
 * @param p_value               Value to be written.
 * @param p_write_ind_post_proc Callback function to set.
 * @return Profile procedure result.
 */
T_APP_RESULT
service_attr_write_cb(
    uint8_t conn_id, T_SERVER_ID service_id, uint16_t attrib_index,
    T_WRITE_TYPE write_type, uint16_t length, uint8_t* p_value,
    P_FUN_WRITE_IND_POST_PROC* p_write_ind_post_proc)
{
    *p_write_ind_post_proc = write_post_callback;
    handle_t handle = {
        .indices = {
            .service_index = service_index_from_id(service_id),
            .attr_index = attrib_index,
        },
    };

    if (0 != mp_bluetooth_gatts_db_write(MP_STATE_PORT(bluetooth_zephyr_root_pointers)->gatts_db, handle.u16, p_value, length)) {
        return APP_RESULT_ATTR_NOT_FOUND;
    }

    mp_bluetooth_gatts_on_write(conn_id, handle.u16);
    
    return APP_RESULT_SUCCESS;
}

/**
 * @brief update CCCD bits from stack.
 *
 * @param conn_id      Connection id.
 * @param service_id   Service ID.
 * @param index        Attribute index of characteristic data.
 * @param cccbits      CCCD bits from stack.
 * @return Profile procedure result.
*/
void service_attr_cccd_update_cb(uint8_t conn_id, T_SERVER_ID service_id, uint16_t index, uint16_t cccbits)
{
}

/**
 * @brief Simple ble Service Callbacks.
 */
const T_FUN_GATT_SERVICE_CBS service_cbs = {
    service_attr_read_cb, // Read callback function pointer
    service_attr_write_cb, // Write callback function pointer
    service_attr_cccd_update_cb // CCCD update callback function
    // pointer
};

int mp_bluetooth_gatts_register_service_end(void) {
    server_init(MP_STATE_PORT(bluetooth_zephyr_root_pointers)->n_services);

    for (size_t i = 0; i < MP_STATE_PORT(bluetooth_zephyr_root_pointers)->n_services; ++i) {
        uint8_t n_attributes = MP_STATE_PORT(bluetooth_zephyr_root_pointers)->services[i].n_attributes;
        Attribute_t *attributes = MP_STATE_PORT(bluetooth_zephyr_root_pointers)->services[i].attributes;
        T_SERVER_ID *id_p = &(MP_STATE_PORT(bluetooth_zephyr_root_pointers)->services[i].id);
        server_add_service(
            id_p,
            (uint8_t*) attributes,
            n_attributes * sizeof(Attribute_t),
            service_cbs);
    }

    server_register_app_cb(app_profile_callback);

    return 0;
}

int mp_bluetooth_gatts_register_service(
    mp_obj_bluetooth_uuid_t *service_uuid,
    mp_obj_bluetooth_uuid_t **characteristic_uuids,
    uint16_t *characteristic_flags,  // contain capability flags (read, write, notify etc.)
    mp_obj_bluetooth_uuid_t **descriptor_uuids,
    uint16_t *descriptor_flags,
    uint8_t *num_descriptors,
    uint16_t *handles,
    size_t num_characteristics
) {
    // primary service declaration
    // uint8_t num_attributes = 1 + 2*num_characteristics;
    uint8_t num_attributes = 1;
    // size_t num_attributes = 1 + 3*num_characteristics;
    // descriptors
    for (size_t i = 0; i < num_characteristics; ++i) {
        PropertyFlags_t property_flags = *((PropertyFlags_t *) &(characteristic_flags[i]));
        // characteristic declaration and value
        num_attributes += 2;
        if (property_flags.notify || property_flags.indicate) {
            // Client Characteristic Configuration Descriptor
            num_attributes += 1;
        }
    }

    uint8_t service_index = MP_STATE_PORT(bluetooth_zephyr_root_pointers)->n_services;
    size_t handle_index = 0;
    uint8_t attr_index = 0;
    Attribute_t *attributes = m_new(Attribute_t, num_attributes);
    
    // primary service attribute
    attributes[attr_index].flags.bits.le = true;
    attributes[attr_index].type.PrimaryValue.uuid = GATT_UUID_PRIMARY_SERVICE;
    attributes[attr_index].value_len = UUID_128BIT_SIZE;
    attributes[attr_index].context.uuid_p = (UUID_t *) &service_uuid->data;
    ++attr_index;

    for (size_t i = 0; i < num_characteristics; ++i) {
        PropertyFlags_t property_flags = *((PropertyFlags_t *) &(characteristic_flags[i]));
        // declaration attribute
        attributes[attr_index].type.UUIDValue.uuid = GATT_UUID_CHARACTERISTIC;
        attributes[attr_index].type.UUIDValue.property_flags.bits = property_flags;
        attributes[attr_index].flags.bits.value_incl = true;
        attributes[attr_index].value_len = 1;
        attributes[attr_index].context.raw = NULL;
        ++attr_index;

        // value attributes
        memcpy(&(attributes[attr_index].type.uuid), characteristic_uuids[i]->data, sizeof(attributes[attr_index].type.uuid));
        attributes[attr_index].flags.bits.uuid_128bit = true;
        attributes[attr_index].flags.bits.value_appl = true;
        attributes[attr_index].value_len = MP_BLUETOOTH_DEFAULT_ATTR_LEN;
        attributes[attr_index].context.raw = NULL;
        attributes[attr_index].permissions.bits.read_authen_req = (property_flags.read) ? All : None;
        attributes[attr_index].permissions.bits.write_authen_req = (property_flags.write || property_flags.write_no_rsp) ? All : None;
        attributes[attr_index].permissions.bits.notify_authen_req = (property_flags.notify || property_flags.indicate) ? All : None;
        
        handle_t *handle_p = (handle_t *)(&handles[handle_index]);
        handle_p->indices.service_index = service_index;
        handle_p->indices.attr_index = attr_index;

        // Allocate the gatts_db storage for this characteristic.
        mp_bluetooth_gatts_db_create_entry(MP_STATE_PORT(bluetooth_zephyr_root_pointers)->gatts_db, handle_p->u16, MP_BLUETOOTH_DEFAULT_ATTR_LEN);

        handle_index++;
        attr_index++;
        
        if (property_flags.notify || property_flags.indicate) {
            // client characteristic configuration description attribute
            attributes[attr_index].type.CharClientValue.uuid = GATT_UUID_CHAR_CLIENT_CONFIG;
            attributes[attr_index].flags.bits.value_incl = true;
            attributes[attr_index].flags.bits.cccd_appl = true;
            attributes[attr_index].value_len = 1;
            attributes[attr_index].context.raw = NULL;
            attributes[attr_index].permissions.bits.write_authen_req = All;
            attributes[attr_index].permissions.bits.read_authen_req = All;
            ++attr_index;
        }
    }

    MP_STATE_PORT(bluetooth_zephyr_root_pointers)->services[service_index].n_attributes = num_attributes;
    MP_STATE_PORT(bluetooth_zephyr_root_pointers)->services[service_index].attributes = attributes;
    MP_STATE_PORT(bluetooth_zephyr_root_pointers)->n_services++;

    return 0;
}

int mp_bluetooth_gap_disconnect(uint16_t conn_handle) {
    if (!mp_bluetooth_is_active()) {
        return ERRNO_BLUETOOTH_NOT_ACTIVE;
    }
    return MP_EOPNOTSUPP;
}

int mp_bluetooth_gatts_read(uint16_t value_handle, const uint8_t **value, size_t *value_len) {
    if (!mp_bluetooth_is_active()) {
        return ERRNO_BLUETOOTH_NOT_ACTIVE;
    }
    return mp_bluetooth_gatts_db_read(MP_STATE_PORT(bluetooth_zephyr_root_pointers)->gatts_db, value_handle, value, value_len);
}

int mp_bluetooth_gatts_write(uint16_t value_handle, const uint8_t *value, size_t value_len, bool send_update) {
    if (!mp_bluetooth_is_active()) {
        return ERRNO_BLUETOOTH_NOT_ACTIVE;
    }
    if (send_update) {
        return MP_EOPNOTSUPP;
    }
    return mp_bluetooth_gatts_db_write(MP_STATE_PORT(bluetooth_zephyr_root_pointers)->gatts_db, value_handle, value, value_len);
}

static inline int server_send(uint8_t conn_id, uint16_t value_handle,
                      uint8_t *p_data, uint16_t data_len, T_GATT_PDU_TYPE type) {
    handle_t handle = {
        .u16 = value_handle,
    };

    T_SERVER_ID service_id = MP_STATE_PORT(bluetooth_zephyr_root_pointers)->services[handle.indices.service_index].id;

    if (!os_sem_take(MP_STATE_PORT(bluetooth_zephyr_root_pointers)->semaphore_send, BLE_TIME_FOREVER)) {
        return MP_EBUSY;
    }
    
    if (!server_send_data(conn_id, service_id, handle.indices.attr_index, p_data, data_len, type)) {
        return MP_EIO;
    }

    return 0;
}

int mp_bluetooth_gatts_notify_indicate(uint16_t conn_handle, uint16_t value_handle, int gatts_op, const uint8_t *value, size_t value_len) {
    if (!mp_bluetooth_is_active()) {
        return ERRNO_BLUETOOTH_NOT_ACTIVE;
    }
    int ret = 0;
    switch (gatts_op) {
        case MP_BLUETOOTH_GATTS_OP_NOTIFY: {
            ret = server_send(conn_handle, value_handle, (uint8_t *)value, value_len, GATT_PDU_TYPE_NOTIFICATION);
            break;
        }
        case MP_BLUETOOTH_GATTS_OP_INDICATE: {
            ret = server_send(conn_handle, value_handle, (uint8_t *)value, value_len, GATT_PDU_TYPE_INDICATION);
            break;
        }

    }
    return ret;
}

// int mp_bluetooth_gatts_notify(uint16_t conn_handle, uint16_t value_handle) {
//     return server_send(conn_handle, value_handle, NULL, 0, GATT_PDU_TYPE_NOTIFICATION);
// }

// int mp_bluetooth_gatts_notify_send(uint16_t conn_handle, uint16_t value_handle, const uint8_t *value, size_t value_len) {
//     return server_send(conn_handle, value_handle, (uint8_t *)value, value_len, GATT_PDU_TYPE_NOTIFICATION);
// }

// int mp_bluetooth_gatts_indicate(uint16_t conn_handle, uint16_t value_handle) {
//     return server_send(conn_handle, value_handle, NULL, 0, GATT_PDU_TYPE_INDICATION);
// }

int mp_bluetooth_gatts_set_buffer(uint16_t value_handle, size_t len, bool append) {
    if (!mp_bluetooth_is_active()) {
        return ERRNO_BLUETOOTH_NOT_ACTIVE;
    }
    return mp_bluetooth_gatts_db_resize(MP_STATE_PORT(bluetooth_zephyr_root_pointers)->gatts_db, value_handle, len, append);
}

int mp_bluetooth_get_preferred_mtu(void) {
    if (!mp_bluetooth_is_active()) {
        mp_raise_OSError(ERRNO_BLUETOOTH_NOT_ACTIVE);
    }
    // just try to get mtu of connection 0
    uint16_t mtu;
    if (GAP_CAUSE_SUCCESS == le_get_conn_param(GAP_PARAM_CONN_MTU_SIZE, &mtu, 0)) {
        return mtu;
    }
    // otherwise assume MP_BLUETOOTH_DEFAULT_ATTR_LEN + 3
    return MP_BLUETOOTH_DEFAULT_ATTR_LEN + 3;
}

int mp_bluetooth_set_preferred_mtu(uint16_t mtu) {
    if (!mp_bluetooth_is_active()) {
        return ERRNO_BLUETOOTH_NOT_ACTIVE;
    }
    gap_config_max_mtu_size(mtu);
    return 0;
}

MP_REGISTER_ROOT_POINTER(struct _mp_bluetooth_zephyr_root_pointers_t *bluetooth_zephyr_root_pointers);

#endif // MICROPY_PY_BLUETOOTH
