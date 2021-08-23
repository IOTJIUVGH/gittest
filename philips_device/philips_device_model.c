// #include "mxos.h"
#include "mkv.h"

#include "philips_device.h"
#include "philips_device_model.h"
#include "philips_device_fltsts_process.h"
#include "philips_device_upload_manage.h"
#include "philips_device_factory.h"

#include "philips_param.h"
#include "philips_uart.h"

#include "philips_log.h"
#define app_log(M, ...)             			philips_custom_log("", M, ##__VA_ARGS__)

merr_t philips_check_product_range(void)
{
    merr_t err = kNoErr;

	app_log("range = %s", get_philips_running_status()->flash_param.product_range);
	if ( strncmp(get_philips_running_status()->flash_param.product_range, "O2", strlen("O2") + 1) == 0 )
	{
		get_philips_running_status()->range_id = PHILIPS_RANGE_O2;
	}
	else if ( strncmp(get_philips_running_status()->flash_param.product_range, "MicroMario", strlen("MicroMario") + 1) == 0 )
	{
		get_philips_running_status()->range_id = PHILIPS_RANGE_MicroMario;
	}
	else if ( strncmp(get_philips_running_status()->flash_param.product_range, "PUMA", strlen("PUMA") + 1) == 0 )
	{
		get_philips_running_status()->range_id = PHILIPS_RANGE_PUMA;
	}
	else if ( strncmp(get_philips_running_status()->flash_param.product_range, "MicroCube2.0", strlen("MicroCube2.0") + 1) == 0 )
	{
		get_philips_running_status()->range_id = PHILIPS_RANGE_MicroCube2;
		get_philips_running_status()->is_support_otau = true;
	}
	else if ( strncmp(get_philips_running_status()->flash_param.product_range, "Thor", strlen("Thor") + 1) == 0 )
	{
		get_philips_running_status()->range_id = PHILIPS_RANGE_Thor;
	}
	else if ( strncmp(get_philips_running_status()->flash_param.product_range, "Loki", strlen("Loki") + 1) == 0 )
	{
		get_philips_running_status()->range_id = PHILIPS_RANGE_Loki;
	}
	else if ( strncmp(get_philips_running_status()->flash_param.product_range, "Comfort", strlen("Comfort") + 1) == 0 )
	{
		get_philips_running_status()->range_id = PHILIPS_RANGE_Comfort;
	}
	else if ( strncmp(get_philips_running_status()->flash_param.product_range, "Simba", strlen("Simba") + 1) == 0 )
	{
		get_philips_running_status()->range_id = PHILIPS_RANGE_Simba;
	}
	else if ( strncmp(get_philips_running_status()->flash_param.product_range, "Mario", strlen("Mario") + 1) == 0 )
	{
		get_philips_running_status()->range_id = PHILIPS_RANGE_Mario;
		get_philips_running_status()->is_support_otau = true;
	}
	else if ( strncmp(get_philips_running_status()->flash_param.product_range, "MicroCube1.0", strlen("MicroCube1.0") + 1) == 0 )
	{
		get_philips_running_status()->range_id = PHILIPS_RANGE_MicroCube1;
	}
	else if ( strncmp(get_philips_running_status()->flash_param.product_range, "MarsHE", strlen("MarsHE") + 1) == 0 )
	{
		get_philips_running_status()->range_id = PHILIPS_RANGE_Mars;
		get_philips_running_status()->is_support_otau = true;
	}
	else if ( strncmp(get_philips_running_status()->flash_param.product_range, "MarsME", strlen("MarsME") + 1) == 0 )
	{
		get_philips_running_status()->range_id = PHILIPS_RANGE_Mars;
		get_philips_running_status()->is_support_otau = true;
	}
	else if ( strncmp(get_philips_running_status()->flash_param.product_range, "MarsLE", strlen("MarsLE") + 1) == 0 )
	{
		get_philips_running_status()->range_id = PHILIPS_RANGE_MarsLE;
		get_philips_running_status()->is_support_otau = true;
	}
	else if ( strncmp(get_philips_running_status()->flash_param.product_range, "MarteHE", strlen("MarteHE") + 1) == 0 )
	{
		get_philips_running_status()->range_id = PHILIPS_RANGE_Marte;
		get_philips_running_status()->is_support_otau = true;
	}
	else if ( strncmp(get_philips_running_status()->flash_param.product_range, "MarteME", strlen("MarteME") + 1) == 0 )
	{
		get_philips_running_status()->range_id = PHILIPS_RANGE_Marte;
		get_philips_running_status()->is_support_otau = true;
	}
	else
	{
		get_philips_running_status()->range_id = PHILIPS_RANGE_NONE;
		err = kRangeErr;
	}

    return err;
}

void philips_process_air_properties(const uint8_t *data)
{
    if(get_philips_running_status()->range_id == PHILIPS_RANGE_NONE)
    {
        app_log("device type unknown");
        return ;
    }
    switch(get_philips_running_status()->range_id)
    {
    case PHILIPS_RANGE_O2:
        philips_process_air_properties_O2(data);
        break;
    case PHILIPS_RANGE_MicroMario:
        philips_process_air_properties_MicroMario(data);
        break;
    case PHILIPS_RANGE_PUMA:
        philips_process_air_properties_PUMA(data);
        break;
    case PHILIPS_RANGE_MicroCube2:
        philips_process_air_properties_MicroCube2(data);
        break;
    case PHILIPS_RANGE_Thor:
        philips_process_air_properties_Thor(data);
        break;
    case PHILIPS_RANGE_Loki:
        philips_process_air_properties_Loki(data);
        break;
    case PHILIPS_RANGE_Comfort:
        philips_process_air_properties_Comfort(data);
        break;
    case PHILIPS_RANGE_Simba:
        philips_process_air_properties_Simba(data);
        break;
    case PHILIPS_RANGE_Mario:
        philips_process_air_properties_Mario(data);
        break;
    case PHILIPS_RANGE_MicroCube1:
        philips_process_air_properties_MicroCube1(data);
        break;
    case PHILIPS_RANGE_Mars:
        philips_process_air_properties_Mars(data);
        break;
    case PHILIPS_RANGE_MarsLE:
        philips_process_air_properties_MarsLE(data);
        break;
    case PHILIPS_RANGE_Marte:
        philips_process_air_properties_Marte(data);
        break;
    case PHILIPS_RANGE_COMMON:
        philips_process_air_properties_Common(data);
        break;
    default:
        break;
    }

    return ;
}

void philips_process_fltsts_properties(const uint8_t *data)
{
    if(get_philips_running_status()->range_id == PHILIPS_RANGE_NONE)
    {
        app_log("device type unknown");
        return ;
    }
    switch(get_philips_running_status()->range_id)
    {
    case PHILIPS_RANGE_O2:
        philips_process_fltsts_properties_common(data);
        break;
    case PHILIPS_RANGE_MicroMario:
        philips_process_fltsts_properties_common(data);
        break;
    case PHILIPS_RANGE_PUMA:
        philips_process_fltsts_properties_common(data);
        break;
    case PHILIPS_RANGE_MicroCube2:
        philips_process_fltsts_properties_common(data);
        break;
    case PHILIPS_RANGE_Thor:
        philips_process_fltsts_properties_common(data);
        break;
    case PHILIPS_RANGE_Loki:
        philips_process_fltsts_properties_common(data);
        break;
    case PHILIPS_RANGE_Comfort:
        philips_process_fltsts_properties_common(data);
        break;
    case PHILIPS_RANGE_Simba:
        philips_process_fltsts_properties_common(data);
        break;
    case PHILIPS_RANGE_Mario:
        philips_process_fltsts_properties_common(data);
        break;
    case PHILIPS_RANGE_MicroCube1:
        philips_process_fltsts_properties_common(data);
        break;
    case PHILIPS_RANGE_Mars:
        philips_process_fltsts_properties_Mars_or_MarsLE(data);
        break;
    case PHILIPS_RANGE_MarsLE:
        philips_process_fltsts_properties_Mars_or_MarsLE(data);
        break;
    case PHILIPS_RANGE_Marte:
        philips_process_fltsts_properties_Mars_or_MarsLE(data);
        break;
    case PHILIPS_RANGE_COMMON:
        philips_process_fltsts_properties_new_common(data);
        break;
    default:
        break;
    }

    return ;
}

void philips_process_cloud_data(char *data, int datalen)
{
    if(get_philips_running_status()->range_id == PHILIPS_RANGE_NONE)
	{
		app_log("device type unknown");
		return ;
	}
	switch(get_philips_running_status()->range_id)
	{
	case PHILIPS_RANGE_O2:
		philips_process_cloud_data_O2(data, datalen);
		break;
	case PHILIPS_RANGE_MicroMario:
		philips_process_cloud_data_MicroMario(data, datalen);
		break;
	case PHILIPS_RANGE_PUMA:
		philips_process_cloud_data_PUMA(data, datalen);
		break;
	case PHILIPS_RANGE_MicroCube2:
		philips_process_cloud_data_MicroCube2(data, datalen);
		break;
	case PHILIPS_RANGE_Thor:
		philips_process_cloud_data_Thor(data, datalen);
		break;
	case PHILIPS_RANGE_Loki:
		philips_process_cloud_data_Loki(data, datalen);
		break;
	case PHILIPS_RANGE_Comfort:
		philips_process_cloud_data_Comfort(data, datalen);
		break;
	case PHILIPS_RANGE_Simba:
		philips_process_cloud_data_Simba(data, datalen);
		break;
	case PHILIPS_RANGE_Mario:
		philips_process_cloud_data_Mario(data, datalen);
		break;
	case PHILIPS_RANGE_MicroCube1:
		philips_process_cloud_data_MicroCube1(data, datalen);
		break;
	case PHILIPS_RANGE_Mars:
		philips_process_cloud_data_Mars(data, datalen);
		break;
	case PHILIPS_RANGE_MarsLE:
		philips_process_cloud_data_MarsLE(data, datalen);
		break;
    case PHILIPS_RANGE_Marte:
        philips_process_cloud_data_Marte(data, datalen);
        break;
	default:
		break;
	}

    return ;
}

void philips_encode_json(char **buf, const char *StatusType, const char *ConnectType, bool head)
{
    mwifi_link_info_t info;

	mwifi_get_link_info(&info);
    get_philips_running_status()->rssi=info.rssi;

    if(get_philips_running_status()->range_id == PHILIPS_RANGE_NONE)
    {
        app_log("device type unknown");
        return ;
    }
    switch(get_philips_running_status()->range_id)
    {
    case PHILIPS_RANGE_O2:
        philips_encode_json_O2(buf, StatusType, ConnectType, head);
        break;
    case PHILIPS_RANGE_MicroMario:
        philips_encode_json_MicroMario(buf, StatusType, ConnectType, head);
        break;
    case PHILIPS_RANGE_PUMA:
        philips_encode_json_PUMA(buf, StatusType, ConnectType, head);
        break;
    case PHILIPS_RANGE_MicroCube2:
        philips_encode_json_MicroCube2(buf, StatusType, ConnectType, head);
        break;
    case PHILIPS_RANGE_Thor:
        philips_encode_json_Thor(buf, StatusType, ConnectType, head);
        break;
    case PHILIPS_RANGE_Loki:
        philips_encode_json_Loki(buf, StatusType, ConnectType, head);
        break;
    case PHILIPS_RANGE_Comfort:
        philips_encode_json_Comfort(buf, StatusType, ConnectType, head);
        break;
    case PHILIPS_RANGE_Simba:
        philips_encode_json_Simba(buf, StatusType, ConnectType, head);
        break;
    case PHILIPS_RANGE_Mario:
        philips_encode_json_Mario(buf, StatusType, ConnectType, head);
        break;
    case PHILIPS_RANGE_MicroCube1:
        philips_encode_json_MicroCube1(buf, StatusType, ConnectType, head);
        break;
    case PHILIPS_RANGE_Mars:
        philips_encode_json_Mars(buf, StatusType, ConnectType, head);
        break;
    case PHILIPS_RANGE_MarsLE:
        philips_encode_json_MarsLE(buf, StatusType, ConnectType, head);
        break;
    case PHILIPS_RANGE_Marte:
        philips_encode_json_Marte(buf, StatusType, ConnectType, head);
        break;
    default:
        break;
    }
}

merr_t philips_process_cloud_ota_request_info(cJSON *root)
{
    merr_t err = kNoErr;
    cJSON *item = NULL;

    require_action( (root != NULL), exit, err = kGeneralErr);

    item = cJSON_GetObjectItem(root, "url");
    require_action(item, exit, err = kGeneralErr);
    memset(get_philips_running_status()->ota_process.url_value, 0, sizeof(get_philips_running_status()->ota_process.url_value));
    strncpy(get_philips_running_status()->ota_process.url_value, item->valuestring, sizeof(get_philips_running_status()->ota_process.url_value)-1);

    item = cJSON_GetObjectItem(root, "md5");
    require_action(item, exit, err = kGeneralErr);
    memset(get_philips_running_status()->ota_process.md5_value, 0, sizeof(get_philips_running_status()->ota_process.md5_value));
    strncpy(get_philips_running_status()->ota_process.md5_value, item->valuestring, sizeof(get_philips_running_status()->ota_process.md5_value)-1);

    item = cJSON_GetObjectItem(root, "type");
    require_action(item, exit, err = kGeneralErr);
    memset(get_philips_running_status()->ota_process.type_value, 0, sizeof(get_philips_running_status()->ota_process.type_value));
    strncpy(get_philips_running_status()->ota_process.type_value, item->valuestring, sizeof(get_philips_running_status()->ota_process.type_value)-1);

    item = cJSON_GetObjectItem(root, "software_version");
    require_action(item, exit, err = kGeneralErr);
    memset(get_philips_running_status()->ota_process.version_value, 0, sizeof(get_philips_running_status()->ota_process.version_value));
    strncpy(get_philips_running_status()->ota_process.version_value, item->valuestring, sizeof(get_philips_running_status()->ota_process.version_value)-1);

    if(strncmp(get_philips_running_status()->ota_process.type_value, OTATYPE_WIFI_CLOUD_TYPE, strlen(OTATYPE_WIFI_CLOUD_TYPE)) == 0)
    {
        app_log("get request ota info is wifi");
        get_philips_running_status()->ota_process.ota_type = OTATYPE_WIFI;
        get_philips_running_status()->ota_process.ota_status = OTAST_REQUESTRECEIVED;
    }
    else if(strncmp(get_philips_running_status()->ota_process.type_value, OTATYPE_DEVICE_CLOUD_TYPE, strlen(OTATYPE_DEVICE_CLOUD_TYPE)) == 0)
    {
        app_log("get request ota info is device");
        if((get_philips_running_status()->is_support_otau)&&(get_philips_running_status()->flash_param.muc_boot))
        {
            strncpy(get_philips_running_status()->otau_flash_param.cloud_device_version, get_philips_running_status()->ota_process.version_value, sizeof(get_philips_running_status()->otau_flash_param.cloud_device_version));
            get_philips_running_status()->ota_process.ota_type = OTATYPE_DEVICE;
            get_philips_running_status()->otau_flash_param.is_otau_file_downloaded_from_cloud = false;
            get_philips_running_status()->ota_process.ota_status = OTAST_REQUESTRECEIVED;
        }
        else
        {
            app_log("device not support ota");
            err = kTypeErr;
        }
    }
    else
    {
        app_log("get request ota info is %s, not support", get_philips_running_status()->ota_process.type_value);
        err = kTypeErr;
    }

exit:
    return err;
}
