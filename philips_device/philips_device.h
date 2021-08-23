#ifndef __PHILIPS_DEVICE_H__
#define __PHILIPS_DEVICE_H__

#include "mxos.h"

#define PHILIPS_CONFIG_WIFI_IGNORE_TIME			8000

enum _DiComm_Operation_ID
{
	DOID_InitializeRequest = 1,
	DOID_InitializeResponse = 2,
	DOID_PutpropsRequest = 3,
	DOID_GetPropsRequest = 4,
	DOID_SubscribeRequest = 5,
	DOID_UnsubscribeRequest = 6,
	DOID_GeneralResponse = 7,
	DOID_ChangeIndicationResponse = 8,
	DOID_ChangeIndicationRequest = 9,
	DOID_GetProdsRequest = 10,
	DOID_GetPortsRequest = 11,
	DOID_AddPropsRequest = 12,
	DOID_DelPropsRequest = 13,
	DOID_ExeMethodRequest = 14,
	DOID_BootOTAURequest = 15,
};

enum _Wifiui_properties_property_no
{
	WPPN_connection = 0x01,
	WPPN_setup = 0x02,
};

enum _Wifiui_properties_connection_value
{
	WPCV_Unknown = 0x00,
	WPCV_notconnected = 0x01,
	WPCV_requested = 0x02,
	WPCV_connecting = 0x03,
	WPCV_connected = 0x04,
	WPCV_error = 0x05,
};

enum _Wifiui_properties_setup_value
{
	WPSV_Unknown = 0x00,
	WPSV_inactive = 0x01,
	WPSV_requested = 0x02,
	WPSV_active = 0x03,
};

enum _Fac_properties_property_no
{
	FPPN_idle = 0x00,
	FPPN_pcba = 0x01,
	FPPN_wifi = 0x02,
	FPPN_reset = 0x03,
};

enum _Fac_properties_value
{
	FPV_success = 0x00,
	FPV_fail = 0x01,
};

enum _OTAU_properties_state_value
{
	OPSV_idle,
	OPSV_downloading,
	OPSV_downloaded,
	OPSV_reset,
	OPSV_UnKnow = 0xFF,
};

#define U32_BIT0_MASK           (0x00000001UL)

//warn code
#define PHILIPS_WARNING_FLAG    (1<<30)
#define PHILIPS_WARNING_F0      (1<<0)      //Pre-filter cleaning
#define PHILIPS_WARNING_F1      (1<<1)      //Filter 1 lock
#define PHILIPS_WARNING_F2      (1<<2)      //Filter 2 lock
#define PHILIPS_WARNING_F3      (1<<3)      //Filter 1 warning
#define PHILIPS_WARNING_F4      (1<<4)      //Filter 2 warning
#define PHILIPS_WARNING_F5      (1<<5)      //Wick cleaning
#define PHILIPS_WARNING_F6      (1<<6)      //Wick waring
#define PHILIPS_WARNING_F7      (1<<7)      //Wick lock
#define PHILIPS_WARNING_F8      (1<<8)      //Water tank empty
#define PHILIPS_WARNING_F9      (1<<9)      //Mars Pre-filter cleaning
#define PHILIPS_WARNING_FA      (1<<10)     //Mars Filter 1 lock
#define PHILIPS_WARNING_FB      (1<<11)     //Mars Filter 1 warning

//error code
#define PHILIPS_ERROR_FLAG      (1<<31)
#define PHILIPS_ERROR_E0        (1<<0)      //Door open
#define PHILIPS_ERROR_E1        (1<<1)      //Motor error
#define PHILIPS_ERROR_E2        (1<<2)      //Particle sensor error
#define PHILIPS_ERROR_E3        (1<<3)      
#define PHILIPS_ERROR_E4        (1<<4)      //Gas sensor error
#define PHILIPS_ERROR_E5        (1<<5)      //T&RH sensor error
#define PHILIPS_ERROR_E6        (1<<6)      //Gear motor error
#define PHILIPS_ERROR_E7        (1<<7)      //Hall error
#define PHILIPS_ERROR_E8        (1<<8)      //other error
#define PHILIPS_ERROR_E9        (1<<9)      //Light Sensor
#define PHILIPS_ERROR_EA        (1<<10)     //RFID error
#define PHILIPS_ERROR_EB        (1<<11)     //Filter ID error

// air properties map
#define PHILIPS_AIR_PROP_MAP_OM									(1<<0)
#define PHILIPS_AIR_PROP_MAP_PWR								(1<<1)
#define PHILIPS_AIR_PROP_MAP_CL									(1<<2)
#define PHILIPS_AIR_PROP_MAP_AQIL								(1<<3)
#define PHILIPS_AIR_PROP_MAP_UIL								(1<<4)
#define PHILIPS_AIR_PROP_MAP_MODE								(1<<5)
#define PHILIPS_AIR_PROP_MAP_PM25								(1<<6)
#define PHILIPS_AIR_PROP_MAP_IAQL								(1<<7)
#define PHILIPS_AIR_PROP_MAP_AQIT								(1<<8)
#define PHILIPS_AIR_PROP_MAP_TVOC								(1<<9)
#define PHILIPS_AIR_PROP_MAP_DDP								(1<<10)
#define PHILIPS_AIR_PROP_MAP_RDDP								(1<<11)
#define PHILIPS_AIR_PROP_MAP_ERR								(1<<12)
#define PHILIPS_AIR_PROP_MAP_DT									(1<<13)
#define PHILIPS_AIR_PROP_MAP_DTRS								(1<<14)
#define PHILIPS_AIR_PROP_MAP_FUNC								(1<<15)
#define PHILIPS_AIR_PROP_MAP_RHSET								(1<<16)
#define PHILIPS_AIR_PROP_MAP_RH									(1<<17)
#define PHILIPS_AIR_PROP_MAP_TEMP								(1<<18)
#define PHILIPS_AIR_PROP_MAP_WL									(1<<19)
#define PHILIPS_AIR_PROP_MAP_UASET								(1<<20)
#define PHILIPS_AIR_PROP_MAP_GAS								(1<<21)
#define PHILIPS_AIR_PROP_MAP_FORMALDEHYDE                       (1<<22)

// filter properties map
#define PHILIPS_FILTER_PROP_MAP_FLTT1							(1<<0)
#define PHILIPS_FILTER_PROP_MAP_FLTT2							(1<<1)
#define PHILIPS_FILTER_PROP_MAP_FLTSTS0							(1<<2)
#define PHILIPS_FILTER_PROP_MAP_FLTSTS1							(1<<3)
#define PHILIPS_FILTER_PROP_MAP_FLTSTS2							(1<<4)
#define PHILIPS_FILTER_PROP_MAP_WICKSTS							(1<<5)
#define PHILIPS_FILTER_PROP_MAP_FILNA							(1<<6)
#define PHILIPS_FILTER_PROP_MAP_FILID							(1<<7)
#define PHILIPS_FILTER_PROP_MAP_FLTTOTAL0						(1<<8)
#define PHILIPS_FILTER_PROP_MAP_FLTTOTAL1						(1<<9)
#define PHILIPS_FILTER_PROP_MAP_FLTTOTAL2						(1<<10)

typedef struct _purifier_status
{
	//设备状态
	char om_str[3];
	char pwr_str[3];
	bool cl;
	char aqil;
	char aqil_str[3];
	uint8_t uilight;
	char uil_str[3];
	uint8_t gas;
	uint8_t formaldehyde;
	char mode_str[3];
	uint16_t pm25;
	uint8_t iaql;
	uint16_t aqit;
	uint8_t tvoc;
	char ddp_str[3];
	char rddp_str[3];
	uint16_t err;
	uint8_t dt;
	uint16_t dtrs;
	char func[3];
	uint8_t rhset;
	uint8_t rh;
	int8_t temp;
	uint8_t wl;
	char uaset_str[3];
	//滤芯状态
	char fltt1[3];
	char fltt2[5];
	uint16_t fltsts0;
	uint16_t fltsts1;
	uint16_t fltsts2;
	uint16_t wicksts;
	char filna[10];
	char filid[23];
	uint16_t flttotal0;
	uint16_t flttotal1;
	uint16_t flttotal2;
	// 属性map
	uint32_t air_prop_map;
	uint32_t filter_prop_map;
}purifier_status_t;

extern purifier_status_t purifier_status;

int uart_yield(mxos_time_t last_send_msg_time, mxos_time_t last_recv_msg_time);
merr_t uart_data_process(uint8_t *buf, int buflen);

void philips_wait_for_all_initialized(void);

merr_t philips_send_PutProps_device_name(const char *name, int namelen);
merr_t philips_send_PutProps_device_wfversion(void);
merr_t philips_send_PutProps_device_language(const char *language, int languagelen);
merr_t philips_send_PutProps_Common(uint8_t port, uint8_t *props, int propslen);
merr_t philips_send_PutProps_fac(enum _Fac_properties_property_no fppn, uint8_t value);
void philips_notify_device_wifiui_init(void);

void philips_check_power_save(void);
merr_t philips_device_reset(void);

#endif