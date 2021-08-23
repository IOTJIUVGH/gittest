#include "mxos.h"
#include "cJSON.h"
#include "mkv.h"

#include "philips_device.h"
#include "philips_device_model.h"
#include "philips_param.h"

//#define device_Mario_log(M, ...)				custom_log("", M, ##__VA_ARGS__)
#define device_Mario_log(M, ...)

//Mario
enum _Air_properties_property_no_Mario
{
	APPN_Mario_FanSpeed							= 0x01,
	APPN_Mario_Power							= 0x02,
	APPN_Mario_ChildLock						= 0x03,
	APPN_Mario_AQILight							= 0x04,
	APPN_Mario_UILight							= 0x05,//
	APPN_Mario_DeviceTimer						= 0x06,
	APPN_Mario_DeviceTimerRemaining				= 0x07,
	APPN_Mario_OperationMode					= 0x08,
	APPN_Mario_Function							= 0x09,//
	APPN_Mario_RHSetting						= 0x0A,
	APPN_Mario_RH								= 0x0B,
	APPN_Mario_Temperature						= 0x0C,
	APPN_Mario_PM2_5							= 0x0D,
	APPN_Mario_iAQL								= 0x0E,
	APPN_Mario_AQIt								= 0x0F,
	APPN_Mario_DeviceDisplayParameter			= 0x10,
	APPN_Mario_RealtimeDeviceDisplayParameter	= 0x11,
	APPN_Mario_ErrorCode						= 0x12,
	APPN_Mario_WaterLevel						= 0x13,
};

void philips_encode_json_Mario(char **buf, const char *StatusType, const char *ConnectType, bool head)
{
	cJSON *root = NULL;
	cJSON *item = NULL;

	root = cJSON_CreateObject();
	require( (root != NULL), exit );

	if(head)
	{
		item = cJSON_AddObjectToObject(root, "state");
		require( (item != NULL), exit );
		item = cJSON_AddObjectToObject(item, "reported");
		require( (item != NULL), exit );
	}
	else
	{
		item = root;
	}

	cJSON_AddStringToObject(item, "name", get_philips_running_status()->device_name);
	cJSON_AddStringToObject(item, "type", get_philips_running_status()->device_type);
	cJSON_AddStringToObject(item, "modelid", get_philips_running_status()->flash_param.device_modelid);
	cJSON_AddStringToObject(item, "swversion", get_philips_running_status()->device_swversion);
	cJSON_AddStringToObject(item, "DeviceVersion", get_philips_running_status()->otau_run_state.local_device_version[0] == '\0' ? "0.0.0" : get_philips_running_status()->otau_run_state.local_device_version);
	cJSON_AddStringToObject(item, "range", "Mario");
	cJSON_AddNumberToObject(item, "Runtime", (double)mos_time());
	cJSON_AddNumberToObject(item, "rssi", get_philips_running_status()->rssi);
	cJSON_AddNumberToObject(item, "free_memory", mos_mallinfo()->free);
	cJSON_AddStringToObject(item, "WifiVersion", FIRMWARE_REVISION);
	cJSON_AddStringToObject(item, "ProductId", get_philips_running_status()->flash_param.product_id);
	cJSON_AddStringToObject(item, "DeviceId", get_philips_running_status()->flash_param.device_id);
	cJSON_AddStringToObject(item, "StatusType", StatusType);
	cJSON_AddStringToObject(item, "ConnectType", ConnectType);
	
	cJSON_AddStringToObject(item, "om", purifier_status.om_str);
	cJSON_AddStringToObject(item, "pwr", purifier_status.pwr_str);
	cJSON_AddBoolToObject(item, "cl", purifier_status.cl ? true : false);
	cJSON_AddNumberToObject(item, "aqil", purifier_status.aqil);
	cJSON_AddStringToObject(item, "uil", purifier_status.uil_str);
	cJSON_AddNumberToObject(item, "dt", purifier_status.dt);
	cJSON_AddNumberToObject(item, "dtrs", purifier_status.dtrs);
	cJSON_AddStringToObject(item, "mode", purifier_status.mode_str);
	cJSON_AddStringToObject(item, "func", purifier_status.func);
	cJSON_AddNumberToObject(item, "rhset", purifier_status.rhset);
	cJSON_AddNumberToObject(item, "rh", purifier_status.rh);
	cJSON_AddNumberToObject(item, "temp", purifier_status.temp);
	cJSON_AddNumberToObject(item, "pm25", purifier_status.pm25);
	cJSON_AddNumberToObject(item, "iaql", purifier_status.iaql);
	cJSON_AddNumberToObject(item, "aqit", get_philips_running_status()->flash_param.aqit);
	cJSON_AddStringToObject(item, "ddp", purifier_status.ddp_str);
	cJSON_AddStringToObject(item, "rddp", purifier_status.rddp_str);
	cJSON_AddNumberToObject(item, "err", purifier_status.err);
	cJSON_AddNumberToObject(item, "wl", purifier_status.wl);
	cJSON_AddStringToObject(item, "fltt1", purifier_status.fltt1);
	cJSON_AddStringToObject(item, "fltt2", purifier_status.fltt2);
	cJSON_AddNumberToObject(item, "fltsts0", purifier_status.fltsts0);
	cJSON_AddNumberToObject(item, "fltsts1", purifier_status.fltsts1);
	cJSON_AddNumberToObject(item, "fltsts2", purifier_status.fltsts2);
	cJSON_AddNumberToObject(item, "wicksts", purifier_status.wicksts);

	if(purifier_status.filter_prop_map & PHILIPS_FILTER_PROP_MAP_FLTTOTAL0)
		cJSON_AddNumberToObject(item, "flttotal0", purifier_status.flttotal0);

	if(purifier_status.filter_prop_map & PHILIPS_FILTER_PROP_MAP_FLTTOTAL1)
		cJSON_AddNumberToObject(item, "flttotal1", purifier_status.flttotal1);

	if(purifier_status.filter_prop_map & PHILIPS_FILTER_PROP_MAP_FLTTOTAL2)
		cJSON_AddNumberToObject(item, "flttotal2", purifier_status.flttotal2);

	*buf = cJSON_PrintUnformatted(root);

exit:
	if(root)
	{
		cJSON_Delete(root);
		root = NULL;
	}
	return ;
}

void philips_process_air_properties_Mario(const uint8_t *data)
{
    const uint8_t *p = data+7;
	int i_value = 0;
	char str_value[10] = {'0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'};
	bool b_value = false;

    while (*(p) != 0x00)
    {
		switch ( *(p) )
		{
		case APPN_Mario_FanSpeed:
		{
			switch ( *(p + 2) )
			{
			case 0x00: str_value[0] = '0'; str_value[1] = '\0';break;
			case 0x01: str_value[0] = '1'; str_value[1] = '\0';break;
			case 0x02: str_value[0] = '2'; str_value[1] = '\0';break;
			case 0x03: str_value[0] = '3'; str_value[1] = '\0';break;
			case 0x04: str_value[0] = 's'; str_value[1] = '\0';break;
			case 0x05: str_value[0] = 't'; str_value[1] = '\0';break;
			default: str_value[0] = '\0'; break;
			}
			if ( str_value[0] == '\0' )
				break;
			device_Mario_log("APPN_FanSpeed: value = %s, purifier_status.om = %s", str_value, purifier_status.om_str);
			if ( strcmp(str_value, purifier_status.om_str) != 0 )
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				strcpy(purifier_status.om_str, str_value);
			}
			break;
		}
		case APPN_Mario_Power:
		{
			itoa(*(p + 2), str_value, 10);
			device_Mario_log("APPN_Power: value = %s, purifier_status.pwr = %s", str_value, purifier_status.pwr_str);
			if ( strcmp(str_value, purifier_status.pwr_str) != 0 )
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				strcpy(purifier_status.pwr_str, str_value);
			}
			break;
		}
		case APPN_Mario_ChildLock:
		{
			b_value = (*(p + 2) == 1) ? true : false;
			device_Mario_log("APPN_ChildLock: value = %d, purifier_status.cl = %d", b_value, purifier_status.cl);
			if ( b_value != purifier_status.cl )
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				purifier_status.cl = b_value;
			}
			break;
		}
		case APPN_Mario_AQILight:
		{
			i_value = *(p + 2);
			device_Mario_log("APPN_AQILight: value = %d, purifier_status.aqil = %d", i_value, purifier_status.aqil);
			if ( i_value != purifier_status.aqil )
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				purifier_status.aqil = (char)i_value;
			}
			break;
		}
		case APPN_Mario_UILight:
		{
			switch ( *(p + 2) )
			{
			case 0x00: str_value[0] = '0'; str_value[1] = '\0'; break;
			case 0x01: str_value[0] = 'D'; str_value[1] = '\0'; break;
			case 0x02: str_value[0] = '1'; str_value[1] = '\0'; break;
			default: str_value[0] = '\0'; break;
			}
			if ( str_value[0] == '\0' )
				break;
			device_Mario_log("APPN_UILight: value = %s, purifier_status.uil = %s", str_value, purifier_status.uil_str);
			if ( strcmp(str_value, purifier_status.uil_str) != 0 )
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				strcpy(purifier_status.uil_str, str_value);
			}
			break;
		}
		case APPN_Mario_DeviceTimer:
		{
			i_value = *(p + 2);
			device_Mario_log("APPN_DeviceTimer: value = %d, purifier_status.dt = %d", i_value, purifier_status.dt);
			if ( i_value != purifier_status.dt )
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				purifier_status.dt = i_value;
			}
			break;
		}
		case APPN_Mario_DeviceTimerRemaining:
		{
			i_value = (*(p + 2) << 8) + *(p + 3);
			device_Mario_log("APPN_DeviceTimerRemaining: value = %d, purifier_status.dtrs = %d", i_value, purifier_status.dtrs);
			if ( i_value != purifier_status.dtrs )
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				purifier_status.dtrs = i_value;
			}
			break;
		}
		case APPN_Mario_OperationMode:
		{
			switch ( *(p + 2) )
			{
			case 0x00: str_value[0] = 'M'; str_value[1] = '\0'; break;
			case 0x01: str_value[0] = 'P'; str_value[1] = '\0'; break;
			case 0x02: str_value[0] = 'A'; str_value[1] = '\0'; break;
			case 0x05: str_value[0] = 'S'; str_value[1] = '\0'; break;
			default: str_value[0] = '\0'; break;
			}
			if ( str_value[0] == '\0' )
				break;
			device_Mario_log("APPN_Mode: value = %s, purifier_status.mode = %s", str_value, purifier_status.mode_str);
			if ( strcmp(str_value, purifier_status.mode_str) != 0 )
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				strcpy(purifier_status.mode_str, str_value);
			}
			break;
		}
		case APPN_Mario_Function:
		{
			str_value[0] = 'P';
			str_value[1] = '\0';
			str_value[2] = '\0';
			switch ( *(p + 2) )
			{
			case 0x00: str_value[0] = 'P'; str_value[1] = '\0'; break;
			case 0x01: str_value[0] = 'P'; str_value[1] = 'H'; break;
			}
			device_Mario_log("APPN_Mario_Function: value = %s, purifier_status.func = %s", str_value, purifier_status.func);
			if ( strncmp(str_value, purifier_status.func, 3) != 0 )
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				purifier_status.func[0] = str_value[0];
				purifier_status.func[1] = str_value[1];
				purifier_status.func[2] = str_value[2];
			}
			break;
		}
		case APPN_Mario_RHSetting:
		{
			i_value = *(p + 2);
			device_Mario_log("APPN_Mario_RHSetting: value = %d, purifier_status.rhset = %d", i_value, purifier_status.rhset);
			if ( i_value != purifier_status.rhset )
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				purifier_status.rhset = i_value;
			}
			break;
		}
		case APPN_Mario_RH:
		{
			i_value = *(p + 2);
			device_Mario_log("APPN_Mario_RH: value = %d, purifier_status.rh = %d", i_value, purifier_status.rh);
			if ( i_value != purifier_status.rh )
			{
				// if ( purifier_status.rh == 0 )
				// 	g_device_running_status.need_update_shadow = true;
				// else
				// 	g_device_running_status.need_update_data = true;
				get_philips_running_status()->device_upload_manage.sensor_change = true;
				purifier_status.rh = i_value;
			}
			break;
		}
		case APPN_Mario_Temperature:
		{
			i_value = (int8_t)(*(p + 2));
			device_Mario_log("APPN_Mario_Temperature: value = %d, purifier_status.temp = %d", i_value, purifier_status.temp);
			if ( i_value != purifier_status.temp )
			{
				// if ( purifier_status.temp == 0 )
				// 	g_device_running_status.need_update_shadow = true;
				// else
				// 	g_device_running_status.need_update_data = true;
				get_philips_running_status()->device_upload_manage.sensor_change = true;
				purifier_status.temp = i_value;
			}
			break;
		}
		case APPN_Mario_PM2_5:
		{
			purifier_status.air_prop_map |= PHILIPS_AIR_PROP_MAP_PM25;
			i_value = (*(p + 2) << 8) + *(p + 3);
			if ( i_value != purifier_status.pm25 )
			{
				// if ( purifier_status.pm25 == 0 )
				// 	g_device_running_status.need_update_shadow = true;
				// else
				// 	g_device_running_status.need_update_data = true;
				get_philips_running_status()->device_upload_manage.sensor_change = true;
				purifier_status.pm25 = i_value;
			}
			break;
		}
		case APPN_Mario_iAQL:
		{
			purifier_status.air_prop_map |= PHILIPS_AIR_PROP_MAP_IAQL;
			i_value = (*(p + 2) << 8) + *(p + 3);
			if ( i_value != purifier_status.iaql )
			{
				// if ( purifier_status.iaql == 0 )
				// 	g_device_running_status.need_update_shadow = true;
				// else
				// 	g_device_running_status.need_update_data = true;
				get_philips_running_status()->device_upload_manage.sensor_change = true;
				purifier_status.iaql = i_value;
			}
			break;
		}
		case APPN_Mario_AQIt:
		{
			purifier_status.air_prop_map |= PHILIPS_AIR_PROP_MAP_AQIT;
			break;
		}
		case APPN_Mario_DeviceDisplayParameter:
		{
			purifier_status.air_prop_map |= PHILIPS_AIR_PROP_MAP_DDP;
			itoa(*(p + 2), str_value, 10);
			device_Mario_log("APPN_DeviceDisplayParameter: value = %s, purifier_status.ddp = %s", str_value, purifier_status.ddp_str);
			if ( strcmp(str_value, purifier_status.ddp_str) != 0 )
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				strcpy(purifier_status.ddp_str, str_value);
			}
			break;
		}
		case APPN_Mario_RealtimeDeviceDisplayParameter:
		{
			purifier_status.air_prop_map |= PHILIPS_AIR_PROP_MAP_RDDP;
			itoa(*(p + 2), str_value, 10);
			device_Mario_log("APPN_RealtimeDeviceDisplayParameter: value = %s, purifier_status.rddp = %s", str_value, purifier_status.rddp_str);
			if ( strcmp(str_value, purifier_status.rddp_str) != 0 )
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				strcpy(purifier_status.rddp_str, str_value);
			}
			break;
		}
		case APPN_Mario_ErrorCode:
		{
			uint32_t deviceerrorcode = (*(p + 2) << 8) + *(p + 3);
			get_philips_running_status()->device_upload_manage.warn = 0;
			get_philips_running_status()->device_upload_manage.error = 0;

			if ( (deviceerrorcode >> 14) & 0x01 )	// warning
			{
				for ( int i = 0; i <= 8; ++i )
				{
					if ( (deviceerrorcode >> i) & 0x01 )
					{
						switch ( i )
						{
						case 0:
							get_philips_running_status()->device_upload_manage.warn |= PHILIPS_WARNING_F0; 	// Pre-filter cleaning
							get_philips_running_status()->device_upload_manage.warn |= PHILIPS_WARNING_F5; break; // wick cleaning
						case 2: get_philips_running_status()->device_upload_manage.warn |= PHILIPS_WARNING_F3; break;	// Filter 1 warning
						case 3: get_philips_running_status()->device_upload_manage.warn |= PHILIPS_WARNING_F1; break;	// Filter 1 lock
						case 4: get_philips_running_status()->device_upload_manage.warn |= PHILIPS_WARNING_F4; break;	// Filter 2 warning
						case 5: get_philips_running_status()->device_upload_manage.warn |= PHILIPS_WARNING_F2; break;	// Filter 2 lock
						case 6: get_philips_running_status()->device_upload_manage.warn |= PHILIPS_WARNING_F6; break;	// Wick waring
						case 7: get_philips_running_status()->device_upload_manage.warn |= PHILIPS_WARNING_F7; break;	// Wick lock
						case 8: get_philips_running_status()->device_upload_manage.warn |= PHILIPS_WARNING_F8; break;	// Water tank empty
						default: break;
						}
					}
				}
			}
			else if ( (deviceerrorcode >> 15) & 0x01 )	// error
			{
				switch ( deviceerrorcode )
				{
				case 0x8000: get_philips_running_status()->device_upload_manage.error |= PHILIPS_ERROR_E0; break;	// Door open
				case 0x8001: get_philips_running_status()->device_upload_manage.error |= PHILIPS_ERROR_E1; break;	// Motor error
				case 0x8002: get_philips_running_status()->device_upload_manage.error |= PHILIPS_ERROR_E2; break;	// Particle sensor error
				case 0x8005: get_philips_running_status()->device_upload_manage.error |= PHILIPS_ERROR_E5; break;	// T&RH sensor error
				default: break;
				}
			}

			device_Mario_log("purifier_status.err = %d, deviceerrorcode = %lu, warn = 0x%08X, error = 0x%08X", purifier_status.err, deviceerrorcode, get_philips_running_status()->device_upload_manage.warn, get_philips_running_status()->device_upload_manage.error);
			device_Mario_log("...");
			if ( deviceerrorcode != purifier_status.err )
			{
				purifier_status.err = deviceerrorcode;
				get_philips_running_status()->device_upload_manage.status_change = true;
			}
			break;
		}
		case APPN_Mario_WaterLevel:
		{
			i_value = *(p + 2);
			device_Mario_log("APPN_Mario_WaterLevel: value = %d, purifier_status.wl = %d", i_value, purifier_status.wl);
			if ( i_value != purifier_status.wl )
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				purifier_status.wl = i_value;
			}
			break;
		}
		default: break;
		}

        p = p + 2 + *(p + 1);
    }

    return ;
}

void philips_process_cloud_data_Mario(char *data, int datalen)
{
	uint8_t value_8 = 0;
	uint8_t props[256] = {0};
	int index = 0;
	uint8_t sleep_spec_flag = 0;

	cJSON *root = NULL;
	cJSON *branch = NULL;
	cJSON *item = NULL;

	root = cJSON_Parse(data);
	require(root, exit);

	branch = cJSON_GetObjectItem(root, "state");
	require_string(branch, exit, "no state object key");
	branch = cJSON_GetObjectItem(branch, "desired");
	require_string(branch, exit, "no desired object key");

	item = cJSON_GetObjectItem(branch, "name");
	if(item)
	{
		if(strlen(item->valuestring))
		{
			philips_send_PutProps_device_name(item->valuestring, strlen(item->valuestring));
			get_philips_running_status()->need_get_deviceinfo = true;
		}
		else
		{
			goto exit;
		}
	}

	item = cJSON_GetObjectItem(branch, "language");
	if(item)
	{
		if(strlen(item->valuestring))
		{
			philips_send_PutProps_device_language(item->valuestring, strlen(item->valuestring));
			get_philips_running_status()->need_get_deviceinfo = true;
		}
		else
		{
			goto exit;
		}
	}

	item = cJSON_GetObjectItem(branch, "url");
	if(item)
	{
		philips_process_cloud_ota_request_info(branch);
	}

	item = cJSON_GetObjectItem(branch, "om");
	if(item)
	{
		if(strcmp(item->valuestring, "0") == 0)
		{
			value_8 = 0x00;
		}
		else if(strcmp(item->valuestring, "1") == 0)
		{
			value_8 = 0x01;
		}
		else if(strcmp(item->valuestring, "2") == 0)
		{
			value_8 = 0x02;
		}
		else if(strcmp(item->valuestring, "3") == 0)
		{
			value_8 = 0x03;
		}
		else if(strcmp(item->valuestring, "s") == 0)
		{
			value_8 = 0x04;
		}
		else if(strcmp(item->valuestring, "t") == 0)
		{
			value_8 = 0x05;
		}
		else
		{
			goto exit;
		}

		props[index++] = 0x01;
		props[index++] = 0x01;
		props[index++] = value_8;
	}

	item = cJSON_GetObjectItem(branch, "pwr");
	if(item)
	{
		if(strcmp(item->valuestring, "0") == 0)
		{
			value_8 = 0x00;
		}
		else if(strcmp(item->valuestring, "1") == 0)
		{
			value_8 = 0x01;
		}
		else
		{
			goto exit;
		}

		props[index++] = 0x02;
		props[index++] = 0x01;
		props[index++] = value_8;
	}

	item = cJSON_GetObjectItem(branch, "cl");
	if(item)
	{
		if(item->valueint == FALSE)
		{
			value_8 = 0x00;
		}
		else if(item->valueint == TRUE)
		{
			value_8 = 0x01;
		}
		else
		{
			goto exit;
		}

		props[index++] = 0x03;
		props[index++] = 0x01;
		props[index++] = value_8;
	}

	item = cJSON_GetObjectItem(branch, "aqil");
	if(item)
	{
		value_8 = item->valueint;

		props[index++] = 0x04;
		props[index++] = 0x01;
		props[index++] = value_8;
	}

	item = cJSON_GetObjectItem(branch, "uil");
	if(item)
	{
		if(strcmp(item->valuestring, "0") == 0)
		{
			value_8 = 0x00;
		}
		else if(strcmp(item->valuestring, "D") == 0)
		{
			value_8 = 0x01;
		}
		else if(strcmp(item->valuestring, "1") == 0)
		{
			value_8 = 0x02;
		}
		else
		{
			goto exit;
		}

		props[index++] = 0x05;
		props[index++] = 0x01;
		props[index++] = value_8;
	}

	item = cJSON_GetObjectItem(branch, "dt");
	if(item)
	{
		value_8 = item->valueint;

		props[index++] = 0x06;
		props[index++] = 0x01;
		props[index++] = value_8;
	}

	item = cJSON_GetObjectItem(branch, "mode");
	if(item)
	{
		if(strcmp(item->valuestring, "M") == 0)
		{
			value_8 = 0x00;
		}
		else if(strcmp(item->valuestring, "P") == 0)
		{
			value_8 = 0x01;
		}
		else if(strcmp(item->valuestring, "A") == 0)
		{
			value_8 = 0x02;
		}
		else if(strcmp(item->valuestring, "B") == 0)
		{
			value_8 = 0x03;
		}
		else if(strcmp(item->valuestring, "S") == 0)
		{
			sleep_spec_flag = 1;

			props[index++] = 0x01;
			props[index++] = 0x01;
			props[index++] = 0x04;
		}
		else
		{
			goto exit;
		}

		if(!sleep_spec_flag)
		{
			props[index++] = 0x08;
			props[index++] = 0x01;
			props[index++] = value_8;
		}
	}

	item = cJSON_GetObjectItem(branch, "func");
	if(item)
	{
		if(strcmp(item->valuestring, "P") == 0)
		{
			value_8 = 0x00;
		}
		else if(strcmp(item->valuestring, "PH") == 0)
		{
			value_8 = 0x01;
		}
		else
		{
			goto exit;
		}

		props[index++] = 0x09;
		props[index++] = 0x01;
		props[index++] = value_8;
	}

	item = cJSON_GetObjectItem(branch, "rhset");
	if(item)
	{
		value_8 = item->valueint;

		props[index++] = 0x0A;
		props[index++] = 0x01;
		props[index++] = value_8;
	}

	item = cJSON_GetObjectItem(branch, "aqit");
	if(item)
	{
		if(get_philips_running_status()->flash_param.aqit != item->valueint)
		{
			get_philips_running_status()->flash_param.aqit = (uint16_t)item->valueint;
			mkv_item_set("pda_aqit", &get_philips_running_status()->flash_param.aqit, sizeof(get_philips_running_status()->flash_param.aqit));
			get_philips_running_status()->device_upload_manage.status_change = true;
		}
	}

	item = cJSON_GetObjectItem(branch, "ddp");
	if(item)
	{
		if(strcmp(item->valuestring, "0") == 0)
		{
			value_8 = 0x00;
		}
		else if(strcmp(item->valuestring, "1") == 0)
		{
			value_8 = 0x01;
		}
		else if(strcmp(item->valuestring, "3") == 0)
		{
			value_8 = 0x03;
		}
		else
		{
			goto exit;
		}

		props[index++] = 0x10;
		props[index++] = 0x01;
		props[index++] = value_8;
	}

	if(index)
	{
		philips_send_PutProps_Common(0x03, props, index);
	}

exit:
	if(root)
		cJSON_Delete(root);

	return ;
}