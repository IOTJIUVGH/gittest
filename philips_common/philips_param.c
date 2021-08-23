#include "mxos.h"
#include "mxos_config.h"
#include "mkv.h"

#include "philips_param.h"
#include "philips_basic_api.h"

#include "philips_uart.h"
// #include "philips_https_request.h"
#include "philips_device_model.h"
#include "philips_mqtt_api.h"

#include "philips_log.h"
#define app_log(format, ...)					philips_custom_log("", format, ##__VA_ARGS__)

#define MAX_QUEUE_NUM							8

#if defined ENVIRONMENT_STAGING
const struct _philips_urls philips_urls[PHILIPS_URL_TYPE_AMOUNT] =
	{
		// PHILIPS_URL_TYPE_NONE
		{
			"",		// url_certification
			"",		// url_activation
			"",		// url_ota
			"",		// url_bind
			"",		// url_reset
			"", 	// url_wifilog
		},
		// PHILIPS_URL_TYPE_CHINA
		{
			"https://60wfrhgis5.execute-api.cn-north-1.amazonaws.com.cn/staging/certification",		// url_certification
			"https://60wfrhgis5.execute-api.cn-north-1.amazonaws.com.cn/staging/activation",		// url_activation
			"https://air_api.fogcloud.io/ota/updatecheck/",		// url_ota
			"https://air_api.fogcloud.io/device/bindUser/",		// url_bind
			"https://air_api.fogcloud.io/device/deviceReset/",		// url_reset
			"https://air_api.fogcloud.io/device/deviceLog/",		// url_wifilog
		},
		// PHILIPS_URL_TYPE_FRANKFORD
		{
			"https://2afww42eoe.execute-api.eu-central-1.amazonaws.com/staging-fra/certification",		// url_certification
			"https://2afww42eoe.execute-api.eu-central-1.amazonaws.com/staging-fra/activation",		// url_activation
			"https://air_api_fr.fogcloud.io/ota/updatecheck/",		// url_ota
			"https://air_api_fr.fogcloud.io/device/bindUser/",		// url_bind
			"https://air_api_fr.fogcloud.io/device/deviceReset/",		// url_reset
			"https://air_api_fr.fogcloud.io/device/deviceLog/",			// url_wifilog
		},
	};

#elif defined ENVIRONMENT_PRODUCTION
const struct _philips_urls philips_urls[PHILIPS_URL_TYPE_AMOUNT] =
	{
		// PHILIPS_URL_TYPE_NONE
		{
			"",		// url_certification
			"",		// url_activation
			"",		// url_ota
			"",		// url_bind
			"",		// url_reset
			"",		// url_wifilog
		},
		// PHILIPS_URL_TYPE_CHINA
		{
			"https://2vjv1xw6ef.execute-api.cn-north-1.amazonaws.com.cn/production/certification",		// url_certification
			"https://2vjv1xw6ef.execute-api.cn-north-1.amazonaws.com.cn/production/activation",		// url_activation
			"https://www.api.air.philips.com.cn/ota/updatecheck/",		// url_ota
			"https://www.api.air.philips.com.cn/device/bindUser/",		// url_bind
			"https://www.api.air.philips.com.cn/device/deviceReset/",		// url_reset
			"https://www.api.air.philips.com.cn/device/deviceLog/",			// url_wifilog
		},
		// PHILIPS_URL_TYPE_FRANKFORD
		{
			"https://3botg66r11.execute-api.eu-central-1.amazonaws.com/production-fra/certification",		// url_certification
			"https://3botg66r11.execute-api.eu-central-1.amazonaws.com/production-fra/activation",		// url_activation
			"https://www.api.air.philips.com/ota/updatecheck/",		// url_ota
			"https://www.api.air.philips.com/device/bindUser/",		// url_bind
			"https://www.api.air.philips.com/device/deviceReset/",		// url_reset
			"https://www.api.air.philips.com/device/deviceLog/",		// url_wifilog
		},
	};

#else
const struct _philips_urls philips_urls[PHILIPS_URL_TYPE_AMOUNT] =
	{
		// PHILIPS_URL_TYPE_NONE
		{
			"",		// geturl_host
			"",		// geturl_certification
			"",		// geturl_activation
			"",		// otaurl
			"",
			"",
		},
		// PHILIPS_URL_TYPE_CHINA ENVIRONMENT_TESTING
		{
			"",		// geturl_host
			"",		// geturl_certification
			"",		// geturl_activation
			"",		// otaurl
			"",
			"",
		},
		// PHILIPS_URL_TYPE_FRANKFORD ENVIRONMENT_STAGING
		{
			"",		// geturl_host
			"",		// geturl_certification
			"",		// geturl_activation
			"",		// otaurl
			"",
			"",
		},
	};

#endif

static struct _philips_wifi_running_status f_philips_wifi_running_status = 
{
	// wifi attribution
	.mac_address = {0},

	// wifi running status
	.wifi_status = WRS_unknow,
	.need_https = false,
	.range_id = PHILIPS_RANGE_NONE,
	.is_support_otau = false,

	// philips url
	.philips_urls = philips_urls,

	// fogid - MICO_PARTITION_SDS - 0: {chipid,sign}
	.chipid = {0},
	.sign = {0},

	// certificate & privatekey - MICO_PARTITION_USER - CERTIFICATE_OFFSET: {certificate}, PRIVATEKEY_OFFSET: {privatekey}
	.certificate = {0},
	.privatekey = {0},

	// device attribution
	.need_get_deviceinfo = false,
	.device_category = {0},
	.device_name = {0},
	.device_type = {0},
	.device_swversion = "0.0.0",
	.device_language = {0},
	.wifi_protocol_ver=0,
	.OTAU_protocol_ver=0,
	.node1bootloaderver={0},
	.node1applicationver={0},
	

	// wifi configuration
	.is_first_config_wifi = false,
	.sta_client_amount = 0,
	.softap_option = 0,
	.extra_data = {0},
	.didt_key = NULL,
	.didt_iv = NULL,
	.bind_token = {0},
	.registration_id = {0},
	.push_type = {0},
	.app_id = {0},
	.app_secret = {0},
	.user_id = {0},
	.is_get_ssid = false,
	.https_client_res = kNoErr,
	.https_client_error_message = NULL,
	.need_report_offlinelog=-1,

	// https request
	.certificate_url = NULL,
	.privatekey_url = NULL,

	// is reconnect cloud
	.is_reconnect_cloud = false,
	// is_rf_power_save
	.is_rf_power_save = false,

	// upload manage
	.device_upload_manage = 
	{
		// device status change
		.status_change = false,
		// device sensor change
		.sensor_change = false,
		// app control upload
		.app_control_upload = false,
		// app get status
		.app_get_status =false,
		.error = 0,
		.last_error = 0,
		.warn = 0,
		.last_warn = 0,
		.last_iaql = 0,	
	},

	// wifi log
	.wifi_log_param = 
	{
		.step = 0,
		.result = false,
		.err_type = 0,
		.err_code = 0,
		.err_line = 0,
		.message = {"none"},
	},

	// ota process
	.ota_process = {
		.ota_type = OTATYPE_UNKNOW,
		.need_check_otau = true,
		.ota_status = OTAST_NO_NEED,
		.url_value = {0},
		.md5_value = {0},
		.type_value = {0},
		.version_value = {0},
	},

	// otau run state
	.otau_run_state = 
	{
		.is_send_idle_before_set_device_otau_sem = false,
		.need_get_otau = false,
		.is_otau_ing = false,
		.local_device_version = {0},
		.device_otau_last_state = 0xFF,
		.otau_state_id_control = 0,
		.otau_current_package = -1,
		.otau_total_package = -1,
		.otau_data_leave = 0,
		.otau_data_index = 0,
	},
	
	// factory_mode
	.factory_ing = false,
	.factory_mode = 0x00,

	// debug_o2_filter
	.enable_o2_filter_debug = false,
	.last_upload_filter0_life = 0,
	.last_upload_filter1_life = 0,

	// watchdog
	.uart_feed_dog_time = 0,
	.mqtt_feed_dog_time = 0,
	.mqtt_monitor_start = false,

	// log on/off
	.uart_log_switch = 0,
	.ssl_log_switch = 0,

	// kv in flash
	.flash_param = 
	{
		// wifi reboot reason
		.wifi_reboot_status = WRBT_unknow,

		// flash param old ver change new ver
		.param_old_convert_new_finish = false,

		// parameter1 convert finish
		.parameter1_convert_kv_finish = false,

		// switch root certificate
		.switch_root_certificate_finish = false,

		.muc_boot =true,
		.philips_uart_protocol_is_common = false,

		// config wifi
		.passwd = {0},

		// device attribution
		.url_type = 0xFF,
		.device_modelid = {0},
		.aqit = 4,

		// device attribution from https request
		.product_range = {0},
		.product_id = {0},
		.device_id = {0},

		// mqtt infomation from https request
		.mqtt_host = {0},
		.mqtt_port = 0,
		.certificate_len = 0,
		.privatekey_len = 0,
	},

	.offline_log_flash_param={
		.bssid={0},
		.runtime=0,
	},

	.otau_flash_param = {
		.magic_word = {0},

		.cloud_device_version = {0},
		.need_ota_device = false,
		.is_otau_file_downloaded_from_cloud = false,
		.otau_data_len = 0,
	},

	.o2_filter_flash_param = {
		.magic_word = {0},
		
		// device attribution for O2
		.need_transform_filter_life = false,
		.filter0_life = 0,
		.filter0_lastadjustvalue = 0,
		.filter1_life = 0,
		.filter1_lastadjustvalue = 0,
		.ms_count = 0,
	},
};
/**
 * *****************************************************************************
 * WiFi模组相关的静态函数
 * *****************************************************************************
 **/
static int user_get_device_mac(char *mac_buff, uint32_t mac_buff_len)
{
	int err = kNoErr;
	uint8_t mac[10] = {0};

	require((mac_buff != NULL) && (mac_buff_len >= 16), exit);

	mwifi_get_mac(mac);

	memset(mac_buff, 0x00, mac_buff_len);
	sprintf(mac_buff, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	exit:
	return err;
}

/**
 * *****************************************************************************
 * 获取FogID
 * *****************************************************************************
 **/
bool get_fogid_from_sds_partition(void)
{
	merr_t err = kNoErr;
	uint32_t read_offset = 0;
	char binbuf[128] = {0};

	err = mhal_flash_read(MODULE_PARTITION_SDS, &read_offset, (uint8_t *)binbuf, 121);

	if ( err != kNoErr )
	{
		return false;
	}
	if ( binbuf[24] != ',' )
	{
		return false;
	}
	for ( int i = 0; i < 121; ++i )
	{
		if ( i == 24 )
		{
			continue;
		}
		if ( !philips_is_base64_char(binbuf[i]) )
		{
			return false;
		}
	}

	memset(f_philips_wifi_running_status.chipid, 0x00, sizeof(f_philips_wifi_running_status.chipid) + sizeof(f_philips_wifi_running_status.sign));

	memcpy(f_philips_wifi_running_status.chipid, binbuf, 24);
	memcpy(f_philips_wifi_running_status.sign, binbuf + 25, 96);

	return true;
}

/**
 * *****************************************************************************
 * 额外的QC打印
 * *****************************************************************************
 **/
void user_qc_output(qc_print_cb_t qc_print)
{
	char buffer[512];
	
	memset(buffer, 0, 512);

	bool havegotfogid = get_fogid_from_sds_partition();
	if ( havegotfogid )
	{
		snprintf(buffer, 512, "ID LIST: \r\n  FOGID: %s%s\r\n  real: %s,%s\r\n", f_philips_wifi_running_status.chipid, f_philips_wifi_running_status.sign, f_philips_wifi_running_status.chipid, f_philips_wifi_running_status.sign);
	}
	else
	{
		snprintf(buffer, 512, "ID LIST: \r\n  FOGID: \r\n");
	}

	qc_print(buffer);

	return ;
}

/**
 * *****************************************************************************
 * running status
 * *****************************************************************************
 **/
merr_t philips_param_init(void)
{
	merr_t err = kNoErr;
	int len = 0;
	uint32_t offset = 0;

	err = philips_mqtt_param_init();
	if ( err != kNoErr )
	{
		while ( 1 )
		{
			app_log("mqtt_send_msg_queue" "init failed");
			mos_msleep(1000);
		}
	}

	err = philips_uart_param_init();
	if ( err != kNoErr )
	{
		while ( 1 )
		{
			app_log("uart_send_msg_queue" "init failed");
			mos_msleep(1000);
		}
	}
	
	user_get_device_mac(f_philips_wifi_running_status.mac_address, sizeof(f_philips_wifi_running_status.mac_address));

	get_fogid_from_sds_partition();

#ifndef PHILIPS_SECURITY
	len = sizeof(f_philips_wifi_running_status.uart_log_switch);
	mkv_item_get("LOGSW_uart", &f_philips_wifi_running_status.uart_log_switch, &len);
	len = sizeof(f_philips_wifi_running_status.ssl_log_switch);
	mkv_item_get("LOGSW_ssl", &f_philips_wifi_running_status.ssl_log_switch, &len);
#endif

	len = sizeof(f_philips_wifi_running_status.flash_param.switch_root_certificate_finish);
	mkv_item_get("src_finish", &f_philips_wifi_running_status.flash_param.switch_root_certificate_finish, &len);

	len = sizeof(f_philips_wifi_running_status.flash_param.wifi_reboot_status);
	mkv_item_get("pwrr_status", &f_philips_wifi_running_status.flash_param.wifi_reboot_status, &len);

	len = sizeof(f_philips_wifi_running_status.flash_param.passwd);
	mkv_item_get("pcw_passwd", f_philips_wifi_running_status.flash_param.passwd, &len);

	len = sizeof(f_philips_wifi_running_status.flash_param.url_type);
	mkv_item_get("pda_urltype", &f_philips_wifi_running_status.flash_param.url_type, &len);
	len = sizeof(f_philips_wifi_running_status.flash_param.device_modelid);
	mkv_item_get("pda_mid", f_philips_wifi_running_status.flash_param.device_modelid, &len);
	len = sizeof(f_philips_wifi_running_status.flash_param.aqit);
	err = mkv_item_get("pda_aqit", &f_philips_wifi_running_status.flash_param.aqit, &len);
	if(err != 0)	//新设备，没有保存aqit
	{
		f_philips_wifi_running_status.flash_param.aqit = 4;
		mkv_item_set("pda_aqit", &f_philips_wifi_running_status.flash_param.aqit, sizeof(f_philips_wifi_running_status.flash_param.aqit));
		len = sizeof(f_philips_wifi_running_status.flash_param.aqit);
		mkv_item_get("pda_aqit", &f_philips_wifi_running_status.flash_param.aqit, &len);
	}

	len = sizeof(f_philips_wifi_running_status.flash_param.product_range);
	mkv_item_get("pdafhr_range", f_philips_wifi_running_status.flash_param.product_range, &len);
	len = sizeof(f_philips_wifi_running_status.flash_param.product_id);
	mkv_item_get("pdafhr_pid", f_philips_wifi_running_status.flash_param.product_id, &len);
	len = sizeof(f_philips_wifi_running_status.flash_param.device_id);
	mkv_item_get("pdafhr_did", f_philips_wifi_running_status.flash_param.device_id, &len);

	len = sizeof(f_philips_wifi_running_status.flash_param.mqtt_host);
	mkv_item_get("pmifhr_host", f_philips_wifi_running_status.flash_param.mqtt_host, &len);
	len = sizeof(f_philips_wifi_running_status.flash_param.mqtt_port);
	mkv_item_get("pmifhr_port", &f_philips_wifi_running_status.flash_param.mqtt_port, &len);
	len = sizeof(f_philips_wifi_running_status.flash_param.certificate_len);
	mkv_item_get("pmifhr_clen", &f_philips_wifi_running_status.flash_param.certificate_len, &len);
	len = sizeof(f_philips_wifi_running_status.flash_param.privatekey_len);
	mkv_item_get("pmifhr_plen", &f_philips_wifi_running_status.flash_param.privatekey_len, &len);
	len = sizeof(f_philips_wifi_running_status.flash_param.philips_uart_protocol_is_common);
	mkv_item_get("uart_protocol_is_common", &f_philips_wifi_running_status.flash_param.philips_uart_protocol_is_common, &len);

	len = sizeof(f_philips_wifi_running_status.o2_filter_flash_param);
	mkv_item_get(CONFIG_DATA_KV_KEY_O2_FILTER, &f_philips_wifi_running_status.o2_filter_flash_param, &len);

	len = sizeof(f_philips_wifi_running_status.otau_flash_param);
	mkv_item_get(CONFIG_DATA_KV_KEY_OTAU, &f_philips_wifi_running_status.otau_flash_param, &len);

	len = sizeof(f_philips_wifi_running_status.offline_log_flash_param);
	f_philips_wifi_running_status.need_report_offlinelog=mkv_item_get(DATA_KV_WIFI_OFFLINE_LOG, &f_philips_wifi_running_status.offline_log_flash_param, &len);

	len = sizeof(f_philips_wifi_running_status.flash_param.muc_boot);
	f_philips_wifi_running_status.flash_param.muc_boot=mkv_item_get(DATA_KV_MCU_BOOT, &f_philips_wifi_running_status.flash_param.muc_boot, &len);

	offset = CERTIFICATE_OFFSET;
	err = mhal_flash_read(MODULE_PARTITION_USER, &offset, (uint8_t *)(f_philips_wifi_running_status.certificate), f_philips_wifi_running_status.flash_param.certificate_len);
	offset = PRIVATEKEY_OFFSET;
	err = mhal_flash_read(MODULE_PARTITION_USER, &offset, (uint8_t *)(f_philips_wifi_running_status.privatekey), f_philips_wifi_running_status.flash_param.privatekey_len);

	if ( (strnlen(f_philips_wifi_running_status.flash_param.product_range, sizeof(f_philips_wifi_running_status.flash_param.product_range)) == 0) || 
		 (philips_check_product_range() != kNoErr) || 
		 (strnlen(f_philips_wifi_running_status.flash_param.product_id, sizeof(f_philips_wifi_running_status.flash_param.product_id)) == 0) || 
		 (strnlen(f_philips_wifi_running_status.flash_param.device_id, sizeof(f_philips_wifi_running_status.flash_param.device_id)) == 0) || 
		 (f_philips_wifi_running_status.flash_param.switch_root_certificate_finish == false) ||
		 (strnlen(f_philips_wifi_running_status.flash_param.mqtt_host, sizeof(f_philips_wifi_running_status.flash_param.mqtt_host)) == 0) || 
		 (f_philips_wifi_running_status.flash_param.mqtt_port == 0) || 
		 (f_philips_wifi_running_status.flash_param.certificate_len == 0) || 
		 (f_philips_wifi_running_status.flash_param.privatekey_len == 0) || 
		 (strncmp(f_philips_wifi_running_status.certificate, "-----BEGIN CERTIFICATE-----", strlen("-----BEGIN CERTIFICATE-----")) != 0 ) || 
		 (strncmp(f_philips_wifi_running_status.certificate + f_philips_wifi_running_status.flash_param.certificate_len - strlen("-----END CERTIFICATE-----") - 1, "-----END CERTIFICATE-----", strlen("-----END CERTIFICATE-----")) != 0 ) || 
		 (strncmp(f_philips_wifi_running_status.privatekey, "-----BEGIN RSA PRIVATE KEY-----", strlen("-----BEGIN RSA PRIVATE KEY-----")) != 0 ) || 
		 (strncmp(f_philips_wifi_running_status.privatekey + f_philips_wifi_running_status.flash_param.privatekey_len - strlen("-----END RSA PRIVATE KEY-----") - 1, "-----END RSA PRIVATE KEY-----", strlen("-----END RSA PRIVATE KEY-----")) != 0 ) || 
		 (strnlen(f_philips_wifi_running_status.privatekey, sizeof(f_philips_wifi_running_status.privatekey)) != f_philips_wifi_running_status.flash_param.privatekey_len) || 
		 (strnlen(f_philips_wifi_running_status.privatekey, sizeof(f_philips_wifi_running_status.privatekey)) != f_philips_wifi_running_status.flash_param.privatekey_len) )
	{
		app_log("=========flash param check fail,need reactive=========");
		f_philips_wifi_running_status.need_https = true;
	}
	else
	{
		app_log("=========flash param check success,don't need reactive=========");
	}

	// exit:
	if ( err != kNoErr )
	{
		// app_log("%s" " exit with err = %d", __FUNCTION__, err);
	}
	return err;
}

struct _philips_wifi_running_status * get_philips_running_status(void)
{
	return &f_philips_wifi_running_status;
}

void philips_param_dump(void)
{
	// app_log("====== ====== ====== ====== ======");
	// app_log("wifi attribution");
	// app_log("mac_address                      = %s", f_philips_wifi_running_status.mac_address);
	// app_log("");
	// app_log("wifi running status");
	// app_log("wifi_status                      = %d", f_philips_wifi_running_status.wifi_status);
	// app_log("need_https                       = %d", f_philips_wifi_running_status.need_https);
	// app_log("range_id                         = %d", f_philips_wifi_running_status.range_id);
	// app_log("is_support_otau                  = %d", f_philips_wifi_running_status.is_support_otau);
	// app_log("");
	// app_log("device attribution");
	// app_log("device_name                      = %s", f_philips_wifi_running_status.device_name);
	// app_log("device_type                      = %s", f_philips_wifi_running_status.device_type);
	// app_log("device_swversion                 = %s", f_philips_wifi_running_status.device_swversion);
	// app_log("device_language                  = %s", f_philips_wifi_running_status.device_language);
	// app_log("");
	// app_log("wifi configuration");
	// app_log("is_first_config_wifi             = %d", f_philips_wifi_running_status.is_first_config_wifi);
	// app_log("sta_client_amount                = %d", f_philips_wifi_running_status.sta_client_amount);
	// app_log("softap_option                    = %d", f_philips_wifi_running_status.softap_option);
	// app_log("extra_data                       = %s", f_philips_wifi_running_status.extra_data);
	// if ( f_philips_wifi_running_status.didt_key == NULL )
	// {
	// 	app_log("didt_key                         = NULL");
	// }
	// else
	// {
	// 	philips_hex_dump("  didt_key: MEMORY", "CLI          =", f_philips_wifi_running_status.didt_key, AES_BLOCK_SIZE, SHORT_FILE, __LINE__);
	// }
	// if ( f_philips_wifi_running_status.didt_key == NULL )
	// {
	// 	app_log("didt_iv                          = NULL");
	// }
	// else
	// {
	// 	philips_hex_dump("  didt_iv:  MEMORY", "CLI          =", f_philips_wifi_running_status.didt_iv, AES_BLOCK_SIZE, SHORT_FILE, __LINE__);
	// }
	// app_log("bind_token                       = %s", f_philips_wifi_running_status.bind_token);
	// app_log("registration_id                  = %s", f_philips_wifi_running_status.registration_id);
	// app_log("push_type                        = %s", f_philips_wifi_running_status.push_type);
	// app_log("app_id                           = %s", f_philips_wifi_running_status.app_id);
	// app_log("app_secret                       = %s", f_philips_wifi_running_status.app_secret);
	// app_log("user_id                          = %s", f_philips_wifi_running_status.user_id);
	// app_log("is_get_ssid                      = %d", f_philips_wifi_running_status.is_get_ssid);
	// app_log("https_client_res                 = %d", f_philips_wifi_running_status.https_client_res);
	// app_log("https_client_error_message       = %s", f_philips_wifi_running_status.https_client_error_message == NULL ? "NULL" : f_philips_wifi_running_status.https_client_error_message);
	// app_log("");
	// app_log("https request");
	// app_log("certificate_url                  = %s", f_philips_wifi_running_status.certificate_url == NULL ? "NULL" : f_philips_wifi_running_status.certificate_url);
	// app_log("privatekey_url                   = %s", f_philips_wifi_running_status.privatekey_url == NULL ? "NULL" : f_philips_wifi_running_status.privatekey_url);
	// app_log("");
	// app_log("semaphore");
	// // app_log("device_otau_sem                  = %p", f_philips_wifi_running_status.device_otau_sem == NULL ? NULL : f_philips_wifi_running_status.device_otau_sem);
	// // app_log("device_init_sem                  = %p", f_philips_wifi_running_status.device_init_sem == NULL ? NULL : f_philips_wifi_running_status.device_init_sem);
	// // app_log("device_info_sem                  = %p", f_philips_wifi_running_status.device_info_sem == NULL ? NULL : f_philips_wifi_running_status.device_info_sem);
	// app_log("");
	// app_log("queue");
	// // app_log("mqtt_send_msg_queue              = %p", f_philips_wifi_running_status.mqtt_send_msg_queue == NULL ? NULL : f_philips_wifi_running_status.mqtt_send_msg_queue);
	// // app_log("uart_send_msg_queue              = %p", f_philips_wifi_running_status.uart_send_msg_queue == NULL ? NULL : f_philips_wifi_running_status.uart_send_msg_queue);
	// app_log("====== ====== ====== ====== ======");
}

void philips_param_dump_cert(void)
{
	// app_log("====== ====== ====== ====== ======");
	// app_log("certificate & privatekey");
	// app_log("certificate                      = [%d]\r\n%s", strnlen(f_philips_wifi_running_status.certificate, sizeof(f_philips_wifi_running_status.certificate)), f_philips_wifi_running_status.certificate);
	// app_log("privatekey                       = [%d]\r\n%s", strnlen(f_philips_wifi_running_status.privatekey, sizeof(f_philips_wifi_running_status.privatekey)), f_philips_wifi_running_status.privatekey);
	// app_log("====== ====== ====== ====== ======");
}

void philips_param_dump_flash(void)
{
	// app_log("====== ====== ====== ====== ======");
	// app_log("wifi_reboot_status               = %d", f_philips_wifi_running_status.flash_param.wifi_reboot_status);
	// app_log("param_old_convert_new_finish     = %d", f_philips_wifi_running_status.flash_param.param_old_convert_new_finish);
	// app_log("config wifi");
	// app_log("passwd                           = %s", f_philips_wifi_running_status.flash_param.passwd);
	// app_log("");
	// app_log("device attribution");
	// app_log("url_type                         = %d", f_philips_wifi_running_status.flash_param.url_type);
	// app_log("device_modelid                   = %s", f_philips_wifi_running_status.flash_param.device_modelid);
	// app_log("aqit                             = %d", f_philips_wifi_running_status.flash_param.aqit);
	// app_log("");
	// app_log("device attribution from https request");
	// app_log("product_range                    = %s", f_philips_wifi_running_status.flash_param.product_range);
	// app_log("product_id                       = %s", f_philips_wifi_running_status.flash_param.product_id);
	// app_log("device_id                        = %s", f_philips_wifi_running_status.flash_param.device_id);
	// app_log("");
	// app_log("mqtt infomation from https request");
	// app_log("mqtt_host                        = %s", f_philips_wifi_running_status.flash_param.mqtt_host);
	// app_log("mqtt_port                        = %d", f_philips_wifi_running_status.flash_param.mqtt_port);
	// app_log("certificate_len                  = %d", f_philips_wifi_running_status.flash_param.certificate_len);
	// app_log("privatekey_len                   = %d", f_philips_wifi_running_status.flash_param.privatekey_len);
	// app_log("====== ====== ====== ====== ======");
}

void philips_param_dump_O2_filter(void)
{
	// app_log("====== ====== ====== ====== ======");
	// app_log("device attribution for O2");
	// app_log("need_transform_filter_life       = %d", f_philips_wifi_running_status.o2_filter_flash_param.need_transform_filter_life);
	// app_log("filter0_life                     = %d", f_philips_wifi_running_status.o2_filter_flash_param.filter0_life);
	// app_log("filter0_lastadjustvalue          = %d", f_philips_wifi_running_status.o2_filter_flash_param.filter0_lastadjustvalue);
	// app_log("filter1_life                     = %d", f_philips_wifi_running_status.o2_filter_flash_param.filter1_life);
	// app_log("filter1_lastadjustvalue          = %d", f_philips_wifi_running_status.o2_filter_flash_param.filter1_lastadjustvalue);
	// app_log("ms_count                         = %lu", f_philips_wifi_running_status.o2_filter_flash_param.ms_count);
	// app_log("====== ====== ====== ====== ======");
}

void philips_param_dump_fogid(void)
{
	// app_log("====== ====== ====== ====== ======");
	// app_log("fogid");
	// app_log("chipid                           = %s", f_philips_wifi_running_status.chipid);
	// app_log("sign                             = %s", f_philips_wifi_running_status.sign);
	// app_log("====== ====== ====== ====== ======");
}

void philips_param_dump_otau(void)
{
	// app_log("====== ====== ====== ====== ======");
	// app_log("otau_flash_param");
	// app_log("cloud_device_version             = %s", f_philips_wifi_running_status.otau_flash_param.cloud_device_version);
	// app_log("need_ota_device                  = %d", f_philips_wifi_running_status.otau_flash_param.need_ota_device);
	// app_log("is_otau_file_downloaded_from_cloud= %d", f_philips_wifi_running_status.otau_flash_param.is_otau_file_downloaded_from_cloud);
	// app_log("otau_data_len                    = %ld", f_philips_wifi_running_status.otau_flash_param.otau_data_len);
	// app_log("====== ====== ====== ====== ======");
	// app_log("otau_run_state");
	// app_log("need_get_otau                    = %d", f_philips_wifi_running_status.otau_run_state.need_get_otau);
	// app_log("is_otau_ing                      = %d", f_philips_wifi_running_status.otau_run_state.is_otau_ing);
	// app_log("device_otau_last_state           = %d", f_philips_wifi_running_status.otau_run_state.device_otau_last_state);
	// app_log("otau_state_id_control            = %ld", f_philips_wifi_running_status.otau_run_state.otau_state_id_control);
	// app_log("local_device_version             = %s", f_philips_wifi_running_status.otau_run_state.local_device_version);
	// app_log("otau_current_package             = %d", f_philips_wifi_running_status.otau_run_state.otau_current_package);
	// app_log("otau_total_package               = %d", f_philips_wifi_running_status.otau_run_state.otau_total_package);
	// app_log("otau_data_leave                  = %ld", f_philips_wifi_running_status.otau_run_state.otau_data_leave);
	// app_log("otau_data_index                  = %ld", f_philips_wifi_running_status.otau_run_state.otau_data_index);
	// app_log("====== ====== ====== ====== ======");
}

int philips_queue_message_init(queue_message_t *msg)
{
	int res = 0;

	require_action(msg != NULL && msg->data_len != 0 && msg->data == NULL, exit, res = kParamErr);

	msg->data = (uint8_t *)malloc(msg->data_len);
	require_action(msg->data != NULL, exit, res = kNoMemoryErr);
	memset(msg->data, 0x00, msg->data_len);

	exit:
	return res;
}

void philips_queue_message_deinit(queue_message_t *msg, int err)
{
	if ( msg != NULL && err != kNoErr && msg->data != NULL )
	{
		free(msg->data);
		msg->data = NULL;
	}
}

bool philips_product_id_param_check(void)
{
	merr_t err = kNoErr;
	bool res = false;
	uint8_t product_id[64];
	int len = sizeof(f_philips_wifi_running_status.flash_param.product_id);

	err = mkv_item_get("pdafhr_pid", product_id, &len);
	require_noerr_action(err, exit, res = false);

	if(f_philips_wifi_running_status.flash_param.product_id[0] != '\0' && strlen(f_philips_wifi_running_status.flash_param.product_id) > 0)
		res = true;
	else
		res = false;
exit:
	return res;
}

void philips_o2_filter_flash_data_update(void)
{
	mkv_item_set(CONFIG_DATA_KV_KEY_O2_FILTER, &f_philips_wifi_running_status.o2_filter_flash_param, sizeof(f_philips_wifi_running_status.o2_filter_flash_param));
}

void philips_o2_filter_flash_data_clear(void)
{
	mkv_item_delete(CONFIG_DATA_KV_KEY_O2_FILTER);
}

void philips_clear_flash_param_for_certificate(void)
{
	uint32_t offset = 0;

	mkv_item_delete("pmifhr_clen");

	offset = CERTIFICATE_OFFSET;
	mhal_flash_erase(MODULE_PARTITION_USER, offset, 2048);

	return ;
}

void philips_clear_flash_param_for_privatekey(void)
{
	uint32_t offset = 0;

	mkv_item_delete("pmifhr_plen");

	offset = PRIVATEKEY_OFFSET;
	mhal_flash_erase(MODULE_PARTITION_USER, offset, 2048);

	return ;
}

void philips_clear_flash_param_for_reactive(void)
{
	uint32_t offset = CERTIFICATE_OFFSET;

	mkv_item_delete("pdafhr_range");
	mkv_item_delete("pdafhr_pid");
	mkv_item_delete("pdafhr_did");

	mkv_item_delete("pmifhr_host");
	mkv_item_delete("pmifhr_port");
	mkv_item_delete("pmifhr_clen");
	mkv_item_delete("pmifhr_plen");

	mkv_item_delete(CONFIG_DATA_KV_KEY_OTAU);

	offset = CERTIFICATE_OFFSET;
	mhal_flash_erase(MODULE_PARTITION_USER, offset, 2048);
	offset = PRIVATEKEY_OFFSET;
	mhal_flash_erase(MODULE_PARTITION_USER, offset, 2048);

	return ;
}

void philips_clear_flash_param_for_all(void)
{
	uint32_t offset = CERTIFICATE_OFFSET;

	mkv_item_delete("pwrr_status");

	mkv_item_delete("pocn_finish");

	mkv_item_delete("pcw_passwd");

	mkv_item_delete("pda_urltype");
	mkv_item_delete("pda_mid");
	mkv_item_delete("pda_aqit");

	mkv_item_delete("pdafhr_range");
	mkv_item_delete("pdafhr_pid");
	mkv_item_delete("pdafhr_did");

	mkv_item_delete("pmifhr_host");
	mkv_item_delete("pmifhr_port");
	mkv_item_delete("pmifhr_clen");
	mkv_item_delete("pmifhr_plen");

	mkv_item_delete(CONFIG_DATA_KV_KEY_O2_FILTER);
	mkv_item_delete(CONFIG_DATA_KV_KEY_OTAU);
	mkv_item_delete(DATA_KV_MCU_BOOT);


#ifndef PHILIPS_SECURITY
	mkv_item_delete("LOGSW_uart");
	mkv_item_delete("LOGSW_ssl");
#endif

	offset = CERTIFICATE_OFFSET;
	mhal_flash_erase(MODULE_PARTITION_USER, offset, 2048);
	offset = PRIVATEKEY_OFFSET;
	mhal_flash_erase(MODULE_PARTITION_USER, offset, 2048);

	return ;
}