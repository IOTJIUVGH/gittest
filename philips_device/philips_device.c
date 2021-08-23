#include "mkv.h"

#include "philips_device.h"
#include "philips_param.h"
#include "philips_uart.h"
#include "philips_device_factory.h"
#include "philips_https_request.h"

#include "philips_device_fltsts_process.h"
#include "philips_device_upload_manage.h"

#include "philips_log.h"
#define app_log(format, ...)					philips_custom_log("pd", format, ##__VA_ARGS__)

purifier_status_t purifier_status = 
{
	.om_str = {'0', '\0', '\0'},
	.pwr_str = {'0', '\0', '\0'},
	.cl = false,
	.aqil = '1',
	.aqil_str = {'0', '\0', '\0'},
	.uil_str = {'0', '\0', '\0'},
	.gas=0,
	.formaldehyde=0,
	.mode_str = {'0', '\0', '\0'},
	.pm25 = 1,
	.iaql = 1,
	.aqit = 0,
	.tvoc = 1,
	.ddp_str = {'0', '\0', '\0'},
	.rddp_str = {'0', '\0', '\0'},
	.err = 0,
	.dt = 0,
	.dtrs = 0,
	.func = {'0', '\0', '\0'},
	.rhset = 0,
	.rh = 1,
	.temp = 0,
	.wl = 0,

	.uaset_str = {'0', '\0', '\0'},

	.fltt1 = {'A', '0', '\0'},
	.fltt2 = {'n', 'o', 'n', 'e', '\0'},
	.fltsts0 = 0,
	.fltsts1 = 0,
	.fltsts2 = 0,

	.filna = "0",
	.filid = "0",

	.flttotal0 = 0,
	.flttotal1 = 0,
	.flttotal2 = 0,

	.air_prop_map = 0,
	.filter_prop_map = 0,
};

uint8_t case_val=0;

/*******************************************************************************
 * philips uart protocol send api
 *******************************************************************************/

/**
 * ID = 1
 * @param id
 * @param cmd
 */
static merr_t philips_send_initialize(void)
{
	merr_t res = kNoErr;
	
	queue_message_t queue_msg = {MT_UART_SEND, NULL, 9};
	res = philips_queue_message_init(&queue_msg);
	require_noerr(res, exit);

	// start ID length
	queue_msg.data[0] = 0xFE;
	queue_msg.data[1] = 0xFF;
	queue_msg.data[2] = 0x01;
	queue_msg.data[3] = 0x00;
	queue_msg.data[4] = 0x02;

	// reqVersion
	queue_msg.data[5] = 0x01;

	// clientHints
	queue_msg.data[6] = 0x00;

	res = philips_uart_queue_push(&queue_msg);
	require_noerr(res, exit);

	exit:
	philips_queue_message_deinit(&queue_msg, res);
	return res;
}

/**
 * ID = 3, port = 1, porpID = 1 or 7 or 8
 * @param config
 * @param configlen
 * @param ppropID
 */
static merr_t philips_send_PutProps_device_config(const char *config, int configlen, unsigned char porpID)
{
	merr_t res = kNoErr;

	int index = 0;
	queue_message_t queue_msg = {MT_UART_SEND, NULL, 12 + configlen};

	res = philips_queue_message_init(&queue_msg);
	require_noerr(res, exit);

	// start ID length
	queue_msg.data[0] = 0xFE;
	queue_msg.data[1] = 0xFF;
	queue_msg.data[2] = 0x03;
	queue_msg.data[3] = 0x00;
	queue_msg.data[4] = 5 + configlen;

	// port
	queue_msg.data[5] = 0x01;
	index = 5;

	// prop config
	queue_msg.data[++index] = porpID;
	queue_msg.data[++index] = configlen + 1;
	memcpy(queue_msg.data + (++index), config, configlen);
	index += configlen;
	queue_msg.data[index] = 0x00;

	// end prop
	queue_msg.data[++index] = 0x00;

	res = philips_uart_queue_push(&queue_msg);
	require_noerr(res, exit);

	exit:
	philips_queue_message_deinit(&queue_msg, res);
	return res;
}

/**
 * ID = 3, port = 1, porpID = 1
 * @param name
 * @param namelen
 */
merr_t philips_send_PutProps_device_name(const char *name, int namelen)
{
	merr_t res = kNoErr;

	app_log("name = [%d]%s", namelen, name);
	require_action(name != NULL && namelen < sizeof(get_philips_running_status()->device_name), exit, res = kParamErr);

	if(get_philips_running_status()->flash_param.philips_uart_protocol_is_common)
	{
		res = philips_send_PutProps_device_config(name, namelen, 0x01);
		require_noerr(res, exit);
	}
	else
	{
		res = philips_send_PutProps_device_config(name, namelen, 0x02);
		require_noerr(res, exit);
	}

	exit:
	return res;
}

/**
 * ID = 3, port = 1, porpID = 7
 */
merr_t philips_send_PutProps_device_wfversion(void)
{
	merr_t res = kNoErr;
	
	int wifiversionlen = strlen(FIRMWARE_REVISION_MINOR);
	app_log("wifiversion = [%d]%s", wifiversionlen, FIRMWARE_REVISION_MINOR);


	if(get_philips_running_status()->flash_param.philips_uart_protocol_is_common)
	{	
		res = philips_send_PutProps_device_config(FIRMWARE_REVISION_MINOR, wifiversionlen, 0x10);
		require_noerr(res, exit);
	}
	else 
	{
		res = philips_send_PutProps_device_config(FIRMWARE_REVISION_MINOR, wifiversionlen, 0x07);
		require_noerr(res, exit);
	}
	exit:
	return res;
}

/**
 * ID = 3, port = 1, porpID = 8
 * @param language
 * @param languagelen
 */
merr_t philips_send_PutProps_device_language(const char *language, int languagelen)
{
	merr_t res = kNoErr;

	app_log("language = [%d]%s", languagelen, language);
	require_action(language != NULL && languagelen < sizeof(get_philips_running_status()->device_language), exit, res = kParamErr);

	res = philips_send_PutProps_device_config(language, languagelen, 0x08);
	require_noerr(res, exit);

	exit:
	return res;
}

/**
 * send common function
 * @param id
 * @param port
 * @param props
 * @param propslen
 */
merr_t philips_send_Common(enum _DiComm_Operation_ID id, uint8_t port, uint8_t *props, int propslen)
{
	merr_t res = kNoErr;

	uint16_t length = propslen + 2;

	queue_message_t queue_msg = {MT_UART_SEND, NULL, 5 + 1 + propslen + 1 + 2};
	res = philips_queue_message_init(&queue_msg);
	require_noerr(res, exit);

	// start
	queue_msg.data[0] = 0xFE;
	queue_msg.data[1] = 0xFF;

	// ID
	queue_msg.data[2] = id;

	// length
	queue_msg.data[3] = length / 256;
	queue_msg.data[4] = length % 256;

	// port
	queue_msg.data[5] = port;

	// prop
	memcpy(queue_msg.data + 6, props, propslen);

	// end prop
	queue_msg.data[5 + 1 + propslen] = 0x00;

	res = philips_uart_queue_push(&queue_msg);
	require_noerr(res, exit);

exit:
	philips_queue_message_deinit(&queue_msg, res);
	return res;
}

/**
 * putprops common function
 * ID = 3
 * @param port
 * @param props
 * @param propslen
 */
merr_t philips_send_PutProps_Common(uint8_t port, uint8_t *props, int propslen)
{
	return philips_send_Common(DOID_PutpropsRequest, port, props, propslen);
}

/**
 * ID = 3, port = 2, porpID = 1 and 2
 * @param connection
 * @param setup
 */
static merr_t philips_send_put_PutProps_wifiui(uint8_t connection, uint8_t setup)
{
	uint8_t props[6] = {0};

	memset(props, 0, sizeof(props));

	// prop connection
	get_philips_running_status()->flash_param.philips_uart_protocol_is_common ==true ? (props[0] = WPPN_connection+1) : (props[0] = WPPN_connection);
	
	props[1] = 1;
	props[2] = connection;

	// prop setup
	(get_philips_running_status()->flash_param.philips_uart_protocol_is_common ==true) ? (props[3] = WPPN_setup+1) : (props[3] = WPPN_setup);
	props[4] = 1;
	props[5] = setup;

	return philips_send_PutProps_Common(0x02, props, 6);
}

/**
 * ID = 3, port = 4
 * @param fac_port
 * @param num
 */
merr_t philips_send_PutProps_fac(enum _Fac_properties_property_no fppn, uint8_t value)
{
	uint8_t props[3] = {0};

	memset(props, 0, sizeof(props));

	// prop
	(get_philips_running_status()->flash_param.philips_uart_protocol_is_common) ? (props[0] = ++fppn) : (props[0] = fppn);
	props[1] = 0x01;
	props[2] = value;

	return philips_send_PutProps_Common(0x04, props, 3);
}

/**
 * ID = 3, port = 7, , porpID = 1 and 2
 */
merr_t philips_send_PutProps_mac(void)
{
	uint8_t props[40] = {0};

	memset(props, 0, sizeof(props));

	// prop mac
	(get_philips_running_status()->flash_param.philips_uart_protocol_is_common ==true) ? (props[0] = 0x02):(props[0] = 0x01);
	
	props[1] = 0x06;
	uint8_t mac[6];
	mwifi_get_mac(mac);
	memcpy(props + 2, mac, 6);

	// prop modelid
	if(!get_philips_running_status()->flash_param.philips_uart_protocol_is_common)	
	{
		props[8] = 0x02;
		props[9] = strlen(get_philips_running_status()->flash_param.device_modelid) + 1;
		memcpy(props + 10, get_philips_running_status()->flash_param.device_modelid, strlen(get_philips_running_status()->flash_param.device_modelid));
		props[strlen(get_philips_running_status()->flash_param.device_modelid)] = 0;

		return philips_send_PutProps_Common(0x07, props, 8 + 3);
	}

	return philips_send_PutProps_Common(0x07, props, 8 + 3 + strlen(get_philips_running_status()->flash_param.device_modelid)); 
}

/**
 * ID = 3, port = 8, propID = 1
 * @param id
 * @param state
 * @return
 */
merr_t philips_send_PutProps_otau_state(enum _DiComm_Operation_ID id, enum _OTAU_properties_state_value state)
{
	uint8_t props[3] = {0};

	memset(props, 0, sizeof(props));

	// prop
	(get_philips_running_status()->flash_param.philips_uart_protocol_is_common ==true) ? (props[0] = 0x02):(props[0] = 0x01);
	props[1] = 0x01;
	props[2] = state;

	return philips_send_Common(id, 0x08, props, 3);
}

/**
 * ID = 3, port = 8, propID = 1,2,3,4
 * @param id
 * @param state
 * @param device_version
 * @param sn
 * @param total
 * @return
 */
merr_t philips_send_PutProps_otau_fours(enum _DiComm_Operation_ID id, enum _OTAU_properties_state_value state, char *device_version, int sn, int total)
{
	app_log("device_version is %s", device_version);

	uint8_t props[19] = {0};

	memset(props, 0, sizeof(props));

	// porp state
	(get_philips_running_status()->flash_param.philips_uart_protocol_is_common ==true) ? (props[0] = 0x02):(props[0] = 0x01);
	props[1] = 0x01;
	props[2] = state;

	// prop version
	(get_philips_running_status()->flash_param.philips_uart_protocol_is_common ==true) ? (props[3] = 0x03):(props[3] = 0x02);
	props[4] = 0x06;
	memcpy(props + 5, (uint8_t *)device_version, 5);
	props[10] = 0x00;

	// sn
	(get_philips_running_status()->flash_param.philips_uart_protocol_is_common ==true) ? (props[11] = 0x04):(props[11] = 0x03);
	props[12] = 0x02;
	props[13] = (sn >> 8) & 0xFF;
	props[14] = sn & 0xFF;

	// total
	(get_philips_running_status()->flash_param.philips_uart_protocol_is_common ==true) ? (props[15] = 0x05):(props[15] = 0x04);
	props[16] = 0x02;
	props[17] = (total >> 8) & 0xFF;
	props[18] = total & 0xFF;

	return philips_send_Common(id, 0x08, props, 19);
}

/**
 * ID = 3, port = 8, propID = 3,5
 */
void philips_send_put_PutProps_otau_data(void)
{
	merr_t res = kNoErr;
	uint8_t props[134] = {0};

	memset(props, 0, sizeof(props));

	uint32_t total_number = 0;
	uint32_t datalen;

	// prop sn
	(get_philips_running_status()->flash_param.philips_uart_protocol_is_common ==true) ? (props[0] = 0x04):(props[0] = 0x03);
	props[1] = 0x02;
	props[2] = (get_philips_running_status()->otau_run_state.otau_current_package >> 8) & 0xFF;
	props[3] = get_philips_running_status()->otau_run_state.otau_current_package & 0xFF;

	// prop data
	(get_philips_running_status()->flash_param.philips_uart_protocol_is_common ==true) ? (props[4] = 0x06):(props[4] = 0x05);
	props[5] = 0x80;

	app_log("current_package = %d", get_philips_running_status()->otau_run_state.otau_current_package);

	if ( get_philips_running_status()->otau_run_state.otau_data_leave > 128 )
	{
		datalen = 128;
	}
	else if ( get_philips_running_status()->otau_run_state.otau_data_leave > 0 )
	{
		datalen = get_philips_running_status()->otau_run_state.otau_data_leave;
	}
	else
	{
		app_log("device_ota = %d", (get_philips_running_status()->otau_run_state.otau_current_package - 1));
		if ( get_philips_running_status()->otau_run_state.is_otau_ing )
		{
			get_philips_running_status()->otau_run_state.is_otau_ing = false;
			get_philips_running_status()->otau_flash_param.need_ota_device = false;
			mkv_item_set(CONFIG_DATA_KV_KEY_OTAU, &get_philips_running_status()->otau_flash_param, sizeof(get_philips_running_status()->otau_flash_param));
			get_philips_running_status()->flash_param.wifi_reboot_status = WRBT_unknow;
			mkv_item_set("pwrr_status", &get_philips_running_status()->flash_param.wifi_reboot_status, sizeof(get_philips_running_status()->flash_param.wifi_reboot_status));
		}
		philips_send_PutProps_otau_fours(DOID_BootOTAURequest, OPSV_downloaded, get_philips_running_status()->otau_flash_param.cloud_device_version, get_philips_running_status()->otau_run_state.otau_current_package, total_number);
		get_philips_running_status()->otau_run_state.otau_current_package = 0x00;
		return;
	}
	get_philips_running_status()->otau_run_state.otau_data_leave -= datalen;

	mhal_flash_read(MODULE_PARTITION_OTA_TEMP, &get_philips_running_status()->otau_run_state.otau_data_index, props + 6, datalen);

	res = philips_send_PutProps_Common(0x08, props, 134);
	if(res == kNoErr)
	{
		++get_philips_running_status()->otau_run_state.otau_current_package;
	}

	return;
}

/**
 * ID = 4
 * @param port
 */
static merr_t philips_send_GetProps(uint8_t port)
{
	merr_t res = kNoErr;
	
	queue_message_t queue_msg = {MT_UART_SEND, NULL, 8};
	res = philips_queue_message_init(&queue_msg);
	require_noerr(res, exit);

	queue_msg.data[0] = 0xFE;
	queue_msg.data[1] = 0xFF;
	queue_msg.data[2] = 0x04;
	queue_msg.data[3] = 0x00;
	queue_msg.data[4] = 0x01;
	queue_msg.data[5] = port;

	res = philips_uart_queue_push(&queue_msg);
	require_noerr(res, exit);

	exit:
	philips_queue_message_deinit(&queue_msg, res);
	return res;
}

void philips_notify_device_wifiui_init(void)
{
	philips_send_put_PutProps_wifiui(WPCV_notconnected, WPSV_inactive);

	return ;
}

/*******************************************************************************
 * philips_wait_for_all_initialized
 *******************************************************************************/
static mos_semphr_id_t device_otau_sem = NULL;
static void philips_wait_for_otau_info(void)
{
	if(device_otau_sem == NULL)
	{
		device_otau_sem = mos_semphr_new(1);
		require((device_otau_sem != NULL), exit);
	}

	for (int i = 0; i < 5; i++)
	{
		philips_send_GetProps(0x08);

		if (mos_semphr_acquire(device_otau_sem, 1000) == kNoErr)
		{
			break;
		}
	}

	mos_semphr_delete(device_otau_sem);
	device_otau_sem = NULL;

exit:
	return ;
}

static mos_semphr_id_t device_init_sem = NULL;
static void philips_wait_for_device_init(void)
{
	if(device_init_sem == NULL)
	{
		device_init_sem = mos_semphr_new(1);
		require((device_init_sem != NULL), exit);
	}

	while(mos_semphr_acquire(device_init_sem, 1000) != kNoErr)
	{
		philips_send_initialize();
	}

	mos_semphr_delete(device_init_sem);
	device_init_sem = NULL;

exit:
	return ;
}

static mos_semphr_id_t device_info_sem = NULL;
static void philips_wait_for_device_info(void)
{
	if(device_info_sem == NULL)
	{
		device_info_sem = mos_semphr_new(1);
		require(device_info_sem, exit);
	}

	while(mos_semphr_acquire(device_info_sem, 1000) != kNoErr)
	{
		philips_send_GetProps(0x01);
	}

	mos_semphr_delete(device_info_sem);
	device_info_sem = NULL;

exit:
	return ;
}

void philips_wait_for_all_initialized(void)
{
	app_log("======wifi_reboot_status:%d======", get_philips_running_status()->flash_param.wifi_reboot_status);

	switch(get_philips_running_status()->flash_param.wifi_reboot_status)
	{
		case WRBT_config:
		{
			app_log("======wifi setup start======");
			get_philips_running_status()->flash_param.wifi_reboot_status = WRBT_unknow;
			mkv_item_set("pwrr_status", &get_philips_running_status()->flash_param.wifi_reboot_status, sizeof(get_philips_running_status()->flash_param.wifi_reboot_status));
			
			philips_wait_for_otau_info();
			get_philips_running_status()->wifi_status = WRS_wait_for_device_info;
			philips_wait_for_device_info();
			get_philips_running_status()->wifi_status = WRS_init_complete;

			get_philips_running_status()->wifi_status = WRS_wifiui_config_ing;
			break;
		}
		case WRBT_factory:
		{
			app_log("======wifi factory start======");
			get_philips_running_status()->flash_param.wifi_reboot_status = WRBT_unknow;
			mkv_item_set("pwrr_status", &get_philips_running_status()->flash_param.wifi_reboot_status, sizeof(get_philips_running_status()->flash_param.wifi_reboot_status));

			get_philips_running_status()->factory_ing = true;
			get_philips_running_status()->factory_mode = FPPN_pcba;

			philips_send_PutProps_fac(FPPN_pcba, 0);

			// mos_msleep(3000);

			// if(get_philips_running_status()->factory_mode == FPPN_idle || get_philips_running_status()->factory_mode == FPPN_pcba)
			// {
			// 	get_philips_running_status()->factory_mode = FPPN_idle;
			// }
			// else
			// {
				while(1)
				{
					app_log("wifi_status:%d", get_philips_running_status()->wifi_status);
					mos_msleep(1000);
				}
			// }
			
			break;
		}
		case WRBT_wifi_ota:
		{
			app_log("======wifi wifi_ota start======");
			get_philips_running_status()->flash_param.wifi_reboot_status = WRBT_unknow;
			mkv_item_set("pwrr_status", &get_philips_running_status()->flash_param.wifi_reboot_status, sizeof(get_philips_running_status()->flash_param.wifi_reboot_status));
			
			philips_wait_for_otau_info();
			get_philips_running_status()->wifi_status = WRS_wait_for_device_info;
			philips_wait_for_device_info();
			get_philips_running_status()->wifi_status = WRS_init_complete;
			break;
		}
		case WRBT_device_ota:
		{
			app_log("======wifi mcu_ota start======");
			if(!get_philips_running_status()->otau_flash_param.is_otau_file_downloaded_from_cloud)
			{
				get_philips_running_status()->otau_flash_param.need_ota_device = false;
				get_philips_running_status()->otau_flash_param.is_otau_file_downloaded_from_cloud = false;
				get_philips_running_status()->otau_flash_param.otau_data_len = 0;
				mkv_item_set(CONFIG_DATA_KV_KEY_OTAU, &get_philips_running_status()->otau_flash_param, sizeof(get_philips_running_status()->otau_flash_param));
				get_philips_running_status()->flash_param.wifi_reboot_status = WRBT_unknow;
				mkv_item_set("pwrr_status", &get_philips_running_status()->flash_param.wifi_reboot_status, sizeof(get_philips_running_status()->flash_param.wifi_reboot_status));
				mos_msleep(300);
				app_log("mcu ota start transfer,device reboot");
				MxosSystemReboot();
			}

			get_philips_running_status()->otau_run_state.is_otau_ing = true;
			get_philips_running_status()->otau_run_state.need_get_otau = true;
			get_philips_running_status()->otau_run_state.device_otau_last_state = OPSV_reset;
			get_philips_running_status()->otau_run_state.otau_current_package = -1;
			get_philips_running_status()->otau_run_state.otau_total_package = get_philips_running_status()->otau_flash_param.otau_data_len / 128;
			if (get_philips_running_status()->otau_run_state.otau_total_package * 128 < get_philips_running_status()->otau_flash_param.otau_data_len)
			{
				++get_philips_running_status()->otau_run_state.otau_total_package;
			}
			get_philips_running_status()->otau_run_state.otau_data_leave = get_philips_running_status()->otau_flash_param.otau_data_len;
			get_philips_running_status()->otau_run_state.otau_data_index = 0;

			while(1)
			{
				mos_msleep(1000);
			}
			break;
		}
		case WRBT_unknow:
		default:
		{
			app_log("======wifi normal start======");
			philips_wait_for_otau_info();
			get_philips_running_status()->wifi_status = WRS_wait_for_device_init;
			philips_wait_for_device_init();
			get_philips_running_status()->wifi_status = WRS_wait_for_device_info;
			philips_wait_for_device_info();
			get_philips_running_status()->wifi_status = WRS_init_complete;

			get_philips_running_status()->wifi_status = WRS_wifiui_idle;
			get_philips_running_status()->wifi_status = WRS_wifiui_idle_out;
			// while (get_philips_running_status()->wifi_status == WRS_wifiui_idle)
			// {
			// 	mos_msleep(200);
			// }
			break;
		}
	}
	return;
}

/*******************************************************************************
 * philips uart protocol process use api
 *******************************************************************************/

// 台湾设备路由器连接失败，重试间隔加长，降低功耗
void philips_check_power_save(void)
{
	if(get_philips_running_status()->is_rf_power_save && strcmp(purifier_status.pwr_str, "0") == 0)
	{
		mos_sleep(30);
	}
}

extern system_context_t *sys_context;

void philips_enter_network_setup(void)
{
	require_quiet( sys_context, exit );

	mos_mutex_lock(sys_context->flashContentInRam_mutex);
	// if(sys_context->flashContentInRam.mxos_config.configured == allConfigured)
	// {
		sys_context->flashContentInRam.mxos_config.configured = wLanUnConfigured;
	// }
	mos_mutex_unlock(sys_context->flashContentInRam_mutex);

	mxos_system_power_perform( &sys_context->flashContentInRam, eState_Software_Reset );

exit:
	return;
}

void philips_kv_partition_erase(void)
{
	mxos_Context_t* context = NULL;
	mxos_logic_partition_t *partition = NULL;

	context = mxos_system_context_get( );
	require( context, exit );

	partition = mhal_flash_get_info( MODULE_PARTITION_KV );
	mhal_flash_erase( MODULE_PARTITION_KV ,0x0, partition->partition_length );

	mxos_system_power_perform( context, eState_Software_Reset );

exit:
	return;
}

merr_t philips_device_reset(void)
{
	merr_t err = kGeneralErr;

	app_log("philips_device_reset");

	philips_kv_partition_erase();

	err = kNoErr;
	return err;
}

void philips_user_async_reboot_thread(void *arg)
{
	uint32_t time_out = 500;

	app_log("===philips_user_async_reboot===");
	mos_msleep(time_out);
	MxosSystemReboot();
}

merr_t philips_user_async_reboot(uint32_t time_out)
{
	merr_t err = kNoErr;
	
	mos_thread_id_t async_reboot_thread_handle = NULL;

    async_reboot_thread_handle = mos_thread_new(MOS_APPLICATION_PRIORITY, "user async reboot", philips_user_async_reboot_thread, 1024, NULL);
	require_action( (async_reboot_thread_handle != NULL), exit, err = kGeneralErr);

exit:
	return err;
}

/*******************************************************************************
 * philips uart protocol process api
 *******************************************************************************/

static void philips_uart_protocol_port1_process(uint8_t *buf, int buflen)
{
	merr_t err = kNoErr;

	const uint8_t *p = buf + 7;

	//device info port
	char device_modelid[33] = {0};
	char ssid[33];
	char key[33];
	char wfversion[7];
	bool need_to_send_mac_wfversion = false;

	while (*(p) != 0x00)
	{
		switch(*(p))
		{
			case 0x01:	//name
			{
				if(get_philips_running_status()->device_name[0] != '\0')	//首次上电
				{

				}
				else
				{
					need_to_send_mac_wfversion = true;
				}
				
				if (*(p + 1) < sizeof(get_philips_running_status()->device_name))
				{
					memset(get_philips_running_status()->device_name, 0x00, sizeof(get_philips_running_status()->device_name));
					memcpy(get_philips_running_status()->device_name, (char *)(p + 2), *(p + 1));
				}
				else
				{
					app_log("%s out of rang,length:%d", "name", *(p + 1));
					memset(get_philips_running_status()->device_name, 0x00, sizeof(get_philips_running_status()->device_name));
					memcpy(get_philips_running_status()->device_name, (char *)(p + 2), sizeof(get_philips_running_status()->device_name) - 1);
				}
			
				break;
			}
			case 0x02:	//type
			{
				if (*(p + 1) < sizeof(get_philips_running_status()->device_type))
				{
					memset(get_philips_running_status()->device_type, 0x00, sizeof(get_philips_running_status()->device_type));
					memcpy(get_philips_running_status()->device_type, (char *)(p + 2), *(p + 1));
				}
				else
				{
					app_log("%s out of rang,length:%d", "type", *(p + 1));
					memset(get_philips_running_status()->device_type, 0x00, sizeof(get_philips_running_status()->device_type));
					memcpy(get_philips_running_status()->device_type, (char *)(p + 2), sizeof(get_philips_running_status()->device_type) - 1);
				}
				break;
			}
			case 0x03:	//modelid
			{
				if (*(p + 1) < sizeof(device_modelid))
				{
					memset(device_modelid, 0x00, sizeof(device_modelid));
					memcpy(device_modelid, (char *)(p + 2), *(p + 1));
				}
				else
				{
					app_log("%s out of rang,length:%d", "modelid", *(p + 1));
					memset(device_modelid, 0x00, sizeof(device_modelid));
					memcpy(device_modelid, (char *)(p + 2), sizeof(device_modelid) - 1);
				}
				break;
			}
			case 0x04:	//swversion
			{
				if (*(p + 1) < sizeof(get_philips_running_status()->device_swversion))
				{
					memset(get_philips_running_status()->device_swversion, 0x00, sizeof(get_philips_running_status()->device_swversion));
					memcpy(get_philips_running_status()->device_swversion, (char *)(p + 2), *(p + 1));
				}
				else
				{
					app_log("%s out of rang,length:%d", "swversion", *(p + 1));
					memset(get_philips_running_status()->device_swversion, 0x00, sizeof(get_philips_running_status()->device_swversion));
					memcpy(get_philips_running_status()->device_swversion, (char *)(p + 2), sizeof(get_philips_running_status()->device_swversion) - 1);
				}
				break;
			}
			case 0x05:	//ssid
			{
				if (*(p + 1) < sizeof(ssid))
				{
					memset(ssid, 0x00, sizeof(ssid));
					memcpy(ssid, (char *)(p + 2), *(p + 1));
				}
				else
				{
					app_log("%s out of rang,length:%d", "ssid", *(p + 1));
					memset(ssid, 0x00, sizeof(ssid));
					memcpy(ssid, (char *)(p + 2), sizeof(ssid) - 1);
				}
				break;
			}
			case 0x06:	//key
			{
				if (*(p + 1) < sizeof(key))
				{
					memset(key, 0x00, sizeof(key));
					memcpy(key, (char *)(p + 2), *(p + 1));
				}
				else
				{
					app_log("%s out of rang,length:%d", "key", *(p + 1));
					memset(key, 0x00, sizeof(key));
					memcpy(key, (char *)(p + 2), sizeof(key) - 1);
				}
				break;
			}
			case 0x07:	//wfversion
			{
				if (*(p + 1) < sizeof(wfversion))
				{
					memset(wfversion, 0x00, sizeof(wfversion));
					memcpy(wfversion, (char *)(p + 2), *(p + 1));
				}
				else
				{
					app_log("%s out of rang,length:%d", "wfversion", *(p + 1));
					memset(wfversion, 0x00, sizeof(wfversion));
					memcpy(wfversion, (char *)(p + 2), sizeof(wfversion) - 1);
				}
				break;
			}
			case 0x08:	//language
			{
				if ( *(p + 2) == 0x00 )
				{
					memset(get_philips_running_status()->device_language, 0x00, sizeof(get_philips_running_status()->device_language));
					strcpy(get_philips_running_status()->device_language, "EN");
				}
				else
				{
					if (*(p + 1) < sizeof(get_philips_running_status()->device_language))
					{
						memset(get_philips_running_status()->device_language, 0x00, sizeof(get_philips_running_status()->device_language));
						memcpy(get_philips_running_status()->device_language, (char *)(p + 2), *(p + 1));
					}
					else
					{
						app_log("%s out of rang,length:%d", "language", *(p + 1));
						memset(get_philips_running_status()->device_language, 0x00, sizeof(get_philips_running_status()->device_language));
						memcpy(get_philips_running_status()->device_language, (char *)(p + 2), sizeof(get_philips_running_status()->device_language) - 1);
					}
				}
				break;
			}
			default:
				break;
		}
		p = p + 2 + *(p + 1);
	}

	app_log("processing modelid = %s, %s", device_modelid, get_philips_running_status()->flash_param.device_modelid);
	if ( get_philips_running_status()->range_id == PHILIPS_RANGE_NONE || 
			get_philips_running_status()->flash_param.product_id[0] == '\0' || 
			strncmp(get_philips_running_status()->flash_param.device_modelid, device_modelid, sizeof(get_philips_running_status()->flash_param.device_modelid)) != 0 )
	{
		strncpy(get_philips_running_status()->flash_param.device_modelid, device_modelid, sizeof(get_philips_running_status()->flash_param.device_modelid));
		app_log("processing modelid = %s, %s", device_modelid, get_philips_running_status()->flash_param.device_modelid);
		mkv_item_set("pda_mid", get_philips_running_status()->flash_param.device_modelid, sizeof(get_philips_running_status()->flash_param.device_modelid));

		if(get_philips_running_status()->flash_param.device_modelid[6] == '/' && get_philips_running_status()->flash_param.device_modelid[7] == '0')
		{
			if (get_philips_running_status()->flash_param.url_type != PHILIPS_URL_TYPE_CHINA)
			{
				get_philips_running_status()->flash_param.url_type = PHILIPS_URL_TYPE_CHINA;
			}
		}
		else
		{
			if (get_philips_running_status()->flash_param.url_type != PHILIPS_URL_TYPE_FRANKFORD)
			{
				get_philips_running_status()->flash_param.url_type = PHILIPS_URL_TYPE_FRANKFORD;
			}
		}
		mkv_item_set("pda_urltype", &get_philips_running_status()->flash_param.url_type, sizeof(get_philips_running_status()->flash_param.url_type));

		app_log("=========flash param check fail,need reactive=========");

		get_philips_running_status()->need_https = true;
	}

	//O2特殊处理的型号
	if ( strncmp(get_philips_running_status()->flash_param.device_modelid, "AC4556/00", 9) == 0 )
	{
		if ( strncmp(get_philips_running_status()->device_swversion, "1.0.2", 5) == 0 )
		{
			if (!get_philips_running_status()->o2_filter_flash_param.need_transform_filter_life)
			{
				get_philips_running_status()->o2_filter_flash_param.need_transform_filter_life = true;
				get_philips_running_status()->o2_filter_flash_param.filter0_life = 0x0168;
				get_philips_running_status()->o2_filter_flash_param.filter0_lastadjustvalue = 0xFF;
				get_philips_running_status()->o2_filter_flash_param.filter1_life = 0x12C0;
				get_philips_running_status()->o2_filter_flash_param.filter1_lastadjustvalue = 0xFF;
				get_philips_running_status()->o2_filter_flash_param.ms_count = 0;
				philips_o2_filter_flash_data_update();
			}
			start_filter_transform_O2();
		}
		else
		{
			if(get_philips_running_status()->o2_filter_flash_param.need_transform_filter_life)
			{
				get_philips_running_status()->o2_filter_flash_param.need_transform_filter_life = false;
				philips_o2_filter_flash_data_clear();
			}
		}
	}

	// rfpowersave for TaiWan China
	if(get_philips_running_status()->flash_param.device_modelid[6] == '/' && get_philips_running_status()->flash_param.device_modelid[7] == '8')
	{
		app_log("Taiwan Device, open power save mode");
		get_philips_running_status()->is_rf_power_save = true;
		mwifi_ps_on();
	}

	//正常上电需要下发mac\type\wfversion
	if(need_to_send_mac_wfversion)
	{
		app_log("need_to_send_mac_wfversion");
		philips_send_PutProps_mac();
		// mos_msleep(150);
		philips_send_PutProps_device_wfversion();
	}

	app_log("===== ===== DEVICE ===== =====");
	app_log("name                         = %s", get_philips_running_status()->device_name);
	app_log("type                         = %s", get_philips_running_status()->device_type);
	app_log("flash modelid                = %s", get_philips_running_status()->flash_param.device_modelid);
	app_log("swversion                    = %s", get_philips_running_status()->device_swversion);
	app_log("ssid                         = %s", ssid);
	app_log("key                          = %s", key);
	app_log("wfversion                    = %s", wfversion);
	app_log("language                     = %s", get_philips_running_status()->device_language);
	app_log("===== ===== ====== ===== =====");

	if(get_philips_running_status()->need_get_deviceinfo)
	{
		get_philips_running_status()->need_get_deviceinfo = false;
		get_philips_running_status()->device_upload_manage.status_change = true;
		device_status_upload_manage();	//上报一次设备信息到云端
	}
	
	if (get_philips_running_status()->wifi_status == WRS_wait_for_device_info)
	{
		if (device_info_sem)
		{
			err = mos_semphr_release(device_info_sem);
			if (err != kNoErr)
				app_log("info sem set fail");
		}
	}

	return ;
}

static void philips_uart_common_protocol_port1_process(uint8_t *buf, int buflen)
{
	merr_t err = kNoErr;

	const uint8_t *p = buf + 7;

	//device info port
	char device_modelid[33] = {0};
	char ssid[33];
	char key[33];
	char wfversion[7];
	bool need_to_send_mac_wfversion = false;

	uint16_t port_polling_interval=0;

	while (*(p) != 0x00)
	{
		switch(*(p))
		{
			case 0x01:
				//port_polling_interval=((uint16_t)*(p + 2)<<8) | (*(p + 3));
				break;
			case 0x02:
				memset(get_philips_running_status()->device_category, 0x00, sizeof(get_philips_running_status()->device_category));
				
				switch(*(p+2))
				{
					case 0: strcpy(get_philips_running_status()->device_category, "Purifier product");break;
					case 1: strcpy(get_philips_running_status()->device_category, "Humidifier product");break;
					case 2: strcpy(get_philips_running_status()->device_category, "Floor Care product");break;
				}break;	
				app_log("device_category:%s", get_philips_running_status()->device_category);					
			case 0x03:	//name
			{
				if(get_philips_running_status()->device_name[0] != '\0')	//首次上电
				{

				}
				else
				{
					need_to_send_mac_wfversion = true;
				}
				
				if (*(p + 1) < sizeof(get_philips_running_status()->device_name))
				{
					memset(get_philips_running_status()->device_name, 0x00, sizeof(get_philips_running_status()->device_name));
					memcpy(get_philips_running_status()->device_name, (char *)(p + 2), *(p + 1));
				}
				else
				{
					app_log("%s out of rang,length:%d", "name", *(p + 1));
					memset(get_philips_running_status()->device_name, 0x00, sizeof(get_philips_running_status()->device_name));
					memcpy(get_philips_running_status()->device_name, (char *)(p + 2), sizeof(get_philips_running_status()->device_name) - 1);
				}
				break;
			}
			case 0x04:	//type
			{
				if (*(p + 1) < sizeof(get_philips_running_status()->device_type))
				{
					memset(get_philips_running_status()->device_type, 0x00, sizeof(get_philips_running_status()->device_type));
					memcpy(get_philips_running_status()->device_type, (char *)(p + 2), *(p + 1));
				}
				else
				{
					app_log("%s out of rang,length:%d", "type", *(p + 1));
					memset(get_philips_running_status()->device_type, 0x00, sizeof(get_philips_running_status()->device_type));
					memcpy(get_philips_running_status()->device_type, (char *)(p + 2), sizeof(get_philips_running_status()->device_type) - 1);
				}
				break;
			}
			case 0x05:	//modelid
			{
				if (*(p + 1) < sizeof(device_modelid))
				{
					memset(device_modelid, 0x00, sizeof(device_modelid));
					memcpy(device_modelid, (char *)(p + 2), *(p + 1));
				}
				else
				{
					app_log("%s out of rang,length:%d", "modelid", *(p + 1));
					memset(device_modelid, 0x00, sizeof(device_modelid));
					memcpy(device_modelid, (char *)(p + 2), sizeof(device_modelid) - 1);
				}
				break;
			}
			case 0x07:	//language
			{
				memset(get_philips_running_status()->device_language, 0x00, sizeof(get_philips_running_status()->device_language));
				
				switch(*(p+2))
				{
					case 0: strcpy(get_philips_running_status()->device_language, "English");break;
					case 1: strcpy(get_philips_running_status()->device_language, "Simplified Chinese");break;
					case 2: strcpy(get_philips_running_status()->device_language, "Traditional Chinese-HK");break;
					case 3: strcpy(get_philips_running_status()->device_language, "Traditional Chinese-TW");break;
					case 4: strcpy(get_philips_running_status()->device_language, "Arabic");break;
					case 5: strcpy(get_philips_running_status()->device_language, "German");break;
					case 6: strcpy(get_philips_running_status()->device_language, "Spanish");break;
					case 7: strcpy(get_philips_running_status()->device_language, "French");break;
					case 8: strcpy(get_philips_running_status()->device_language, "Japanese");break;
					case 9: strcpy(get_philips_running_status()->device_language, "Russian");break;
					case 10: strcpy(get_philips_running_status()->device_language, "English");break;
				}
				break;	
			}
			case 0x08:
				get_philips_running_status()->wifi_protocol_ver=*(p+2);
			break;
			case 0x09:
				get_philips_running_status()->OTAU_protocol_ver=*(p+2);
			break;
			case 0x10:	//wfversion
			{
				if (*(p + 1) < sizeof(wfversion))
				{
					memset(wfversion, 0x00, sizeof(wfversion));
					memcpy(wfversion, (char *)(p + 2), *(p + 1));
				}
				else
				{
					app_log("%s out of rang,length:%d", "wfversion", *(p + 1));
					memset(wfversion, 0x00, sizeof(wfversion));
					memcpy(wfversion, (char *)(p + 2), sizeof(wfversion) - 1);
				}
				break;
			}
			case 0x16:
				if (*(p + 1) < sizeof(get_philips_running_status()->node1bootloaderver))
				{
					memset(get_philips_running_status()->node1bootloaderver, 0x00, sizeof(get_philips_running_status()->node1bootloaderver));
					memcpy(get_philips_running_status()->node1bootloaderver, (char *)(p + 2), *(p + 1));
				}
				else
				{
					app_log("%s out of rang,length:%d", "type", *(p + 1));
					memset(get_philips_running_status()->node1bootloaderver, 0x00, sizeof(get_philips_running_status()->node1bootloaderver));
					memcpy(get_philips_running_status()->node1bootloaderver, (char *)(p + 2), sizeof(get_philips_running_status()->node1bootloaderver) - 1);
				}
				break;
			break;
			case 0x17:
				if (*(p + 1) < sizeof(get_philips_running_status()->node1applicationver))
				{
					memset(get_philips_running_status()->node1applicationver, 0x00, sizeof(get_philips_running_status()->node1applicationver));
					memcpy(get_philips_running_status()->node1applicationver, (char *)(p + 2), *(p + 1));
				}
				else
				{
					app_log("%s out of rang,length:%d", "type", *(p + 1));
					memset(get_philips_running_status()->node1applicationver, 0x00, sizeof(get_philips_running_status()->node1applicationver));
					memcpy(get_philips_running_status()->node1applicationver, (char *)(p + 2), sizeof(get_philips_running_status()->node1applicationver) - 1);
				}
				break;
			break;
		
			default:
				break;
		}
		p = p + 2 + *(p + 1);
	}

	app_log("processing modelid = %s, %s", device_modelid, get_philips_running_status()->flash_param.device_modelid);
	if ( get_philips_running_status()->range_id == PHILIPS_RANGE_NONE || 
			get_philips_running_status()->flash_param.product_id[0] == '\0' || 
			strncmp(get_philips_running_status()->flash_param.device_modelid, device_modelid, sizeof(get_philips_running_status()->flash_param.device_modelid)) != 0 )
	{
		strncpy(get_philips_running_status()->flash_param.device_modelid, device_modelid, sizeof(get_philips_running_status()->flash_param.device_modelid));
		app_log("processing modelid = %s, %s", device_modelid, get_philips_running_status()->flash_param.device_modelid);
		mkv_item_set("pda_mid", get_philips_running_status()->flash_param.device_modelid, sizeof(get_philips_running_status()->flash_param.device_modelid));

		if(get_philips_running_status()->flash_param.device_modelid[6] == '/' && get_philips_running_status()->flash_param.device_modelid[7] == '0')
		{
			if (get_philips_running_status()->flash_param.url_type != PHILIPS_URL_TYPE_CHINA)
			{
				get_philips_running_status()->flash_param.url_type = PHILIPS_URL_TYPE_CHINA;
			}
		}
		else
		{
			if (get_philips_running_status()->flash_param.url_type != PHILIPS_URL_TYPE_FRANKFORD)
			{
				get_philips_running_status()->flash_param.url_type = PHILIPS_URL_TYPE_FRANKFORD;
			}
		}
		mkv_item_set("pda_urltype", &get_philips_running_status()->flash_param.url_type, sizeof(get_philips_running_status()->flash_param.url_type));

		app_log("=========flash param check fail,need reactive=========");

		get_philips_running_status()->need_https = true;
	}

	//正常上电需要下发mac\type\wfversion
	if(need_to_send_mac_wfversion)
	{
		app_log("need_to_send_mac_wfversion");
		philips_send_PutProps_mac();
		// mos_msleep(150);
		if(!get_philips_running_status()->flash_param.philips_uart_protocol_is_common)
			philips_send_PutProps_device_wfversion();
	}

	app_log("===== ===== DEVICE ===== =====");
	app_log("category                     = %s", get_philips_running_status()->device_category);
	app_log("name                         = %s", get_philips_running_status()->device_name);
	app_log("productrange                 = %s", get_philips_running_status()->device_type);
	app_log("flash modelid                = %s", get_philips_running_status()->flash_param.device_modelid);
	app_log("swversion                    = %s", get_philips_running_status()->device_swversion);
	app_log("wifiprotocolversion          = %d", get_philips_running_status()->wifi_protocol_ver);
	app_log("OTAUprotocolversion          = %d", get_philips_running_status()->OTAU_protocol_ver);
	app_log("node0bootloaderversion       = %s", get_philips_running_status()->node1bootloaderver);
	app_log("node0applicationversion      = %s", get_philips_running_status()->node1applicationver);
	app_log("wfversion                    = %s", wfversion);
	app_log("language                     = %s", get_philips_running_status()->device_language);
	app_log("===== ===== ====== ===== =====");

	if(get_philips_running_status()->need_get_deviceinfo)
	{
		get_philips_running_status()->need_get_deviceinfo = false;
		get_philips_running_status()->device_upload_manage.status_change = true;
		device_status_upload_manage();	//上报一次设备信息到云端
	}
	
	if (get_philips_running_status()->wifi_status == WRS_wait_for_device_info)
	{
		if (device_info_sem)
		{
			err = mos_semphr_release(device_info_sem);
			if (err != kNoErr)
				app_log("info sem set fail");
		}
	}

	return ;
}

static void philips_uart_protocol_port2_process(uint8_t *buf, int buflen)
{
	merr_t err = kNoErr;

	const uint8_t *p = buf + 7;

	//wifi_ui port
	uint8_t connection = WPCV_Unknown;
	uint8_t setup = WPSV_Unknown;

	while (*(p) != 0x00)
	{
		if(get_philips_running_status()->flash_param.philips_uart_protocol_is_common)
		{
			case_val=*(p)-1;
		}
		switch (case_val)
		{
			case 0x01: //connection
			{
				connection = *(p + 2);
				break;
			}
			case 0x02: //setup
			{
				setup = *(p + 2);
				break;
			}
			default:
				break;
		}
		p = p + 2 + *(p + 1);
	}

	app_log("connection = %d, setup = %d, wifi_status = %d", connection, setup, get_philips_running_status()->wifi_status);
	// idle
	if ( connection == WPCV_notconnected && setup == WPSV_inactive )
	{
		switch ( get_philips_running_status()->wifi_status )
		{
		case WRS_wifiui_idle:
			break;
		case WRS_wifiui_config_ing:
			// fog_wifi_config_stop();
			break;
		case WRS_wifiui_config_app_connected:
			break;
		case WRS_wifiui_connect_ing:
			break;
		case WRS_wifiui_config_timeout:
			break;
		case WRS_wifiui_connect_router:
		case WRS_wifiui_connect_cloud:
			 philips_send_put_PutProps_wifiui(WPCV_connected, WPSV_inactive);
			break;
		default:
			break;
		}
	}
	// start config
	else if ( connection == WPCV_notconnected && setup == WPSV_requested )
	{
		switch ( get_philips_running_status()->wifi_status )
		{
		case WRS_wifiui_idle:
			get_philips_running_status()->wifi_status = WRS_wifiui_config_ing;
			break;
		case WRS_wifiui_config_ing:
			if ( mos_time() < PHILIPS_CONFIG_WIFI_IGNORE_TIME )
			{
				philips_send_put_PutProps_wifiui(WPCV_notconnected, WPSV_active);
				break;
			}
		case WRS_wifiui_config_app_connected:
		case WRS_wifiui_connect_ing:
		case WRS_wifiui_config_timeout:
		case WRS_wifiui_connect_router:
		case WRS_wifiui_connect_cloud:
		default:
		{
			app_log("=========enter network config=========");
			get_philips_running_status()->flash_param.wifi_reboot_status = WRBT_config;
			mkv_item_set("pwrr_status", &get_philips_running_status()->flash_param.wifi_reboot_status, sizeof(get_philips_running_status()->flash_param.wifi_reboot_status));
			mos_msleep(100);
			philips_enter_network_setup();
			mos_msleep(200);
			// MxosSystemReboot();
			break;
		}
		}
	}
	// configing
	else if ( connection == WPCV_notconnected && setup == WPSV_active )
	{
		switch ( get_philips_running_status()->wifi_status )
		{
		case WRS_wifiui_idle:
			get_philips_running_status()->wifi_status = WRS_wifiui_idle_out;
			break;
		case WRS_wifiui_config_ing:
			break;
		case WRS_wifiui_config_app_connected:
			philips_send_put_PutProps_wifiui(WPCV_connected, WPSV_active);
			break;
		case WRS_wifiui_config_timeout:
			philips_send_put_PutProps_wifiui(WPCV_notconnected, WPSV_inactive);
			break;
		case WRS_wifiui_connect_ing:
			philips_send_put_PutProps_wifiui(WPCV_connecting, WPSV_inactive);
			break;
		case WRS_wifiui_connect_router:
		case WRS_wifiui_connect_cloud:
			philips_send_put_PutProps_wifiui(WPCV_connected, WPSV_inactive);
			break;
		default:
			break;
		}
	}
	// start connect
	else if ( connection == WPCV_requested && setup == WPSV_inactive )
	{
		switch ( get_philips_running_status()->wifi_status )
		{
		case WRS_wifiui_idle:
			get_philips_running_status()->wifi_status = WRS_wifiui_idle_out;
			break;
		case WRS_wifiui_config_ing:
			philips_send_put_PutProps_wifiui(WPCV_notconnected, WPSV_active);
			break;
		case WRS_wifiui_config_app_connected:
			philips_send_put_PutProps_wifiui(WPCV_connected, WPSV_active);
			break;
		case WRS_wifiui_config_timeout:
			philips_send_put_PutProps_wifiui(WPCV_notconnected, WPSV_inactive);
			break;
		case WRS_wifiui_connect_ing:
			philips_send_put_PutProps_wifiui(WPCV_connecting, WPSV_inactive);
			break;
		case WRS_wifiui_connect_router:
		case WRS_wifiui_connect_cloud:
			philips_send_put_PutProps_wifiui(WPCV_connected, WPSV_inactive);
			break;
		default:
			break;
		}
	}
	// connecting
	else if ( connection == WPCV_connecting && setup == WPSV_inactive )
	{
		switch ( get_philips_running_status()->wifi_status )
		{
		case WRS_wifiui_idle:
			get_philips_running_status()->wifi_status = WRS_wifiui_idle_out;
			break;
		case WRS_wifiui_config_ing:
			philips_send_put_PutProps_wifiui(WPCV_notconnected, WPSV_active);
			break;
		case WRS_wifiui_config_app_connected:
			philips_send_put_PutProps_wifiui(WPCV_connected, WPSV_active);
			break;
		case WRS_wifiui_config_timeout:
			philips_send_put_PutProps_wifiui(WPCV_notconnected, WPSV_inactive);
			break;
		case WRS_wifiui_connect_ing:
			break;
		case WRS_wifiui_connect_router:
		case WRS_wifiui_connect_cloud:
			philips_send_put_PutProps_wifiui(WPCV_connected, WPSV_inactive);
			break;
		default:
			break;
		}
	}
	// connected
	else if ( connection == WPCV_connected && setup == WPSV_inactive )
	{
		switch ( get_philips_running_status()->wifi_status )
		{
		case WRS_wifiui_idle:
			get_philips_running_status()->wifi_status = WRS_wifiui_idle_out;
			break;
		case WRS_wifiui_config_ing:
			philips_send_put_PutProps_wifiui(WPCV_notconnected, WPSV_active);
			break;
		case WRS_wifiui_config_app_connected:
			philips_send_put_PutProps_wifiui(WPCV_connected, WPSV_active);
			break;
		case WRS_wifiui_config_timeout:
			philips_send_put_PutProps_wifiui(WPCV_notconnected, WPSV_inactive);
			break;
		case WRS_wifiui_connect_ing:
			philips_send_put_PutProps_wifiui(WPCV_connecting, WPSV_inactive);
			break;
		case WRS_wifiui_connect_router:
		case WRS_wifiui_connect_cloud:
			break;
		default:
			break;
		}
	}
	// config app connected
	else if ( connection == WPCV_connected && setup == WPSV_active )
	{
		switch ( get_philips_running_status()->wifi_status )
		{
		case WRS_wifiui_idle:
			get_philips_running_status()->wifi_status = WRS_wifiui_idle_out;
			break;
		case WRS_wifiui_config_ing:
			philips_send_put_PutProps_wifiui(WPCV_notconnected, WPSV_active);
			break;
		case WRS_wifiui_config_app_connected:
			break;
		case WRS_wifiui_config_timeout:
			philips_send_put_PutProps_wifiui(WPCV_notconnected, WPSV_inactive);
			break;
		case WRS_wifiui_connect_ing:
			philips_send_put_PutProps_wifiui(WPCV_connecting, WPSV_inactive);
			break;
		case WRS_wifiui_connect_router:
		case WRS_wifiui_connect_cloud:
			philips_send_put_PutProps_wifiui(WPCV_connected, WPSV_inactive);
			break;
		default:
			break;
		}
	}
	// others
	else
	{
		switch ( get_philips_running_status()->wifi_status )
		{
		case WRS_wifiui_idle:
			get_philips_running_status()->wifi_status = WRS_wifiui_idle_out;
			break;
		case WRS_wifiui_config_ing:
			philips_send_put_PutProps_wifiui(WPCV_notconnected, WPSV_active);
			break;
		case WRS_wifiui_config_app_connected:
			philips_send_put_PutProps_wifiui(WPCV_connected, WPSV_active);
			break;
		case WRS_wifiui_config_timeout:
			philips_send_put_PutProps_wifiui(WPCV_notconnected, WPSV_inactive);
			break;
		case WRS_wifiui_connect_ing:
			philips_send_put_PutProps_wifiui(WPCV_connecting, WPSV_inactive);
			break;
		case WRS_wifiui_connect_router:
		case WRS_wifiui_connect_cloud:
			philips_send_put_PutProps_wifiui(WPCV_connected, WPSV_inactive);
			break;
		default:
			break;
		}
	}
}

static void philips_uart_protocol_port4_process(uint8_t *buf, int buflen)
{
	merr_t err = kNoErr;

	const uint8_t *p = buf + 7;

	//factory port
	uint8_t fac_pcba = 0x00;
	uint8_t fac_wifi = 0x00;
	uint8_t fac_reset = 0x00;

	while (*(p) != 0x00)
	{
		if(get_philips_running_status()->flash_param.philips_uart_protocol_is_common)
			case_val=*(p)-1;
		switch (case_val)
		{
		case 0x01: //pcba
		{
			fac_pcba = *(p + 2);
			break;
		}
		case 0x02: //WiFi
		{
			fac_wifi = *(p + 2);
			break;
		}
		case 0x03: //reset
		{
			fac_reset = *(p + 2);
			break;
		}
		default:
			break;
		}

		p = p + 2 + *(p + 1);
	}

	if (fac_pcba == 0x01 && fac_wifi == 0x00 && fac_reset == 0x00)
	{
		app_log("=========factory mode pcba=========");
		get_philips_running_status()->flash_param.wifi_reboot_status = WRBT_factory;
		mkv_item_set("pwrr_status", &get_philips_running_status()->flash_param.wifi_reboot_status, sizeof(get_philips_running_status()->flash_param.wifi_reboot_status));
		mos_msleep(500);
		app_log("start factory mode,device reboot");
		MxosSystemReboot();
	}
	else if (fac_pcba == 0x00 && fac_wifi == 0x01 && fac_reset == 0x00)
	{
		app_log("=========factory mode wifi=========");
		if(!get_fogid_from_sds_partition())
		{
			philips_send_PutProps_fac(FPPN_wifi, 1);
		}
		else
		{
			if(get_philips_running_status()->factory_mode == FPPN_pcba)
			{
				get_philips_running_status()->factory_mode = FPPN_wifi;
				philips_device_factory_mode_wifi();
			}
		}
	}
	else if (fac_pcba == 0x00 && fac_wifi == 0x00 && fac_reset == 0x01)
	{
		app_log("=========factory mode reset=========");
		if(get_philips_running_status()->factory_mode == FPPN_wifi)
		{
			get_philips_running_status()->factory_mode = FPPN_reset;
			philips_device_factory_mode_reset();
		}
	}
	else
	{
		get_philips_running_status()->factory_mode = FPPN_idle;
	}
}

static void philips_uart_protocol_port8_process(uint8_t *buf, int buflen)
{
	merr_t err = kNoErr;

	const uint8_t *p = buf + 7;

	//otau port
	char version[6] = {0};
	uint8_t otaustate = 0;

	while (*(p) != 0)
	{
		if(get_philips_running_status()->flash_param.philips_uart_protocol_is_common)
			case_val=*(p)-1;
		switch (case_val)
		{
		case 0x01:
		{
			otaustate = *(p + 2);
			break;
		}
		case 0x02:
		{
			if (*(p + 1) < sizeof(version))
			{
				memset(version, 0x00, sizeof(version));
				strncpy(version, (char *)(p + 2), *(p + 1));
			}
			else
			{
				memset(version, 0x00, sizeof(version));
				strncpy(version, (char *)(p + 2), sizeof(version) - 1);
			}
			break;
		}
		default:
			break;
		}
		p = p + 2 + *(p + 1);
	}

	app_log("version is %s", version);
	app_log("otaustate is %d", otaustate);

	// 更新获取的设备固件版本号
	if (strncmp(get_philips_running_status()->otau_run_state.local_device_version, version, 6) != 0)
	{
		memset(get_philips_running_status()->otau_run_state.local_device_version, 0x00, sizeof(get_philips_running_status()->otau_run_state.local_device_version));
		strncpy(get_philips_running_status()->otau_run_state.local_device_version, version, sizeof(get_philips_running_status()->otau_run_state.local_device_version) - 1);
	}

	// 非OTAU重启，并且第一次接收到idle后，发送idle模式，让设备从BOOT进入APPLICATION。
	if ( !get_philips_running_status()->otau_run_state.need_get_otau && !get_philips_running_status()->otau_run_state.is_send_idle_before_set_device_otau_sem )
	{
		memset(get_philips_running_status()->otau_run_state.local_device_version, 0x00, sizeof(get_philips_running_status()->otau_run_state.local_device_version));
		strncpy(get_philips_running_status()->otau_run_state.local_device_version, version, sizeof(get_philips_running_status()->otau_run_state.local_device_version) - 1);
		if ( strncmp(get_philips_running_status()->otau_run_state.local_device_version, "0.0.0", 5) == 0 )
		{
			app_log("local_device_version:0.0.0");
			if ( get_philips_running_status()->otau_flash_param.is_otau_file_downloaded_from_cloud )
			{
				app_log("is_otau_file_downloaded_from_cloud");
			}
			else
			{
				philips_send_PutProps_otau_state(DOID_PutpropsRequest, OPSV_idle);
			}
		}
		else
		{
			philips_send_PutProps_otau_state(DOID_PutpropsRequest, OPSV_idle);
		}
		
		get_philips_running_status()->otau_run_state.is_send_idle_before_set_device_otau_sem = true;
		if(device_otau_sem)
		{
			err = mos_semphr_release(device_otau_sem);
			if(err != kNoErr)
				app_log("otau sem set fail");
		}
		// break;
		return ;
	}

	switch(otaustate)
	{
	case OPSV_idle:
	{
		if (get_philips_running_status()->otau_run_state.device_otau_last_state == OPSV_UnKnow && get_philips_running_status()->otau_flash_param.need_ota_device && get_philips_running_status()->otau_flash_param.is_otau_file_downloaded_from_cloud)
		{
			get_philips_running_status()->otau_run_state.need_get_otau = true;
			get_philips_running_status()->otau_run_state.device_otau_last_state = OPSV_reset;
			get_philips_running_status()->otau_run_state.otau_current_package = -1;
			get_philips_running_status()->otau_run_state.otau_total_package = get_philips_running_status()->otau_flash_param.otau_data_len / 128;
			if (get_philips_running_status()->otau_run_state.otau_total_package * 128 < get_philips_running_status()->otau_flash_param.otau_data_len)
			{
				++get_philips_running_status()->otau_run_state.otau_total_package;
			}
			get_philips_running_status()->otau_run_state.otau_data_leave = get_philips_running_status()->otau_flash_param.otau_data_len;
			get_philips_running_status()->otau_run_state.otau_data_index = 0;

			get_philips_running_status()->otau_run_state.is_otau_ing = true;

			mkv_item_set(CONFIG_DATA_KV_KEY_OTAU, &get_philips_running_status()->otau_flash_param, sizeof(get_philips_running_status()->otau_flash_param));
			get_philips_running_status()->flash_param.wifi_reboot_status = WRBT_device_ota;
			mkv_item_set("pwrr_status", &get_philips_running_status()->flash_param.wifi_reboot_status, sizeof(get_philips_running_status()->flash_param.wifi_reboot_status));
			mos_msleep(500);
			philips_param_dump_otau();
			philips_send_PutProps_otau_state(DOID_PutpropsRequest, OPSV_reset);
			philips_user_async_reboot(500);
		}
		break;
	}
	case OPSV_downloading:
	{
		get_philips_running_status()->otau_run_state.device_otau_last_state = OPSV_downloading;
		philips_send_put_PutProps_otau_data();
		break;
	}
	case OPSV_downloaded:
	{
		if (get_philips_running_status()->otau_run_state.device_otau_last_state == OPSV_downloading)
		{
			get_philips_running_status()->otau_run_state.device_otau_last_state = OPSV_downloaded;
			get_philips_running_status()->otau_run_state.need_get_otau = false;

			get_philips_running_status()->otau_run_state.is_otau_ing = false;
			get_philips_running_status()->otau_flash_param.need_ota_device = false;

			mkv_item_set(CONFIG_DATA_KV_KEY_OTAU, &get_philips_running_status()->otau_flash_param, sizeof(get_philips_running_status()->otau_flash_param));
			get_philips_running_status()->flash_param.wifi_reboot_status = WRBT_unknow;
			mkv_item_set("pwrr_status", &get_philips_running_status()->flash_param.wifi_reboot_status, sizeof(get_philips_running_status()->flash_param.wifi_reboot_status));
			mos_msleep(500);
			philips_param_dump_otau();
			app_log("mcu ota transfer over,device reboot");
			MxosSystemReboot();
		}
		break;
	}
	case OPSV_reset:
	{
		//WiFi模组发送reset，收到回复，开始发送downloading状态
		if(get_philips_running_status()->otau_run_state.device_otau_last_state == OPSV_reset)
		{
			get_philips_running_status()->otau_run_state.device_otau_last_state = OPSV_reset;
			get_philips_running_status()->otau_run_state.otau_current_package = 1;
			philips_send_PutProps_otau_fours(DOID_BootOTAURequest, OPSV_downloading, get_philips_running_status()->otau_flash_param.cloud_device_version, 0, get_philips_running_status()->otau_run_state.otau_total_package);
		}
		break;
	}
	default:
		break;
	}
}

/*******************************************************************************
 * uart_data_process override
 * 函数说明：串口接收数据处理，根据串口指令类型调用相应的API
 *******************************************************************************/

merr_t uart_data_process(uint8_t *buf, int buflen)
{
	merr_t err = kNoErr;

	const uint8_t *p = buf + 7;

	switch(buf[2])	//ID
	{
		case 2:		//Initialize response
		{
			if(get_philips_running_status()->wifi_status == WRS_wait_for_device_init)
			{
				if(device_init_sem)
				{
					err = mos_semphr_release(device_init_sem);
					if(err != kNoErr)
						app_log("device_init_sem set fail");
					if(buf[6]==2)
					{
						app_log("new philips uart protocol");
						get_philips_running_status()->flash_param.philips_uart_protocol_is_common=true;
						mkv_item_set("uart_protocol_is_common", &get_philips_running_status()->flash_param.philips_uart_protocol_is_common, sizeof(get_philips_running_status()->flash_param.philips_uart_protocol_is_common));
					}
				}
			}
			break;
		}
		case 7:		//Putprops or Getprops response
		{
			switch(buf[5])	//response status
			{
				case 0x00:	//no error
				{
					switch(buf[6])
					{
						case 0x01:	//device info
						{
							if((get_philips_running_status()->otau_run_state.device_otau_last_state == OPSV_reset)&&(get_philips_running_status()->flash_param.muc_boot))
							{	
								get_philips_running_status()->flash_param.muc_boot=false;
								mkv_item_set(DATA_KV_MCU_BOOT, &get_philips_running_status()->flash_param.muc_boot, sizeof(get_philips_running_status()->flash_param.muc_boot));
								get_philips_running_status()->flash_param.wifi_reboot_status=WRBT_unknow;
								mkv_item_delete(CONFIG_DATA_KV_KEY_OTAU);
								mkv_item_delete("pwrr_status");
								
								mos_msleep(500);
								MxosSystemReboot();
							}else
							{
								if(get_philips_running_status()->flash_param.philips_uart_protocol_is_common)							
									philips_uart_common_protocol_port1_process(buf, buflen);
								else
									philips_uart_protocol_port1_process(buf, buflen);
							}
							break;
						}
						case 0x02:	//wifi_ui
						{
							philips_uart_protocol_port2_process(buf, buflen);
							break;
						}
						case 0x03:	//Purifier properties
						{
							philips_process_air_properties(buf);
							device_status_upload_manage();
							break;
						}
						case 0x04:	//factory test
						{
							philips_uart_protocol_port4_process(buf, buflen);
							break;
						}
						case 0x05:	//filter properties
						{
							philips_process_fltsts_properties(buf);
							device_status_upload_manage();
							break;
						}
						case 0x08:	//otau
						{
							get_philips_running_status()->is_support_otau = true;	//支持MCU OTA
							philips_uart_protocol_port8_process(buf, buflen);
							break;
						}
						default:
							break;
					}	//buf[6]---port
					break;
				}
				case 0x03:	//no such port
				{
					app_log("No such port, port = %02X", buf[6]);
					switch ( buf[6] )
					{
						case 0x08:
						{
							get_philips_running_status()->is_support_otau = false;	//不支持MCU OTA

							get_philips_running_status()->otau_run_state.is_send_idle_before_set_device_otau_sem = true;
							if(device_otau_sem)
							{
								err = mos_semphr_release(device_otau_sem);
								if(err != kNoErr)
									app_log("otau sem set fail");
							}
							break;
						}
						default: 
							break;
					}
					break;
				}
				case 0x07:	//operation ID not implemented
				{
					app_log("No such operation ID, operation ID = %02X", buf[6]);
					switch ( buf[6] )
					{
						case 0x08:
						{
							break;
						}
						default: 
							break;
					}
					break;
				}
				default:
					break;
			}	//buf[5]---response status
			break;
		}
		default:
			break;
	}	//buf[2]---ID

	return err;
}

/*******************************************************************************
 * uart_yield override
 * 函数说明：处理定时轮询发送的串口查询指令
 *******************************************************************************/

int uart_yield(mxos_time_t last_send_msg_time, mxos_time_t last_recv_msg_time)
{
	static uint8_t f_send_port = 1;
	static mxos_time_t f_time_last_send_port = 0;
	static uint32_t resend_count = 1;
	static uint32_t port1_try_count = 0;
	static uint8_t send_port_switch=0;

	mxos_time_t time_now;

	time_now = mos_time();

	// otau进行中，不进行port 2\3\4\5(wifi_ui\air property\factory\filter)的轮询查询
	if ( !get_philips_running_status()->otau_run_state.is_otau_ing )
	{
		if ( get_philips_running_status()->wifi_status >= WRS_init_complete )
		{
			if ( time_now > f_time_last_send_port + 250 )
			{
				philips_send_GetProps(++f_send_port);
				if ( f_send_port == 5 )
				{
					if ( (get_philips_running_status()->otau_run_state.need_get_otau)&&(get_philips_running_status()->flash_param.muc_boot))
					{
						philips_send_GetProps(0x08);
					}
					if (get_philips_running_status()->need_get_deviceinfo)
					{
						philips_send_GetProps(0x01);
					}
					f_send_port = 1;
				}
				f_time_last_send_port = time_now;
			}
		}
	}
	else
	{
		if ( time_now > last_send_msg_time + 1000 )
		{
			if ( get_philips_running_status()->otau_run_state.otau_current_package == -1 )
			{
				app_log("get_philips_running_status()->otau_state_id_control = %lu", get_philips_running_status()->otau_run_state.otau_state_id_control);

				if(port1_try_count<3)
				{	
					if ( ++get_philips_running_status()->otau_run_state.otau_state_id_control % 6 == 0 )
					{
						philips_send_PutProps_otau_state(DOID_PutpropsRequest, OPSV_reset);
					}
					else
					{
						philips_send_PutProps_otau_state(DOID_BootOTAURequest, OPSV_reset);
					}
				}else if((port1_try_count<14)&&(port1_try_count>=3)&&(get_philips_running_status()->flash_param.muc_boot))
				{		
					if(send_port_switch%2)
					{
						philips_send_GetProps(0x01);
					}
					else
						philips_send_GetProps(0x08);	

					send_port_switch++;
				}else
				{
					port1_try_count=0;
				}				

				port1_try_count++;
			}
			else if ( get_philips_running_status()->otau_run_state.otau_current_package == 0 )
			{
				philips_send_PutProps_otau_fours(DOID_BootOTAURequest, OPSV_downloading, get_philips_running_status()->otau_flash_param.cloud_device_version, 0, get_philips_running_status()->otau_run_state.otau_total_package);
			}else
			{

			}
		}
		//otau过程中，如果3S内没有数据交互，WiFi模组重启
		if ( time_now > last_send_msg_time + 15000 || time_now > last_recv_msg_time + 15000)
		{
			app_log("3S no uart data,device reboot");
			MxosSystemReboot();
		}
	}

	// 2S未收到回复，重发历史串口指令最后一次
	if ( time_now > last_recv_msg_time + (2000 * resend_count) )
	{
		resend_count++;
		philips_uart_queue_push_history_last();
	}
	else if( time_now < last_recv_msg_time +2000 )
	{
		resend_count = 1;
	}

	return 0;
}