#include "mxos.h"
#include "http_short_connection.h"
#include "cJSON.h"

#include "device_https_request.h"
#include "philips_https_request.h"

#include "philips_param.h"

#include "philips_log.h"
#define app_log(M, ...)             			philips_custom_log("", M, ##__VA_ARGS__)

static char f_url[256] = {0};

static merr_t https_response_data_process(cJSON *json_data)
{
	merr_t err = kNoErr;

	cJSON *json_temp = NULL;
	int url_len = 0;

	// ls certificate_url
	json_temp = NULL;
	json_temp = cJSON_GetObjectItem(json_data, "certificate_url");
	require_action(json_temp != NULL && cJSON_IsString(json_temp), exit, err = kResponseErr);
	url_len = strnlen(json_temp->valuestring, 512) + 1;
	get_philips_running_status()->certificate_url = realloc(get_philips_running_status()->certificate_url, url_len);
	require_action(get_philips_running_status()->certificate_url != NULL, exit, err = kNoMemoryErr);
	memset(get_philips_running_status()->certificate_url, 0x00, url_len);
	strncpy(get_philips_running_status()->certificate_url, json_temp->valuestring, url_len);

	// ls privatekey_url
	json_temp = NULL;
	json_temp = cJSON_GetObjectItem(json_data, "privatekey_url");
	require_action(json_temp != NULL && cJSON_IsString(json_temp), exit, err = kResponseErr);
	url_len = strnlen(json_temp->valuestring, 512) + 1;
	get_philips_running_status()->privatekey_url = realloc(get_philips_running_status()->privatekey_url, url_len);
	require_action(get_philips_running_status()->privatekey_url != NULL, exit, err = kNoMemoryErr);
	memset(get_philips_running_status()->privatekey_url, 0x00, url_len);
	strncpy(get_philips_running_status()->privatekey_url, json_temp->valuestring, url_len);

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

	err = device_http_param_init(&req, f_url, &body, max_body_len, max_req_len);
	require_noerr(err, exit);

	MEMORY_DUMP;

	req.req_len = snprintf(body, max_body_len, "{\"mac\":\"%s\",\"chipid\":\"%s\",\"sign\":\"%s\"}", get_philips_running_status()->mac_address, get_philips_running_status()->chipid, get_philips_running_status()->sign);
	require_action(req.req_len != 0 && req.req_len < max_body_len, exit, err = kSizeErr);

	philips_User_Agent_generate(user_agent, sizeof(user_agent));

	req.req_len = snprintf(req.http_req, max_req_len, HTTP_COMMON_POST, f_url, req.domain_name, user_agent, strlen(body), body);
	require_action(req.req_len != 0 && req.req_len < max_req_len, exit, err = kSizeErr);

	app_log("https request = [%d]\r\n%.*s", (int)req.req_len, (int)req.req_len, req.http_req);

	MEMORY_DUMP;

	// start https connection
	err = device_http_post_request(&req, https_response_data_process, REQUEST_MAX_TIME);

	MEMORY_DUMP;

	if ( err == kNoErr )
	{
	}
	else
	{
		app_log("certification error, enter block mode");

		if (get_philips_running_status()->is_first_config_wifi && !(get_philips_running_status()->softap_option & 0x08))
		{
			get_philips_running_status()->wifi_log_param.step = PHILIPS_WIFI_LOG_STEP_CERTIFICATION;
			get_philips_running_status()->wifi_log_param.result = false;
			get_philips_running_status()->wifi_log_param.err_code = req.err_code;
			get_philips_running_status()->wifi_log_param.err_type = req.err_type;
			get_philips_running_status()->wifi_log_param.err_line = req.err_line;

			if (err == kGeneralErr)
				strcpy(get_philips_running_status()->wifi_log_param.message, "none");
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

merr_t philips_device_certification(char *url)
{
	merr_t err = kNoErr;
	mos_thread_id_t thread = NULL;

	MEMORY_DUMP;

	require_action(url != NULL, exit, err = kParamErr);
	strncpy(f_url, url, sizeof(f_url));

	thread = mos_thread_new(MOS_APPLICATION_PRIORITY, "device_certification", https_request_thread, 0x1000, NULL);
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
