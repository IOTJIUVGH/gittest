#include "mxos.h"
#include "mkv.h"

#include "philips_param.h"
#include "philips_uart.h"

#include "philips_log.h"
#define app_log(M, ...)             			philips_custom_log("", M, ##__VA_ARGS__)

bool philips_set_device_token(uint8_t *arg1, uint32_t arg1len, uint8_t *arg2, uint32_t arg2len)
{
	srand((mos_time() << 16) + philips_crc_ccitt(arg1, arg1len) + philips_crc_ccitt(arg2, arg2len) + philips_crc_ccitt((uint8_t *)&purifier_status + 200, 30));
	for ( int i = 0; i < 10; ++i )
	{
		switch ( rand() & 0x00000003 )
		{
		case 0: get_philips_running_status()->flash_param.passwd[i] = rand() % 10 + 48; break;
		case 1: get_philips_running_status()->flash_param.passwd[i] = rand() % 26 + 65; break;
		case 2: get_philips_running_status()->flash_param.passwd[i] = rand() % 26 + 97; break;
		case 3: --i; break;
		}
	}
	app_log("device token = %s", get_philips_running_status()->flash_param.passwd);

	mkv_item_set("pcw_passwd", get_philips_running_status()->flash_param.passwd, sizeof(get_philips_running_status()->flash_param.passwd));

	return true;
}

char *philips_get_device_token(bool autogenerate)
{
	if ( autogenerate )
	{
		if ( get_philips_running_status()->flash_param.passwd[0] == '\0' )
		{
			srand((mos_time() << 16) + philips_get_memory_dump_time() + philips_get_uart_recv_msg_time() + philips_get_uart_send_msg_time() + philips_crc_ccitt((uint8_t *)&purifier_status + 200, 30));
			for ( int i = 0; i < 10; ++i )
			{
				switch ( rand() & 0x00000003 )
				{
				case 0: get_philips_running_status()->flash_param.passwd[i] = rand() % 10 + 48; break;
				case 1: get_philips_running_status()->flash_param.passwd[i] = rand() % 26 + 65; break;
				case 2: get_philips_running_status()->flash_param.passwd[i] = rand() % 26 + 97; break;
				case 3: --i; break;
				}
			}
			app_log("auto generate device token = %s", get_philips_running_status()->flash_param.passwd);

			mkv_item_set("pcw_passwd", get_philips_running_status()->flash_param.passwd, sizeof(get_philips_running_status()->flash_param.passwd));
		}
	}

	return get_philips_running_status()->flash_param.passwd;
}

char *generate_bind_token(void)
{
	memset(get_philips_running_status()->bind_token, 0x00, sizeof(get_philips_running_status()->bind_token));
	snprintf(get_philips_running_status()->bind_token, sizeof(get_philips_running_status()->bind_token), "%s%lu", get_philips_running_status()->mac_address, mos_time());

	return get_philips_running_status()->bind_token;
}
