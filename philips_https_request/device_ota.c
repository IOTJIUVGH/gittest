#include "mxos.h"
#include "mxos_config.h"
#include "mkv.h"

#include "cJSON.h"
#include "http_short_connection.h"
#include "http_file_download.h"

#include "device_https_request.h"
#include "philips_param.h"
#include "philips_device.h"

#include "philips_log.h"
#define app_log(M, ...)             			philips_custom_log("", M, ##__VA_ARGS__)

#define BIN_BUFF_LEN							2048

#define PHILIPS_OTA_REQUEST_BODY_LENGTH			128
#define PHILIPS_OTA_REQUEST_LENGTH				512
#define PHILIPS_OTA_REQUEST_BODY				"{\"product_id\":\"%s\"}"

#define OTA_URL_SIZE							256
#define OTA_PARAM_SIZE							50

static char f_url[256] = {0};

static char *f_ota_url = NULL;
static char *f_cloud_md5 = NULL;

static FILE_DOWNLOAD_CONTEXT file_download_context_user = NULL;

static mos_semphr_id_t ota_fail_sem = NULL;

static unsigned char* user_strupr(unsigned char* szMsg)
{
    unsigned char *pcMsg = NULL;

    for (pcMsg = szMsg; ('\0' != *pcMsg); pcMsg++)
    {
        if (('a' <= *pcMsg) && (*pcMsg <= 'z'))
        {
            *pcMsg += ('A' - 'a');
        }
    }

    return szMsg;
}

static bool user_str2hex(unsigned char *src, uint8_t *dest, uint32_t dest_size)
{
    unsigned char hb = 0;
    unsigned char lb = 0;
    uint32_t i = 0, j = 0;
    uint32_t src_size = strlen((const char *)src);

    if ( (src_size % 2 != 0) || (src_size <= 0))
        return false;

    src = user_strupr( src );

    for ( i = 0; i < src_size; i ++ )
    {
        if(i > dest_size * 2)
            return false;

        hb = src[i];
        if ( hb >= 'A' && hb <= 'F' )
            hb = hb - 'A' + 10;
        else if ( hb >= '0' && hb <= '9' )
            hb = hb - '0';
        else
            return false;

        i++;
        lb = src[i];
        if ( lb >= 'A' && lb <= 'F' )
            lb = lb - 'A' + 10;
        else if ( lb >= '0' && lb <= '9' )
            lb = lb - '0';
        else
            return false;

        dest[j++] = (hb << 4) | (lb);
    }

    return true;
}

static merr_t ota_check(uint32_t ota_file_len)
{
    merr_t err = kGeneralErr;
    md5_context ctx;
    uint8_t md5_calc[16] = {0};
    uint8_t md5_recv[16] = {0};
    uint16_t crc = 0;
    CRC16_Context crc16_contex;
    uint8_t *bin_buf = NULL;
    uint32_t read_index = 0;
    uint32_t file_len = ota_file_len;
    uint32_t need_read_len = 0;
    bool ret = 0;

    // require_string(mhal_flash_get_info(MODULE_PARTITION_OTA_TEMP)->partition_owner != MICO_FLASH_NONE, exit, "OTA storage is not exist");

    InitMd5(&ctx);
    CRC16_Init( &crc16_contex );

    bin_buf = malloc(BIN_BUFF_LEN);
    require_string(bin_buf != NULL, exit, "malloc bin_buff failed");

    while(1)
    {
        if(file_len - read_index >=  BIN_BUFF_LEN)
        {
            need_read_len = BIN_BUFF_LEN;
        }else
        {
            need_read_len = file_len - read_index;
        }

        err = mhal_flash_read(MODULE_PARTITION_OTA_TEMP, &read_index, bin_buf, need_read_len);
        require_noerr(err, exit);

        Md5Update(&ctx, bin_buf, need_read_len);
        CRC16_Update( &crc16_contex, bin_buf, need_read_len );

        if((read_index == ota_file_len) && (read_index != 0))
        {
            break;
        }
    }

    Md5Final(&ctx, md5_calc);
    CRC16_Final( &crc16_contex, &crc );

    app_log("FLASH READ: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
            md5_calc[0],md5_calc[1],md5_calc[2],md5_calc[3],
            md5_calc[4],md5_calc[5],md5_calc[6],md5_calc[7],
            md5_calc[8],md5_calc[9],md5_calc[10],md5_calc[11],
            md5_calc[12],md5_calc[13],md5_calc[14],md5_calc[15]);

    ret = user_str2hex((uint8_t *)f_cloud_md5, md5_recv, sizeof(md5_recv));
    require_action_string(ret == true, exit, err = kGeneralErr, "user_str2hex() is error");

    if ( memcmp( md5_recv, md5_calc, sizeof(md5_recv) ) == 0 )
    {
        app_log( "OTA MD5 CHECK SUCCESS!\r\n" );

        if ( get_philips_running_status()->ota_process.ota_type == OTATYPE_WIFI )
		{
			err = mxos_ota_switch_to_new_fw(ota_file_len, crc);
			require_noerr(err, exit);
            app_log("OTA SUCCESS!\r\n");
            get_philips_running_status()->ota_process.ota_status = OTAST_INSTALL;
            get_philips_running_status()->flash_param.wifi_reboot_status = WRBT_wifi_ota;
            mkv_item_set("pwrr_status", &get_philips_running_status()->flash_param.wifi_reboot_status, sizeof(get_philips_running_status()->flash_param.wifi_reboot_status));
            mos_msleep(300);
            MxosSystemReboot();
			// g_device_running_status.need_update_shadow = true;
			// mico_system_power_perform(mico_system_context_get(), eState_Software_Reset);
		}
		else if ( get_philips_running_status()->ota_process.ota_type == OTATYPE_DEVICE )
		{
            get_philips_running_status()->otau_flash_param.otau_data_len = ota_file_len;
            get_philips_running_status()->otau_flash_param.is_otau_file_downloaded_from_cloud = true;
            get_philips_running_status()->otau_flash_param.need_ota_device = true;
            mkv_item_set(CONFIG_DATA_KV_KEY_OTAU, &get_philips_running_status()->otau_flash_param, sizeof(get_philips_running_status()->otau_flash_param));

            get_philips_running_status()->otau_run_state.otau_total_package = ota_file_len / 128;
            if (get_philips_running_status()->otau_run_state.otau_total_package * 128 < ota_file_len)
                ++get_philips_running_status()->otau_run_state.otau_total_package;
            get_philips_running_status()->otau_run_state.otau_current_package = -1;
            get_philips_running_status()->otau_run_state.otau_data_leave = ota_file_len;
            get_philips_running_status()->otau_run_state.otau_data_index = 0;
            get_philips_running_status()->otau_run_state.device_otau_last_state = OPSV_UnKnow;

            get_philips_running_status()->otau_run_state.otau_state_id_control = 0;
            get_philips_running_status()->otau_run_state.need_get_otau = true;  //触发GETProps(8),进行OTA升级
		}
    }else
    {
        app_log("ERROR!! MD5 Error.");
        app_log("HTTP RECV:   %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
                 md5_recv[0],md5_recv[1],md5_recv[2],md5_recv[3],
                 md5_recv[4],md5_recv[5],md5_recv[6],md5_recv[7],
                 md5_recv[8],md5_recv[9],md5_recv[10],md5_recv[11],
                 md5_recv[12],md5_recv[13],md5_recv[14],md5_recv[15]);

        err = kGeneralErr;
    }

 exit:
    if(bin_buf != NULL)
    {
        free(bin_buf);
        bin_buf = NULL;
    }

    return err;
}

void file_download_state_cb(void *context, HTTP_FILE_DOWNLOAD_STATE_E state, uint32_t sys_args, uint32_t user_args)
{
    merr_t err = kNoErr;
    FILE_DOWNLOAD_CONTEXT file_download_context = (FILE_DOWNLOAD_CONTEXT) context;

    require(file_download_context != NULL && state < HTTP_FILE_DOWNLOAD_STATE_MAX, exit);

    if (state == HTTP_FILE_DOWNLOAD_STATE_START)
    {
        // erase flash
        // require_string(mhal_flash_get_info(MODULE_PARTITION_OTA_TEMP)->partition_owner != MICO_FLASH_NONE, exit_http_start, "OTA storage is not exist!");

        // 如果OTA分区保存MCUOTA固件，需要把flash中保存的OTAU信息清空。
        if (get_philips_running_status()->otau_flash_param.is_otau_file_downloaded_from_cloud == true)
        {
            app_log("clear kv mcu ota firmware info");
            get_philips_running_status()->otau_flash_param.is_otau_file_downloaded_from_cloud = false;
            get_philips_running_status()->otau_flash_param.need_ota_device = false;
            get_philips_running_status()->otau_flash_param.otau_data_len = 0;
	        mkv_item_set(CONFIG_DATA_KV_KEY_OTAU, &get_philips_running_status()->otau_flash_param, sizeof(get_philips_running_status()->otau_flash_param));
            // mos_msleep(300);
        }

        app_log("erase MICO_PARTITION_OTA_TEMP flash start");

        err = mhal_flash_erase(MODULE_PARTITION_OTA_TEMP, 0, mhal_flash_get_info(MODULE_PARTITION_OTA_TEMP)->partition_length);
        require_noerr(err, exit_http_start);

        app_log("erase MICO_PARTITION_OTA_TEMP flash success");

        exit_http_start:
        if (err != kNoErr)
        {
            http_file_download_stop(&file_download_context, false);
        }
    }
    else if (state == HTTP_FILE_DOWNLOAD_STATE_SUCCESS)
    {
        app_log("f_total_ota_len = %ld", http_file_download_get_total_file_len(&file_download_context));

        //calculate MD5 and crc16
        err = ota_check(http_file_download_get_total_file_len(&file_download_context));
        // if(err == kNoErr)
        // {
        // }
        // else
        // {
        // }
    }
    else if (state == HTTP_FILE_DOWNLOAD_STATE_LOADING)
    {
        app_log("file downloading: %ld%%", sys_args);
    }
    else if (state == HTTP_FILE_DOWNLOAD_STATE_FAILED_AND_RETRY)
    {
        app_log("state == HTTP_FILE_DOWNLOAD_STATE_FAILED_AND_RETRY");
    }
    else if (state == HTTP_FILE_DOWNLOAD_STATE_FAILED)
    {
        app_log("state == HTTP_FILE_DOWNLOAD_STATE_FAILED");
        err = kNoResourcesErr;
    }
    else if (state == HTTP_FILE_DOWNLOAD_STATE_REDIRET)
    {
        app_log("state == HTTP_FILE_DOWNLOAD_STATE_REDIRET");
    }
exit:
    if ( err != kNoErr )
		mos_semphr_release(ota_fail_sem);
    return;
}

bool file_download_data_cb(void *context, const char *data, uint32_t data_len, uint32_t user_args)
{
	merr_t err = kGeneralErr;
    FILE_DOWNLOAD_CONTEXT file_download_context = (FILE_DOWNLOAD_CONTEXT)context;
    uint32_t index = http_file_download_get_download_len(&file_download_context);

    // app_log("file download, total len:%lu, get:%lu, len:%lu", file_download_context->file_info.file_total_len, file_download_context->file_info.download_len, data_len);
    require_action_string(file_download_check_control_state(file_download_context) == true, exit, err = kGeneralErr, "user set stop download!");
    require_string(mhal_flash_get_info(MODULE_PARTITION_OTA_TEMP)->partition_length > http_file_download_get_total_file_len(&file_download_context), exit, "file len error!");
    // require_string(mhal_flash_get_info(MODULE_PARTITION_OTA_TEMP)->partition_owner != MICO_FLASH_NONE, exit, "OTA storage is not exist");

	require(get_philips_running_status()->ota_process.ota_type == OTATYPE_WIFI || get_philips_running_status()->ota_process.ota_type == OTATYPE_DEVICE, exit);
    //copy data into flash
    err = mhal_flash_write(MODULE_PARTITION_OTA_TEMP, &index, (uint8_t *) data, data_len);
    require_noerr(err, exit);

exit:
    if(err == kNoErr)
    {
        return true;
    }
    else
    {
        app_log("flash write error");
        return false;
    }
}

merr_t file_start_download(void)
{
    merr_t err = kGeneralErr;

    require_action(get_philips_running_status()->ota_process.ota_type == OTATYPE_WIFI || get_philips_running_status()->ota_process.ota_type == OTATYPE_DEVICE, exit, err = kGeneralErr);

    app_log("http_file_download_start, f_ota_url = %s, f_cloud_md5 = %s", f_ota_url, f_cloud_md5);
    err = http_file_download_start(&file_download_context_user, f_ota_url, file_download_state_cb, file_download_data_cb, 0);
    require_noerr_string(err, exit, "http_file_download_start fail");

exit:
    return err;
}

merr_t philips_start_download_file(void)
{
    merr_t err = kNoErr;
    int retry = 0;

    get_philips_running_status()->ota_process.ota_status = OTAST_DOWNLOADING;

    ota_fail_sem = mos_semphr_new(1);
    require_action((ota_fail_sem != NULL), exit, err = kGeneralErr);

download_again:
    MEMORY_DUMP;
    while (mos_semphr_acquire(ota_fail_sem, 0) == kNoErr); //清空残余的信号量

    err = file_start_download();
    if(err == kNoErr)
    {
        if (get_philips_running_status()->ota_process.ota_type == OTATYPE_WIFI)
        {
            mos_semphr_acquire(ota_fail_sem, MOS_WAIT_FOREVER);
        }
        else if (get_philips_running_status()->ota_process.ota_type == OTATYPE_DEVICE)
        {
        get_otau_sem:
            err = mos_semphr_acquire(ota_fail_sem, 1000);
            if (err != kNoErr)
            {
                if (!get_philips_running_status()->otau_flash_param.is_otau_file_downloaded_from_cloud)     //MCU OTA固件未下载完成
                {
                    goto get_otau_sem;
                }
                else    //MCU OTA固件下载完成，正常退出
                {
                    err = kNoErr;
                    goto exit;
                }
            }
            else    //MCU OTA固件下载失败
            {
                err = kExecutionStateErr;
                // goto exit;
            }
        }
        else
        {
            err = kUserRequiredErr;
            goto exit;
        }
    }
    else
    {
        app_log("file download start fail");
        mos_msleep(1000);
    }

    if (++retry < 3)
    {
        goto download_again;
    }
    else
    {
        app_log("ota retry 3, still fail");
        err = kTimeoutErr;
        goto exit;
    }

exit:
    if (ota_fail_sem)
    {
        mos_semphr_delete(ota_fail_sem);
        ota_fail_sem = NULL;
    }

    if(err == kNoErr)
        get_philips_running_status()->ota_process.ota_status = OTAST_DOWNLOADED;
    else
        get_philips_running_status()->ota_process.ota_status = OTAST_FAILED;

    return err;
}

merr_t philips_ota_url_md5_buff_init(void)
{
    merr_t err = kNoErr;

    if(f_ota_url == NULL)
    {
        f_ota_url = (char *)malloc(OTA_URL_SIZE);
    }
    if(f_cloud_md5 == NULL)
    {
        f_cloud_md5 = (char *)malloc(OTA_PARAM_SIZE);
    }
    if ( f_ota_url == NULL || f_cloud_md5 == NULL )
	{
		app_log("malloc err");
		err = kNoMemoryErr;
		goto exit;
	}
    memset(f_ota_url, 0x00, OTA_URL_SIZE);
	memset(f_cloud_md5, 0x00, OTA_PARAM_SIZE);
exit:
    return err;
}

merr_t philips_ota_url_md5_buff_deinit(void)
{
    merr_t err = kNoErr;

    // if ( f_ota_url != NULL )
	// {
	// 	free(f_ota_url);
	// 	f_ota_url = NULL;
	// }
	// if ( f_cloud_md5 != NULL )
	// {
	// 	free(f_cloud_md5);
	// 	f_cloud_md5 = NULL;
	// }

    return err;
}

merr_t philips_start_download_file_with_url_md5(char *url, int urllen, char *md5, int md5len)
{
    merr_t err = kNoErr;

    require_action(url != NULL && urllen < OTA_URL_SIZE && md5 != NULL && md5len == 32, exit, err = kParamErr);

    err = philips_ota_url_md5_buff_init();
    require_noerr(err, exit);

    memcpy(f_ota_url, url, urllen);
	memcpy(f_cloud_md5, md5, md5len);

    err = philips_start_download_file();
    require_noerr(err, exit);

exit:
    philips_ota_url_md5_buff_deinit();

    return err;
}

merr_t philips_ota_check_rquest_data_process(cJSON *root)
{
    char productid[OTA_PARAM_SIZE] = {0};
	char cloud_software_version[OTA_PARAM_SIZE] = {0};
	char *cloud_version_data = NULL;
	int cloud_version_major = 0;
	int cloud_version_minor = 0;

    merr_t err = kGeneralErr;

    cJSON *item = NULL;

    // ls product_id
    item = cJSON_GetObjectItem(root, "product_id");
    require_action(item, exit, err = kResponseErr);
    strncpy(productid, item->valuestring, sizeof(productid)-1);

    item = cJSON_GetObjectItem(root, "software_version");
    require_action(item, exit, err = kResponseErr);
    strncpy(cloud_software_version, item->valuestring, sizeof(cloud_software_version)-1);

    cloud_version_data = strstr(cloud_software_version, WIFIWARE_VERSION_PREFIX);
    cloud_version_data += strlen(WIFIWARE_VERSION_PREFIX);
    cloud_version_minor = atoi(cloud_version_data);

    app_log("cloud_version_minor    = %d, cloud_version_major    = %d", cloud_version_minor, cloud_version_major);
	app_log("WIFIWARE_VERSION_MINOR = %d, WIFIWARE_VERSION_MAJOR = %d", WIFIWARE_VERSION_MINOR, WIFIWARE_VERSION_MAJOR);

    if(cloud_version_minor > WIFIWARE_VERSION_MINOR)
    {
        app_log("need ota wifi");

        item = cJSON_GetObjectItem(root, "file_url");
        require_action(item, exit, err = kResponseErr);
        strncpy(f_ota_url, item->valuestring, OTA_URL_SIZE);

        item = cJSON_GetObjectItem(root, "md5");
        require_action(item, exit, err = kResponseErr);
        strncpy(f_cloud_md5, item->valuestring, OTA_PARAM_SIZE);

        get_philips_running_status()->ota_process.ota_type = OTATYPE_WIFI;
        err = kNoErr;
    }
    else
    {
        app_log("needn't ota wifi");

        if((get_philips_running_status()->is_support_otau) && (get_philips_running_status()->ota_process.need_check_otau) && (get_philips_running_status()->flash_param.muc_boot))
        {
            app_log("get_philips_running_status()->is_support_otau");

            item = cJSON_GetObjectItem(root, "software_version_1");
            require_action(item, exit, err = kResponseErr);
            strncpy(get_philips_running_status()->otau_flash_param.cloud_device_version, item->valuestring, sizeof(get_philips_running_status()->otau_flash_param.cloud_device_version));

            app_log("cloud_device_version    = %s", get_philips_running_status()->otau_flash_param.cloud_device_version);
			app_log("local_device_version    = %s", get_philips_running_status()->otau_run_state.local_device_version);

            int res = strncmp(get_philips_running_status()->otau_flash_param.cloud_device_version, get_philips_running_status()->otau_run_state.local_device_version, sizeof(get_philips_running_status()->otau_run_state.local_device_version));
            if ( res > 0)
            {
                app_log("need ota device");

                item = cJSON_GetObjectItem(root, "file_url_1");
                require_action(item, exit, err = kResponseErr);
                strncpy(f_ota_url, item->valuestring, OTA_URL_SIZE);

                item = cJSON_GetObjectItem(root, "md5_1");
                require_action(item, exit, err = kResponseErr);
                strncpy(f_cloud_md5, item->valuestring, OTA_PARAM_SIZE);

                get_philips_running_status()->ota_process.ota_type = OTATYPE_DEVICE;
                err = kNoErr;
            }
            else if(res == 0)
            {
                app_log("needn't ota device, same version");
                err = kVersionErr;
                goto exit;
            }
            else
            {
                app_log("needn't ota device, cloud version low");
                err = kVersionErr;
                goto exit;
            }
        }
        else
        {
            app_log("this model is not support otau");
            err = kVersionErr;
            goto exit;
        }
    }
exit:
    return err;
}

merr_t philips_device_ota_check_request_and_download(void)
{
    merr_t err = kNoErr;
	HTTP_REQ_S req = HTTP_REQ_INIT_PARAM;
	char *body = NULL;
	int max_body_len = PHILIPS_OTA_REQUEST_BODY_LENGTH;
	int max_req_len = PHILIPS_OTA_REQUEST_LENGTH;
    char user_agent[100] = {0};

	MEMORY_DUMP;

    err = philips_ota_url_md5_buff_init();
    require_noerr(err, exit);

    MEMORY_DUMP;

    memset(f_url, 0, sizeof(f_url));
    strcpy(f_url, get_philips_running_status()->philips_urls[get_philips_running_status()->flash_param.url_type].url_ota);

    err = device_http_param_init(&req, f_url, &body, max_body_len, max_req_len);
	require_noerr(err, exit);

	MEMORY_DUMP;

    req.req_len = snprintf(body, PHILIPS_OTA_REQUEST_BODY_LENGTH, PHILIPS_OTA_REQUEST_BODY, get_philips_running_status()->flash_param.product_id);
    require_action(req.req_len != 0 && req.req_len < PHILIPS_OTA_REQUEST_BODY_LENGTH, exit, err = kSizeErr);

    philips_User_Agent_generate(user_agent, sizeof(user_agent));

    req.req_len = snprintf(req.http_req, PHILIPS_OTA_REQUEST_LENGTH, HTTP_COMMON_POST, f_url, req.domain_name, user_agent, strlen(body), body);
	require_action(req.req_len != 0 && req.req_len < PHILIPS_OTA_REQUEST_LENGTH, exit, err = kSizeErr);

    app_log("https request = [%d]\r\n%.*s", (int)req.req_len, (int)req.req_len, req.http_req);

    MEMORY_DUMP;

    err = device_http_post_request(&req, philips_ota_check_rquest_data_process, REQUEST_MAX_TIME);
    require_noerr_action_string(err, exit, err = kGeneralErr, "philips_ota_check_rquest_data_process fail");

    MEMORY_DUMP;

    err = philips_start_download_file();
    require_noerr(err, exit);

    MEMORY_DUMP;

exit:

    philips_ota_url_md5_buff_deinit();
	device_http_param_deinit(&req, &body);
	MEMORY_DUMP;

    return err;
}