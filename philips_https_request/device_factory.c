#include "mxos.h"

#include "device_https_request.h"
#include "philips_param.h"
#include "philips_device.h"

#include "philips_log.h"
#define app_log(M, ...)             			philips_custom_log("", M, ##__VA_ARGS__)

// device reset
#define PHILIPS_DEVICE_RESET_REQUEST_BODY_LENGTH								128
#define PHILIPS_DEVICE_RESET_REQUEST_LENGTH										512
#define PHILIPS_DEVICE_RESET_REQUEST_BODY										"{\"device_id\":\"%s\"}"

static char f_url[256] = {0};

static merr_t philips_device_reset_data_process(cJSON *root)
{
    merr_t err = kNoErr;
    
	return err;
}

void philips_device_factory_mode_reset_thread(void *arg)
{
    merr_t err = kGeneralErr;
	HTTP_REQ_S req = HTTP_REQ_INIT_PARAM;
	char *body = NULL;
    int max_body_len = PHILIPS_DEVICE_RESET_REQUEST_BODY_LENGTH;
    int max_req_len = PHILIPS_DEVICE_RESET_REQUEST_LENGTH;
    char user_agent[100] = {0};

    require_string( (get_philips_running_status()->flash_param.device_id[0] != '\0' && strlen(get_philips_running_status()->flash_param.device_id) > 0), exit, "factory: philips device_id is null, don't report cloud");

    MEMORY_DUMP;

    memset(f_url, 0, sizeof(f_url));
    strcpy(f_url, get_philips_running_status()->philips_urls[get_philips_running_status()->flash_param.url_type].url_reset);
    
    err = device_http_param_init(&req, f_url, &body, max_body_len, max_req_len);
    require_noerr(err, exit);

    MEMORY_DUMP;

	req.req_len = snprintf(body, PHILIPS_DEVICE_RESET_REQUEST_BODY_LENGTH, PHILIPS_DEVICE_RESET_REQUEST_BODY, get_philips_running_status()->flash_param.device_id);
	require_action(req.req_len != 0 && req.req_len < PHILIPS_DEVICE_RESET_REQUEST_BODY_LENGTH, exit, err = kSizeErr);

    philips_User_Agent_generate(user_agent, sizeof(user_agent));

	req.req_len = snprintf(req.http_req, PHILIPS_DEVICE_RESET_REQUEST_LENGTH, HTTP_COMMON_POST, f_url, req.domain_name, user_agent, strlen(body), body);
	require_action(req.req_len != 0 && req.req_len < PHILIPS_DEVICE_RESET_REQUEST_LENGTH, exit, err = kSizeErr);

	app_log("https request = [%d]\r\n%.*s", (int)req.req_len, (int)req.req_len, req.http_req);

    MEMORY_DUMP;

    err = device_http_post_request(&req, philips_device_reset_data_process, REQUEST_MIN_TIME);
    // require_noerr_action_string(err, exit, err = kGeneralErr, "philips_device_reset_data_process fail");     //产测结果上报无论成功还是失败，都要发送成功指令给设备。

    MEMORY_DUMP;

exit:
    app_log("start send factory result");
    // 产测结果上报，无论失败或者成功，发送给设备都是成功
    philips_send_PutProps_fac(FPPN_reset, 0);
    app_log("end send factory result");
    mos_msleep(500);
    philips_clear_flash_param_for_all();
    mos_msleep(500);
    philips_device_reset();
    // MicoSystemReboot();
	
    device_http_param_deinit(&req, &body);
    MEMORY_DUMP;

	mos_thread_delete(NULL);
}

merr_t philips_device_factory_mode_reset(void)
{
    merr_t err = kNoErr;
    mos_thread_id_t thread = NULL;

    thread = mos_thread_new(MOS_APPLICATION_PRIORITY, "factory reset", philips_device_factory_mode_reset_thread, 1024*4, NULL);
    require_action_string((thread != NULL), exit, err = kGeneralErr, "create factory reset thread fail");

exit:
    return err;
}
