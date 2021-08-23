#ifndef __PHILIPS_PARAM_H__
#define __PHILIPS_PARAM_H__

#include "mxos.h"
#include "mxos_config.h"
#include "cJSON.h"

#include "philips_device.h"
#include "philips_device_model.h"

#define DEFAULT_PASSWD							"asd34rtaef"

// watch dog
#define PHILIPS_SOFT_WATCHDOG_TIMEOUT			70000	//ms
#define PHILIPS_SOFT_FEEDDOG_PERIOD				30000	//ms
#define PHILIPS_WATCHDOG_TIMEOUT_MAX			(0xFFFFFFFF)

// OTA
#define OTATYPE_WIFI_CLOUD_TYPE					"wifi"
#define OTATYPE_DEVICE_CLOUD_TYPE				"device"
#define OTATYPE_FLASH_CLOUD_TYPE				"flash"

#define UPLOAD_STATUS_MAX_LENGTH				1024
#define UPLOAD_DATA_MAX_COUNT					4

#define CERTIFICATE_OFFSET						0x11000
#define PRIVATEKEY_OFFSET						0x12000

#define CONFIG_DATA_KV_KEY_OTAU					"philips_otau"
#define DATA_KV_WIFI_OFFLINE_LOG				"philips_offlinelog"
#define DATA_KV_MCU_BOOT                       	"philips_mcuboot"

#define CONFIG_DATA_KV_KEY_O2_FILTER			"philips_O2_filter"

// WiFi log step
#define PHILIPS_WIFI_LOG_STEP_WIFIDISCONNECT            6
#define PHILIPS_WIFI_LOG_STEP_CERTIFICATION				8
#define PHILIPS_WIFI_LOG_STEP_ACTIVATION				9
#define PHILIPS_WIFI_LOG_STEP_UDP						10
#define PHILIPS_WIFI_LOG_STEP_DEVICE_BIND				11
#define PHILIPS_WIFI_LOG_STEP_COAPSERVER				12
#define PHILIPS_WIFI_LOG_STEP_DOWNLOAD_CERTIFICATION	13
#define PHILIPS_WIFI_LOG_STEP_DOWNLOAD_PRIVATEKEY		14
#define PHILIPS_WIFI_LOG_STEP_MQTT						15

//url type
enum _philips_url_type
{
	PHILIPS_URL_TYPE_NONE						= 0,
	PHILIPS_URL_TYPE_CHINA						= 1,
	PHILIPS_URL_TYPE_FRANKFORD					= 2,
	PHILIPS_URL_TYPE_AMOUNT						= 3,
};

struct _philips_urls
{
	char *url_certification;
	char *url_activation;
	char *url_ota;
	char *url_bind;
	char *url_reset;
	char *url_wifilog;
};

enum _wifi_reboot_status
{
	WRBT_unknow,
	WRBT_config,
	WRBT_factory,
	WRBT_wifi_ota,
	WRBT_device_ota,
};

enum _wifi_running_status
{
	WRS_unknow,
	WRS_wait_for_device_init,
	WRS_wait_for_device_info,
	WRS_init_complete,
	WRS_wifiui_idle,
	WRS_wifiui_idle_out,
	WRS_wifiui_config_ing,
	WRS_wifiui_config_app_connected,
	WRS_wifiui_config_timeout,
	WRS_wifiui_connect_ing,
	WRS_wifiui_connect_router,
	WRS_wifiui_connect_cloud,
};

enum _ota_type
{
	OTATYPE_WIFI,
	OTATYPE_DEVICE,
	OTATYPE_FLASH,
	OTATYPE_UNKNOW = 0xFF,
};

enum _ota_status
{
	OTAST_NO_NEED,
	OTAST_CHECKING,
	OTAST_REQUESTRECEIVED,
	OTAST_READY_TO_DOWNLOAD,
	OTAST_DOWNLOADING,
	OTAST_DOWNLOADED,
	OTAST_FAILED,
	OTAST_INSTALL,
	OTAST_SUCCESS,
	OTAST_NEED_CHECK,
	OTAST_AMOUNT,
};

typedef struct _upload_manage
{
	// device status change
	bool status_change;
	// device sensor change
	bool sensor_change;
	// app control upload
	bool app_control_upload;
	// app get status
	bool app_get_status;
	uint32_t error;
	uint32_t last_error;
	uint32_t warn;
	uint32_t last_warn;
	uint16_t last_iaql;
	uint16_t last_tvoc;
}upload_manage_t;

typedef struct 
{	
	int step;
	bool result;
	int err_type;
	int err_code;
	uint32_t err_line;
	char message[128];
}wifi_log_t;

typedef struct _ota_process
{
	enum _ota_type ota_type;
	bool need_check_otau;
	enum _ota_status ota_status;
	char url_value[256];
	char md5_value[50];
	char type_value[50];
	char version_value[50];
}ota_process_t;

typedef struct _otau_run_state
{
	bool is_send_idle_before_set_device_otau_sem;
	bool need_get_otau;
	bool is_otau_ing;
	char local_device_version[6];
	uint8_t device_otau_last_state;
	uint32_t otau_state_id_control;
	int otau_current_package;
	int otau_total_package;
	uint32_t otau_data_leave;
	uint32_t otau_data_index;
}otau_run_state_t;

typedef struct _offline_log_flash_param
{
	int rssi;
	char bssid[13];
	int free_memory;
	double runtime;
	int step;
	int err_code;
	int err_type;
}offline_log_flash_param_t;

typedef struct _otau_flash_param
{
	char magic_word[6];

	char cloud_device_version[6];
	bool need_ota_device;
	bool is_otau_file_downloaded_from_cloud;
	uint32_t otau_data_len;
}otau_flash_param_t;

typedef struct _o2_filter_flash_param
{
	char magic_word[6];

	// device attribution for O2
	bool need_transform_filter_life;	/* pdafo2_need */
	uint16_t filter0_life;	/* pdafo2_f0life */
	uint8_t filter0_lastadjustvalue;	/* pdafo2_f0ad */
	uint16_t filter1_life;	/* pdafo2_f1life */
	uint8_t filter1_lastadjustvalue;	/* pdafo2_f1ad */
	mxos_time_t ms_count;	/* pdafo2_ms */
}o2_filter_flash_param_t;

typedef struct _flash_param
{
	// wifi reboot reason
	enum _wifi_reboot_status wifi_reboot_status;	/*pwrr_status*/

	// flash param old ver change new ver
	bool param_old_convert_new_finish;				/*pocn_finish*/

	// parameter1 convert finish 					/*pck_finish*/
	bool parameter1_convert_kv_finish;

	// switch root certificate						/*src_finish*/
	bool switch_root_certificate_finish;

	bool muc_boot;
	bool philips_uart_protocol_is_common;

	// config wifi
	char passwd[20];	/* pcw_passwd */

	// device attribution
	uint8_t url_type;	/* pda_urltype */
	char device_modelid[32];	/* pda_mid */
	uint16_t aqit;	/* pda_aqit */

	// device attribution from https request
	char product_range[32];	/* pdafhr_range */
	char product_id[64];	/* pdafhr_pid */
	char device_id[64];	/* pdafhr_did */

	// mqtt infomation from https request
	char mqtt_host[128];	/* pmifhr_host */
	int mqtt_port;	/* pmifhr_port */
	int certificate_len;	/* pmifhr_clen */
	int privatekey_len;	/* pmifhr_plen */
}flash_param_t;

struct _philips_wifi_running_status
{
	// wifi attribution
	char mac_address[20];

	// wifi running status
	enum _wifi_running_status wifi_status; 
	bool need_https;
	enum _philips_range range_id;
	bool is_support_otau;

	// philips url
	const struct _philips_urls (*philips_urls);

	// fogid - MICO_PARTITION_SDS - 0: {chipid,sign}
	char chipid[25];
	char sign[97];

	// certificate & privatekey - MICO_PARTITION_USER - CERTIFICATE_OFFSET: {certificate}, PRIVATEKEY_OFFSET: {privatekey}
	char certificate[2048];
	char privatekey[2048];

	// device attribution
	bool need_get_deviceinfo;
	char device_category[20];
	char device_name[33];
	char device_type[33];
	char device_swversion[12];
	char device_language[11];
	uint8_t wifi_protocol_ver;
	uint8_t OTAU_protocol_ver;
	char node1bootloaderver[7];
	char node1applicationver[7];

	// wifi configuration
	bool is_first_config_wifi;
	int sta_client_amount;
	int softap_option;
	char extra_data[20];
	uint8_t *didt_key;
	uint8_t *didt_iv;
	char bind_token[32];
	char registration_id[128];
	char push_type[32];
	char app_id[64];
	char app_secret[64];
	char user_id[128];
	bool is_get_ssid;
	merr_t https_client_res;
	char *https_client_error_message;
	int rssi;
	int need_report_offlinelog;

	// https request
	char *certificate_url;
	char *privatekey_url;

	// is reconnect cloud
	bool is_reconnect_cloud;
	// is_rf_power_save
	bool is_rf_power_save;

	// upload manage
	upload_manage_t device_upload_manage;
	// wifi log
	wifi_log_t wifi_log_param;
	// ota process
	ota_process_t ota_process;
	// otau run state
	otau_run_state_t otau_run_state;
	// factory mode
	bool factory_ing;
	uint8_t factory_mode;
	// debug_o2_filter
	bool enable_o2_filter_debug;
	uint16_t last_upload_filter0_life;
	uint16_t last_upload_filter1_life;
	// watchdog
	mxos_system_monitor_t uart_monitor;
	mxos_time_t uart_feed_dog_time;
	mxos_system_monitor_t mqtt_monitor;
	mxos_time_t mqtt_feed_dog_time;
	bool mqtt_monitor_start;
	// log on/off
	uint8_t uart_log_switch;
	uint8_t ssl_log_switch;

	// kv in flash
	flash_param_t flash_param;
	offline_log_flash_param_t offline_log_flash_param;
	o2_filter_flash_param_t o2_filter_flash_param;
	otau_flash_param_t otau_flash_param;
};

enum _message_type
{
	MT_Unknow,

	MT_UART_SEND,
	MT_UART_RECV,

	MT_MQTT_PUBLISH,
	MT_PUBLISH_NONE,
	MT_SHADOW_UPDATE,
	MT_SHADOW_DELETE,
	MT_SHADOW_GET,
	MT_DEVICE_NOTICE,
	MT_DEVICE_DATA,
	MT_MQTT_SUBSCRIBE,
};

typedef struct _queue_message
{
	enum _message_type type;
	uint8_t *data;
	int data_len;
} queue_message_t;

/**
 * *****************************************************************************
 * 与系统、平台无关的函数
 * 如果外部使用，需要用extern声明而非包含xxxx_param.h头文件
 * *****************************************************************************
 **/

/**
 * *****************************************************************************
 * 获取FogID
 * *****************************************************************************
 **/
bool get_fogid_from_sds_partition(void);

/**
 * *****************************************************************************
 * running status
 * *****************************************************************************
 **/
merr_t philips_param_init(void);
struct _philips_wifi_running_status * get_philips_running_status(void);

void philips_param_dump(void);
void philips_param_dump_cert(void);
void philips_param_dump_flash(void);
void philips_param_dump_fogid(void);
void philips_param_dump_otau(void);

bool philips_product_id_param_check(void);

void philips_o2_filter_flash_data_update(void);
void philips_o2_filter_flash_data_clear(void);
void philips_clear_flash_param_for_certificate(void);
void philips_clear_flash_param_for_privatekey(void);
void philips_clear_flash_param_for_reactive(void);
void philips_clear_flash_param_for_all(void);

int philips_queue_message_init(queue_message_t *msg);
void philips_queue_message_deinit(queue_message_t *msg, int err);

#endif
