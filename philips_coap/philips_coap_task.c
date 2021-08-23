#include "mxos.h"
#include "mxos_config.h"

#include "philips_coap_internal.h"

#include "philips_param.h"
#include "philips_device_model.h"

#include "philips_log.h"
#define app_log(M, ...)							philips_custom_log("coap", M, ##__VA_ARGS__)

static fog_context_t *fog_context = NULL;
static mos_queue_id_t task_queue = NULL;
static mos_queue_id_t lan_send_queue = NULL;

extern merr_t coap_server_start(void);

static void task_thread(void *arg)
{
	merr_t err = kNoErr;
	task_msg_t msg;

	while ( 1 )
	{
		err = mos_queue_pop(task_queue, &msg, MOS_WAIT_FOREVER);
		if ( err != kNoErr )
			continue;
		// app_log("msg (%ld):%.*s", msg.len, (int)msg.len, msg.msg );

		if ( msg.msg_type == MSG_TYPE_LOCAL_CTL )
		{
			if ( fog_context->status.cloud_status.set_device_status_cb )
				(fog_context->status.cloud_status.set_device_status_cb)((char *)msg.msg, msg.len);
		}
		else if ( msg.msg_type == MSG_TYPE_LOCAL_CMD )
		{
			if ( fog_context->status.cloud_status.get_device_status_cb )
				(fog_context->status.cloud_status.get_device_status_cb)();
		}

		if ( msg.msg != NULL )
		{
			free(msg.msg);
			msg.msg = NULL;
		}
	}
}

//msg_type = 0; control
//msg_type = 0; command
merr_t task_queue_push(char *buf, uint32_t len, uint8_t msg_type)
{
	merr_t err = kNoErr;
	task_msg_t msg;

	require_noerr_action(task_queue == NULL, exit, err = kParamErr);

	msg.msg_type = msg_type;
	msg.len = len;
	if ( buf == NULL )
	{
		msg.msg = NULL;
	}
	else
	{
		msg.msg = malloc(len + 1);
		require_noerr_action(msg.msg == NULL, exit, err = kNoMemoryErr);
		memset(msg.msg, 0x00, len + 1);
		memcpy(msg.msg, buf, len);
	}

	err = mos_queue_push(task_queue, &msg, 1000);

	exit:
	return err;
}

static merr_t fog_register_callback(uint8_t type, void *callback)
{
	merr_t err = kNoErr;

	if ( fog_context == NULL )
	{
		fog_context = malloc(sizeof(fog_context_t));
		memset(fog_context, 0x00, sizeof(fog_context_t));
	}

	switch ( type )
	{
	case FOG_GET_DEVICE_STATUS:
		fog_context->status.cloud_status.get_device_status_cb = callback;
		break;
	case FOG_SET_DEVICE_STATUS:
		fog_context->status.cloud_status.set_device_status_cb = callback;
		break;
	default:
		break;
	}

	return err;
}

void local_device_status_report(char *status)
{
	app_log("try to report status by local");
	fog_locol_report(status, strlen(status));

	return;
}

static void local_device_control(char *buf, uint32_t len)
{
	app_log("get data = %s, length = %lu", buf, len);

	philips_process_cloud_data(buf, (int)len);
}

merr_t philips_coap_server_start(void)
{
	merr_t err = kUnknownErr;

	task_queue = mos_queue_new(sizeof(task_msg_t), 5);
	lan_send_queue = mos_queue_new(sizeof(task_msg_t), 5);

	mos_thread_new(MOS_APPLICATION_PRIORITY, "task", task_thread, 0x1200, NULL);

	err = fog_register_callback(FOG_GET_DEVICE_STATUS, local_device_status_report);
	app_log("res = %d", err);

	err = fog_register_callback(FOG_SET_DEVICE_STATUS, local_device_control);
	app_log("res = %d", err);

	err = coap_server_start();
	require_noerr(err, exit);

	return 1;
	exit:
	app_log("start coap server error");
	return err;
}
