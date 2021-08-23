#ifndef __PHILIPS_HTTPS_REQUEST_H__
#define __PHILIPS_HTTPS_REQUEST_H__

#include "mxos.h"

merr_t philips_device_certification(char *url);
merr_t philips_device_activation(char *url);
merr_t philips_device_download_certificate(void);
merr_t philips_device_download_privatekey(void);
merr_t philips_device_bind(char *url);
merr_t philips_wifi_log(char *url);
merr_t kv_save_offlinelog(int step,int err_code,int err_type);

merr_t philips_device_factory_mode_reset(void);
merr_t philips_device_ota_check_request_and_download(void);
merr_t philips_start_download_file_with_url_md5(char *url, int urllen, char *md5, int md5len);
void philips_wifi_log_package(char **out);

#endif
