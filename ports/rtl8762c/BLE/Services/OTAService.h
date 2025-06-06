//****************************************************************************
/**
 * @file          OTAService.h
 * @ingroup       MlBleOTAServiceGroup
 * @brief         
 *
 * @details       
 *
 * @par           Project:
 *                Satisfyer Firmware
 * @par           Copyright:
 *                2020 by Simplexion
 * @par           Produced by:
 *                Simplexion
 *
 * @author        marble
 * @date          21.06.2021
 *
 *****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef _SF_OTA_BLE_SERVICE_H_
#define _SF_OTA_BLE_SERVICE_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <profile_server.h>

/**
 * @defgroup MlBleOTAServiceGroup OTA Ble Service
 * @ingroup MlBleServiceGroup
 * @brief OTA BLE service
 * @{
 */

bool OTAService_isOTAPending();

/**
 * @brief Add OTA BLE service to the BLE stack database.
 */
bool OTAService_Add();

/** @} End of OTA_Service */

#ifdef __cplusplus
}
#endif

#endif /* _SF_OTA_BLE_SERVICE_H_ */
