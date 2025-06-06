//****************************************************************************
/**
 * @file          Peripheral.c
 * @ingroup       MlBlePeripheralGroup
 * @brief         This file handles BLE peripheral app routines and states.
 *
 * @details       It contains the state machine for all app relevant things
 *                that can happen with gap, bonding, ota, and the services.
 *
 *                The design rationale is to keep the whole ble state machine
 *                in this file and do concrete things elsewhere.
 *
 *                The Services/.[c|h] and GAP.[c|h] contain only
 *                factory functions, their lifecycle happens here.
 *
 *                Because this will file will get quite large please keep
 *                code in case- and if/else-blocks short (1 line, if possible).
 *
 * @par           Project:
 *                Satisfyer Firmware
 * @par           Copyright:
 *                2020 by Simplexion
 * @par           Produced by:
 *                Simplexion
 *
 * @author        Marc Delling
 * @date          14.04.2020
 *
 *****************************************************************************/

/*============================================================================*
 *                              Header Files
 *============================================================================*/

#include <stdio.h>
#include <string.h>

#include "extmod/modbluetooth.h"

#include "Services/OTAService.h"

#include "BLE/BLE.h"
// #include "BLE/Utils.h"
#include "BLE/Peripheral.h"

#include "os/os_sched.h"
#include "os/os_task.h"
#include "os/os_msg.h"
#include "app/app_msg.h"
#include "app/dfu_api.h"
#include "bluetooth/gap/gap.h"
#include "bluetooth/gap/gap_adv.h"
#include "bluetooth/gap/gap_bond_le.h"
#include "bluetooth/gap/gap_conn_le.h"
#include "bluetooth/gap/gap_msg.h"
#include "bluetooth/profile/gatt_builtin_services.h"
#include "bluetooth/profile/profile_server.h"
#include "bluetooth/profile/server/bas.h"
#include "bluetooth/profile/server/dis.h"
#include "bluetooth/profile/server/dis_config.h"
#include "bluetooth/profile/server/ota_service.h"

#define LogLevel (Device.Consts.LogLevels.BluetoothLogLevel)

/*============================================================================*
 *                              Variables
 *============================================================================*/

T_SERVER_ID g_motor_srv_id; /**< Motor ble service id */
T_SERVER_ID g_settings_srv_id; /**< Simple ble service id */
T_SERVER_ID g_bas_srv_id; /**< Battery service id */
T_SERVER_ID g_dis_srv_id; /**< DeviceInformation service id */
T_SERVER_ID g_ota_srv_id; /**< OTA service id */
T_SERVER_ID g_config_srv_id; /**< Config service id */

T_GAP_DEV_STATE gap_dev_state = { 0, 0, 0, 0 }; /**< GAP device state */
T_GAP_CONN_STATE gap_conn_state = GAP_CONN_STATE_DISCONNECTED; /**< GAP connection state */

/// The app privacy state.
T_PRIVACY_STATE app_privacy_state = PRIVACY_STATE_INIT;
/// The app privacy resolution state.
T_PRIVACY_ADDR_RESOLUTION_STATE app_privacy_resolution_state = PRIVACY_ADDR_RESOLUTION_DISABLED;
/// Work mode of the app. This state is to decide if the toy should search for a
/// reconnectable bond, or publicly advertise.
T_APP_WORK_MODE app_work_mode = APP_RECONNECTION_MODE;

/*============================================================================*
 *                              Functions
 *============================================================================*/

void app_handle_gap_msg(T_IO_MSG* p_gap_msg);

/**
 * All the application messages are pre-handled in this function
 * @note     All the IO MSGs are sent to this function, then the event handling
 *           function shall be called according to the MSG type.
 * @param[in] io_msg  IO message data
 */
void app_handle_io_msg(T_IO_MSG io_msg)
{
    switch (io_msg.type) {
    case IO_MSG_TYPE_BT_STATUS: {
        app_handle_gap_msg(&io_msg);
    } break;
    default:
        break;
    }
}

/**
 * @brief Returns the current gap connection state
 *
 * @return the connection state
 */
T_GAP_CONN_STATE
BLE_GetGapConnectionState(void) { return gap_conn_state; }

/**
 * @brief Returns if there are >0 bondings
 *
 * @return true if there is at least one bonding, false otherwise
 */
bool BLE_HasBondings(void) { return le_get_bond_dev_num() > 0; }

/**
 * @brief Turn the Bluetooth Low Energy radio on
 *
 * @param publicAdvertising On false, does whitelisting if bonds are present or
 * advertise publicly if no bonds are present; If true, always advertises
 * publicly
 */
void BLE_Peripheral_On(bool publicAdvertising)
{
    // SLEDsMessage_t led_msg = {0};

    if (publicAdvertising == true) {
        app_work_mode = APP_PAIRABLE_MODE;
        // led_msg.LEDPattern = BLINK3;
    } else {
        app_work_mode = APP_RECONNECTION_MODE;
        // led_msg.LEDPattern = BLINK2;
    }

    // Logf(LogLevel, Debug, "Entering BLE %s mode", (publicAdvertising == true) ? "pairable" : "reconnection");

    app_adv_start();

    // UL_UserIO_UserLEDs_SendMessage(led_msg);

    // #ifdef DEFAULT_BT_INTENSITY_LEVEL
    //     ML_DevicePrograms_IntensitySet(DEFAULT_BT_INTENSITY_LEVEL, 0xff);
    // #endif
}

/**
 * @brief Stop advertising and connections.
 *
 */
void BLE_Peripheral_Off(void)
{
    // SLEDsMessage_t led_msg = {0};

    if (gap_conn_state == GAP_CONN_STATE_CONNECTED) {
        le_disconnect(0);
    }

    // UL_UserIO_UserLEDs_SendMessage(led_msg);
    // Log(LogLevel, Info, "Stopped ble.");
}

/**
 * @brief Start advertising.
 *
 */
void app_adv_start(void)
{
    T_GAP_CAUSE retval;
    uint8_t adv_evt_type = GAP_ADTYPE_ADV_IND;
    uint8_t adv_filter_policy = GAP_ADV_FILTER_ANY;
    uint8_t adv_local_addr = GAP_LOCAL_ADDR_LE_PUBLIC;
    // uint8_t bond_dev_num = le_get_bond_dev_num();

    // if (gap_dev_state.gap_init_state != GAP_INIT_STATE_STACK_READY) {
    // Log(LogLevel, Error, "GAP Stack not ready.");
    // return;
    // }

    // if (bond_dev_num > 0) {
    //     uint8_t random_addr[6] = { 0 };
    //     adv_local_addr = GAP_LOCAL_ADDR_LE_RANDOM;
    //     le_gen_rand_addr(GAP_RAND_ADDR_RESOLVABLE, random_addr);
    //     le_set_rand_addr(random_addr);

    //     if (app_work_mode == APP_RECONNECTION_MODE) {
    //         adv_filter_policy = GAP_ADV_FILTER_WHITE_LIST_ALL;
    //         if (app_privacy_resolution_state == PRIVACY_ADDR_RESOLUTION_DISABLED) {
    //             privacy_set_addr_resolution(true);
    //         }
    //     }
    // } else {
        adv_filter_policy = GAP_ADV_FILTER_ANY;
        if (app_privacy_resolution_state == PRIVACY_ADDR_RESOLUTION_ENABLED) {
            privacy_set_addr_resolution(false);
        }
    // }

    // Logf(LogLevel, Info, "Has %d of %d bonded devices. %s address. %s advertising.",
    //     bond_dev_num,
    //     bond_storage_num,
    //     (adv_local_addr == GAP_LOCAL_ADDR_LE_PUBLIC) ? "Public" : "Random",
    //     (app_work_mode == APP_RECONNECTION_MODE && bond_dev_num > 0) ? "Whitelisted" : "Public");

    le_adv_set_param(GAP_PARAM_ADV_EVENT_TYPE, sizeof(adv_evt_type),
        &adv_evt_type);
    le_adv_set_param(GAP_PARAM_ADV_FILTER_POLICY, sizeof(adv_filter_policy),
        &adv_filter_policy);
    le_adv_set_param(GAP_PARAM_ADV_LOCAL_ADDR_TYPE, sizeof(adv_local_addr),
        &adv_local_addr);

    retval = le_adv_start();
    (void)retval;

    // Logf(LogLevel, Trace, "le_adv_start() retval=%d", retval);
}

/**
 * Callback for BLE privacy management module to notify app.
 *
 * @param[in] type     Callback msy type.
 * @param[in] cb_data  Callback data.
 */
void app_privacy_callback(T_PRIVACY_CB_TYPE type, T_PRIVACY_CB_DATA cb_data)
{
    // Logf(LogLevel, Debug, "app_privacy_callback: type %d", type);
    switch (type) {
    case PRIVACY_STATE_MSGTYPE:
        app_privacy_state = cb_data.privacy_state;
        // Logf(LogLevel, Debug, "PRIVACY_STATE_MSGTYPE: status %d", app_privacy_state);
        break;

    case PRIVACY_RESOLUTION_STATUS_MSGTYPE:
        app_privacy_resolution_state = cb_data.resolution_state;
        // Logf(LogLevel, Debug, "PRIVACY_RESOLUTION_STATUS_MSGTYPE: status %d",
        //     app_privacy_resolution_state);
        break;

    default:
        break;
    }
}

/**
 * Handle msg GAP_MSG_LE_DEV_STATE_CHANGE
 * @note     All the gap device state events are pre-handled in this function.
 *           Then the event handling function shall be called according to the
 * new_state
 * @param[in] new_state  New gap device state
 * @param[in] cause GAP device state change cause
 */
void app_handle_dev_state_evt(T_GAP_DEV_STATE new_state, uint16_t cause)
{
    // Logf(LogLevel, Debug,
    //     "app_handle_dev_state_evt: init state %d, adv state %d, cause 0x%x",
    //     new_state.gap_init_state, new_state.gap_adv_state, cause);

    if ((new_state.gap_init_state == GAP_INIT_STATE_STACK_READY) && (new_state.gap_adv_state == GAP_ADV_STATE_IDLE) && (new_state.gap_conn_state == GAP_CONN_DEV_STATE_IDLE)) {
        privacy_handle_resolv_list();
    }

    if (gap_dev_state.gap_init_state != new_state.gap_init_state) {
        if (new_state.gap_init_state == GAP_INIT_STATE_STACK_READY) {
            // Log(LogLevel, Debug, "GAP stack ready");
            app_adv_start();
        }
    }

    if (gap_dev_state.gap_adv_state != new_state.gap_adv_state) {
        // if (new_state.gap_adv_state == GAP_ADV_STATE_IDLE) {
        //     if (new_state.gap_adv_sub_state == GAP_ADV_TO_IDLE_CAUSE_CONN) {
        //         Log(LogLevel, Debug, "GAP adv stopped because connection created");
        //     } else {
        //         Log(LogLevel, Debug, "GAP adv stopped");
        //     }
        // } else if (new_state.gap_adv_state == GAP_ADV_STATE_ADVERTISING) {
        //     Log(LogLevel, Debug, "GAP adv active");
        // }
    }

    gap_dev_state = new_state;
}

/**
 * Handle msg GAP_MSG_LE_CONN_STATE_CHANGE
 * @note     All the gap conn state events are pre-handled in this function.
 *           Then the event handling function shall be called according to the
 * new_state
 * @param[in] conn_id Connection ID
 * @param[in] new_state  New gap connection state
 * @param[in] disc_cause Use this cause when new_state is
 * GAP_CONN_STATE_DISCONNECTED
 */
void app_handle_conn_state_evt(uint8_t conn_id, T_GAP_CONN_STATE new_state,
    uint16_t disc_cause)
{
    uint16_t conn_interval;
    uint16_t conn_latency;
    uint16_t conn_supervision_timeout;
    uint8_t remote_bd[6];
    T_GAP_REMOTE_ADDR_TYPE remote_bd_type;

    le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &conn_interval, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_LATENCY, &conn_latency, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &conn_supervision_timeout,
        conn_id);
    le_get_conn_addr(conn_id, remote_bd, &remote_bd_type);
    // SLEDsMessage_t led_msg =
    // {
    //     .LEDPattern = OFF,
    //     .LEDNum = 0,
    //     .once = 0
    // };

    // Logf(LogLevel, Debug,
    //     "app_handle_conn_state_evt: conn_id %d old_state %d new_state %d, "
    //     "disc_cause 0x%x",
    //     conn_id, gap_conn_state, new_state, disc_cause);
    switch (new_state) {
    case GAP_CONN_STATE_DISCONNECTED: {
        mp_bluetooth_gap_on_connected_disconnected(MP_BLUETOOTH_IRQ_CENTRAL_DISCONNECT, conn_id, remote_bd_type, remote_bd);
        // ML_HeaterControl_TurnHeatingOff();

        // BatteryService_DisconnectCB();

        // if ((disc_cause != (HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE)) && (disc_cause != (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE))) {
        //     // APP_PRINT_ERROR1(
        //     //     "app_handle_conn_state_evt: connection lost cause 0x%x",
        //     //     disc_cause);
        // }

        // if (OTAService_isOTAPending()) {
        //     // ok to call, since reset is imminent
        //     // Log(LogLevel, Warning, "Switch to OTA mode and resetting device...");
        //     dfu_switch_to_ota_mode();
        //     // HAL_Power_Reset(HAL_POWER_SWITCH_TO_OTA);
        // } else {
        //     // HAL_Power_GlobalDeviceState_t global_device_state =
        //     // HAL_Power_GetGlobalDeviceState(); if (HAL_POWER_STATE_BLE ==
        //     // global_device_state)
        //     //{
            app_adv_start(); // FIXME: check correct behaviour, this starts
        //         // advertising after connection loss
        //         // if (app_work_mode == APP_PAIRABLE_MODE)
        //         // {
        //         //     led_msg.LEDPattern = BLINK3;
        //         // }
        //         // else
        //         // {
        //     //     led_msg.LEDPattern = BLINK2; // FIXME: correct blink pattern?
        //     // }
        //     // UL_UserIO_UserLEDs_SendMessage(led_msg);
        //     //}
        // }
    } break;

    case GAP_CONN_STATE_CONNECTED: {

        mp_bluetooth_gap_on_connected_disconnected(MP_BLUETOOTH_IRQ_CENTRAL_CONNECT, conn_id, remote_bd_type, remote_bd);
        /// @fixme: for reason crashes logging at this point the app...
        // Logf(LogLevel, Debug,
        //     "GAP_CONN_STATE_CONNECTED:remote_bd %s, remote_addr_type %d, conn_interval 0x%x, conn_latency 0x%x, conn_supervision_timeout 0x%x",
        //     TRACE_BDADDR(remote_bd), remote_bd_type, conn_interval, conn_latency,
        //     conn_supervision_timeout);

        /// @todo: only do service changed indications when re-connecting after
        /// changing the services in an ota update
        gatts_service_changed_indicate(conn_id, 0x0001, 0xFFFF);

        // led_msg.LEDPattern = ON;
        // UL_UserIO_UserLEDs_SendMessage(led_msg);
    } break;

    default:
        break;
    }
    gap_conn_state = new_state;
}

/**
 * Handle msg GAP_MSG_LE_AUTHEN_STATE_CHANGE
 * @note     All the gap authentication state events are pre-handled in this
 * function. Then the event handling function shall be called according to the
 * new_state
 * @param[in] conn_id Connection ID
 * @param[in] new_state  New authentication state
 * @param[in] cause Use this cause when new_state is GAP_AUTHEN_STATE_COMPLETE
 */
void app_handle_authen_state_evt(uint8_t conn_id, uint8_t new_state,
    uint16_t cause)
{
    // Logf(LogLevel, Debug, "app_handle_authen_state_evt:conn_id %d, cause 0x%x", conn_id,
    //     cause);

    switch (new_state) {
    case GAP_AUTHEN_STATE_STARTED: {
        // Log(LogLevel, Debug, "app_handle_authen_state_evt: GAP_AUTHEN_STATE_STARTED");
    } break;

    case GAP_AUTHEN_STATE_COMPLETE: {
        if (cause == GAP_SUCCESS) {
            // Log(LogLevel, Debug,
            //     "app_handle_authen_state_evt: GAP_AUTHEN_STATE_COMPLETE pair "
            //     "success");
        } else {
            // Log(LogLevel, Debug,
            //     "app_handle_authen_state_evt: GAP_AUTHEN_STATE_COMPLETE pair "
            //     "failed");
            if (cause == (SM_ERR | SM_ERR_LINK_KEY_MISSING)) {
                // Log(LogLevel, Warning, "Link key missing. Trying to re-bond...");
                le_bond_pair(conn_id); // this triggers re-bonding on android, does
                    // not work for iOS though (supposedly a
                    // security feature), concepter used
                    // le_disconnect() which was totally wrong;
            } else if (cause == (SM_ERR | SM_ERR_LINK_KEY_MISSING | SMP_ERR_PASSKEY_ENTRY_FAIL)) {
                // Log(LogLevel, Warning, "Re-bonding failed due to stale link key."); // iOS comes here after re-bonding attempt
            }
        }
    } break;

    default: {
        // APP_PRINT_ERROR1("app_handle_authen_state_evt: unknown newstate %d",
        //     new_state);
    } break;
    }
}

/**
 * Handle msg GAP_MSG_LE_CONN_MTU_INFO
 * @note     This msg is used to inform APP that exchange mtu procedure is
 * completed.
 * @param[in] conn_id Connection ID
 * @param[in] mtu_size  New mtu size
 */
void app_handle_conn_mtu_info_evt(uint8_t conn_id, uint16_t mtu_size)
{
    // Logf(LogLevel, Debug, "app_handle_conn_mtu_info_evt: conn_id %d, mtu_size %d",
    //     conn_id, mtu_size);
}

/**
 * Handle msg GAP_MSG_LE_CONN_PARAM_UPDATE
 * @note     All the connection parameter update change  events are pre-handled
 * in this function.
 * @param[in] conn_id Connection ID
 * @param[in] status  New update state
 * @param[in] cause Use this cause when status is
 * GAP_CONN_PARAM_UPDATE_STATUS_FAIL
 */
void app_handle_conn_param_update_evt(uint8_t conn_id, uint8_t status,
    uint16_t cause)
{
    switch (status) {
    case GAP_CONN_PARAM_UPDATE_STATUS_SUCCESS: {
        uint16_t conn_interval;
        uint16_t conn_slave_latency;
        uint16_t conn_supervision_timeout;

        le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &conn_interval, conn_id);
        le_get_conn_param(GAP_PARAM_CONN_LATENCY, &conn_slave_latency, conn_id);
        le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &conn_supervision_timeout,
            conn_id);
        // Logf(LogLevel, Debug,
        //     "app_handle_conn_param_update_evt update success:conn_interval 0x%x, "
        //     "conn_slave_latency 0x%x, conn_supervision_timeout 0x%x",
        //     conn_interval, conn_slave_latency, conn_supervision_timeout);
    } break;

    case GAP_CONN_PARAM_UPDATE_STATUS_FAIL: {
        // APP_PRINT_ERROR1(
        //     "app_handle_conn_param_update_evt update failed: cause 0x%x", cause);
    } break;

    case GAP_CONN_PARAM_UPDATE_STATUS_PENDING: {
        // Log(LogLevel, Debug, "app_handle_conn_param_update_evt update pending.");
    } break;

    default:
        break;
    }
}

/**
 * All the BT GAP MSG are pre-handled in this function.
 * @note     Then the event handling function shall be called according to the
 *           subtype of T_IO_MSG
 * @param[in] p_gap_msg Pointer to GAP msg
 */
void app_handle_gap_msg(T_IO_MSG* p_gap_msg)
{
    T_LE_GAP_MSG gap_msg;
    uint8_t conn_id;
    memcpy(&gap_msg, &p_gap_msg->u.param, sizeof(p_gap_msg->u.param));

    // APP_PRINT_TRACE1("app_handle_gap_msg: subtype %d", p_gap_msg->subtype);
    switch (p_gap_msg->subtype) {
    case GAP_MSG_LE_DEV_STATE_CHANGE: {
        app_handle_dev_state_evt(gap_msg.msg_data.gap_dev_state_change.new_state,
            gap_msg.msg_data.gap_dev_state_change.cause);
    } break;

    case GAP_MSG_LE_CONN_STATE_CHANGE: {
        app_handle_conn_state_evt(
            gap_msg.msg_data.gap_conn_state_change.conn_id,
            (T_GAP_CONN_STATE)gap_msg.msg_data.gap_conn_state_change.new_state,
            gap_msg.msg_data.gap_conn_state_change.disc_cause);
    } break;

    case GAP_MSG_LE_CONN_MTU_INFO: {
        app_handle_conn_mtu_info_evt(gap_msg.msg_data.gap_conn_mtu_info.conn_id,
            gap_msg.msg_data.gap_conn_mtu_info.mtu_size);
    } break;

    case GAP_MSG_LE_CONN_PARAM_UPDATE: {
        app_handle_conn_param_update_evt(
            gap_msg.msg_data.gap_conn_param_update.conn_id,
            gap_msg.msg_data.gap_conn_param_update.status,
            gap_msg.msg_data.gap_conn_param_update.cause);
    } break;

    case GAP_MSG_LE_AUTHEN_STATE_CHANGE: {
        app_handle_authen_state_evt(gap_msg.msg_data.gap_authen_state.conn_id,
            gap_msg.msg_data.gap_authen_state.new_state,
            gap_msg.msg_data.gap_authen_state.status);
    } break;

    case GAP_MSG_LE_BOND_JUST_WORK: {
        conn_id = gap_msg.msg_data.gap_bond_just_work_conf.conn_id;
        le_bond_just_work_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
        // Log(LogLevel, Debug, "GAP_MSG_LE_BOND_JUST_WORK");
    } break;

    case GAP_MSG_LE_BOND_PASSKEY_DISPLAY: {
        uint32_t display_value = 0;
        conn_id = gap_msg.msg_data.gap_bond_passkey_display.conn_id;
        le_bond_get_display_key(conn_id, &display_value);
        // Logf(LogLevel, Debug, "GAP_MSG_LE_BOND_PASSKEY_DISPLAY:passkey %d",
        //     display_value);
        le_bond_passkey_display_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
    } break;

    case GAP_MSG_LE_BOND_USER_CONFIRMATION: {
        uint32_t display_value = 0;
        conn_id = gap_msg.msg_data.gap_bond_user_conf.conn_id;
        le_bond_get_display_key(conn_id, &display_value);
        // Logf(LogLevel, Debug, "GAP_MSG_LE_BOND_USER_CONFIRMATION: passkey %d",
        //     display_value);
        le_bond_user_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
    } break;

    case GAP_MSG_LE_BOND_PASSKEY_INPUT: {
        uint32_t passkey = 888888;
        conn_id = gap_msg.msg_data.gap_bond_passkey_input.conn_id;
        // Logf(LogLevel, Debug, "GAP_MSG_LE_BOND_PASSKEY_INPUT: conn_id %d", conn_id);
        le_bond_passkey_input_confirm(conn_id, passkey, GAP_CFM_CAUSE_ACCEPT);
    } break;

    case GAP_MSG_LE_BOND_OOB_INPUT: {
        uint8_t oob_data[GAP_OOB_LEN] = { 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0 };
        conn_id = gap_msg.msg_data.gap_bond_oob_input.conn_id;
        // Log(LogLevel, Debug, "GAP_MSG_LE_BOND_OOB_INPUT");
        le_bond_set_param(GAP_PARAM_BOND_OOB_DATA, GAP_OOB_LEN, oob_data);
        le_bond_oob_input_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
    } break;

    default:
        // APP_PRINT_ERROR1("app_handle_gap_msg: unknown subtype %d",
        //     p_gap_msg->subtype);
        break;
    }
}

/**
 * @defgroup  PERIPH_GAP_CALLBACK GAP Callback Event Handler
 * @ingroup MlBlePeripheralGroup
 * @brief Handle GAP callback event
 * @{
 */

/**
 * Callback for gap le to notify app.
 *
 * @param[in] cb_type callback msy type.
 * @param[in] p_cb_data point to callback data.
 * @retval Success of operation.
 */
T_APP_RESULT
app_gap_callback(uint8_t cb_type, void* p_cb_data)
{
    T_APP_RESULT result = APP_RESULT_SUCCESS;
    T_LE_CB_DATA* p_data = (T_LE_CB_DATA*)p_cb_data;

    switch (cb_type) {
    case GAP_MSG_LE_DATA_LEN_CHANGE_INFO:
        // Logf(LogLevel, Debug,
        //     "GAP_MSG_LE_DATA_LEN_CHANGE_INFO: conn_id %d, tx octets 0x%x, "
        //     "max_tx_time 0x%x",
        //     p_data->p_le_data_len_change_info->conn_id,
        //     p_data->p_le_data_len_change_info->max_tx_octets,
        //     p_data->p_le_data_len_change_info->max_tx_time);
        break;

    case GAP_MSG_LE_MODIFY_WHITE_LIST:
        // Logf(LogLevel, Debug, "GAP_MSG_LE_MODIFY_WHITE_LIST: operation %d, cause 0x%x",
        //     p_data->p_le_modify_white_list_rsp->operation,
        //     p_data->p_le_modify_white_list_rsp->cause);
        break;
    case GAP_MSG_LE_BOND_MODIFY_INFO:
        // Logf(LogLevel, Debug, "GAP_MSG_LE_BOND_MODIFY_INFO: type 0x%x",
        //     p_data->p_le_bond_modify_info->type);
        privacy_handle_bond_modify_msg(p_data->p_le_bond_modify_info->type,
            p_data->p_le_bond_modify_info->p_entry,
            true);
        break;
    default:
        // APP_PRINT_ERROR1("app_gap_callback: unhandled cb_type 0x%x", cb_type);
        break;
    }
    return result;
}
/** @} */ /* End of group PERIPH_GAP_CALLBACK */
