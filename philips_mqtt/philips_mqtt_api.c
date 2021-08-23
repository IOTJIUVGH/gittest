#include "mxos.h"

#include "philips_param.h"
#include "philips_mqtt_api.h"
#include "philips_https_request.h"

#include "philips_log.h"
#define app_log(M, ...)             			philips_custom_log("", M, ##__VA_ARGS__)

merr_t philips_mqtt_public_status(const char *status)
{
	app_log("free_memory = %d", mos_mallinfo()->free);
	merr_t err = kNoErr;

	queue_message_t queue_msg = {MT_Unknow, NULL, 0};

	require_action(get_philips_running_status()->wifi_status == WRS_wifiui_connect_cloud, exit, err = kConnectionErr);
	
	require_action(status != NULL, exit, err = kParamErr);

	queue_msg.data_len = UPLOAD_STATUS_MAX_LENGTH;
	queue_msg.data = (uint8_t *)malloc(queue_msg.data_len);
	require_action(queue_msg.data != NULL, exit, err = kNoMemoryErr);
	memset(queue_msg.data, 0x00, queue_msg.data_len);

	// set message type
	queue_msg.type = MT_SHADOW_UPDATE;

	// set json data string
	strcpy(queue_msg.data, status);
	queue_msg.data_len = strlen((char *)queue_msg.data);

    err = philips_mqtt_msg_push_queue(&queue_msg);
	require_noerr(err, exit);

exit:
	if ( err != kNoErr )
	{
		if (queue_msg.data)
		{
			free(queue_msg.data);
			queue_msg.data = NULL;
		}
	}
	app_log("free_memory = %d", mos_mallinfo()->free);
	return err;
}

/**
 * {"DeviceId":"","err":"xxxxx"}
 * @param error_code
 * @return
 */
merr_t philips_mqtt_public_notice(const char *code)
{
	app_log("free_memory = %d", mos_mallinfo()->free);
	merr_t err = kNoErr;

	cJSON *json = NULL;
	queue_message_t queue_msg = {MT_Unknow, NULL, 0};

	require_action(get_philips_running_status()->wifi_status == WRS_wifiui_connect_cloud, exit, err = kConnectionErr);

	require_action(code != NULL, exit, err = kParamErr);

	json = cJSON_CreateObject();
	require_action(json, exit, err = kNoResourcesErr);

	cJSON_AddStringToObject(json, "DeviceId", get_philips_running_status()->flash_param.device_id);
	cJSON_AddStringToObject(json, "err", code);

	// set message type
	queue_msg.type = MT_DEVICE_NOTICE;

	// set json data string
	queue_msg.data = (uint8_t *)(cJSON_PrintUnformatted(json));
	queue_msg.data_len = strlen((char *)(queue_msg.data));

	err = philips_mqtt_msg_push_queue(&queue_msg);
    require_noerr(err, exit);

	exit:
	if ( err != kNoErr )
	{
		if ( queue_msg.data != NULL )
		{
			free(queue_msg.data);
			queue_msg.data = NULL;
		}
		app_log("%s exit with err = %d", __FUNCTION__, err);
	}
	if ( json != NULL )
	{
		cJSON_Delete(json);
		json = NULL;
	}
	app_log("free_memory = %d", mos_mallinfo()->free);
	return err;
}

merr_t philips_mqtt_public_notice_json_str(const char *code)
{
	app_log("free_memory = %d", mos_mallinfo()->free);
	merr_t err = kNoErr;

	queue_message_t queue_msg = {MT_Unknow, NULL, 0};

	require_action(get_philips_running_status()->wifi_status == WRS_wifiui_connect_cloud, exit, err = kConnectionErr);

	require_action(code != NULL, exit, err = kParamErr);

	queue_msg.data_len = strlen(code) + 1;
	queue_msg.data = (uint8_t *)malloc(queue_msg.data_len);
	require_action(queue_msg.data != NULL, exit, err = kNoMemoryErr);
	memset(queue_msg.data, 0x00, queue_msg.data_len);

	// set message type
	queue_msg.type = MT_DEVICE_NOTICE;

	// set json data string
	strcpy(queue_msg.data, code);
	queue_msg.data_len = strlen((char *)(queue_msg.data));

	err = philips_mqtt_msg_push_queue(&queue_msg);
    require_noerr(err, exit);

	exit:
	if ( err != kNoErr )
	{
		if ( queue_msg.data != NULL )
		{
			free(queue_msg.data);
			queue_msg.data = NULL;
		}
	}
	app_log("free_memory = %d", mos_mallinfo()->free);
	
	return err;
}

static cJSON *json_array = NULL;		//每10min设备状态存储一次

merr_t philips_mqtt_public_data(const char *status)
{
	app_log("free_memory = %d", mos_mallinfo()->free);
	merr_t err = kNoErr;

	queue_message_t queue_msg = {MT_Unknow, NULL, 0};
	cJSON *json = NULL;

	require_action(status != NULL, exit, err = kParamErr);

	json = cJSON_Parse(status);
	require_action(json, exit, err = kParamErr);

	if ( json_array == NULL )
	{
		json_array = cJSON_CreateArray();
		require_action(json_array, exit, err = kNoResourcesErr);
	}

	cJSON_AddItemToArray(json_array, json);
	json = NULL;

	if ( cJSON_GetArraySize(json_array) == UPLOAD_DATA_MAX_COUNT )
	{
		if ( get_philips_running_status()->wifi_status == WRS_wifiui_connect_cloud )
		{
			queue_msg.data = (uint8_t *)(cJSON_PrintUnformatted(json_array));
			queue_msg.data_len = strlen((char *)(queue_msg.data));
			queue_msg.type = MT_DEVICE_DATA;

            err = philips_mqtt_msg_push_queue(&queue_msg);
			require_noerr(err, exit);

			cJSON_Delete(json_array);
			json_array = NULL;
		}
		else
		{
			cJSON_DeleteItemFromArray(json_array, 0);
		}
	}

	exit:
	if ( err != kNoErr )
	{
		if(queue_msg.data)
		{
			free(queue_msg.data);
			queue_msg.data = NULL;
		}
		app_log("%s exit with err = %d", __FUNCTION__, err);
	}
	if ( json != NULL )
	{
		cJSON_Delete(json);
		json = NULL;
	}
	app_log("free_memory = %d", mos_mallinfo()->free);
	return err;
}

merr_t philips_mqtt_publish_notice_wifi_log(void)
{
	app_log("free_memory = %d", mos_mallinfo()->free);
	merr_t err = kNoErr;
	char *json = NULL;

	queue_message_t queue_msg = {MT_Unknow, NULL, 0};

	require_action(get_philips_running_status()->wifi_status == WRS_wifiui_connect_cloud, exit, err = kConnectionErr);

	philips_wifi_log_package(&json);
	require_action( (json != NULL), exit, err = kNoResourcesErr);

	// set message type
	queue_msg.type = MT_DEVICE_NOTICE;

	// set json data string
	queue_msg.data = (uint8_t *)json;
	queue_msg.data_len = strlen((char *)(queue_msg.data));

	err = philips_mqtt_msg_push_queue(&queue_msg);
    require_noerr(err, exit);

exit:
	if ( err != kNoErr )
	{
		if ( json != NULL )
		{
			free(json);
			json = NULL;
		}
	}
	app_log("free_memory = %d", mos_mallinfo()->free);
	return err;
}