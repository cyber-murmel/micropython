//****************************************************************************
/**
 * @file          BLE.c
 * @ingroup       MlBleGroup
 * @brief         Implementation of the interface for the Bluetooth Low Energy
 *component.
 *
 * @details       BLE task initialization. BLE only has a ML.
 *
 * @par           Project:
 *                Satisfyer Firmware
 * @par           Copyright:
 *                2020 by Simplexion
 * @par           Produced by:
 *                Simplexion
 *
 * @author        Marc Delling
 * @date          18.05.2020
 *
 *****************************************************************************/

/*============================================================================*
 *                              Header Files
 *============================================================================*/

// #include <os_msg.h>
// #include <os_task.h>
// #include <os_timer.h>

// #include <app_msg.h>
#include "os/os_sched.h"
#include "os/os_task.h"
#include "os/os_msg.h"
#include "app/app_msg.h"
// #include <bas.h>
// #include <gap.h>
// #include <gap_le.h>
// #include <gap_lib/gap_config.h>
#include "bluetooth/gap/gap.h"
#include "bluetooth/gap/gap_adv.h"
#include "bluetooth/gap/gap_msg.h"
// #include <trace.h>

#include "app_flags.h"

#include "BLE/BLE.h"
#include "BLE/Peripheral.h"

// #include "Log.h"

#define LogLevel (Device.Consts.LogLevels.BluetoothLogLevel)

/*============================================================================*
 *                              Macros
 *============================================================================*/
#define BLE_TASK_PRIORITY 1 //!< Task priorities
#define BLE_TASK_STACK_SIZE 256 * 8 //!<  Task stack size

#define MAX_NUMBER_OF_GAP_MESSAGE 0x20 //!<  GAP message queue size
#define MAX_NUMBER_OF_IO_MESSAGE 0x20 //!<  IO message queue size
#define MAX_NUMBER_OF_MESSAGES 1 //!<  Message queue sizes
#define MAX_NUMBER_OF_EVENT_MESSAGE \
    (MAX_NUMBER_OF_GAP_MESSAGE + MAX_NUMBER_OF_IO_MESSAGE) //!< Event message queue size

/** @brief  Default minimum advertising interval when device is discoverable (units of 625us, 160=100ms) */
#define DEFAULT_ADVERTISING_INTERVAL_MIN            320
/** @brief  Default maximum advertising interval */
#define DEFAULT_ADVERTISING_INTERVAL_MAX            320

/*============================================================================*
 *                              Variables
 *============================================================================*/

/// The Id of the log channel for the BLE module.
uint8_t bleLogId = 0;

void* ble_task_handle; //!< APP Task handle
void* evt_queue_handle; //!< Event queue handle
void* io_queue_handle; //!< IO queue handle

/*============================================================================*
 *                              Functions
 *============================================================================*/

// /**
//   * @brief  Initialize peripheral and gap bond manager related parameters
//   * @return void
//   */
// void app_le_gap_init(void)
// {
//     /* Device name and device appearance */
//     uint8_t  device_name[GAP_DEVICE_NAME_LEN] = "BLE_PERIPHERAL";
//     uint16_t appearance = GAP_GATT_APPEARANCE_GENERIC_WATCH;
//     uint8_t  slave_init_mtu_req = false;


//     /* Advertising parameters */
//     uint8_t  adv_evt_type = GAP_ADTYPE_ADV_IND;
//     uint8_t  adv_direct_type = GAP_REMOTE_ADDR_LE_PUBLIC;
//     uint8_t  adv_direct_addr[GAP_BD_ADDR_LEN] = {0};
//     uint8_t  adv_chann_map = GAP_ADVCHAN_ALL;
//     uint8_t  adv_filter_policy = GAP_ADV_FILTER_ANY;
//     uint16_t adv_int_min = DEFAULT_ADVERTISING_INTERVAL_MIN;
//     uint16_t adv_int_max = DEFAULT_ADVERTISING_INTERVAL_MAX;

//     /* GAP Bond Manager parameters */
//     uint8_t  auth_pair_mode = GAP_PAIRING_MODE_PAIRABLE;
//     uint16_t auth_flags = GAP_AUTHEN_BIT_BONDING_FLAG;
//     uint8_t  auth_io_cap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
//     uint8_t  auth_oob = false;
//     uint8_t  auth_use_fix_passkey = false;
//     uint32_t auth_fix_passkey = 0;
// #if F_BT_ANCS_CLIENT_SUPPORT
//     uint8_t  auth_sec_req_enable = true;
// #else
//     uint8_t  auth_sec_req_enable = false;
// #endif
//     uint16_t auth_sec_req_flags = GAP_AUTHEN_BIT_BONDING_FLAG;

//     /* Set device name and device appearance */
//     le_set_gap_param(GAP_PARAM_DEVICE_NAME, GAP_DEVICE_NAME_LEN, device_name);
//     le_set_gap_param(GAP_PARAM_APPEARANCE, sizeof(appearance), &appearance);
//     le_set_gap_param(GAP_PARAM_SLAVE_INIT_GATT_MTU_REQ, sizeof(slave_init_mtu_req),
//                      &slave_init_mtu_req);

//     /* Set advertising parameters */
//     le_adv_set_param(GAP_PARAM_ADV_EVENT_TYPE, sizeof(adv_evt_type), &adv_evt_type);
//     le_adv_set_param(GAP_PARAM_ADV_DIRECT_ADDR_TYPE, sizeof(adv_direct_type), &adv_direct_type);
//     le_adv_set_param(GAP_PARAM_ADV_DIRECT_ADDR, sizeof(adv_direct_addr), adv_direct_addr);
//     le_adv_set_param(GAP_PARAM_ADV_CHANNEL_MAP, sizeof(adv_chann_map), &adv_chann_map);
//     le_adv_set_param(GAP_PARAM_ADV_FILTER_POLICY, sizeof(adv_filter_policy), &adv_filter_policy);
//     le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MIN, sizeof(adv_int_min), &adv_int_min);
//     le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MAX, sizeof(adv_int_max), &adv_int_max);
//     le_adv_set_param(GAP_PARAM_ADV_DATA, sizeof(adv_data), (void *)adv_data);
//     le_adv_set_param(GAP_PARAM_SCAN_RSP_DATA, sizeof(scan_rsp_data), (void *)scan_rsp_data);

//     /* Setup the GAP Bond Manager */
//     gap_set_param(GAP_PARAM_BOND_PAIRING_MODE, sizeof(auth_pair_mode), &auth_pair_mode);
//     gap_set_param(GAP_PARAM_BOND_AUTHEN_REQUIREMENTS_FLAGS, sizeof(auth_flags), &auth_flags);
//     gap_set_param(GAP_PARAM_BOND_IO_CAPABILITIES, sizeof(auth_io_cap), &auth_io_cap);
//     gap_set_param(GAP_PARAM_BOND_OOB_ENABLED, sizeof(auth_oob), &auth_oob);
//     le_bond_set_param(GAP_PARAM_BOND_FIXED_PASSKEY, sizeof(auth_fix_passkey), &auth_fix_passkey);
//     le_bond_set_param(GAP_PARAM_BOND_FIXED_PASSKEY_ENABLE, sizeof(auth_use_fix_passkey),
//                       &auth_use_fix_passkey);
//     le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(auth_sec_req_enable), &auth_sec_req_enable);
//     le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_REQUIREMENT, sizeof(auth_sec_req_flags),
//                       &auth_sec_req_flags);

//     /* register gap message callback */
//     le_register_app_cb(app_gap_callback);
// }

// /**
//  * BT inactivity timer callback.
//  *
//  * Will be called when the BT inactivity timeout expires.
//  *
//  * @param[in] handle  Timer handle.
//  */
// static void BLE_InactivityTimeoutCb(void* handle)
// {
//     // TODO: this function should pass a message to App_task
//     BLE_Off();
// }

/**
 * Task for handling BLE messages.
 *
 * @param p_param Unused parameter.
 */
static void BLE_task(void* p_param)
{
    uint8_t event; //, bat_msg;
    // heater_notification_t htr_msg;

    // os_msg_queue_create(&io_queue_handle, MAX_NUMBER_OF_IO_MESSAGE,
    //     sizeof(T_IO_MSG));
    // os_msg_queue_create(&evt_queue_handle, MAX_NUMBER_OF_EVENT_MESSAGE,
    //     sizeof(uint8_t));
    // os_msg_queue_create(&bat_queue_handle, MAX_NUMBER_OF_MESSAGES,
    // sizeof(uint8_t)); os_msg_queue_create(&htr_queue_handle,
    // MAX_NUMBER_OF_MESSAGES, sizeof(heater_notification_t));

    // gap_start_bt_stack(evt_queue_handle, io_queue_handle,
    //     MAX_NUMBER_OF_GAP_MESSAGE);

    // uint32_t btInactivityTimerId = 42; // FIXME
    // if ((os_timer_create(&btInactivityTimerHandle, "BtInactivity",
    //         btInactivityTimerId, BT_INACTIVITY_TIMEOUT_S * 1000,
    //         false, BLE_InactivityTimeoutCb))
    //     != true) {
    //     // HAL_Com_Log_Send(pwrCtrlLogId, HAL_COM_LOG_ERROR, "Could not create
    //     // btInactivityTimer!");

    //     Log(LogLevel, Error, "Could not create btInactivityTimer!");
    //     return;
    // }

    while (true) {
        if (os_msg_recv(evt_queue_handle, &event, 250) == true) {
            if (event == EVENT_IO_TO_APP) {
                T_IO_MSG io_msg;
                if (os_msg_recv(io_queue_handle, &io_msg, 0) == true) {
                    app_handle_io_msg(io_msg);
                }
            } else {
                gap_handle_msg(event);
            }
        }
        // if (os_msg_recv(bat_queue_handle, &bat_msg, 0) == true)
        // {
        //     /// @todo: get conn_id from Peripheral.c
        //     bas_battery_level_value_notify(0, g_bas_srv_id, bat_msg);
        // }
        // if (os_msg_recv(htr_queue_handle, &htr_msg, 0) == true)
        // {
        //     sf_settings_ble_service_send_notify_heater(0, g_settings_srv_id,
        //     &htr_msg);
        // }
    }
}

/**
 * @brief Start BLE.
 *
 */
void BLE_On(void)
{
    os_task_resume(ble_task_handle);

    gap_start_bt_stack(evt_queue_handle, io_queue_handle,
        MAX_NUMBER_OF_GAP_MESSAGE);

    // // gap_write_airplan_mode(0x01); // Start off

    // le_gap_init(APP_MAX_LINKS);
    // gap_lib_init();
    // app_le_gap_init();
    // app_le_profile_init();

    // // Log(LogLevel, Trace, "Turning BLE on");
    // // We are disabling the RC receiver here, because it conflicts with BLE!
    // // HAL_Output_GpioOutput_DisableRcReceiver();

    // // gap_write_airplan_mode(0x00);

    // // uint32_t btInactivityTimerState;
    // // os_timer_state_get(&btInactivityTimerHandle, &btInactivityTimerState);
    // // if (btInactivityTimerState == false) {
    // //     os_timer_start(&btInactivityTimerHandle);
    // // }

    // BLE_Peripheral_On(true); // TODO
    // // Log(LogLevel, Debug, "Turned BLE on");
}

/**
 * @brief Stop BLE.
 *
 */
void BLE_Off(void)
{
    // Log(LogLevel, Trace, "Turning BLE off");
    BLE_Peripheral_Off();
    gap_write_airplan_mode(0x01);
    // Log(LogLevel, Debug, "Turned BLE off");
}

// /**
//  * Gets the BT inactivity timer.
//  *
//  * @return the current bt inactivity timeout in seconds.
//  */
// uint32_t BLE_GetInactivityTimer(void) { return btInactivityTimeout; }

// /**
//  * Sets the BT inactivity timer.
//  * @details This function does not need to reset the timer implicitly because it
//  * always happens explicitly in the state machine after accessing ble
//  * characteristics.
//  * @see ML_PowerControl_ResetBtInactivityTimer
//  *
//  * @param timeout in seconds
//  */
// void BLE_SetInactivityTimer(uint32_t timeout)
// {
//     if (timeout >= BT_INACTIVITY_TIMEOUT_MIN_S && timeout <= BT_INACTIVITY_TIMEOUT_MAX_S) {
//         btInactivityTimeout = timeout;
//     }
// }

// /**
//  * Resets the BT inactivity timer.
//  *
//  */
// void BLE_ResetInactivityTimer(void)
// {
//     os_timer_restart(&btInactivityTimerHandle, btInactivityTimeout * 1000);
// }

/**
 * The BLE components of the middle layer will be initialized.
 */
void init_BLE(void)
{

    os_msg_queue_create(&io_queue_handle, MAX_NUMBER_OF_IO_MESSAGE,
        sizeof(T_IO_MSG));
    os_msg_queue_create(&evt_queue_handle, MAX_NUMBER_OF_EVENT_MESSAGE,
        sizeof(uint8_t));

    os_task_create(&ble_task_handle, "ble", BLE_task, 0, BLE_TASK_STACK_SIZE,
        BLE_TASK_PRIORITY);
    
    os_task_suspend(ble_task_handle);
}
