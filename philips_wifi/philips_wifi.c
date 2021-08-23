#include "mxos.h"
#include "mxos_config.h"

#include "cJSON.h"

#include "philips_wifi.h"

#include "philips_param.h"
#include "philips_device.h"

#include "philips_log.h"
#define app_log(M, ...)             			philips_custom_log("", M, ##__VA_ARGS__)

#define FOG_WIFI_CONFIG_SERVER_PORT				30123
#define FOG_WIFI_CONFIG_SERVER_BUFF_LEN			2048
#define MAX_SSID_LEN							32
#define MAX_KEY_LEN								64
#define WIFI_RESULT_BUFF						2048

#define SOFTAP_CONN_TIMEOUT						(60*1000)		//1min
#define SOFTAP_SETUP_TIMEOUT					(15*60*1000)	//15min

typedef struct _wifi_config_t
{
	char ssid[MAX_SSID_LEN + 1];
	char key[MAX_KEY_LEN + 1];
	merr_t err;
} wifi_config_t;

static bool should_close = false;
static wifi_config_t *wifi_config = NULL;
static bool wifi_config_complete_flag = true;

static mos_semphr_id_t setup_finish_sem = NULL;
static mos_semphr_id_t net_sem = NULL;

extern void wifi_ap_sta_reg_hdl(void *usr_hdl);
extern merr_t SocketSend(int fd, const uint8_t *inBuf, size_t inBufLen);

/*
 * return 1 ,should close
 */
static int wifi_config_handle(int fd, uint8_t *data, uint32_t data_len)
{
	merr_t err = kNoErr;

	cJSON *json = NULL, *json_data = NULL, *json_temp = NULL;

	char string_temp[100];
	char string_result[WIFI_RESULT_BUFF] = {0};
	md5_context ctx;
	uint8_t md5_res[16];

	app_log("config recv = %s", data);
	require_action_string(((*data == '{') && (*(data + data_len - 1) == '}')), exit, err = kFormatErr, "body JSON format error");

	json = cJSON_Parse((char *)data);
	require_action(json != NULL, exit, err = kFormatErr);

	json_temp = NULL;
	json_temp = cJSON_GetObjectItem(json, "type");
	require_action(json_temp != NULL && cJSON_IsString(json_temp), exit, err = kParamErr);

	if ( strcmp(json_temp->valuestring, "config") == 0 )
	{
		json_data = cJSON_GetObjectItem(json, "data");
		require_action(json_data != NULL && cJSON_IsObject(json_data), exit, err = kParamErr);

		json_temp = NULL;
		json_temp = cJSON_GetObjectItem(json_data, "ssid");
		require_action(json_temp != NULL && cJSON_IsString(json_temp), exit, err = kParamErr);
		strncpy(wifi_config->ssid, json_temp->valuestring, sizeof(wifi_config->ssid));

		json_temp = NULL;
		json_temp = cJSON_GetObjectItem(json_data, "password");
		require_action(json_temp != NULL && cJSON_IsString(json_temp), exit, err = kParamErr);
		strncpy(wifi_config->key, json_temp->valuestring, sizeof(wifi_config->key));

		json_temp = NULL;
		json_temp = cJSON_GetObjectItem(json_data, "ExtraData");
		require_action(json_temp != NULL && cJSON_IsString(json_temp), exit, err = kParamErr);
		strncpy(get_philips_running_status()->extra_data, json_temp->valuestring, sizeof(get_philips_running_status()->extra_data));

		json_temp = NULL;
		json_temp = cJSON_GetObjectItem(json_data, "option");
		require_action(json_temp != NULL && cJSON_IsNumber(json_temp), exit, err = kParamErr);
		get_philips_running_status()->softap_option = json_temp->valueint;
		app_log("option = %d", get_philips_running_status()->softap_option);
		if ( get_philips_running_status()->softap_option & 0x02 )
		{
			if ( ((char *)(philips_get_device_token(false)))[0] == '\0' )
			{
				philips_set_device_token((uint8_t *)wifi_config->ssid, strnlen(wifi_config->ssid, MAX_SSID_LEN), (uint8_t *)wifi_config->key, strnlen(wifi_config->key, MAX_KEY_LEN));
			}

			if ( get_philips_running_status()->didt_key == NULL )
			{
				get_philips_running_status()->didt_key = (uint8_t *)malloc(AES_BLOCK_SIZE + 1);
			}
			if ( get_philips_running_status()->didt_iv == NULL )
			{
				get_philips_running_status()->didt_iv = (uint8_t *)malloc(AES_BLOCK_SIZE + 1);
			}
			require_action(get_philips_running_status()->didt_key != NULL && get_philips_running_status()->didt_iv != NULL, exit, err = kNoMemoryErr);

			InitMd5(&ctx);
			Md5Update(&ctx, (uint8_t *)wifi_config->ssid, strlen(wifi_config->ssid));
			Md5Update(&ctx, (uint8_t *)wifi_config->key, strlen(wifi_config->key));
			Md5Update(&ctx, (uint8_t *)get_philips_running_status()->extra_data, strlen(get_philips_running_status()->extra_data));
			Md5Final(&ctx, md5_res);
			snprintf((char *)get_philips_running_status()->didt_key, AES_BLOCK_SIZE + 1, "%02X%02X%02X%02X%02X%02X%02X%02X", md5_res[0], md5_res[1], md5_res[2], md5_res[3], md5_res[4], md5_res[5], md5_res[6], md5_res[7]);
			snprintf((char *)get_philips_running_status()->didt_iv, AES_BLOCK_SIZE + 1, "%02X%02X%02X%02X%02X%02X%02X%02X", md5_res[8], md5_res[9], md5_res[10], md5_res[11], md5_res[12], md5_res[13], md5_res[14], md5_res[15]);
		}
		if ( get_philips_running_status()->softap_option & 0x04 )
		{
			json_temp = NULL;
			json_temp = cJSON_GetObjectItem(json_data, "registration_id");
			require_action(json_temp != NULL && cJSON_IsString(json_temp), exit, err = kParamErr);
			strncpy(get_philips_running_status()->registration_id, json_temp->valuestring, sizeof(get_philips_running_status()->registration_id));

			json_temp = NULL;
			json_temp = cJSON_GetObjectItem(json_data, "push_type");
			require_action(json_temp != NULL && cJSON_IsString(json_temp), exit, err = kParamErr);
			strncpy(get_philips_running_status()->push_type, json_temp->valuestring, sizeof(get_philips_running_status()->push_type));

			json_temp = NULL;
			json_temp = cJSON_GetObjectItem(json_data, "app_id");
			require_action(json_temp != NULL && cJSON_IsString(json_temp), exit, err = kParamErr);
			strncpy(get_philips_running_status()->app_id, json_temp->valuestring, sizeof(get_philips_running_status()->app_id));

			json_temp = NULL;
			json_temp = cJSON_GetObjectItem(json_data, "secret");
			require_action(json_temp != NULL && cJSON_IsString(json_temp), exit, err = kParamErr);
			strncpy(get_philips_running_status()->app_secret, json_temp->valuestring, sizeof(get_philips_running_status()->app_secret));

			json_temp = NULL;
			json_temp = cJSON_GetObjectItem(json_data, "user_id");
			require_action(json_temp != NULL && cJSON_IsString(json_temp), exit, err = kParamErr);
			strncpy(get_philips_running_status()->user_id, json_temp->valuestring, sizeof(get_philips_running_status()->user_id));
		}

		json_temp = NULL;
		json_temp = cJSON_GetObjectItem(json_data, "DeviceName");
		if ( json_temp != NULL && cJSON_IsString(json_temp) )
		{
			strncpy(string_temp, json_temp->valuestring, sizeof(string_temp));
			app_log("set name: value = %s", string_temp);
			if ( string_temp[0] != 0x00 && strnlen(string_temp, 100) < sizeof(get_philips_running_status()->device_name) )
			{
				app_log("set name: value = %s, get_philips_running_status()->device_name = %s", string_temp, get_philips_running_status()->device_name);
				philips_send_PutProps_device_name(string_temp, strlen(string_temp));
			}
		}
	}

	exit:
	if ( json != NULL )
	{
		cJSON_Delete(json);
		json = NULL;
	}

	if ( err == kNoErr )
	{
		snprintf(string_result, sizeof(string_result), "{\"type\":\"config\",\"meta\":{\"message\":\"%s\",\"code\":%d}}", "config success", 0);
		get_philips_running_status()->is_get_ssid = true;
		err = 1;
	}
	else if ( err == kFormatErr )
	{
		snprintf(string_result, sizeof(string_result), "{\"type\":\"config\",\"meta\":{\"message\":\"%s\",\"code\":%d}}", "no json", 1000);
	}
	else
	{
		snprintf(string_result, sizeof(string_result), "{\"type\":\"config\",\"meta\":{\"message\":\"%s\",\"code\":%d}}", "param err", 1001);
	}
	
	app_log("send = %s", string_result);
	SocketSend(fd, (const uint8_t *)string_result, strlen(string_result));

	return err;
}

#define DEVICE_INFO_JSON "{\"type\":\"deviceinfo\",\"meta\":{\"message\":\"check device info\",\"code\":100},\"data\":{\"option\":7,\"name\":\"%s\",\"type\":\"%s\",\"modelid\":\"%s\",\"swversion\":\"%s\",\"bind_token\":\"%s\"}}"
static void local_client_thread(void *arg)
{
	merr_t err = kNoErr;
	int socket_fd = *(int *)arg;
	int len;
	uint8_t *inDataBuffer = NULL;
	struct timeval t;
	fd_set readfds;

	inDataBuffer = calloc(1, FOG_WIFI_CONFIG_SERVER_BUFF_LEN);
	require_action(inDataBuffer, exit, err = kNoMemoryErr);

	char *deviceinfo = (char *)malloc(256);
	if ( deviceinfo == NULL )
	{
		app_log("no memory to deviceinfo");
		goto exit;
	}
	sprintf(deviceinfo, DEVICE_INFO_JSON, get_philips_running_status()->device_name, get_philips_running_status()->device_type, get_philips_running_status()->flash_param.device_modelid, get_philips_running_status()->device_swversion, get_philips_running_status()->bind_token);
	err = SocketSend(socket_fd, (const uint8_t *)deviceinfo, strlen(deviceinfo));
	app_log("tcp send to fd = %d, res = %d, msg = %s", socket_fd, err, deviceinfo);
	free(deviceinfo);
	deviceinfo = NULL;

	while ( 1 )
	{
		t.tv_sec = 2;
		t.tv_usec = 0;

		FD_ZERO(&readfds);
		FD_SET(socket_fd, &readfds);

		select(socket_fd + 1, &readfds, NULL, NULL, &t);

		if ( FD_ISSET(socket_fd, &readfds) )
		{
			len = recv(socket_fd, inDataBuffer, FOG_WIFI_CONFIG_SERVER_BUFF_LEN, 0);
			if ( len <= 0 )
			{
				goto exit;
			}

			if ( 1 == wifi_config_handle(socket_fd, inDataBuffer, len) )
			{
				should_close = true;
				wifi_config->err = kNoErr;
				if(setup_finish_sem)
				{
					if(mos_semphr_release(setup_finish_sem) != kNoErr)
					{
						app_log("release finish sem fail");
					}
				}
				goto exit;
			}
		}

		if ( should_close == true )
		{
			goto exit;
		}
	}

exit:
	if ( socket_fd >= 0 )
	{
		close(socket_fd);
	}
	if ( inDataBuffer )
	{
		free(inDataBuffer);
	}

	app_log("close port %d, err = %d", socket_fd, err);
	mos_thread_delete(NULL);
}

static void wifi_config_server_thread(void *arg)
{
	merr_t err = kNoErr;
	int local_fd = -1;
	int client_fd = -1;
	struct sockaddr_in addr;
	fd_set readfds;
	int sockaddr_t_size;
	struct timeval t;

	local_fd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP);
	require_action(IsValidSocket( local_fd ), exit, err = kNoResourcesErr);

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(FOG_WIFI_CONFIG_SERVER_PORT);
	err = bind(local_fd, (struct sockaddr *)&addr, sizeof(addr));
	require_noerr(err, exit);

	err = listen(local_fd, 0);
	require_noerr(err, exit);

	app_log( "config server established at port %d, fd %d", FOG_WIFI_CONFIG_SERVER_PORT, local_fd );

	t.tv_sec = 1;
	t.tv_usec = 0;

	while ( 1 )
	{
		FD_ZERO(&readfds);
		FD_SET(local_fd, &readfds);

		select(local_fd + 1, &readfds, NULL, NULL, &t);

		if ( FD_ISSET(local_fd, &readfds) )
		{
			sockaddr_t_size = sizeof(struct sockaddr_in);
			client_fd = accept(local_fd, (struct sockaddr *)&addr, (socklen_t *)&sockaddr_t_size);

			if ( IsValidFD(client_fd) )
			{

				app_log( "Client %s:%d connected, fd = %d", inet_ntoa( addr.sin_addr ), ntohs( addr.sin_port ), client_fd );

				if(mos_thread_new(MOS_APPLICATION_PRIORITY, "local clients", local_client_thread, 0x1000, (void *)&client_fd) == NULL)
				{
					close(client_fd);
					client_fd = -1;
				}
			}
		}

		if ( should_close == true )
		{
			goto exit;
		}
	}

exit:
	close(local_fd);
	app_log("config server exit");
	
	mos_thread_delete(NULL);

	return ;
}

mos_thread_id_t wifi_config_server_thread_handle = NULL;

merr_t wifi_config_server_start(void)
{
	merr_t err = kNoErr;

	should_close = false;

	wifi_config_server_thread_handle = mos_thread_new(MOS_APPLICATION_PRIORITY, "config server", wifi_config_server_thread, 1024*2, NULL);
	require_action((wifi_config_server_thread_handle != NULL), exit, err = kGeneralErr);
exit:
	return err;
}

merr_t wifi_config_server_stop(void)
{
	should_close = true;

	return kNoErr;
}

static void awsNotify_WifiStatusHandler(WiFiEvent event, void *inContext)
{
	require(inContext, exit);

	switch ( event )
	{
	case NOTIFY_STATION_UP:
		if ( net_sem != NULL )
		{
			mos_semphr_release(net_sem);
		}
		break;
	default:
		break;
	}
	exit:
	return;
}

int fog_softap_connect_ap(char *ssid, char *passwd)
{
	merr_t err = kNoErr;

	mxos_Context_t *mxos_context = mxos_system_context_get();

	net_sem = mos_semphr_new(1);
	require_action((net_sem != NULL), exit, err = kGeneralErr);

	err = mxos_system_notify_register(mxos_notify_WIFI_STATUS_CHANGED, (void *)awsNotify_WifiStatusHandler, (void *)mxos_context);
	require_noerr(err, exit);

	err = mwifi_connect(ssid, passwd, strlen(passwd), NULL, NULL);
	require_noerr(err, exit);

	err = mos_semphr_acquire(net_sem, SOFTAP_CONN_TIMEOUT);		//wait until get semaphore
	require_noerr_action(err, exit, app_log("CONNECTED ERR"));

	memcpy(mxos_context->mxos_config.ssid, ssid, maxSsidLen);
	memcpy(mxos_context->mxos_config.user_key, passwd, maxKeyLen);
	mxos_context->mxos_config.user_keyLength = strlen(passwd);
	memcpy(mxos_context->mxos_config.key, passwd, maxKeyLen);
	mxos_context->mxos_config.keyLength = strlen(passwd);
	mxos_context->mxos_config.configured = allConfigured;
	mxos_context->mxos_config.dhcpEnable = true;
	mxos_system_context_update(mxos_context);    //Update Flash content

exit:
	mxos_system_notify_remove(mxos_notify_WIFI_STATUS_CHANGED, (void *)awsNotify_WifiStatusHandler);
	if(net_sem)
	{
		mos_semphr_delete(net_sem);
		net_sem = NULL;
	}
	
	if ( err != kNoErr )
	{
		mwifi_disconnect();
	}
	else
	{
	}

	return (err == kNoErr) ? 0 : -1;
}

static void wifi_config_thread(void *arg)
{
	merr_t err = kNoErr;

	setup_finish_sem = mos_semphr_new(1);
	require_action((setup_finish_sem != NULL), exit, err = kGeneralErr);

	if ( wifi_config == NULL )
	{
		wifi_config = malloc(sizeof(wifi_config_t));
	}
	memset(wifi_config, 0x00, sizeof(wifi_config_t));

	wifi_config->err = kGeneralErr;

	generate_bind_token();

	wifi_config_server_start();

	err = mos_semphr_acquire(setup_finish_sem, SOFTAP_SETUP_TIMEOUT);
	if(err != kNoErr)
	{
		app_log("philips get setup_finish_sem timeout!");
	}

	wifi_config_server_stop();

	mos_thread_join(wifi_config_server_thread_handle);

	mwifi_softap_stop();

	if ( (err == kNoErr) && (wifi_config->err == kNoErr) && (strlen(wifi_config->ssid) > 0) )
	{
		get_philips_running_status()->wifi_status = WRS_wifiui_connect_ing;
		err = fog_softap_connect_ap(wifi_config->ssid, wifi_config->key);
		if ( err != kNoErr )
		{
			get_philips_running_status()->wifi_status = WRS_wifiui_config_timeout;
			app_log(" wifi connect err ");
		}
	}
	else
	{
		app_log("philips softap setup network fail, notify device wifiui init");
		get_philips_running_status()->wifi_status = WRS_wifiui_config_timeout;
		philips_notify_device_wifiui_init();
	}
	
exit:
	app_log("wifi config thread exit, %d", err);
	if(setup_finish_sem)
	{
		mos_semphr_delete(&setup_finish_sem);
		setup_finish_sem = NULL;
	}
	if ( wifi_config )
	{
		free(wifi_config);
		wifi_config = NULL;
	}
	wifi_config_complete_flag = true;

	mos_thread_delete(NULL);
}

static mos_thread_id_t wifi_config_thread_handle = NULL;

void fog_wifi_config_start(void)
{
	static int softap_flag;
	wifi_config_complete_flag = false;

	if ( softap_flag != 1 )
	{
		softap_flag = 1;
		wifi_config_thread_handle = mos_thread_new(MOS_APPLICATION_PRIORITY, "wifi config", wifi_config_thread, 0x800, NULL);
		require((wifi_config_thread_handle != NULL), exit);
	}
exit:
	return ;
}

void fog_wifi_config_stop(void)
{
	if(setup_finish_sem)
	{
		if(mos_semphr_release(setup_finish_sem) != kNoErr)
		{
			app_log("release finish sem fail");
		}
	}

	return ;
}

static void wifi_ap_sta_hdl(int event, char* buf, int buf_len, int flags, void* userdata)
{
	if (event == 1)
	{
		app_log("Station association");
		++get_philips_running_status()->sta_client_amount;
		// philips_send_put_PutProps_wifiui(WPCV_connected, WPSV_active);
		get_philips_running_status()->wifi_status = WRS_wifiui_config_app_connected;
	}
	else
	{
		app_log("Station disassociation");
		--get_philips_running_status()->sta_client_amount;
		if ( get_philips_running_status()->sta_client_amount == 0 )
		{
			if ( !get_philips_running_status()->is_get_ssid )
			{
				// philips_send_put_PutProps_wifiui(WPCV_notconnected, WPSV_active);
				get_philips_running_status()->wifi_status = WRS_wifiui_config_ing;
			}
		}
	}
}

void fog_softap_start(void)
{
	merr_t err = kNoErr;
	// philips_send_put_PutProps_wifiui(WPCV_notconnected, WPSV_active);

	wifi_ap_sta_reg_hdl(wifi_ap_sta_hdl);

	mwifi_ip_attr_t ip_attr;
	strcpy( ip_attr.localip, "10.0.0.1" );
    strcpy( ip_attr.netmask, "255.255.255.0" );
    strcpy( ip_attr.gateway, "10.0.0.1" );

	err = mwifi_softap_start("PHILIPS Setup", "", 6, &ip_attr);
	require_noerr(err, exit);

	app_log("softap start: %s", "PHILIPS Setup");

exit:
	return;
}

merr_t philips_wifi_config(void)
{
	mxos_Context_t *mxos_context = mxos_system_context_get();

	if ( mxos_context->mxos_config.configured == unConfigured )
	{
		app_log("Empty configuration. Enter idle mode...");
		// get_philips_running_status()->is_first_config_wifi = true;
		// // philips_send_put_PutProps_wifiui(WPCV_notconnected, WPSV_active);
		// get_philips_running_status()->wifi_status = WRS_wifiui_config_ing;
		// fog_softap_start();
		// fog_wifi_config_start();
	}
#ifdef EasyLink_Needs_Reboot
	else if ( mxos_context->mxos_config.configured == wLanUnConfigured )
	{
		app_log("Re-config wlan configuration. Starting configuration mode...");
		get_philips_running_status()->is_first_config_wifi = true;
		// philips_send_put_PutProps_wifiui(WPCV_notconnected, WPSV_active);
		get_philips_running_status()->wifi_status = WRS_wifiui_config_ing;
		fog_softap_start();
		fog_wifi_config_start();
	}
#endif
	else
	{
		app_log("Available configuration. Starting Wi-Fi connection...");
		// philips_send_put_PutProps_wifiui(WPCV_connecting, WPSV_active);
		get_philips_running_status()->wifi_status = WRS_wifiui_connect_ing;
		system_connect_wifi_fast(system_context());
	}
	return 0;
}
