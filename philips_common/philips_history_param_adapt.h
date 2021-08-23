#ifndef __PHILIPS_HISTORY_PARAM_ADAPT_H__
#define __PHILIPS_HISTORY_PARAM_ADAPT_H__

#include "mxos.h"

// #define MAX_HISTORY_USER_FLASH_DATA_LENGTH		1024*5

#define CA_MAX_DATA								2048
#define INFO_MAX_DATA							200

// 60.1版本
// #define CA_MAX_DATA								2048
// #define INFO_MAX_DATA							200
#pragma pack(1)
typedef struct _philips_config_data_60
{
	uint8_t philips_config_crc[2];

	bool wifiui_notconnect_requested;

	uint8_t url_type;
	uint8_t device_type;
	uint8_t device_moduleid;
	uint8_t __reserve_1; // remove otau_id otaf_id
	bool is_support_otau;
	bool is_support_otaf;
	uint8_t ota_progress;
	// OTAU
	bool need_get_otau;
	char cloud_device_version[6];
	bool is_otau_file_downloaded_from_cloud;
	bool is_otau_ing;
	bool is_send_idle_before_set_device_otau_sem;
	uint8_t device_otau_last_state;
	uint32_t otau_current_package;
	uint32_t otau_total_package;
	uint32_t otau_file_total_length;
	uint32_t send_to_device_start_address;
	bool user_partition_have_fw;
	char user_partition_device_version[6];
	uint32_t user_partition_otau_total_package;
	uint32_t user_partition_otau_file_total_length;

	char device_id[64];

	char certificate_id[INFO_MAX_DATA];

	char mqtt_host[INFO_MAX_DATA];
	int mqtt_port;

	uint8_t certificate[CA_MAX_DATA];
	int certificate_len;
	uint8_t privatekey[CA_MAX_DATA];
	int privatekey_len;

	char mac_address[20];
	char passwd[20];

	uint16_t aqit;

	bool need_transform_filter_life;
	uint16_t filter0_life;
	uint8_t filter0_lastadjustvalue;
	uint16_t filter1_life;
	uint8_t filter1_lastadjustvalue;
	mxos_time_t ms_count;

	bool is_factory_testing;

	char device_modelid[32];
	char product_id[64];
	char product_range[32];
} philips_config_data_60_t;
#pragma pack()

// 59版本\58版本\57版本\56.4版本
// #define CA_MAX_DATA								2048
// #define INFO_MAX_DATA							200
#pragma pack(1)
typedef struct _philips_config_data_59
{
	uint8_t philips_config_crc[2];

	bool wifiui_notconnect_requested;

	uint8_t url_type;
	uint8_t device_type;
	uint8_t device_moduleid;
	uint8_t __reserve_1; // remove otau_id otaf_id
	bool is_support_otau;
	bool is_support_otaf;
	uint8_t ota_progress;
	// OTAU
	bool need_get_otau;
	char cloud_device_version[6];
	bool is_otau_file_downloaded_from_cloud;
	bool is_otau_ing;
	bool is_send_idle_before_set_device_otau_sem;
	uint8_t device_otau_last_state;
	uint32_t otau_current_package;
	uint32_t otau_total_package;
	uint32_t otau_file_total_length;
	uint32_t send_to_device_start_address;
	bool user_partition_have_fw;
	char user_partition_device_version[6];
	uint32_t user_partition_otau_total_package;
	uint32_t user_partition_otau_file_total_length;

	char device_id[64];

	char certificate_id[INFO_MAX_DATA];

	char mqtt_host[INFO_MAX_DATA];
	int mqtt_port;

	uint8_t certificate[CA_MAX_DATA];
	int certificate_len;
	uint8_t privatekey[CA_MAX_DATA];
	int privatekey_len;

	char mac_address[20];
	char passwd[20];

	uint16_t aqit;

	bool need_transform_filter_life;
	uint16_t filter0_life;
	uint8_t filter0_lastadjustvalue;
	uint16_t filter1_life;
	uint8_t filter1_lastadjustvalue;
	mxos_time_t ms_count;

	bool is_factory_testing;
} philips_config_data_59_t;
#pragma pack()

// 54.2版本
// #define CA_MAX_DATA								2048
// #define INFO_MAX_DATA							200
#pragma pack(1)
typedef struct _philips_config_data_54
{
	uint8_t philips_config_crc[2];

	bool wifiui_notconnect_requested;

	uint8_t url_type;
	uint8_t device_type;
	uint8_t device_moduleid;
	uint8_t __reserve_1; // remove otau_id otaf_id
	bool is_support_otau;
	bool is_support_otaf;
	uint8_t ota_progress;
	// OTAU
	bool need_get_otau;
	char cloud_device_version[6];
	bool is_otau_file_downloaded_from_cloud;
	bool is_otau_ing;
	bool is_send_idle_before_set_device_otau_sem;
	uint8_t device_otau_last_state;
	uint32_t otau_current_package;
	uint32_t otau_total_package;
	uint32_t otau_file_total_length;
	uint32_t send_to_device_start_address;
	bool user_partition_have_fw;
	char user_partition_device_version[6];
	uint32_t user_partition_otau_total_package;
	uint32_t user_partition_otau_file_total_length;

	char device_id[64];

	char certificate_id[INFO_MAX_DATA];

	char mqtt_host[INFO_MAX_DATA];
	int mqtt_port;

	uint8_t certificate[CA_MAX_DATA];
	int certificate_len;
	uint8_t privatekey[CA_MAX_DATA];
	int privatekey_len;

	char mac_address[20];

	uint16_t aqit;

	bool need_transform_filter_life;
	uint16_t filter0_life;
	uint8_t filter0_lastadjustvalue;
	uint16_t filter1_life;
	uint8_t filter1_lastadjustvalue;
	mxos_time_t ms_count;
} philips_config_data_54_t;
#pragma pack()

// 53版本
// #define CA_MAX_DATA								2048
// #define INFO_MAX_DATA							200
#pragma pack(1)
typedef struct _philips_config_data_53
{
	uint8_t philips_config_crc[2];

	bool wifiui_notconnect_requested;

	uint8_t url_type;
	uint8_t device_type;
	uint8_t device_moduleid;
	uint8_t otau_id;
	uint8_t otaf_id;
	uint8_t ota_progress;
	bool need_get_otau;
	char cloud_device_version[6];
	bool is_otau_file_downloaded_from_cloud;
	bool is_otau_ing;
	bool is_send_idle_before_set_device_otau_sem;
	uint8_t device_otau_last_state;
	uint32_t otau_current_package;
	uint32_t otau_total_package;
	uint32_t otau_file_total_length;
	uint32_t send_to_device_start_address;
	bool user_partition_have_fw;
	char user_partition_device_version[6];
	uint32_t user_partition_otau_total_package;
	uint32_t user_partition_otau_file_total_length;

	char device_id[64];

	char certificate_id[INFO_MAX_DATA];

	char mqtt_host[INFO_MAX_DATA];
	int mqtt_port;

	uint8_t certificate[CA_MAX_DATA];
	int certificate_len;
	uint8_t privatekey[CA_MAX_DATA];
	int privatekey_len;

	char mac_address[20];

	uint16_t aqit;

	bool need_transform_filter_life;
	uint16_t filter0_life;
	uint8_t filter0_lastadjustvalue;
	uint16_t filter1_life;
	uint8_t filter1_lastadjustvalue;
	mxos_time_t ms_count;
} philips_config_data_53_t;
#pragma pack()

// 51.1版本\47.1版本
// #define CA_MAX_DATA								2048
// #define INFO_MAX_DATA							200
#pragma pack(1)
typedef struct _philips_config_data_51
{
	uint8_t philips_config_crc[2];

	bool wifiui_notconnect_requested;

	uint8_t url_type;
	uint8_t device_type;
	uint8_t device_moduleid;
	uint8_t otau_id;
	uint8_t otaf_id;

	char device_id[64];

	char certificate_id[INFO_MAX_DATA];

	char mqtt_host[INFO_MAX_DATA];
	int mqtt_port;

	uint8_t certificate[CA_MAX_DATA];
	int certificate_len;
	uint8_t privatekey[CA_MAX_DATA];
	int privatekey_len;

	char mac_address[20];

	uint16_t aqit;

	bool need_transform_filter_life;
	uint16_t filter0_life;
	uint8_t filter0_lastadjustvalue;
	uint16_t filter1_life;
	uint8_t filter1_lastadjustvalue;
	mxos_time_t ms_count;
} philips_config_data_51_t;
#pragma pack()

// 43版本\42版本
// #define CA_MAX_DATA								2048
// #define INFO_MAX_DATA							200
#pragma pack(1)
typedef struct _philips_config_data_43
{
	uint8_t philips_config_crc[2];

	bool wifiui_notconnect_requested;

	uint8_t url_type;
	uint8_t device_type;
	uint8_t device_moduleid;
	uint8_t otau_id;

	char device_id[64];

	char certificate_id[INFO_MAX_DATA];

	char mqtt_host[INFO_MAX_DATA];
	int mqtt_port;

	uint8_t certificate[CA_MAX_DATA];
	int certificate_len;
	uint8_t privatekey[CA_MAX_DATA];
	int privatekey_len;

	char mac_address[20];

	uint16_t aqit;

	bool need_transform_filter_life;
	uint16_t filter0_life;
	uint8_t filter0_lastadjustvalue;
	uint16_t filter1_life;
	uint8_t filter1_lastadjustvalue;
	mxos_time_t ms_count;
} philips_config_data_43_t;
#pragma pack()

// 39版本\35版本\32版本
// #define CA_MAX_DATA								2048
// #define INFO_MAX_DATA							200
#pragma pack(1)
typedef struct _philips_config_data_39
{
	uint8_t philips_config_crc[2];

	bool wifiui_notconnect_requested;

	uint8_t url_type;
	uint8_t device_type;
	uint8_t device_moduleid;

	char device_id[64];

	char certificate_id[INFO_MAX_DATA];

	char mqtt_host[INFO_MAX_DATA];
	int mqtt_port;

	uint8_t certificate[CA_MAX_DATA];
	int certificate_len;
	uint8_t privatekey[CA_MAX_DATA];
	int privatekey_len;

	char mac_address[20];

	uint16_t aqit;
} philips_config_data_39_t;
#pragma pack()

void philips_history_param_adapt_process(void);

#endif