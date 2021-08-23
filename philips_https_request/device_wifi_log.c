#include "mxos.h"
#include "cJSON.h"
#include "mkv.h"
#include "device_https_request.h"
#include "philips_param.h"

#include "philips_log.h"
#define app_log(M, ...)             			philips_custom_log("", M, ##__VA_ARGS__)

static char f_url[256] = {0};

int _rssi;
char _bssid[13]={0};
int _step;
int _err_code;
int _err_type;
typedef struct _ap_list_t
{
    char ssid[32];
    int8_t rssi;
} ap_list_t;
typedef struct _ap_scan_t
{
    uint8_t num;
    ap_list_t list[20];
} ap_scan_t;

static void wifi_scan_notify(int num, mwifi_ap_info_t *ap_list)
{
    uint8_t ret_success =0x00;
    uint8_t ret_fail =0x01;
    int i = 0;


	if (num > 0) {
		_rssi=ap_list[0].rssi;
		sprintf(_bssid, "%02X%02X%02X%02X%02X%02X",ap_list->bssid[0],ap_list->bssid[1],ap_list->bssid[2],ap_list->bssid[3],ap_list->bssid[4],ap_list->bssid[5]);


	if(get_philips_running_status()->offline_log_flash_param.step!=get_philips_running_status()->wifi_log_param.step)
		{
			get_philips_running_status()->offline_log_flash_param.step=_step;
			get_philips_running_status()->offline_log_flash_param.rssi=_rssi;
			strcpy(get_philips_running_status()->offline_log_flash_param.bssid,_bssid);
			get_philips_running_status()->offline_log_flash_param.free_memory=mos_mallinfo()->free;
			get_philips_running_status()->offline_log_flash_param.runtime=(double)mos_time();
			
			get_philips_running_status()->offline_log_flash_param.err_code=_err_code;
			get_philips_running_status()->offline_log_flash_param.err_type=_err_type;
			mkv_item_set(DATA_KV_WIFI_OFFLINE_LOG, &get_philips_running_status()->offline_log_flash_param, sizeof(get_philips_running_status()->offline_log_flash_param));
		}
	}
}

merr_t kv_save_offlinelog(int step,int err_code,int err_type)
{
	system_context_t * inContext=system_context();

	mxos_system_notify_register(mxos_notify_WIFI_SCAN_COMPLETED, wifi_scan_notify, NULL);
    mwifi_scan(inContext->flashContentInRam.mxos_config.ssid);

	_step=step;
	_err_code=err_code;
	_err_type=err_type;
}

static merr_t https_response_data_process(cJSON *json_data)
{
    merr_t err = kNoErr;

	// exit:
	return err;
}

void philips_wifi_log_package(char **out)
{
    cJSON *root = NULL;

	root = cJSON_CreateObject();
	require( (root != NULL) , exit);

	cJSON_AddNumberToObject(root, "Runtime", get_philips_running_status()->offline_log_flash_param.runtime);
	cJSON_AddNumberToObject(root, "rssi", get_philips_running_status()->offline_log_flash_param.rssi);
	cJSON_AddNumberToObject(root, "free_memory", get_philips_running_status()->offline_log_flash_param.free_memory);
	cJSON_AddStringToObject(root, "bssid", get_philips_running_status()->offline_log_flash_param.bssid);

	cJSON_AddNumberToObject(root, "step",get_philips_running_status()->offline_log_flash_param.step);

	if(get_philips_running_status()->wifi_log_param.result)
		cJSON_AddStringToObject(root, "status", "success");
	else
		cJSON_AddStringToObject(root, "status", "fail");
	
	cJSON_AddNumberToObject(root, "err_type", get_philips_running_status()->offline_log_flash_param.err_type);
	cJSON_AddNumberToObject(root, "err_code",get_philips_running_status()->offline_log_flash_param.err_code);
	cJSON_AddNumberToObject(root, "err_line", get_philips_running_status()->wifi_log_param.err_line);
	cJSON_AddStringToObject(root, "message", get_philips_running_status()->wifi_log_param.message);

	cJSON_AddStringToObject(root, "mac", get_philips_running_status()->mac_address);
	cJSON_AddStringToObject(root, "model_id", get_philips_running_status()->flash_param.device_modelid);
	cJSON_AddStringToObject(root, "wifi_version", FIRMWARE_REVISION);
	cJSON_AddStringToObject(root, "device_version", get_philips_running_status()->device_swversion);
	
	if(get_philips_running_status()->is_first_config_wifi)
	{
		cJSON_AddBoolToObject(root, "is_first_pair", get_philips_running_status()->is_first_config_wifi);
		cJSON_AddStringToObject(root, "user_id", get_philips_running_status()->user_id);
	}
	else
	{
		cJSON_AddBoolToObject(root, "is_first_pair", get_philips_running_status()->is_first_config_wifi);
	}
	
	if(get_philips_running_status()->flash_param.device_id[0] != '\0' && strlen(get_philips_running_status()->flash_param.device_id) > 0)
		cJSON_AddStringToObject(root, "device_id", get_philips_running_status()->flash_param.device_id);

	*out = cJSON_PrintUnformatted(root);

exit:
	if(root)
	{
		cJSON_Delete(root);
		root = NULL;
	}

    return ;
}

static void philips_wifi_log_thread(void *arg)
{
    merr_t err = kNoErr;
	HTTP_REQ_S req = HTTP_REQ_INIT_PARAM;
	char *body = NULL;
	int max_body_len = 512;
	int max_req_len = 1024;
	char user_agent[100] = {0};

	char *generate_body = NULL;

	MEMORY_DUMP;

    err = device_http_param_init(&req, f_url, &body, max_body_len, max_req_len);
	require_noerr(err, exit);

    MEMORY_DUMP;

	philips_wifi_log_package(&generate_body);
	require_action( (generate_body != NULL), exit, err = kParamErr );

	strncpy(body, generate_body, max_body_len);
	req.req_len = strlen(body);
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
		app_log("wifi log error, don't enter block mode");
	}
	require_noerr(err, exit);

	err = kNoErr;
exit:
	device_http_param_deinit(&req, &body);

	if(generate_body)
	{
		free(generate_body);
		generate_body = NULL;
	}

    MEMORY_DUMP;

	mos_thread_delete(NULL);
}

merr_t philips_wifi_log(char *url)
{
    merr_t err = kNoErr;
	mos_thread_id_t thread = NULL;

	MEMORY_DUMP;

	require_action(url != NULL, exit, err = kParamErr);
	strncpy(f_url, url, sizeof(f_url));

	thread = mos_thread_new(MOS_APPLICATION_PRIORITY, "wifi_log", philips_wifi_log_thread, 1024*4, NULL);
	require((thread != NULL), exit);

	mos_thread_join(thread);

	MEMORY_DUMP;

	exit:
	if ( err != kNoErr )
	{
		app_log("%s" " exit with err = %d", __FUNCTION__, err);
	}
	return err;
}