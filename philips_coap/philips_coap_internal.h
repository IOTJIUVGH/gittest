#ifndef __PHILIPS_COAP_INTERNAL_H__
#define __PHILIPS_COAP_INTERNAL_H__

// #include "common.h"
#include "mxos.h"

typedef enum _conn_e
{
	FOG_CONNECT,
	FOG_DISCONNECT
} conn_e;

typedef enum _cb_e
{
	FOG_GET_DEVICE_STATUS,       //void    (*get_device_status)( void );
	FOG_SET_DEVICE_STATUS,       //void    (*set_device_status)( char *buf, uint32_t len );
} cb_e;

typedef enum _msg_type_e
{
	MSG_TYPE_LOCAL_CTL,
	MSG_TYPE_LOCAL_CMD,
} msg_type_e;

typedef struct _task_msg_t
{
	uint8_t msg_type;
	uint32_t len;
	uint8_t *msg;
} task_msg_t;

typedef struct _fog_flash_cb_t
{
	void (*flash_read_cb)(uint8_t *buf, uint32_t len);
	void (*flash_write_cb)(uint8_t *buf, uint32_t len);
} fog_flash_cb_t;

typedef struct _fog_cloud_status_t
{
	void (*get_device_status_cb)(void);
	void (*set_device_status_cb)(char *buf, uint32_t len);
} fog_cloud_status_t;

typedef struct _fog_status_t
{
	fog_cloud_status_t cloud_status;
} fog_status_t;

typedef struct _fog_context_t
{
	fog_status_t status;
} fog_context_t;


/*******************************************************************************
 * lib ---> application
 * used by philips_coap_process.c
 *******************************************************************************/
void fog_locol_report(char *data, uint32_t len);
merr_t task_queue_push(char *buf, uint32_t len, uint8_t msg_type);

/*******************************************************************************
 * application ---> lib
 * used by philips_coap_task.c
 *******************************************************************************/
merr_t coap_server_start(void);

#endif
