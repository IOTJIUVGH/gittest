#include "mxos.h"
#include "cJSON.h"
#include "mkv.h"

#include "philips_device.h"
#include "philips_device_model.h"
#include "philips_param.h"

#define device_Common_log(M, ...)				custom_log("", M, ##__VA_ARGS__)
//#define device_Common_log(M, ...)

// Mars
enum _Air_properties_property_no_Mars
{
	APPN_Common_portpollinterval				    = 0x01,
	APPN_Commo_Power								= 0x02,
	APPN_Commo_ChildLock							= 0x03,
	APPN_Commo_AQILight								= 0x04,
	APPN_Commo_UILight								= 0x05,
	APPN_Commo_UserAutoModeSetting					= 0x0b,
	APPN_Commo_OperationMode						= 0x0c,
	APPN_Commo_FanSpeed								= 0x0d,
	APPN_Commo_iAQL									= 0x20,
	APPN_Commo_PM25								    = 0x21,
	APPN_Commo_GAS									= 0x22,
	APPN_Commo_Formaldehyde							= 0x23,
	APPN_Commo_Temperature							= 0x24,
	APPN_Commo_RH									= 0x25,
	APPN_Commo_Ddp									= 0x2a,
	APPN_Commo_Rddp									= 0x2b,
	APPN_Commo_Aqit									= 0x2c,
	APPN_Commo_ErrorCode							= 0x40,
};

void philips_encode_json_Common(char **buf, const char *StatusType, const char *ConnectType, bool head)
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

	cJSON_AddStringToObject(item, "D01-03", get_philips_running_status()->device_name);
	cJSON_AddStringToObject(item, "D01-04", get_philips_running_status()->device_type);
	cJSON_AddStringToObject(item, "D01-05", get_philips_running_status()->flash_param.device_modelid);
	cJSON_AddStringToObject(item, "D01-07", get_philips_running_status()->device_language);
	cJSON_AddStringToObject(item, "D01-08", get_philips_running_status()->wifi_protocol_ver);
	cJSON_AddStringToObject(item, "D01-09", get_philips_running_status()->OTAU_protocol_ver);
	//cJSON_AddStringToObject(item, "swversion", get_philips_running_status()->device_swversion);
	cJSON_AddStringToObject(item, "D01-20", get_philips_running_status()->node1bootloaderver);
	cJSON_AddStringToObject(item, "D01-21", get_philips_running_status()->node1applicationver);

	cJSON_AddBoolToObject(item, "MCUBoot", get_philips_running_status()->flash_param.muc_boot);
	cJSON_AddStringToObject(item, "DeviceVersion", get_philips_running_status()->otau_run_state.local_device_version[0] == '\0' ? "0.0.0" : get_philips_running_status()->otau_run_state.local_device_version);
	cJSON_AddStringToObject(item, "range", get_philips_running_status()->flash_param.product_range);
	cJSON_AddNumberToObject(item, "Runtime", (double)mos_time());
	cJSON_AddNumberToObject(item, "rssi", get_philips_running_status()->rssi);
	cJSON_AddNumberToObject(item, "free_memory", mos_mallinfo()->free);
	cJSON_AddStringToObject(item, "WifiVersion", FIRMWARE_REVISION);
	cJSON_AddStringToObject(item, "ProductId", get_philips_running_status()->flash_param.product_id);
	cJSON_AddStringToObject(item, "DeviceId", get_philips_running_status()->flash_param.device_id);
	cJSON_AddStringToObject(item, "StatusType", StatusType);
	cJSON_AddStringToObject(item, "ConnectType", ConnectType);
	
	if(purifier_status.air_prop_map & PHILIPS_AIR_PROP_MAP_PWR)
		cJSON_AddStringToObject(item, "D03-02", purifier_status.pwr_str);

	if(purifier_status.air_prop_map & PHILIPS_AIR_PROP_MAP_CL)
		cJSON_AddBoolToObject(item, "D03-03", purifier_status.cl ? true : false);

	if(purifier_status.air_prop_map & PHILIPS_AIR_PROP_MAP_AQIL)
		cJSON_AddNumberToObject(item, "D03-04", purifier_status.aqil);

	if(purifier_status.air_prop_map & PHILIPS_AIR_PROP_MAP_UIL)
		cJSON_AddStringToObject(item, "D03-05", purifier_status.uil_str);

	if(purifier_status.air_prop_map & PHILIPS_AIR_PROP_MAP_UASET)
		cJSON_AddStringToObject(item, "D03-11", purifier_status.uaset_str);

	if(purifier_status.air_prop_map & PHILIPS_AIR_PROP_MAP_MODE)
		cJSON_AddStringToObject(item, "D03-12", purifier_status.mode_str);
	
	if(purifier_status.air_prop_map & PHILIPS_AIR_PROP_MAP_OM)
		cJSON_AddStringToObject(item, "D03-13", purifier_status.om_str);

	if(purifier_status.air_prop_map & PHILIPS_AIR_PROP_MAP_IAQL)
		cJSON_AddNumberToObject(item, "D03-32", purifier_status.iaql);

	if(purifier_status.air_prop_map & PHILIPS_AIR_PROP_MAP_PM25)
		cJSON_AddNumberToObject(item, "D03-33", purifier_status.pm25);
	
	if(purifier_status.air_prop_map & PHILIPS_AIR_PROP_MAP_GAS)
		cJSON_AddNumberToObject(item, "D03-34", purifier_status.gas);

	if(purifier_status.air_prop_map & PHILIPS_AIR_PROP_MAP_FORMALDEHYDE)
		cJSON_AddNumberToObject(item, "D03-35", purifier_status.formaldehyde);
	
	if(purifier_status.air_prop_map & PHILIPS_AIR_PROP_MAP_TEMP)
		cJSON_AddNumberToObject(item, "D03-36", purifier_status.temp);
	
	if(purifier_status.air_prop_map & PHILIPS_AIR_PROP_MAP_RH)
		cJSON_AddNumberToObject(item, "D03-37", purifier_status.rh);

	if(purifier_status.air_prop_map & PHILIPS_AIR_PROP_MAP_DDP)
		cJSON_AddStringToObject(item, "D03-42", purifier_status.ddp_str);

	if(purifier_status.air_prop_map & PHILIPS_AIR_PROP_MAP_RDDP)
		cJSON_AddStringToObject(item, "D03-43", purifier_status.rddp_str);

	cJSON_AddNumberToObject(item, "D03-44", get_philips_running_status()->flash_param.aqit);

	if(purifier_status.air_prop_map & PHILIPS_AIR_PROP_MAP_ERR)
		cJSON_AddNumberToObject(item, "D03-64", purifier_status.err);

	if(purifier_status.filter_prop_map & PHILIPS_FILTER_PROP_MAP_FLTT1)
		cJSON_AddStringToObject(item, "D05-02", purifier_status.fltt1);

	if(purifier_status.filter_prop_map & PHILIPS_FILTER_PROP_MAP_FLTT2)
		cJSON_AddStringToObject(item, "D05-03", purifier_status.fltt2);

	if(purifier_status.filter_prop_map & PHILIPS_FILTER_PROP_MAP_FLTSTS0)
		cJSON_AddNumberToObject(item, "D05-07", purifier_status.fltsts0);
	
	if(purifier_status.filter_prop_map & PHILIPS_FILTER_PROP_MAP_FLTSTS1)
		cJSON_AddNumberToObject(item, "D05-08", purifier_status.fltsts1);

	if(purifier_status.filter_prop_map & PHILIPS_FILTER_PROP_MAP_FLTSTS2)
		cJSON_AddNumberToObject(item, "D05-09", purifier_status.fltsts2);

	if(purifier_status.filter_prop_map & PHILIPS_FILTER_PROP_MAP_FLTTOTAL0)
		cJSON_AddNumberToObject(item, "D05-13", purifier_status.flttotal0);

	if(purifier_status.filter_prop_map & PHILIPS_FILTER_PROP_MAP_FLTTOTAL1)
		cJSON_AddNumberToObject(item, "D05-14", purifier_status.flttotal1);

	if(purifier_status.filter_prop_map & PHILIPS_FILTER_PROP_MAP_FLTTOTAL2)
		cJSON_AddNumberToObject(item, "D05-15", purifier_status.flttotal2);

	*buf = cJSON_PrintUnformatted(root);

exit:
	if(root)
	{
		cJSON_Delete(root);
		root = NULL;
	}
	return ;
}

void philips_process_air_properties_Common(const uint8_t *data)
{
    const uint8_t *p = data+7;
	int i_value = 0;
	char str_value[20] = {0};
	bool b_value = false;

    while (*(p) != 0x00)
    {
		switch ( *(p) )
		{
			case APPN_Common_portpollinterval:break;
			case APPN_Commo_Power:
			{
				purifier_status.air_prop_map |= PHILIPS_AIR_PROP_MAP_PWR;
				
				(*(p + 2) == 1) ? strcpy(str_value, "ON") : strcpy(str_value, "OFF");

				device_Common_log("APPN_Power: value = %s, purifier_status.pwr = %s", str_value, purifier_status.pwr_str);
				if ( strcmp(str_value, purifier_status.pwr_str) != 0 )
				{
					get_philips_running_status()->device_upload_manage.status_change = true;
					strcpy(purifier_status.pwr_str, str_value);
				}			
				break;
			}
			case APPN_Commo_ChildLock:
			{
				purifier_status.air_prop_map |= PHILIPS_AIR_PROP_MAP_CL;
				b_value = (*(p + 2) == 1) ? true : false;
				device_Common_log("APPN_ChildLock: value = %d, purifier_status.cl = %d", b_value, purifier_status.cl);
				if ( b_value != purifier_status.cl )
				{
					get_philips_running_status()->device_upload_manage.status_change = true;
					purifier_status.cl = b_value;
				}
				break;
			}
			case APPN_Commo_AQILight:
			{
				purifier_status.air_prop_map |= PHILIPS_AIR_PROP_MAP_AQIL;
				i_value = *(p + 2);
				device_Common_log("APPN_AQILight: value = %d, purifier_status.aqil = %d", i_value, purifier_status.aqil);
				if ( i_value != purifier_status.aqil )
				{
					get_philips_running_status()->device_upload_manage.status_change = true;
					purifier_status.aqil = i_value;
				}
				break;
			}
			case APPN_Commo_UILight:
			{
				purifier_status.air_prop_map |= PHILIPS_AIR_PROP_MAP_UIL;
				i_value = *(p + 2);
		
				device_Common_log("APPN_UILight: value = %d, purifier_status.uil = %d", i_value, purifier_status.uilight);
				if ( i_value!=purifier_status.uilight)
				{
					get_philips_running_status()->device_upload_manage.status_change = true;
					purifier_status.uilight = i_value;
				}
				break;
			}
			case APPN_Commo_UserAutoModeSetting:
			{
				purifier_status.air_prop_map |= PHILIPS_AIR_PROP_MAP_UASET;
				switch ( *(p + 2) )
				{
				case 0x00: str_value[0] = 'A'; str_value[1] = '\0'; break;
				case 0x01: str_value[0] = 'P'; str_value[1] = '\0'; break;
				case 0x02: str_value[0] = 'G'; str_value[1] = '\0'; break;
				case 0x03: str_value[0] = 'F'; str_value[1] = '\0'; break;
				default: str_value[0] = '\0'; break;
				}
				if ( str_value[0] == '\0' )
					break;
				device_Common_log("APPN_Mode: value = %s, purifier_status.uaset = %s", str_value, purifier_status.uaset_str);
				if ( strcmp(str_value, purifier_status.uaset_str) != 0 )
				{
					get_philips_running_status()->device_upload_manage.status_change = true;
					strcpy(purifier_status.uaset_str, str_value);
				}
				break;
			}
			case APPN_Commo_OperationMode:
			{
				purifier_status.air_prop_map |= PHILIPS_AIR_PROP_MAP_MODE;
				switch ( *(p + 2) )
				{
				case 0x00: strcpy(str_value, "Auto General"); break;
				case 0x01: strcpy(str_value, "Sleep"); break;
				case 0x02: strcpy(str_value, "Turbo");break;
				case 0x03: strcpy(str_value, "Allergy Sleep");break;
				case 0x04: strcpy(str_value, "Gentle/Speed 1");break;
				case 0x05: strcpy(str_value, "Speed 2");break;
				default: str_value[0] = '\0'; break;
				}
				if ( str_value[0] == '\0' )
					break;
				device_Common_log("APPN_Mode: value = %s, purifier_status.mode = %s", str_value, purifier_status.mode_str);
				if ( strcmp(purifier_status.mode_str, str_value) != 0 )
				{
					get_philips_running_status()->device_upload_manage.status_change = true;
					strcpy(purifier_status.mode_str, str_value);
				}
				break;
			}
		case APPN_Commo_FanSpeed:
		{
			purifier_status.air_prop_map |= PHILIPS_AIR_PROP_MAP_OM;
			switch ( *(p + 2) )
			{
			case 0x00: str_value[0] = '0'; str_value[1] = '\0'; break;
			case 0x01: str_value[0] = 's'; str_value[1] = '\0'; break;
			case 0x02: str_value[0] = 't'; str_value[1] = '\0'; break;
			case 0x03: str_value[0] = '1'; str_value[1] = '\0'; break;
			case 0x04: str_value[0] = '2'; str_value[1] = '\0'; break;
			case 0x05: str_value[0] = '3'; str_value[1] = '\0'; break;
			default: str_value[0] = '\0'; break;
			}
			if ( str_value[0] == '\0' )
				break;
			device_Common_log("APPN_FanSpeed: value = %s, purifier_status.om = %s", str_value, purifier_status.om_str);
			if ( strcmp(str_value, purifier_status.om_str) != 0 )
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				strcpy(purifier_status.om_str, str_value);
			}
			break;
		}
		case APPN_Commo_iAQL:
		{
			purifier_status.air_prop_map |= PHILIPS_AIR_PROP_MAP_IAQL;
			i_value = *(p + 2);
			device_Common_log("APPN_FanSpeed: value = %s, purifier_status.om = %s", i_value, purifier_status.iaql);
			if ( i_value != purifier_status.iaql )
			{
				get_philips_running_status()->device_upload_manage.sensor_change = true;
				purifier_status.iaql = i_value;
			}
			break;
		}
		case APPN_Commo_PM25:
		{
			purifier_status.air_prop_map |= PHILIPS_AIR_PROP_MAP_PM25;
			i_value = (*(p + 2) << 8) + *(p + 3);
			if ( i_value != purifier_status.pm25 )
			{
				get_philips_running_status()->device_upload_manage.sensor_change = true;
				purifier_status.pm25 = i_value;
			}
			break;
		}
		case APPN_Commo_GAS:
		{
			purifier_status.air_prop_map |= PHILIPS_AIR_PROP_MAP_GAS;
			i_value = (*(p + 2) << 8) + *(p + 3);
			if ( i_value != purifier_status.gas )
			{
				get_philips_running_status()->device_upload_manage.sensor_change = true;
				purifier_status.gas = i_value;
			}
			break;
		}
		case APPN_Commo_Formaldehyde:
		{
			purifier_status.air_prop_map |= PHILIPS_AIR_PROP_MAP_FORMALDEHYDE;
			i_value = *(p + 2);
			if ( i_value != purifier_status.formaldehyde )
			{
				get_philips_running_status()->device_upload_manage.sensor_change = true;
				purifier_status.formaldehyde = i_value;
			}
			
			break;
		}
		case APPN_Commo_Temperature:
		{
			purifier_status.air_prop_map |= PHILIPS_AIR_PROP_MAP_TEMP;
			i_value = *(p + 2);
			if ( i_value != purifier_status.temp )
			{
				get_philips_running_status()->device_upload_manage.sensor_change = true;
				purifier_status.temp = i_value;
			}		
			break;
		}
		case APPN_Commo_RH:
		{
			i_value = *(p + 2);
			device_Common_log("APPN_MicroMario_RH: value = %d, purifier_status.rh = %d", i_value, purifier_status.rh);
			if ( i_value != purifier_status.rh )
			{
				get_philips_running_status()->device_upload_manage.sensor_change = true;
				purifier_status.rh = i_value;
			}
		}
		case APPN_Commo_Ddp:
		{
			purifier_status.air_prop_map |= PHILIPS_AIR_PROP_MAP_DDP;
			itoa(*(p + 2), str_value, 10);
			device_Common_log("APPN_DeviceDisplayParameter: value = %s, purifier_status.ddp = %s", str_value, purifier_status.ddp_str);
			if ( strcmp(str_value, purifier_status.ddp_str) != 0 )
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				strcpy(purifier_status.ddp_str, str_value);
			}
			break;
		}
		case APPN_Commo_Rddp:
		{
			purifier_status.air_prop_map |= PHILIPS_AIR_PROP_MAP_RDDP;
			itoa(*(p + 2), str_value, 10);
			device_Common_log("APPN_RealtimeDeviceDisplayParameter: value = %s, purifier_status.rddp = %s", str_value, purifier_status.rddp_str);
			if ( strcmp(str_value, purifier_status.rddp_str) != 0 )
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				strcpy(purifier_status.rddp_str, str_value);
			}
			break;
			
			break;
		}
		case APPN_Commo_Aqit:
		{
			purifier_status.air_prop_map |= PHILIPS_AIR_PROP_MAP_AQIT;
			i_value = *(p + 2);
			if ( i_value != purifier_status.aqit )
			{
				get_philips_running_status()->device_upload_manage.sensor_change = true;
				purifier_status.aqit = i_value;
			}
			break;
		}
		case APPN_Commo_ErrorCode:
		{
			purifier_status.air_prop_map |= PHILIPS_AIR_PROP_MAP_ERR;

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
						case 0: get_philips_running_status()->device_upload_manage.warn |= PHILIPS_WARNING_F9; break;	// Mars Pre-filter cleaning
						case 2: get_philips_running_status()->device_upload_manage.warn |= PHILIPS_WARNING_FB; break;	// Mars Filter 1 warning
						case 3: get_philips_running_status()->device_upload_manage.warn |= PHILIPS_WARNING_FA; break;	// Mars Filter 1 lock
//						case 4: get_philips_running_status()->device_upload_manage.warn |= (1 << 13); break;	// Mars Filter 2 warning
//						case 5: get_philips_running_status()->device_upload_manage.warn |= (1 << 11); break;	// Mars Filter 2 lock
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
				case 0x8004: get_philips_running_status()->device_upload_manage.error |= PHILIPS_ERROR_E4; break;	// Gas sensor error
				case 0x800D: get_philips_running_status()->device_upload_manage.error |= PHILIPS_ERROR_EA; break;	// RFID error
				case 0x800E: get_philips_running_status()->device_upload_manage.error |= PHILIPS_ERROR_EB; break;	// Filter ID error
				default: break;
				}
			}

			device_Common_log("purifier_status.err = %d, deviceerrorcode = %lu, warn = 0x%08X, error = 0x%08X", purifier_status.err, deviceerrorcode, get_philips_running_status()->device_upload_manage.warn, get_philips_running_status()->device_upload_manage.error);
			device_Common_log("...");
			if ( deviceerrorcode != purifier_status.err )
			{
				purifier_status.err = deviceerrorcode;
				get_philips_running_status()->device_upload_manage.status_change = true;
			}
			break;
		}
		default: break;
		}

        p = p + 2 + *(p + 1);
    }

    return ;
}

void philips_process_cloud_data_Common(char *data, int datalen)
{
	uint8_t value_8 = 0;
	uint8_t props[256] = {0};
	int index = 0;

	cJSON *root = NULL;
	cJSON *branch = NULL;
	cJSON *item = NULL;

	root = cJSON_Parse(data);
	require(root, exit);

	branch = cJSON_GetObjectItem(root, "state");
	require_string(branch, exit, "no state object key");
	branch = cJSON_GetObjectItem(branch, "desired");
	require_string(branch, exit, "no desired object key");

	// item = cJSON_GetObjectItem(branch, "name");
	// if(item)
	// {
	// 	if(strlen(item->valuestring))
	// 	{
	// 		philips_send_PutProps_device_name(item->valuestring, strlen(item->valuestring));
	// 		get_philips_running_status()->need_get_deviceinfo = true;
	// 	}
	// 	else
	// 	{
	// 		goto exit;
	// 	}
	// }

	// item = cJSON_GetObjectItem(branch, "language");
	// if(item)
	// {
	// 	if(strlen(item->valuestring))
	// 	{
	// 		philips_send_PutProps_device_language(item->valuestring, strlen(item->valuestring));
	// 		get_philips_running_status()->need_get_deviceinfo = true;
	// 	}
	// 	else
	// 	{
	// 		goto exit;
	// 	}
	// }

	item = cJSON_GetObjectItem(branch, "url");
	if(item)
	{
		philips_process_cloud_ota_request_info(branch);
	}

	item = cJSON_GetObjectItem(branch, "D03-12");//operation mode
	if(item)
	{
		if(strcmp(item->valuestring, "Auto General") == 0)
		{
			value_8 = 0x00;
		}
		else if(strcmp(item->valuestring, "Sleep") == 0)
		{
			value_8 = 0x01;
		}
		else if(strcmp(item->valuestring, "Turbo") == 0)
		{
			value_8 = 0x02;
		}
		else if(strcmp(item->valuestring, "Allergy Sleep") == 0)
		{
			value_8 = 0x03;
		}
		else if(strcmp(item->valuestring, "Gentle/Speed 1") == 0)
		{
			value_8 = 0x04;
		}
		else if(strcmp(item->valuestring, "Speed 2") == 0)
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

	item = cJSON_GetObjectItem(branch, "D03-02");//power
	if(item)
	{
		if(strcmp(item->valuestring, "OFF") == 0)
		{
			value_8 = 0x00;
		}
		else if(strcmp(item->valuestring, "ON") == 0)
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

	item = cJSON_GetObjectItem(branch, "D03-03");//childlock
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

	item = cJSON_GetObjectItem(branch, "D03-04");//aqil
	if(item)
	{
		value_8 = item->valueint;

		props[index++] = 0x04;
		props[index++] = 0x01;
		props[index++] = value_8;
	}

	item = cJSON_GetObjectItem(branch, "D03-05");//uil
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

	item = cJSON_GetObjectItem(branch, "D03-11");//User Auto Mode Setting
	if(item)
	{
		if(strcmp(item->valuestring, "A") == 0)
		{
			value_8 = 0x00;
		}
		else if(strcmp(item->valuestring, "P") == 0)
		{
			value_8 = 0x01;
		}
		else if(strcmp(item->valuestring, "G") == 0)
		{
			value_8 = 0x02;
		}
		else if(strcmp(item->valuestring, "F") == 0)
		{
			value_8 = 0x03;
		}
		else
		{
			goto exit;
		}

		props[index++] = 0x06;
		props[index++] = 0x01;
		props[index++] = value_8;
	}

	item = cJSON_GetObjectItem(branch, "D03-13");//Fan Speed
	if(item)
	{
		if(strcmp(item->valuestring, "0") == 0)
		{
			value_8 = 0x00;
		}
		else if(strcmp(item->valuestring, "s") == 0)
		{
			value_8 = 0x01;
		}
		else if(strcmp(item->valuestring, "t") == 0)
		{
			value_8 = 0x02;
		}
		else if(strcmp(item->valuestring, "1") == 0)
		{
			value_8 = 0x03;
		}
		else if(strcmp(item->valuestring, "2") == 0)
		{
			value_8 = 0x04;
		}
		else if(strcmp(item->valuestring, "3") == 0)
		{
			value_8 = 0x05;
		}
		else
		{
			goto exit;
		}

		props[index++] = 0x07;
		props[index++] = 0x01;
		props[index++] = value_8;
	}

	item = cJSON_GetObjectItem(branch, "D03-44");//aqit
	if(item)
	{
		if(get_philips_running_status()->flash_param.aqit != item->valueint)
		{
			get_philips_running_status()->flash_param.aqit = (uint16_t)item->valueint;
			mkv_item_set("pda_aqit", &get_philips_running_status()->flash_param.aqit, sizeof(get_philips_running_status()->flash_param.aqit));
			get_philips_running_status()->device_upload_manage.status_change = true;
		}
	}

	item = cJSON_GetObjectItem(branch, "D03-42");//ddp
	if(item)
	{
		if(strcmp(item->valuestring, "IAI") == 0)
		{
			value_8 = 0x00;
		}
		else if(strcmp(item->valuestring, "PM2.5") == 0)
		{
			value_8 = 0x01;
		}
		else if(strcmp(item->valuestring, "GAS") == 0)
		{
			value_8 = 0x02;
		}else if(strcmp(item->valuestring, "Formaldehyde") == 0)
		{
			value_8 = 0x03;
		}
		else
		{
			goto exit;
		}

		props[index++] = 0x0B;
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