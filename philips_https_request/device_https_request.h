#ifndef __DEVICE_HTTPS_REQUEST_H__
#define __DEVICE_HTTPS_REQUEST_H__

#include "mxos.h"
#include "mxos_config.h"
#include "http_short_connection.h"
#include "cJSON.h"
#include "philips_log.h"

#define MEMORY_DUMP								philips_custom_log("", "free_memory = %d", mos_mallinfo()->free)
// #define MEMORY_DUMP

#define REQUEST_MIN_TIME                        1
#define REQUEST_MAX_TIME						3

#define PHILIPS_HTTP_USERAGENT	                "MXCHIP-%s,%s,%s"

#define HTTP_COMMON_GET							"GET %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: %s\r\n\r\n"

#define HTTP_COMMON_POST						"POST %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: %s\r\nConnection: close\r\nContent-Length: %d\r\n\r\n%s\r\n\r\n"
#define HTTP_COMMON_POST_SIGN					"POST %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: %s\r\nsign: %s\r\nAccept: */*\r\nCache-Control: no-cache\r\nConnection: close\r\nContent-Length: %d\r\n\r\n%s\r\n\r\n"

typedef merr_t (*http_json_callback)(cJSON *);
typedef merr_t (*http_direct_callback)(char *, int, char *, int);

merr_t device_http_param_init(HTTP_REQ_S *req, char *url, char **body, int max_body_len, int max_req_len);
merr_t device_http_get_request(HTTP_REQ_S *req, http_direct_callback func, uint32_t retry);
merr_t device_http_post_request(HTTP_REQ_S *req, http_json_callback func, uint32_t retry);
merr_t device_http_param_deinit(HTTP_REQ_S *req, char **body);
void philips_User_Agent_generate(char *data, int len);

#endif
