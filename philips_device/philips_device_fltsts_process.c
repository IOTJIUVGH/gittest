// #include "mxos.h"

#include "philips_device_fltsts_process.h"
#include "philips_device.h"
#include "philips_param.h"

#include "philips_log.h"
// #define app_log(M, ...)             			philips_custom_log("", M, ##__VA_ARGS__)
#define app_log(M, ...)
// #define app_log_o2(format, ...)				philips_custom_log("", format, ##__VA_ARGS__)
#define app_log_o2(format, ...)

#define FltstsPPN_FilterType1  					0x01
#define FltstsPPN_FilterType2  					0x02
#define FltstsPPN_FilterLife0  					0x03
#define FltstsPPN_FilterLife1  					0x04
#define FltstsPPN_FilterLife2  					0x05
#define FltstsPPN_WickLife  					0x06
#define FltstsPPN_Filtername  					0x06
#define FltstsPPN_FilterID  					0x07
#define FltstsPPN_FilterTotalLife0				0x13	
#define FltstsPPN_FilterTotalLife1				0x14
#define FltstsPPN_FilterTotalLife2				0x15
#define FltstsPPN_Filtername_1					0x20
#define FltstsPPN_FilterID_1					0x21

void philips_process_fltsts_properties_common(const uint8_t *data)
{
	const uint8_t *p = &data[7];
    int i_value = 0;

	while ( *(p) != 0x00 )
	{
		switch ( *(p) )
		{
		case FltstsPPN_FilterType1:
		{
			i_value = *(p + 2);
			if ( i_value + '0' != purifier_status.fltt1[1] )
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				purifier_status.fltt1[0] = 'A';
				purifier_status.fltt1[1] = i_value + '0';
				purifier_status.fltt1[2] = '\0';
			}
			break;
		}
		case FltstsPPN_FilterType2:
		{
			i_value = *(p + 2);
			if ( i_value == 0 && memcmp(purifier_status.fltt2, "none", 4) != 0 )
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				memset(purifier_status.fltt2, 0x00, 5);
				memcpy(purifier_status.fltt2, "none", 4);
			}
			else if ( i_value != 0 && i_value + '0' != purifier_status.fltt2[1] )
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				memset(purifier_status.fltt2, 0x00, 5);
				purifier_status.fltt2[0] = 'C';
				purifier_status.fltt2[1] = i_value + '0';
			}
			break;
		}
		case FltstsPPN_FilterLife0:
		{
			i_value = (*(p + 2) << 8) + *(p + 3);
			if ( i_value != purifier_status.fltsts0 )
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				purifier_status.fltsts0 = i_value;
			}
			break;
		}
		case FltstsPPN_FilterLife1:
		{
			i_value = (*(p + 2) << 8) + *(p + 3);
			if ( i_value != purifier_status.fltsts1 )
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				purifier_status.fltsts1 = i_value;
			}
			break;
		}
		case FltstsPPN_FilterLife2:
		{
			i_value = (*(p + 2) << 8) + *(p + 3);
			if ( i_value != purifier_status.fltsts2 )
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				purifier_status.fltsts2 = i_value;
			}
			break;
		}
		case FltstsPPN_WickLife:
		{
			i_value = (*(p + 2) << 8) + *(p + 3);
			if ( i_value != purifier_status.wicksts )
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				purifier_status.wicksts = i_value;
			}
			break;
		}
		case FltstsPPN_FilterTotalLife0:
		{
			purifier_status.filter_prop_map |= PHILIPS_FILTER_PROP_MAP_FLTTOTAL0;
			i_value = (*(p + 2) << 8) + *(p + 3);
			if( i_value !=  purifier_status.flttotal0)
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				purifier_status.flttotal0 = i_value;
			}
			break;
		}
		case FltstsPPN_FilterTotalLife1:
		{
			purifier_status.filter_prop_map |= PHILIPS_FILTER_PROP_MAP_FLTTOTAL1;
			i_value = (*(p + 2) << 8) + *(p + 3);
			if( i_value !=  purifier_status.flttotal1)
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				purifier_status.flttotal1 = i_value;
			}
			break;
		}
		case FltstsPPN_FilterTotalLife2:
		{
			purifier_status.filter_prop_map |= PHILIPS_FILTER_PROP_MAP_FLTTOTAL2;
			i_value = (*(p + 2) << 8) + *(p + 3);
			if( i_value !=  purifier_status.flttotal2)
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				purifier_status.flttotal2 = i_value;
			}
			break;
		}
		default: break;
		}

		p = p + 2 + *(p + 1);
	}

	if ( get_philips_running_status()->o2_filter_flash_param.need_transform_filter_life )
	{
		philips_o2_filter_adjust();
	}
}

void philips_process_fltsts_properties_new_common(const uint8_t *data)
{
	const uint8_t *p = &data[7];
    int i_value = 0;
	uint8_t case_value=0;

	while ( *(p) != 0x00 )
	{
		switch(*(p))
		{
			case 0x01:case 0x02:case_value=*(p)-1;break;
			case 0x07:case 0x08:case 0x09:case_value=*(p)-4;break;
		}

		switch (case_value)
		{
		case FltstsPPN_FilterType1:
		{
			i_value = *(p + 2);
			if ( i_value + '0' != purifier_status.fltt1[1] )
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				purifier_status.fltt1[0] = 'A';
				purifier_status.fltt1[1] = i_value + '0';
				purifier_status.fltt1[2] = '\0';
			}
			break;
		}
		case FltstsPPN_FilterType2:
		{
			i_value = *(p + 2);
			if ( i_value == 0 && memcmp(purifier_status.fltt2, "none", 4) != 0 )
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				memset(purifier_status.fltt2, 0x00, 5);
				memcpy(purifier_status.fltt2, "none", 4);
			}
			else if ( i_value != 0 && i_value + '0' != purifier_status.fltt2[1] )
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				memset(purifier_status.fltt2, 0x00, 5);
				purifier_status.fltt2[0] = 'C';
				purifier_status.fltt2[1] = i_value + '0';
			}
			break;
		}
		case FltstsPPN_FilterLife0:
		{
			i_value = (*(p + 2) << 8) + *(p + 3);
			if ( i_value != purifier_status.fltsts0 )
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				purifier_status.fltsts0 = i_value;
			}
			break;
		}
		case FltstsPPN_FilterLife1:
		{
			i_value = (*(p + 2) << 8) + *(p + 3);
			if ( i_value != purifier_status.fltsts1 )
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				purifier_status.fltsts1 = i_value;
			}
			break;
		}
		case FltstsPPN_FilterLife2:
		{
			i_value = (*(p + 2) << 8) + *(p + 3);
			if ( i_value != purifier_status.fltsts2 )
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				purifier_status.fltsts2 = i_value;
			}
			break;
		}
		case FltstsPPN_FilterTotalLife0:
		{
			purifier_status.filter_prop_map |= PHILIPS_FILTER_PROP_MAP_FLTTOTAL0;
			i_value = (*(p + 2) << 8) + *(p + 3);
			if( i_value !=  purifier_status.flttotal0)
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				purifier_status.flttotal0 = i_value;
			}
			break;
		}
		case FltstsPPN_FilterTotalLife1:
		{
			purifier_status.filter_prop_map |= PHILIPS_FILTER_PROP_MAP_FLTTOTAL1;
			i_value = (*(p + 2) << 8) + *(p + 3);
			if( i_value !=  purifier_status.flttotal1)
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				purifier_status.flttotal1 = i_value;
			}
			break;
		}
		case FltstsPPN_FilterTotalLife2:
		{
			purifier_status.filter_prop_map |= PHILIPS_FILTER_PROP_MAP_FLTTOTAL2;
			i_value = (*(p + 2) << 8) + *(p + 3);
			if( i_value !=  purifier_status.flttotal2)
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				purifier_status.flttotal2 = i_value;
			}
			break;
		}
		default: break;
		}

		p = p + 2 + *(p + 1);
	}

	if ( get_philips_running_status()->o2_filter_flash_param.need_transform_filter_life )
	{
		philips_o2_filter_adjust();
	}
}

void philips_process_fltsts_properties_Mars_or_MarsLE(const uint8_t *data)
{
    const uint8_t *p = data+7;
	int i_value = 0;

    while (*(p) != 0x00)
    {
		switch ( *(p) )
		{
		case FltstsPPN_FilterType1:
		{
			purifier_status.filter_prop_map |= PHILIPS_FILTER_PROP_MAP_FLTT1;
			i_value = *(p + 2);
			if ( i_value + '0' != purifier_status.fltt1[1] )
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				purifier_status.fltt1[0] = 'A';
				purifier_status.fltt1[1] = i_value + '0';
				purifier_status.fltt1[2] = '\0';
			}
			break;
		}
		case FltstsPPN_FilterType2:
		{
			purifier_status.filter_prop_map |= PHILIPS_FILTER_PROP_MAP_FLTT2;
			i_value = *(p + 2);
			if ( i_value == 0 && memcmp(purifier_status.fltt2, "none", 4) != 0 )
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				memset(purifier_status.fltt2, 0x00, 5);
				memcpy(purifier_status.fltt2, "none", 4);
			}
			else if ( i_value != 0 && i_value + '0' != purifier_status.fltt2[1] )
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				memset(purifier_status.fltt2, 0x00, 5);
				purifier_status.fltt2[0] = 'C';
				purifier_status.fltt2[1] = i_value + '0';
			}
			break;
		}
		case FltstsPPN_FilterLife0:
		{
			purifier_status.filter_prop_map |= PHILIPS_FILTER_PROP_MAP_FLTSTS0;
			i_value = (*(p + 2) << 8) + *(p + 3);
			if ( i_value != purifier_status.fltsts0 )
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				purifier_status.fltsts0 = i_value;
			}
			break;
		}
		case FltstsPPN_FilterLife1:
		{
			purifier_status.filter_prop_map |= PHILIPS_FILTER_PROP_MAP_FLTSTS1;
			i_value = (*(p + 2) << 8) + *(p + 3);
			if ( i_value != purifier_status.fltsts1 )
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				purifier_status.fltsts1 = i_value;
			}
			break;
		}
		case FltstsPPN_FilterLife2:
		{
			purifier_status.filter_prop_map |= PHILIPS_FILTER_PROP_MAP_FLTSTS2;
			i_value = (*(p + 2) << 8) + *(p + 3);
			if ( i_value != purifier_status.fltsts2 )
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				purifier_status.fltsts2 = i_value;
			}
			break;
		}
		case FltstsPPN_Filtername:
		{
			purifier_status.filter_prop_map |= PHILIPS_FILTER_PROP_MAP_FILNA;
			if ( *(p + 2) != 0x00 )
			{
				if ( *(p + 1) < sizeof(purifier_status.filna) )
				{
					memset(purifier_status.filna, 0x00, sizeof(purifier_status.filna));
					strncpy(purifier_status.filna, (char *)(p + 2), *(p + 1));
				}
				else
				{
					memset(purifier_status.filna, 0x00, sizeof(purifier_status.filna));
					strncpy(purifier_status.filna, (char *)(p + 2), sizeof(purifier_status.filna) - 1);
				}
			}
			break;
		}
		case FltstsPPN_FilterID:
		{
			purifier_status.filter_prop_map |= PHILIPS_FILTER_PROP_MAP_FILID;
			if ( *(p + 2) != 0x00 )
			{
				if ( *(p + 1) < sizeof(purifier_status.filid) )
				{
					memset(purifier_status.filid, 0x00, sizeof(purifier_status.filid));
					strncpy(purifier_status.filid, (char *)(p + 2), *(p + 1));
				}
				else
				{
					memset(purifier_status.filid, 0x00, sizeof(purifier_status.filid));
					strncpy(purifier_status.filid, (char *)(p + 2), sizeof(purifier_status.filid) - 1);
				}
			}
			break;
		}
		case FltstsPPN_FilterTotalLife0:
		{
			purifier_status.filter_prop_map |= PHILIPS_FILTER_PROP_MAP_FLTTOTAL0;
			i_value = (*(p + 2) << 8) + *(p + 3);
			if( i_value !=  purifier_status.flttotal0)
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				purifier_status.flttotal0 = i_value;
			}
			break;
		}
		case FltstsPPN_FilterTotalLife1:
		{
			purifier_status.filter_prop_map |= PHILIPS_FILTER_PROP_MAP_FLTTOTAL1;
			i_value = (*(p + 2) << 8) + *(p + 3);
			if( i_value !=  purifier_status.flttotal1)
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				purifier_status.flttotal1 = i_value;
			}
			break;
		}
		case FltstsPPN_FilterTotalLife2:
		{
			purifier_status.filter_prop_map |= PHILIPS_FILTER_PROP_MAP_FLTTOTAL2;
			i_value = (*(p + 2) << 8) + *(p + 3);
			if( i_value !=  purifier_status.flttotal2)
			{
				get_philips_running_status()->device_upload_manage.status_change = true;
				purifier_status.flttotal2 = i_value;
			}
			break;
		}
		case FltstsPPN_Filtername_1:
		{
			purifier_status.filter_prop_map |= PHILIPS_FILTER_PROP_MAP_FILNA;
			if ( *(p + 2) != 0x00 )
			{
				if ( *(p + 1) < sizeof(purifier_status.filna) )
				{
					memset(purifier_status.filna, 0x00, sizeof(purifier_status.filna));
					strncpy(purifier_status.filna, (char *)(p + 2), *(p + 1));
				}
				else
				{
					memset(purifier_status.filna, 0x00, sizeof(purifier_status.filna));
					strncpy(purifier_status.filna, (char *)(p + 2), sizeof(purifier_status.filna) - 1);
				}
			}
			break;
		}
		case FltstsPPN_FilterID_1:
		{
			purifier_status.filter_prop_map |= PHILIPS_FILTER_PROP_MAP_FILID;
			if ( *(p + 2) != 0x00 )
			{
				if ( *(p + 1) < sizeof(purifier_status.filid) )
				{
					memset(purifier_status.filid, 0x00, sizeof(purifier_status.filid));
					strncpy(purifier_status.filid, (char *)(p + 2), *(p + 1));
				}
				else
				{
					memset(purifier_status.filid, 0x00, sizeof(purifier_status.filid));
					strncpy(purifier_status.filid, (char *)(p + 2), sizeof(purifier_status.filid) - 1);
				}
			}
			break;
		}
		default: break;
		}

        p = p + 2 + *(p + 1);
    }

    return ;
}


// void philips_process_fltsts_properties(const uint8_t *data)
// {
//     const uint8_t *p = data+7;

//     while (*(p) != 0x00)
//     {
//         p = p + 2 + *(p + 1);
//     }

//     return ;
// }

void filter_transform_thread_O2(void *arg)
{
	mxos_time_t last_time = 0;
	mxos_time_t now_time = 0;

	while(1)
	{
		now_time = mos_time();

		if(strcmp(purifier_status.pwr_str, "1") == 0)
		{	
			if(last_time == 0)
			{
				get_philips_running_status()->o2_filter_flash_param.ms_count += now_time;
			}
			else
			{
				get_philips_running_status()->o2_filter_flash_param.ms_count += (now_time - last_time);
			}
			
			if(!get_philips_running_status()->enable_o2_filter_debug)
			{
				if(get_philips_running_status()->o2_filter_flash_param.ms_count >= 3600000)
				{
					get_philips_running_status()->o2_filter_flash_param.ms_count -= 3600000;
					if(get_philips_running_status()->o2_filter_flash_param.filter0_life != 0)
					{
						--get_philips_running_status()->o2_filter_flash_param.filter0_life;
					}
					if(get_philips_running_status()->o2_filter_flash_param.filter1_life != 0)
					{
						--get_philips_running_status()->o2_filter_flash_param.filter1_life;
					}
					get_philips_running_status()->device_upload_manage.status_change = true;
					// 更新flash中滤芯数据
					philips_o2_filter_flash_data_update();
				}
			}
			else
			{
				app_log_o2("enable_o2_filter_debug = true");

				if(get_philips_running_status()->o2_filter_flash_param.filter0_life != 0)
				{
					get_philips_running_status()->o2_filter_flash_param.filter0_life -= 10;
					if(get_philips_running_status()->o2_filter_flash_param.filter0_life > 65000)
						get_philips_running_status()->o2_filter_flash_param.filter0_life = 0;
				}
				if(get_philips_running_status()->o2_filter_flash_param.filter1_life != 0)
				{
					get_philips_running_status()->o2_filter_flash_param.filter1_life -= 10;
					if(get_philips_running_status()->o2_filter_flash_param.filter1_life > 65000)
						get_philips_running_status()->o2_filter_flash_param.filter1_life = 0;
				}
				get_philips_running_status()->device_upload_manage.status_change = true;
			}
		}
		last_time = now_time;
		mos_msleep(1500);
	}

	mos_thread_delete(NULL);
}

static bool is_transform_thread_running = false;
static mos_thread_id_t filter_transform_thread_O2_handle = NULL;

void start_filter_transform_O2(void)
{
	// merr_t err = kGeneralErr;

	if(!is_transform_thread_running)
	{
		filter_transform_thread_O2_handle = mos_thread_new(MOS_APPLICATION_PRIORITY, "filter transform O2", filter_transform_thread_O2, 1024, NULL);
		require((filter_transform_thread_O2_handle != NULL), exit);
		app_log("filter transform O2 start process");
	}

	is_transform_thread_running = true;
	exit:
	return ;
}

void philips_o2_filter_adjust(void)
{
	bool filter0_flash_data_update_flag = false;
	bool filter1_flash_data_update_flag = false;

	app_log_o2("fltsts0 = 0x%04X, fltsts1 = 0x%04X, err = 0x%04X, life0 = %d, life1 = %d, adjust0 = %d, adjust1 = %d",
				   purifier_status.fltsts0, purifier_status.fltsts1, purifier_status.err,
				   get_philips_running_status()->o2_filter_flash_param.filter0_life, get_philips_running_status()->o2_filter_flash_param.filter1_life,
				   get_philips_running_status()->o2_filter_flash_param.filter0_lastadjustvalue, get_philips_running_status()->o2_filter_flash_param.filter1_lastadjustvalue);

	if ( ((purifier_status.err >> 14) & 0x01) && ((purifier_status.err >> 0) & 0x01) )
	{
		if ( get_philips_running_status()->o2_filter_flash_param.filter0_life != 0 )
		{
			get_philips_running_status()->o2_filter_flash_param.filter0_life = 0x00;
			get_philips_running_status()->o2_filter_flash_param.filter0_lastadjustvalue = 0x00;

			filter0_flash_data_update_flag = true; 
		}
	}
	else
	{
		uint8_t device_fltsts0 = (purifier_status.fltsts0 >> 8) & 0xFF;
		if ( device_fltsts0 >= 0x00 && device_fltsts0 <= 0x01 )
		{
			if ( device_fltsts0 != get_philips_running_status()->o2_filter_flash_param.filter0_lastadjustvalue )
			{
				if ( device_fltsts0 == 0x01 )
					get_philips_running_status()->o2_filter_flash_param.filter0_life = 0x0168;
				else
					get_philips_running_status()->o2_filter_flash_param.filter0_life = device_fltsts0 + 0xFF;

				get_philips_running_status()->o2_filter_flash_param.filter0_lastadjustvalue = device_fltsts0;
				
				filter0_flash_data_update_flag = true;
			}
		}
	}

	if ( ((purifier_status.err >> 14) & 0x01) && (((purifier_status.err >> 2) & 0x01) || ((purifier_status.err >> 4) & 0x01) ) )
	{
		if ( get_philips_running_status()->o2_filter_flash_param.filter1_lastadjustvalue != 0xFE )
		{
			get_philips_running_status()->o2_filter_flash_param.filter1_life = 120;
			get_philips_running_status()->o2_filter_flash_param.filter1_lastadjustvalue = 0xFE;

			filter1_flash_data_update_flag = true;
		}
	}
	else if ( ((purifier_status.err >> 14) & 0x01) && (((purifier_status.err >> 3) & 0x01) || ((purifier_status.err >> 5) & 0x01)) )
	{
		if ( get_philips_running_status()->o2_filter_flash_param.filter1_lastadjustvalue != 0x00 )
		{
			get_philips_running_status()->o2_filter_flash_param.filter1_life = 0x00;
			get_philips_running_status()->o2_filter_flash_param.filter1_lastadjustvalue = 0x00;

			filter1_flash_data_update_flag = true;
		}
	}
	else
	{
		uint8_t device_fltsts1 = (purifier_status.fltsts1 >> 8) & 0xFF;
		if ( device_fltsts1 >= 0x00 && device_fltsts1 <= 0x12 )
		{
			if ( device_fltsts1 != get_philips_running_status()->o2_filter_flash_param.filter1_lastadjustvalue )
			{
				if ( device_fltsts1 == 0x12 )
					get_philips_running_status()->o2_filter_flash_param.filter1_life = 0x12C0;
				else
					get_philips_running_status()->o2_filter_flash_param.filter1_life = (device_fltsts1 << 8) + 0xFF;

				get_philips_running_status()->o2_filter_flash_param.filter1_lastadjustvalue = device_fltsts1;

				filter1_flash_data_update_flag = true;
			}
		}
	}

	if(filter0_flash_data_update_flag || filter1_flash_data_update_flag)
	{
		// 更新滤芯的flash数据
		philips_o2_filter_flash_data_update();
		mos_msleep(100);
	}

	if ( get_philips_running_status()->o2_filter_flash_param.filter0_life == get_philips_running_status()->last_upload_filter0_life && get_philips_running_status()->o2_filter_flash_param.filter1_life == get_philips_running_status()->last_upload_filter1_life )
	{
		get_philips_running_status()->last_upload_filter0_life = get_philips_running_status()->o2_filter_flash_param.filter0_life;
		get_philips_running_status()->last_upload_filter1_life = get_philips_running_status()->o2_filter_flash_param.filter1_life;
		get_philips_running_status()->device_upload_manage.status_change = true;
	}
}