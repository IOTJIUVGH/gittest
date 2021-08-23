/**
******************************************************************************
* @file    mxos_config.h
* @author  William Xu
* @version V1.0.0
* @date    08-Aug-2018
* @brief   This file provide application options diff to default.
******************************************************************************
*/

#ifndef __MXOS_CONFIG_H
#define __MXOS_CONFIG_H

#define APP_INFO   "PHILIPS_AWS_2021"

// #define ENVIRONMENT_STAGING
// #define ENVIRONMENT_PRODUCTION

// #define PHILIPS_SECURITY
#define PHILIPS_COAP_DEBUG_ENABLE
// #define PHILIPS_COAP_TRC_ENABLE

#define WIFIWARE_VERSION_PREFIX					"AWS_Philips_AIR@"
#define WIFIWARE_VERSION_MAJOR					0
#define WIFIWARE_VERSION_MINOR					67

#ifndef ENVIRONMENT_PRODUCTION
#define ENVIRONMENT_FLAG                        "@S"
#else
#define ENVIRONMENT_FLAG
#endif

#ifndef PHILIPS_SECURITY
#define SECURITY_FLAG                           "@NOS"
#else
#define SECURITY_FLAG
#endif

#define _VERSION(V)                             #V
#define VERSION(V)                              _VERSION(V)

#define FIRMWARE_REVISION                       WIFIWARE_VERSION_PREFIX VERSION(WIFIWARE_VERSION_MINOR) ENVIRONMENT_FLAG SECURITY_FLAG

#define FIRMWARE_REVISION_MINOR                 "0.6.7"
#define SERIAL_NUMBER                           "0163.PO11.0067"

#define MANUFACTURER                            "MXCHIP Inc."
#define PROTOCOL                                "com.mxchip.basic"

/************************************************************************
 * Application thread stack size */
#define MXOS_DEFAULT_APPLICATION_STACK_SIZE         (4 * 1024)

/************************************************************************
 * Enable wlan connection, start easylink configuration if no wlan settings are existed */
#define MXOS_WLAN_CONNECTION_ENABLE     0

#define MXOS_WLAN_CONFIG_MODE WIFI_CONFIG_MODE_AWS

#define EasyLink_Needs_Reboot

#define EasyLink_TimeOut                60000 /**< EasyLink timeout 60 seconds. */

#define EasyLink_ConnectWlan_Timeout    20000 /**< Connect to wlan after configured by easylink.
                                                   Restart easylink after timeout: 20 seconds. */

/************************************************************************
 * Device enter MFG mode if MICO settings are erased. */
// #define MFG_MODE_AUTO 

/************************************************************************
 * Command line interface */
#ifndef PHILIPS_SECURITY
#define MXOS_CLI_ENABLE                             1
#endif

/************************************************************************
 * Start a system monitor daemon, application can register some monitor  
 * points, If one of these points is not executed in a predefined period, 
 * a watchdog reset will occur. */
#define MXOS_SYSTEM_MONITOR_ENABLE                  1

/************************************************************************
 * Add service _easylink._tcp._local. for discovery */
#define MXOS_SYSTEM_DISCOVERY_ENABLE                0

/************************************************************************
 * MiCO TCP server used for configuration and ota. */
#define MXOS_CONFIG_SERVER_ENABLE                   0
#define MXOS_CONFIG_SERVER_PORT                     8000

/**
 *  MXOS_CONFIG_EASYLINK_BTN_ENABLE: Enable EasyLink Button,
 *  - Press to start easylink
 *  - Long pressed  @ref MXOS_CONFIG_EASYLINK_BTN_LONG_PRESS_TIMEOUT milliseconds
 *    to clear all settings
 *  Default: Enable
 */
#define MXOS_CONFIG_EASYLINK_BTN_ENABLE             0

#endif
