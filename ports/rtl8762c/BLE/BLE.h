//****************************************************************************
/**
 * @file          BLE.h
 * @ingroup       BleGroup
 * @brief         Interface for the Bluetooth Low Energy component.
 *
 * @details       With this interface basic BLE communication is declared.
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

#ifndef _BLE_H
#define _BLE_H

/**
 * @defgroup BleGroup BLE
 * @ingroup Group
 * @brief The BLE component implements the middle level for BLE communication
 * between the App and the firmware.
 *
 * This component contains the BLE pairing and characteristics.
 * @{
 */

/**
 * @defgroup BleServiceGroup BLE Services
 * @ingroup BleGroup
 * @brief The BLE services implements the middle layer services for BLE
 * communication between the App and the firmware.
 *
 * This component contains available services by the app like Info-, Motor- and
 * CommandService.
 */

/*============================================================================*
 *                              Includes
 *============================================================================*/

#include <stdint.h>

/*============================================================================*
 *                              Declarations
 *============================================================================*/

/// Battery queue handle.
// extern void* bat_queue_handle;
/// Heater queue handle.
// extern void* htr_queue_handle;

/// The log channel ID for the BLE module.
extern uint8_t bleLogId;

/*============================================================================*
 *                              Functions
 *============================================================================*/

void BLE_On(void);
void BLE_Off(void);
uint32_t BLE_GetInactivityTimer(void);
void BLE_SetInactivityTimer(uint32_t timeout);
void BLE_ResetInactivityTimer(void);

/**
 * @brief Initializes the BLE components of the middle layer.
 */
void init_BLE(void);

/**@}*/ // BleGroup

#endif // _BLE_H
