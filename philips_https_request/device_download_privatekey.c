#include "mxos.h"
#include "http_short_connection.h"
#include "cJSON.h"
#include "mkv.h"
#include "HTTPUtils.h"

#include "device_https_request.h"
#include "philips_https_request.h"

#include "philips_basic_api.h"
#include "philips_param.h"

#include "philips_log.h"
#define app_log(M, ...)             			philips_custom_log("", M, ##__VA_ARGS__)

static merr_t https_response_data_process(char *data, int len, char *head, int head_len)
{
	merr_t err = kNoErr;
	uint32_t offset = PRIVATEKEY_OFFSET;

	char *value;
	size_t value_len;
	md5_context ctx;
	uint8_t md5_calc[16] = {0};
	char md5_recv_str[36] = {0};
	char md5_calc_str[36] = {0};

	err = HTTPGetHeaderField(head, (size_t)head_len, "ETag", NULL, NULL, (const char **)&value, &value_len, NULL);
	if(err == kNoErr)
	{
		app_log("MD5:%.*s\r\n", value_len, value);
		if(value_len == 34 && value[0] == '"' && value[33] == '"')
		{
			memcpy(md5_recv_str, value + 1, value_len - 2);
		}
		else if(value_len == 32)
		{
			memcpy(md5_recv_str, value, value_len);
		}
		else
		{
			err = kParamErr;
			goto exit;
		}
	}
	else
	{
		err = kParamErr;
		goto exit;
	}

	if ( (get_philips_running_status()->flash_param.privatekey_len != len) || (memcmp(get_philips_running_status()->privatekey, data, len) != 0) )
	{
		app_log("update privatekey");

		// 擦除私钥分区
		mhal_flash_erase(MODULE_PARTITION_USER, PRIVATEKEY_OFFSET, 2048);

		// 私钥写入flash
		memset(get_philips_running_status()->privatekey, 0x00, sizeof(get_philips_running_status()->privatekey));
		memcpy(get_philips_running_status()->privatekey, data, len);
		err = mhal_flash_write(MODULE_PARTITION_USER, &offset, (uint8_t *)(get_philips_running_status()->privatekey), (uint32_t)len);
		require_noerr_action(err, exit, err = kWriteErr);

		// 私钥从flash中重新读取
		offset = PRIVATEKEY_OFFSET;
		memset(get_philips_running_status()->privatekey, 0x00, sizeof(get_philips_running_status()->privatekey));
		err = mhal_flash_read(MODULE_PARTITION_USER, &offset, (uint8_t *)(get_philips_running_status()->privatekey), (uint32_t)len);
		require_noerr_action(err, exit, err = kReadErr);

		// 私钥MD5计算
		InitMd5(&ctx);
		Md5Update(&ctx, (uint8_t *)(get_philips_running_status()->privatekey), len);
		Md5Final(&ctx, md5_calc);

		// 计算私钥MD5的hex转换成str
		philips_hex2str(md5_calc_str, md5_calc, sizeof(md5_calc));
		// 接收私钥MD5小写转大写
		philips_str_lower2upper(md5_recv_str, strlen(md5_recv_str));

		// 证书MD5校验
		if( memcmp(md5_recv_str, md5_calc_str, strlen(md5_recv_str)) != 0 )
		{
			app_log("privatekey md5 verify fail, recv_md5:%s, calc_md5:%s", md5_recv_str, md5_calc_str);
			err = kChecksumErr;
			goto exit;
		}
		else
		{
			app_log("privatekey md5 verify success");
		}

		get_philips_running_status()->flash_param.privatekey_len = len;
		mkv_item_set("pmifhr_plen", &get_philips_running_status()->flash_param.privatekey_len, sizeof(get_philips_running_status()->flash_param.privatekey_len));
	}
	else
	{
		app_log("privatekey don't channge");
	}

	exit:
	return err;
}

static void https_request_thread(void *arg)
{
	merr_t err = kNoErr;
	HTTP_REQ_S req = HTTP_REQ_INIT_PARAM;
	char *body = NULL;
	int max_body_len = 512;
	int max_req_len = 1024;
	char user_agent[100] = {0};

	MEMORY_DUMP;

	err = device_http_param_init(&req, get_philips_running_status()->privatekey_url, &body, max_body_len, max_req_len);
	require_noerr(err, exit);

	MEMORY_DUMP;

	philips_User_Agent_generate(user_agent, sizeof(user_agent));

	req.req_len = snprintf(req.http_req, max_req_len, HTTP_COMMON_GET, get_philips_running_status()->privatekey_url, req.domain_name, user_agent);
	require_action(req.req_len != 0 && req.req_len < max_req_len, exit, err = kSizeErr);

	app_log("https request = [%d]\r\n%.*s", (int)req.req_len, (int)req.req_len, req.http_req);

	MEMORY_DUMP;

	// start https connection
	err = device_http_get_request(&req, https_response_data_process, REQUEST_MAX_TIME);

	MEMORY_DUMP;

	if ( err == kNoErr )
	{
	}
	else
	{
		app_log("download_privatekey error, enter block mode");

		if (get_philips_running_status()->is_first_config_wifi && !(get_philips_running_status()->softap_option & 0x08))
		{
			get_philips_running_status()->wifi_log_param.step = PHILIPS_WIFI_LOG_STEP_DOWNLOAD_PRIVATEKEY;
			get_philips_running_status()->wifi_log_param.result = false;
			get_philips_running_status()->wifi_log_param.err_code = req.err_code;
			get_philips_running_status()->wifi_log_param.err_type = req.err_type;
			get_philips_running_status()->wifi_log_param.err_line = req.err_line;

			if (err == kGeneralErr)
				strcpy(get_philips_running_status()->wifi_log_param.message, "none");
			else if (err == kParamErr)
				strcpy(get_philips_running_status()->wifi_log_param.message, "ETag get md5 fail");
			else if (err == kWriteErr)
				strcpy(get_philips_running_status()->wifi_log_param.message, "write flash fail");
			else if (err == kReadErr)
				strcpy(get_philips_running_status()->wifi_log_param.message, "read flash fail ");
			else if (err == kChecksumErr)
				strcpy(get_philips_running_status()->wifi_log_param.message, "md5 verify fail");
			else
				strcpy(get_philips_running_status()->wifi_log_param.message, "parse error");

			philips_wifi_log(get_philips_running_status()->philips_urls[get_philips_running_status()->flash_param.url_type].url_wifilog);
		}

		while(1)
		{
			mos_msleep(1000);
		}
	}
	require_noerr(err, exit);

	err = kNoErr;
	// exit the function
	exit:

	MEMORY_DUMP;

	device_http_param_deinit(&req, &body);

	MEMORY_DUMP;

	mos_thread_delete(NULL);
}

merr_t philips_device_download_privatekey(void)
{
	merr_t err = kNoErr;
	mos_thread_id_t thread = NULL;

	MEMORY_DUMP;

	thread = mos_thread_new(MOS_APPLICATION_PRIORITY, "download_privatekey", https_request_thread, 0x1000, NULL);
	require((thread != NULL), exit);

	mos_thread_join(thread);

	MEMORY_DUMP;

	exit:
	if ( err != kNoErr )
	{
		app_log("%s" " exit with err = %d", __FUNCTION__, err);
		while ( 1 )
		{
			app_log("%s" " - " "%s", __FUNCTION__, "some param is invalid");
			mos_msleep(1000);
		}
	}
	return err;
}
