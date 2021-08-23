#ifndef __PHILIPS_DEVICE_UPLOAD_MANAGE_H__
#define __PHILIPS_DEVICE_UPLOAD_MANAGE_H__

#include "mxos.h"

#define STATUS_TYPE_STATUS "status"
#define STATUS_YTPE_CONTROL "control"
#define STATUS_TYPE_CONNECT "connect"
#define STATUS_TYPE_DISCONNECT "disconnect"

#define CONNECT_TYPE_ONLINE "Online"
#define CONNECT_TYPE_OFFLINE "Offline"

void device_status_upload_manage(void);

#endif