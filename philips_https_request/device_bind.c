#include "mxos.h"
#include "http_short_connection.h"
#include "cJSON.h"
#include "mkv.h"

#include "device_https_request.h"
#include "philips_https_request.h"

#include "philips_param.h"
#include "philips_basic_api.h"

#include "philips_log.h"
#define app_log(M, ...)             			philips_custom_log("", M, ##__VA_ARGS__)

static char f_url[256] = {0};

static merr_t https_response_data_process(cJSON *json_data)
{
	merr_t err = kNoErr;

	// exit:
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

	HMACContext usercontext;
	char *body_text = NULL;
	int body_text_len = 0;
	uint8_t *hmacSha256 = NULL;
	char *signature = NULL;
	char *userid_encoded = NULL;
	int userid_encoded_len = 0;

	MEMORY_DUMP;

	err = device_http_param_init(&req, f_url, &body, max_body_len, max_req_len);
	require_noerr(err, exit);

	body_text = (char *)malloc(max_body_len);
	require_action(body_text != NULL, exit, err = kNoMemoryErr);
	memset(body_text, 0x00, max_body_len);
	hmacSha256 = (uint8_t *)malloc(SHA256HashSize);
	require_action(hmacSha256 != NULL, exit, err = kNoMemoryErr);
	memset(hmacSha256, 0x00, SHA256HashSize);
	signature = (char *)malloc((SHA256HashSize << 1) + 1);
	require_action(signature != NULL, exit, err = kNoMemoryErr);
	memset(signature, 0x00, (SHA256HashSize << 1) + 1);
	userid_encoded = (char *)malloc(sizeof(get_philips_running_status()->user_id));
	require_action(userid_encoded != NULL, exit, err = kNoMemoryErr);
	memset(userid_encoded, 0x00, sizeof(get_philips_running_status()->user_id));

	userid_encoded = php_url_encode(get_philips_running_status()->user_id, strnlen(get_philips_running_status()->user_id, sizeof(get_philips_running_status()->user_id)), &userid_encoded_len);
	require_action(body_text != NULL, exit, err = kNoMemoryErr);

	MEMORY_DUMP;

	req.req_len = snprintf(body, max_body_len, 
					"{\"app_id\":\"%s\",\"bind_token\":\"%s\",\"device_id\":\"%s\",\"push_type\":\"%s\",\"registration_id\":\"%s\",\"user_id\":\"%s\"}", 
					get_philips_running_status()->app_id, 
					get_philips_running_status()->bind_token, 
					get_philips_running_status()->flash_param.device_id, 
					get_philips_running_status()->push_type, 
					get_philips_running_status()->registration_id, 
					get_philips_running_status()->user_id);
	require_action(req.req_len != 0 && req.req_len < max_body_len, exit, err = kSizeErr);

	body_text_len = snprintf(body_text, max_body_len, 
					"app_id=%s&bind_token=%s&device_id=%s&push_type=%s&registration_id=%s&user_id=%s", 
					get_philips_running_status()->app_id, 
					get_philips_running_status()->bind_token, 
					get_philips_running_status()->flash_param.device_id, 
					get_philips_running_status()->push_type, 
					get_philips_running_status()->registration_id, 
					userid_encoded);
	app_log("body_text = [%d]%s", body_text_len, body_text);
	hmacReset(&usercontext, SHA256, (uint8_t *)get_philips_running_status()->app_secret, strlen(get_philips_running_status()->app_secret));
	hmacInput(&usercontext, (uint8_t *)body_text, body_text_len);
	hmacResult(&usercontext, hmacSha256);
	philips_hex2str_lowercase(signature, hmacSha256, SHA256HashSize);
	app_log("pre_signature = %s", signature);
	hmacReset(&usercontext, SHA256, (uint8_t *)get_philips_running_status()->user_id, strlen(get_philips_running_status()->user_id));
	hmacInput(&usercontext, (uint8_t *)signature, SHA256HashSize << 1);
	hmacResult(&usercontext, hmacSha256);
	philips_hex2str_lowercase(signature, hmacSha256, SHA256HashSize);
	app_log("signature = %s", signature);

	philips_User_Agent_generate(user_agent, sizeof(user_agent));

	req.req_len = snprintf(req.http_req, max_req_len, HTTP_COMMON_POST_SIGN, f_url, req.domain_name, user_agent, signature, strlen(body), body);
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
		app_log("bind error, don't enter block mode");

		if (get_philips_running_status()->is_first_config_wifi && !(get_philips_running_status()->softap_option & 0x08))
		{
			get_philips_running_status()->wifi_log_param.step = PHILIPS_WIFI_LOG_STEP_DEVICE_BIND;
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

merr_t philips_device_bind(char *url)
{
	merr_t err = kNoErr;
	mos_thread_id_t thread = NULL;

	MEMORY_DUMP;

	require_action(url != NULL, exit, err = kParamErr);
	strncpy(f_url, url, sizeof(f_url));

	thread = mos_thread_new(MOS_APPLICATION_PRIORITY, "device_bind", https_request_thread, 0x1000, NULL);
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
