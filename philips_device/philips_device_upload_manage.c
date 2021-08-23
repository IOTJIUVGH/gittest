// #include "mxos.h"

#include "philips_device_upload_manage.h"

#include "philips_device.h"
#include "philips_device_model.h"
#include "philips_param.h"
#include "philips_mqtt_api.h"
#include "philips_coap.h"

#include "philips_log.h"
#define app_log(M, ...)             philips_custom_log("pdum", M, ##__VA_ARGS__)

static mxos_time_t last_upload_data_time = 0;

void philips_dynamic_notification(char **out);

void device_status_upload_manage(void)
{
    merr_t err = kNoErr;
    mxos_time_t time_now = 0;

    char notice_upload_buff[10] = {0};

    char *device_status_josn = NULL;
    char *device_status_josn_without_head = NULL;

    bool iaql_report = false;
    bool tvoc_report = false;
    uint16_t tvoc_level = 0;
    char *dynamic_notice_json = NULL;

    time_now = mos_time();

    if(get_philips_running_status()->device_upload_manage.status_change || get_philips_running_status()->device_upload_manage.sensor_change || 
        get_philips_running_status()->device_upload_manage.app_get_status || get_philips_running_status()->is_reconnect_cloud)
    {
        if(get_philips_running_status()->is_reconnect_cloud)
        {
            philips_encode_json(&device_status_josn, STATUS_TYPE_CONNECT, CONNECT_TYPE_ONLINE, true);
        }
        else if(get_philips_running_status()->device_upload_manage.app_control_upload)
        {
            get_philips_running_status()->device_upload_manage.app_control_upload = false;
            philips_encode_json(&device_status_josn, STATUS_YTPE_CONTROL, CONNECT_TYPE_ONLINE, true);
        }
        else
        {
            philips_encode_json(&device_status_josn, STATUS_TYPE_STATUS, CONNECT_TYPE_ONLINE, true);
        }
        
        if (get_philips_running_status()->device_upload_manage.status_change || get_philips_running_status()->device_upload_manage.app_get_status ||
            get_philips_running_status()->is_reconnect_cloud)
        {
            philips_mqtt_public_status(device_status_josn);
            local_device_status_report(device_status_josn);
        }
        else if (get_philips_running_status()->device_upload_manage.sensor_change)
        {
            local_device_status_report(device_status_josn);
        }
        
        get_philips_running_status()->device_upload_manage.status_change = false;
        get_philips_running_status()->device_upload_manage.sensor_change = false;
        get_philips_running_status()->device_upload_manage.app_get_status = false;
        get_philips_running_status()->is_reconnect_cloud = false;
    }

    if (time_now > last_upload_data_time + 10 * 60 * 1000) //10 min
    {
        if(purifier_status.pm25 != 0 && purifier_status.iaql != 0 && purifier_status.tvoc != 0 && purifier_status.rh != 0)  //传感器的值为0时代表传感器正在初始化，此时传感器数据不能上报云端
        {
            app_log("======10 min up, device status push json array======");
            last_upload_data_time = time_now;

            philips_encode_json(&device_status_josn_without_head, STATUS_TYPE_STATUS, CONNECT_TYPE_ONLINE, false);

            philips_mqtt_public_data(device_status_josn_without_head);
        }
    }

    if (get_philips_running_status()->device_upload_manage.last_warn != get_philips_running_status()->device_upload_manage.warn)
    {
        err = kNoErr;
        if (get_philips_running_status()->device_upload_manage.warn)
        {
            app_log("======need report warn======");
            for (int i = 0; i <= 12; i++)
            {
                if ( ( (get_philips_running_status()->device_upload_manage.warn >> i) & U32_BIT0_MASK ) && !( ( get_philips_running_status()->device_upload_manage.last_warn >> i ) & U32_BIT0_MASK ) )
                {
                    notice_upload_buff[0] = 'F';
                    if(i > 9)
                    {
                        notice_upload_buff[1] = 'A' + i - 10;
                    }
                    else
                    {
                        notice_upload_buff[1] = '0' + i;
                    }
                    notice_upload_buff[2] = '\0';
                    err = philips_mqtt_public_notice(notice_upload_buff);
                }
            }
        }
        if(err == kNoErr)
        {
            get_philips_running_status()->device_upload_manage.last_warn = get_philips_running_status()->device_upload_manage.warn;
        }
    }

    if (get_philips_running_status()->device_upload_manage.last_error != get_philips_running_status()->device_upload_manage.error)
    {
        err = kNoErr;
        if (get_philips_running_status()->device_upload_manage.error)
        {
            app_log("======need report error======");
            switch (get_philips_running_status()->device_upload_manage.error)
            {
            case PHILIPS_ERROR_E0: //E0
                notice_upload_buff[0] = 'E';
                notice_upload_buff[1] = '0';
                notice_upload_buff[2] = '\0';
                break;
            case PHILIPS_ERROR_E1: //E1
                notice_upload_buff[0] = 'E';
                notice_upload_buff[1] = '1';
                notice_upload_buff[2] = '\0';
                break;
            case PHILIPS_ERROR_E2: //E2
                notice_upload_buff[0] = 'E';
                notice_upload_buff[1] = '2';
                notice_upload_buff[2] = '\0';
                break;
            case PHILIPS_ERROR_E3: //E3
                notice_upload_buff[0] = 'E';
                notice_upload_buff[1] = '3';
                notice_upload_buff[2] = '\0';
                break;
            case PHILIPS_ERROR_E4: //E4
                notice_upload_buff[0] = 'E';
                notice_upload_buff[1] = '4';
                notice_upload_buff[2] = '\0';
                break;
            case PHILIPS_ERROR_E5: //E5
                notice_upload_buff[0] = 'E';
                notice_upload_buff[1] = '5';
                notice_upload_buff[2] = '\0';
                break;
            case PHILIPS_ERROR_E6: //E6
                notice_upload_buff[0] = 'E';
                notice_upload_buff[1] = '6';
                notice_upload_buff[2] = '\0';
                break;
            case PHILIPS_ERROR_E7: //E7
                notice_upload_buff[0] = 'E';
                notice_upload_buff[1] = '7';
                notice_upload_buff[2] = '\0';
                break;
            case PHILIPS_ERROR_E8: //E8
                notice_upload_buff[0] = 'E';
                notice_upload_buff[1] = '8';
                notice_upload_buff[2] = '\0';
                break;
            case PHILIPS_ERROR_E9: //E9
                notice_upload_buff[0] = 'E';
                notice_upload_buff[1] = '9';
                notice_upload_buff[2] = '\0';
                break;
            case PHILIPS_ERROR_EA: //EA
                notice_upload_buff[0] = 'E';
                notice_upload_buff[1] = 'A';
                notice_upload_buff[2] = '\0';
                break;
            case PHILIPS_ERROR_EB: //EB
                notice_upload_buff[0] = 'E';
                notice_upload_buff[1] = 'B';
                notice_upload_buff[2] = '\0';
                break;
            default:
                break;
            }
            err = philips_mqtt_public_notice(notice_upload_buff);
        }
        if(err == kNoErr)
        {
            get_philips_running_status()->device_upload_manage.last_error = get_philips_running_status()->device_upload_manage.error;
        }
    }
    
    if (get_philips_running_status()->device_upload_manage.last_iaql < get_philips_running_status()->flash_param.aqit && purifier_status.iaql >= get_philips_running_status()->flash_param.aqit)
    {
        app_log("======need report iaql======");
        iaql_report = true;
    }

    if (get_philips_running_status()->device_upload_manage.last_iaql != purifier_status.iaql)
    {
        get_philips_running_status()->device_upload_manage.last_iaql = purifier_status.iaql;
    }
    
    if (purifier_status.air_prop_map & PHILIPS_AIR_PROP_MAP_TVOC)
    {
        tvoc_level = get_philips_running_status()->flash_param.aqit/3 + 1;

        if (get_philips_running_status()->device_upload_manage.last_tvoc < tvoc_level && purifier_status.tvoc >= tvoc_level)
        {
            app_log("======need report tvoc======");
            tvoc_report = true;
        }

        if (get_philips_running_status()->device_upload_manage.last_tvoc != purifier_status.tvoc)
        {
            get_philips_running_status()->device_upload_manage.last_tvoc = purifier_status.tvoc;
        }
    }

    if (iaql_report || tvoc_report)
    {
        philips_dynamic_notification(&dynamic_notice_json);
        if(dynamic_notice_json)
        {
            philips_mqtt_public_notice_json_str(dynamic_notice_json);
            free(dynamic_notice_json);
            dynamic_notice_json = NULL;
        }
    }
    
    if(device_status_josn)
    {
        free(device_status_josn);
        device_status_josn = NULL;
    }
    if(device_status_josn_without_head)
    {
        free(device_status_josn_without_head);
        device_status_josn_without_head = NULL;
    }
    return ;  
}

void philips_dynamic_notification(char **out)
{
    cJSON *root = NULL;

    root = cJSON_CreateObject();
    require((root != NULL), exit);

    cJSON_AddStringToObject(root, "DeviceId", get_philips_running_status()->flash_param.device_id);
    cJSON_AddStringToObject(root, "DeviceName", get_philips_running_status()->device_name);
    cJSON_AddStringToObject(root, "err", "A1");
    cJSON_AddNumberToObject(root, "pm25", purifier_status.pm25);
    cJSON_AddNumberToObject(root, "iaql", purifier_status.iaql);
    cJSON_AddNumberToObject(root, "tvoc", purifier_status.tvoc);
    cJSON_AddNumberToObject(root, "aqit", get_philips_running_status()->flash_param.aqit);
    cJSON_AddStringToObject(root, "ddp", purifier_status.ddp_str);
    if(purifier_status.air_prop_map & PHILIPS_AIR_PROP_MAP_RDDP)
        cJSON_AddStringToObject(root, "rddp", purifier_status.rddp_str);
    else
        cJSON_AddStringToObject(root, "rddp", "0");

    *out = cJSON_PrintUnformatted(root);

exit:
    if (root)
    {
        cJSON_Delete(root);
        root = NULL;
    }

    return;
}