//****************************************************************************
/**
 * @file          Peripheral.h
 * @ingroup       MlBleGroup
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

#ifndef _PERIPHERAL_BLE__
#define _PERIPHERAL_BLE__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup BlePeripheralGroup BLE Peripheral
 * @ingroup BleGroup
 * @brief BLE peripheral application routines.
 * @{
 */

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include <app_msg.h>
#include <gap_le.h>
#include <gap_msg.h>
#include <profile_server.h>
#include <stdbool.h>

#include "privacy_mgnt.h"

#include "app_flags.h"

/**
 * Possible workmodes of the App.
 */
typedef enum {
    APP_PAIRABLE_MODE, /**< Pairable mode. */
    APP_RECONNECTION_MODE, /**< Reconnection mode. */
} T_APP_WORK_MODE;

/**
 * @todo FIXME: Realtek BUG. Defined in a .c file, not a header. Redefined here.
 */
#define GATT_UUID_DEVICE_INFORMATION_SERVICE 0x180A

/*============================================================================*
 *                              Variables
 *============================================================================*/
extern T_SERVER_ID g_bas_srv_id; /**< Battery service id */
extern T_SERVER_ID g_dis_srv_id; /**< DeviceInformation service id */
extern T_SERVER_ID g_ota_srv_id; /**< OTA service id */

/*============================================================================*
 *                              Functions
 *============================================================================*/

/**
 * @brief    All the application messages are pre-handled in this function
 */
void app_handle_io_msg(T_IO_MSG io_msg);

/**
 * @brief Callback for gap le to notify app
 */
T_APP_RESULT app_gap_callback(uint8_t cb_type, void* p_cb_data);

/**
 * @brief Stop advertising and connections.
 */
void app_adv_conn_stop(void);

/**
 * @brief Start advertising.
 *
 */
void app_adv_start(void);

/**
 * Callback for BLE privacy management module to notify app.
 *
 * @param[in] type     Callback msy type.
 * @param[in] cb_data  Callback data.
 */
void app_privacy_callback(T_PRIVACY_CB_TYPE type, T_PRIVACY_CB_DATA cb_data);

/**
 * @brief Turn the Bluetooth Low Energy radio on
 *
 * @param publicAdvertising On false, does whitelisting if bonds are present
 * or advertise publicly if no bonds are present; If true, always advertises
 * publicly
 */
void BLE_Peripheral_On(bool publicAdvertising);

/**
 * @brief Returns the current gap connection state
 */
T_GAP_CONN_STATE BLE_GetGapConnectionState(void);

/**
 * @brief Returns if there are >0 bondings
 *
 * @return true if there is at least one bonding, false otherwise
 */
bool BLE_HasBondings(void);

/**
 * @brief Turn the Bluetooth Low Energy radio off
 *
 */
void BLE_Peripheral_Off(void);

/**@}*/ // BlePeripheralGroup

#ifdef __cplusplus
}
#endif

#endif
