// #include "mxos.h"

#include "philips_device_ota.h"
#include "philips_param.h"
#include "philips_device.h"
#include "philips_mqtt_api.h"
#include "philips_https_request.h"

#include "philips_log.h"
#define app_log(M, ...)             			philips_custom_log("", M, ##__VA_ARGS__)

#define PHILIPS_OTA_CHECK_REQUEST_INTERVAL 7*24*60*60*1000          //一周
// #define PHILIPS_OTA_CHECK_REQUEST_INTERVAL 1*60*60*1000          //一小时
// #define PHILIPS_OTA_CHECK_REQUEST_INTERVAL 5*60*1000             //5分钟

void philips_ota_check_thread(void *arg)
{
    merr_t err = kNoErr;
    mxos_time_t last_ota_request_time = 0;
    mxos_time_t time_now = 0;

    while(1)
    {
        time_now = mos_time();

        if(time_now > last_ota_request_time + PHILIPS_OTA_CHECK_REQUEST_INTERVAL)
        {
            app_log("=========a weekly ota check=========");
            get_philips_running_status()->ota_process.ota_status = OTAST_CHECKING;

            philips_mqtt_client_close();

            err = philips_device_ota_check_request_and_download();
            if(err == kNoErr)
            {
                app_log("a weekly ota check and download success");
            }
            else
            {
                get_philips_running_status()->ota_process.ota_status = OTAST_NO_NEED;
                app_log("a weekly ota check and download fail");
                if(philips_mqtt_client_is_running() == false)
                {
                    app_log("======philips_mqtt_client_start======");
                    philips_mqtt_client_start();
                }
            }

            last_ota_request_time = time_now;
        }
        if(get_philips_running_status()->ota_process.ota_status == OTAST_REQUESTRECEIVED)
        {
            app_log("=========mqtt recvive ota request, start download=========");

            philips_mqtt_client_close();

            err = philips_start_download_file_with_url_md5(get_philips_running_status()->ota_process.url_value, strlen(get_philips_running_status()->ota_process.url_value), get_philips_running_status()->ota_process.md5_value, strlen(get_philips_running_status()->ota_process.md5_value));
            if(err == kNoErr)
            {
                app_log("mqtt direct push OTA download success");
            }
            else
            {
                get_philips_running_status()->ota_process.ota_status = OTAST_NO_NEED;
                app_log("mqtt direct push OTA downlaod fail");
                if(philips_mqtt_client_is_running() == false)
                {
                    app_log("======philips_mqtt_client_start======");
                    philips_mqtt_client_start();
                }
                MxosSystemReboot();
            }
        }
        mos_sleep(5);
    }
    mos_thread_delete(NULL);
}

merr_t philips_ota_check_start(void)
{
    merr_t err = kNoErr;

    mos_thread_id_t thread = NULL;
    
    thread = mos_thread_new(MOS_APPLICATION_PRIORITY, "OTA CHECK THREAD PROCESS", philips_ota_check_thread, 1024*4, NULL);
    require_action_string( (thread != NULL), exit, err = kGeneralErr, "create philips_ota_check_thread fail");

exit:
    return err;
}