/**
 ******************************************************************************
 * @file    main.c
 * @author  
 * @version V1.0.0
 * @date    
 * @brief   
 ******************************************************************************
 *
 *  The MIT License
 *  Copyright (c) 2016 MXCHIP Inc.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is furnished
 *  to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 *  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 ******************************************************************************
 */

#include "mxos.h"
#include "mxos_config.h"
#include "mkv.h"

#include "philips_uart.h"
#include "philips_param.h"
#include "philips_history_param_adapt.h"
#include "philips_device.h"
#include "philips_device_model.h"
#include "philips_wifi.h"
#include "philips_https_request.h"
#include "philips_coap.h"
#include "philips_mqtt_api.h"
#include "philips_log.h"
#include "philips_cli.h"
#include "philips_device_ota.h"

#define app_log(M, ...)             philips_custom_log("", M, ##__VA_ARGS__)

static mos_semphr_id_t wait_sem = NULL;

/*******************************************************************************
 * security options
 *******************************************************************************/
#ifdef PHILIPS_SECURITY
void set_flash_security(void)
{
	mhal_gpio_open(MODULE_PIN_1, OUTPUT_PUSH_PULL);
	mhal_gpio_low(MODULE_PIN_1);
	mhal_gpio_open(MODULE_PIN_2, OUTPUT_PUSH_PULL);
	mhal_gpio_low(MODULE_PIN_2);
}
#endif

/*******************************************************************************
 * wifi status
 *******************************************************************************/
static void mxosNotify_WifiStatusHandler(WiFiEvent status, void* const inContext)
{
	static uint8_t reconnect_flag=0;

	switch ( status )
	{
		case NOTIFY_STATION_UP:
			app_log("%s", "Station up");
			if ( wait_sem != NULL )
			{
				mos_semphr_release(wait_sem);
			}
			if(get_philips_running_status()->factory_ing == false)
			{
				get_philips_running_status()->wifi_status = WRS_wifiui_connect_router;
			}

			if((reconnect_flag==0)&&(PHILIPS_WIFI_LOG_STEP_WIFIDISCONNECT==get_philips_running_status()->offline_log_flash_param.step)&&((-30)==get_philips_running_status()->offline_log_flash_param.err_code)||(0xff==get_philips_running_status()->offline_log_flash_param.err_type))
			{
				reconnect_flag=1;
				kv_item_delete(DATA_KV_WIFI_OFFLINE_LOG);
				// philips_wifi_log(get_philips_running_status()->philips_urls[get_philips_running_status()->flash_param.url_type].url_wifilog);
			}
			break;
		case NOTIFY_STATION_DOWN:
			app_log("%s", "Station down");
			if(get_philips_running_status()->factory_ing == false)
			{
				get_philips_running_status()->wifi_status = WRS_wifiui_connect_ing;
			}

			get_philips_running_status()->wifi_log_param.step=PHILIPS_WIFI_LOG_STEP_WIFIDISCONNECT;
			get_philips_running_status()->wifi_log_param.err_code=-30;
			get_philips_running_status()->wifi_log_param.err_type=0xff;
			get_philips_running_status()->wifi_log_param.err_line = __LINE__;

			kv_save_offlinelog(get_philips_running_status()->offline_log_flash_param.step,get_philips_running_status()->offline_log_flash_param.err_code,get_philips_running_status()->offline_log_flash_param.err_type);
			
			break;
		case NOTIFY_AP_UP:
		case NOTIFY_AP_DOWN:
			break;
	}
}

static void mxosNotify_ConnectFailedHandler(merr_t err, system_context_t * const inContext)
{
	app_log("%s" " exit with err = %d", __FUNCTION__, err);

	get_philips_running_status()->wifi_status = WRS_wifiui_connect_ing;

	if(get_philips_running_status()->factory_mode == FPPN_idle)
	{
		// 低功耗
		philips_check_power_save();
	}
}

#ifndef PHILIPS_SECURITY
void philips_ssl_log(const int logLevel, const char *const logMessage)
{
	if(get_philips_running_status()->ssl_log_switch)
	{
		app_log("%s", logMessage);
	}
}
#endif

int main( void )
{
  	merr_t err = kNoErr;

#ifdef PHILIPS_SECURITY
	set_flash_security();
#endif

	/* Start MiCO system functions according to mxos_config.h */
	err = mxos_system_init( );
    require_noerr( err, exit );

	app_log("====== ====== DEFINE ====== ======");
	app_log("APP_INFO                           %s", APP_INFO);
	app_log("MICO_SN                            %s", SERIAL_NUMBER);
	app_log("FIRMWARE_REVISION                  %s", FIRMWARE_REVISION);
#if defined ENVIRONMENT_STAGING
	app_log("ENVIRONMENT: STAGING");
#elif defined ENVIRONMENT_PRODUCTION
	app_log("ENVIRONMENT: PRODUCTION");
#endif
	app_log("====== ====== ====== ====== ======");

	/* param init */
	wait_sem = mos_semphr_new(1);
	require((wait_sem != NULL), exit);

	err = mxos_system_notify_register(mxos_notify_WIFI_STATUS_CHANGED, (void *)mxosNotify_WifiStatusHandler, NULL);
	require_noerr(err, exit);
	err = mxos_system_notify_register(mxos_notify_WIFI_CONNECT_FAILED, (void *)mxosNotify_ConnectFailedHandler, NULL);
	require_noerr(err, exit);

#ifndef PHILIPS_SECURITY
	mtls_set_loggingcb(philips_ssl_log);
#endif
	
	philips_history_param_adapt_process();

	philips_param_init();

#ifndef PHILIPS_SECURITY
	philips_cli_init();
#endif

	philips_uart_start();

	philips_wait_for_all_initialized();

	philips_wifi_config();

	mos_semphr_acquire(wait_sem, MOS_WAIT_FOREVER);

	// 二次OTA检测标志位
	bool ota_second_check_flag = false;

	if ( !get_philips_running_status()->is_first_config_wifi )
	{
		if( philips_product_id_param_check() )
		{
			get_philips_running_status()->ota_process.ota_status = OTAST_CHECKING;

			err = philips_device_ota_check_request_and_download();
			if(err == kNoErr)
			{

			}
		}
		else
		{
			app_log("======product_id is error. after param active success, second ota will check======");
			ota_second_check_flag = true;
		}
	}
	get_philips_running_status()->ota_process.ota_status = OTAST_NO_NEED;

	if(get_philips_running_status()->need_report_offlinelog==0)
	{
		app_log("wifi offline log report");
		kv_item_delete(DATA_KV_WIFI_OFFLINE_LOG);
		// philips_wifi_log(get_philips_running_status()->philips_urls[get_philips_running_status()->flash_param.url_type].url_wifilog);
	}

	if ( get_philips_running_status()->need_https )
	{
		err = philips_device_certification(get_philips_running_status()->philips_urls[get_philips_running_status()->flash_param.url_type].url_certification);
		require_noerr(err, exit);

		err = philips_device_activation(get_philips_running_status()->philips_urls[get_philips_running_status()->flash_param.url_type].url_activation);
		require_noerr(err, exit);

		philips_check_product_range();
	}

	if ( get_philips_running_status()->is_first_config_wifi )
	{
		philips_start_udp();
		philips_device_bind(get_philips_running_status()->philips_urls[get_philips_running_status()->flash_param.url_type].url_bind);
	}

	philips_coap_server_start();

	if ( get_philips_running_status()->need_https )
	{
		err = philips_device_download_certificate();
		require_noerr(err, exit);

		err = philips_device_download_privatekey();
		require_noerr(err, exit);

		if(get_philips_running_status()->flash_param.switch_root_certificate_finish == false)	//root根证书切换完成，保存标志位
		{
			get_philips_running_status()->flash_param.switch_root_certificate_finish = true;
			mkv_item_set("src_finish", &get_philips_running_status()->flash_param.switch_root_certificate_finish, sizeof(get_philips_running_status()->flash_param.switch_root_certificate_finish));
		}
	}

	// 第一次OTA检测由于product_id问题造成的OTA失败，所有的参数激活后，再次进行OTA检测。
	if ( ota_second_check_flag )
	{
		ota_second_check_flag = false;
		if( philips_product_id_param_check() )
		{
			get_philips_running_status()->ota_process.ota_status = OTAST_CHECKING;

			err = philips_device_ota_check_request_and_download();
			if(err == kNoErr)
			{

			}
		}
		else
		{
			app_log("======product_id is error, second ota check fail======");
		}
	}
	get_philips_running_status()->ota_process.ota_status = OTAST_NO_NEED;

	// WiFi模组上电OTA检测后，循环OTA检测不需要检测MCU的OTA信息。
	get_philips_running_status()->ota_process.need_check_otau = false;

	philips_mqtt_client_start();

	philips_ota_check_start();

exit:
	if ( err != kNoErr )
	{
		app_log("%s" " exit with err = %d", __FUNCTION__, err);
	}
	mos_thread_delete(NULL);
	return 0;
}


