#include "mxos.h"
#include "mxos_config.h"

#include "philips_wifi.h"

#include "philips_param.h"
#include "philips_basic_api.h"

#include "philips_log.h"
#define app_log(M, ...)             			philips_custom_log("", M, ##__VA_ARGS__)

#define MAX_SEND_TIME							90

char *udp_send_success = "{\"Status\":\"Device_Discovery\",\"Device_id\":\"%s\",\"ExtraData\":\"%s\"}";
//char *udp_send_success_with_token = "{\"Status\":\"Device_Discovery\",\"Device_id\":\"%s\",\"ExtraData\":\"%s\",\"deviceToken\":\"%s\",\"option\":%d}";
char *udp_send_success_with_token = "{\"Status\":\"Device_Discovery\",\"ExtraData\":\"%s\",\"didt\":\"%s\",\"option\":%d}";

char *udp_send_fail = "{\"Status\":\"Device_Failed\",\"ExtraData\":\"%s\",\"message\":\"%s\"}";
char *udp_send_fail_with_token = "{\"Status\":\"Device_Failed\",\"ExtraData\":\"%s\",\"message\":\"%s\",\"option\":%d}";

void device_discovery_thread()
{
	merr_t err = kNoErr;

	struct sockaddr_in addr;
	int udp_fd = -1;
	char *udp_send_buf = NULL;
	int Send_Count = 0;
	Aes enc;
	char *textbuf = NULL;
	uint32_t textbuflen = 0;
	uint8_t *ciphertext = NULL;

	udp_send_buf = malloc(512);
	require_action(udp_send_buf != NULL, exit, err = kNoMemoryErr);

	/*Establish a UDP port to receive any data sent to this port*/
	udp_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	require_action(IsValidSocket(udp_fd), exit, err = kNoResourcesErr);

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_BROADCAST;
	addr.sin_port = htons(8000);

	// generate udp_send_buf
	if ( get_philips_running_status()->softap_option & 0x02 )
	{
		// set key and iv
		AesSetKey(&enc, get_philips_running_status()->didt_key, AES_BLOCK_SIZE, get_philips_running_status()->didt_iv, AES_ENCRYPTION);

		// generate plaintext
		textbuf = (char *)malloc(1024);
		require_action(textbuf != NULL, exit, err = kNoMemoryErr);
		memset(textbuf, 0x00, 1024);
		textbuflen = sprintf(textbuf, "%s&%s", get_philips_running_status()->flash_param.device_id, philips_get_device_token(false));

		// generate ciphertext
		ciphertext = (uint8_t *)malloc(1024);
		require_action(ciphertext != NULL, exit, err = kNoMemoryErr);
		memset(ciphertext, 0x00, 1024);
		int aescbclen = philipsAesCbcEncryptPkcs5Padding(&enc, ciphertext, (uint8_t *)textbuf, textbuflen);
		if ( aescbclen == 0 )
		{
			err = kAuthenticationErr;
			goto exit;
		}

		// transform ciphertext to hex string
		memset(textbuf, 0x00, 1024);
		textbuflen = 0;
		for ( int i = 0; i < aescbclen; ++i )
		{
			sprintf(textbuf + (i << 1), "%02X", ciphertext[i]);
		}

		// generate udp_send_buf
		if ( get_philips_running_status()->https_client_res == kNoErr )
		{
			sprintf(udp_send_buf, udp_send_success_with_token, get_philips_running_status()->extra_data, textbuf, get_philips_running_status()->softap_option);
		}
		else
		{
			sprintf(udp_send_buf, udp_send_fail_with_token, get_philips_running_status()->extra_data, get_philips_running_status()->https_client_error_message == NULL ? "" : get_philips_running_status()->https_client_error_message, get_philips_running_status()->softap_option);
		}
	}
	else
	{
		if ( get_philips_running_status()->https_client_res == kNoErr )
		{
			sprintf(udp_send_buf, udp_send_success, get_philips_running_status()->flash_param.device_id, get_philips_running_status()->extra_data);
		}
		else
		{
			sprintf(udp_send_buf, udp_send_fail, get_philips_running_status()->extra_data, get_philips_running_status()->https_client_error_message == NULL ? "" : get_philips_running_status()->https_client_error_message);
		}
	}

	while ( 1 )
	{
		/*the receiver should bind at port=8000*/
		int res = sendto(udp_fd, udp_send_buf, strlen(udp_send_buf), 0, (struct sockaddr *)&addr, sizeof(addr));
		app_log("UDP send res = %d , data = %s", res, udp_send_buf);
		if ( ++Send_Count >= MAX_SEND_TIME )
		{
			break;
		}
		mos_msleep(1000);
	}

	exit:
	if ( udp_send_buf )
	{
		free(udp_send_buf);
		udp_send_buf = NULL;
	}
	if ( textbuf != NULL )
	{
		free(textbuf);
		textbuf = NULL;
	}
	if ( ciphertext != NULL )
	{
		free(ciphertext);
		ciphertext = NULL;
	}
	if ( err != kNoErr )
	{
		app_log("UDP thread exit with err: %d", err);
	}
	mos_thread_delete(NULL);
}

void philips_start_udp(void)
{
	// int err = -1;

	mos_thread_id_t udp_thread_handle = NULL;

	udp_thread_handle = mos_thread_new(MOS_APPLICATION_PRIORITY, "device discovery", device_discovery_thread, 0x1000, NULL);
	require((udp_thread_handle != NULL), exit);
	return;

	exit:
	app_log("creat_thread failed");
}
