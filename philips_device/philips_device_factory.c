// #include "mxos.h"

#include "philips_device_factory.h"
#include "philips_device.h"
#include "philips_param.h"

#include "philips_log.h"
#define app_log(M, ...)             			philips_custom_log("", M, ##__VA_ARGS__)

#define PHILIPS_FACTORY_SSID                    "Comfort_test"
#define PHILIPS_FACTORY_PASSWD                  ""
#define PHILIPS_FACTORY_CONNECT_TIMMEOUT        30000 //ms

static mos_semphr_id_t factory_con_sem = NULL;

static void philips_FactoryNotify_WifiStatusHandler( WiFiEvent status, void *inContext )
{
    switch ( status )
    {
        case NOTIFY_STATION_UP: 
            if(factory_con_sem)
                mos_semphr_release(factory_con_sem);
            break;
        case NOTIFY_STATION_DOWN:
            case NOTIFY_AP_UP:
            case NOTIFY_AP_DOWN:
            break;
    }
}

void philips_device_factory_mode_wifi_thread(void *arg)
{
    merr_t err = kUnknownErr;

    factory_con_sem = mos_semphr_new(1);
    require( (factory_con_sem != NULL), exit );

    err = mxos_system_notify_register( mxos_notify_WIFI_STATUS_CHANGED, (void *) philips_FactoryNotify_WifiStatusHandler,  NULL );
    require_noerr( err, exit );
 
    mxos_system_notify_remove_all(mxos_notify_WiFI_PARA_CHANGED);       //防止产测路由器连接成功，更改flash中路由器参数信息

    err = mwifi_connect(PHILIPS_FACTORY_SSID, PHILIPS_FACTORY_PASSWD, strlen(PHILIPS_FACTORY_PASSWD), NULL, NULL);

    app_log("connect to %s - %s.....", PHILIPS_FACTORY_SSID, PHILIPS_FACTORY_PASSWD);

    err = mos_semphr_acquire(factory_con_sem, PHILIPS_FACTORY_CONNECT_TIMMEOUT);
    if(err == kNoErr)   //连接成功
    {
        app_log("connect factory router success");
        philips_send_PutProps_fac(FPPN_wifi, 0);
    }
    else    //连接失败
    {
        app_log("connect factory router %d timeout", PHILIPS_FACTORY_CONNECT_TIMMEOUT);
        philips_send_PutProps_fac(FPPN_wifi, 1);
    }


exit:
    mxos_system_notify_remove(mxos_notify_WIFI_STATUS_CHANGED, (void *)philips_FactoryNotify_WifiStatusHandler);
    if(factory_con_sem)
    {
        mos_semphr_delete(factory_con_sem);
        factory_con_sem = NULL;
    }
    mos_thread_delete(NULL);
}

merr_t philips_device_factory_mode_wifi(void)
{
    merr_t err = kNoErr;

    mos_thread_id_t factory_wifi_thread_handle = NULL;

    factory_wifi_thread_handle = mos_thread_new(MOS_APPLICATION_PRIORITY, "factory wifi", philips_device_factory_mode_wifi_thread, 1024*2, NULL);
    require_action_string((factory_wifi_thread_handle != NULL), exit, err = kGeneralErr, "create factory wifi thread fail");

exit:
    return err;
}