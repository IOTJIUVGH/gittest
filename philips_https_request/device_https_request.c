#include "mxos.h"

#include "device_https_request.h"
#include "philips_https_request.h"

#include "philips_param.h"

#include "philips_log.h"
#define app_log(M, ...)             			philips_custom_log("", M, ##__VA_ARGS__)

#define HTTP_HOST_LENGTH						128
#define HTTP_RESPONSE_LENGTH					2048

merr_t device_http_param_init(HTTP_REQ_S *req, char *url, char **body, int max_body_len, int max_req_len)
{
	merr_t err = kNoErr;

	int hoststartpos = 0;
	int hostendpos = 0;
	int fullurl_len = strlen(url);
	if ( strstr(url, "https") != NULL )
	{
		hoststartpos = 8;
		req->port = 443;
	}
	else if ( strstr(url, "http") != NULL )
	{
		hoststartpos = 7;
		req->port = 80;
	}

	char *p = strchr(url + hoststartpos, '/');
	hostendpos = fullurl_len - strlen(p);
	req->domain_name = (char *)malloc(HTTP_HOST_LENGTH);
	memset(req->domain_name, 0x00, HTTP_HOST_LENGTH);
	strncpy(req->domain_name, url + hoststartpos, hostendpos - hoststartpos);
	strcpy(url, p);

	// init https params
	req->timeout_ms = 10000;
	req->is_ssl = true;
	req->is_sni = true;

	req->http_res = (char *)malloc(HTTP_RESPONSE_LENGTH);
	require_action(req->http_res != NULL, exit, err = kNoMemoryErr);
	memset(req->http_res, 0x00, HTTP_RESPONSE_LENGTH);
	req->res_len = HTTP_RESPONSE_LENGTH;

	*body = (char *)malloc(max_body_len);
	require_action(*body != NULL, exit, err = kNoMemoryErr);
	memset(*body, 0x00, max_body_len);

	req->http_req = (char *)malloc(max_req_len);
	require_action(req->http_req != NULL, exit, err = kNoMemoryErr);
	memset(req->http_req, 0x00, max_req_len);

	req->http_res_head = (char *)malloc(1024);
	require_action(req->http_res_head != NULL, exit, err = kNoMemoryErr);
	memset(req->http_res_head, 0x00, 1024);
	req->res_head_len = 1024;

	exit:
	if ( err != kNoErr )
	{
		app_log("%s" " exit with err = %d", __FUNCTION__, err);
		device_http_param_deinit(req, body);
	}
	return err;
}

merr_t device_http_get_request(HTTP_REQ_S *req, http_direct_callback func, uint32_t retry)
{
	merr_t err = kNoErr;
	uint32_t connect_cnt = 0;

	SSL_REQUEST:
	MEMORY_DUMP;
	err = http_short_connection_ssl(req);
	MEMORY_DUMP;
	if(err != kNoErr || req->is_success != true)
	{
		if(++connect_cnt >= retry)
		{
			err = kGeneralErr;
			goto exit;
		}
		else
		{
			goto SSL_REQUEST;
		}
	}

	// response process
	app_log("https reponse code = %d\r\nhead[%d]-data[%d]\r\n%.*s%.*s", req->status_code, (int)req->real_res_head_len, (int)req->real_res_len, (int)req->real_res_head_len, req->http_res_head, (int)req->real_res_len, req->http_res);

	err = func(req->http_res, (int)req->real_res_len, req->http_res_head, (int)req->real_res_head_len);
	require_noerr(err, SSL_REQUEST);
exit:
	return err;
}

merr_t device_http_post_request(HTTP_REQ_S *req, http_json_callback func, uint32_t retry)
{
	merr_t err = kNoErr;
	uint32_t connect_cnt = 0;
	cJSON *json = NULL, *json_meta = NULL, *json_data = NULL, *json_temp = NULL;

	SSL_REQUEST:
	MEMORY_DUMP;
	err = http_short_connection_ssl(req);
	MEMORY_DUMP;
	if(err != kNoErr || req->is_success == false)
	{
		if(++connect_cnt >= retry)
		{
			err = kGeneralErr;
			goto exit;
		}
		else
		{
			goto SSL_REQUEST;
		}
	}

	// response process
	app_log("https reponse code = %d\r\nhead[%d]-data[%d]\r\n%.*s%.*s", req->status_code, (int)req->real_res_head_len, (int)req->real_res_len, (int)req->real_res_head_len, req->http_res_head, (int)req->real_res_len, req->http_res);

	// cd
	json = cJSON_Parse(req->http_res);
	require_action(json != NULL, exit, err = kExecutionStateErr);

	// cd meta
	json_meta = cJSON_GetObjectItem(json, "meta");
	require_action(json_meta != NULL && cJSON_IsObject(json_meta), exit, err = kResponseErr);

	// check code
	json_temp = NULL;
	json_temp = cJSON_GetObjectItem(json_meta, "code");
	require_action(json_temp != NULL && cJSON_IsNumber(json_temp), exit, err = kResponseErr);
	require_action(json_temp->valueint == 0, exit, err = kStateErr);

	// cd data
	json_data = cJSON_GetObjectItem(json, "data");
	require_action(json_data != NULL && cJSON_IsObject(json_data), exit, err = kResponseErr);

	err = func(json_data);

	exit:
	if ( json != NULL )
	{
		cJSON_Delete(json);
		json = NULL;
	}
	return err;
}

merr_t device_http_param_deinit(HTTP_REQ_S *req, char **body)
{
	if ( req->domain_name != NULL )
	{
		free(req->domain_name);
		req->domain_name = NULL;
	}
	if ( req->http_res != NULL )
	{
		free(req->http_res);
		req->http_res = NULL;
	}
	if ( *body != NULL )
	{
		free(*body);
		*body = NULL;
	}
	if ( req->http_req != NULL )
	{
		free(req->http_req);
		req->http_req = NULL;
	}
	if ( req->http_res_head != NULL )
	{
		free(req->http_res_head);
		req->http_res_head = NULL;
	}

	return kNoErr;
}

void philips_User_Agent_generate(char *data, int len)
{

	require( (data != NULL && len > 0), exit );

 	snprintf(data, len, PHILIPS_HTTP_USERAGENT, get_philips_running_status()->mac_address, FIRMWARE_REVISION, get_philips_running_status()->device_swversion);

exit:

	return ;
}