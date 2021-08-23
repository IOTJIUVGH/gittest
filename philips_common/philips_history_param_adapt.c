#include "mxos.h"
#include "mkv.h"

#include "philips_param.h"
#include "philips_history_param_adapt.h"
#include "philips_uart.h"

#include "philips_log.h"
#define app_log(format, ...)					philips_custom_log("", format, ##__VA_ARGS__)

static void philips_60ver_param_adapt(void *p)
{
    philips_config_data_60_t *history_p = (philips_config_data_60_t *)p;

    strncpy(get_philips_running_status()->flash_param.passwd, history_p->passwd, sizeof(get_philips_running_status()->flash_param.passwd));
    get_philips_running_status()->flash_param.aqit = history_p->aqit;
    
    get_philips_running_status()->o2_filter_flash_param.need_transform_filter_life = history_p->need_transform_filter_life;
    get_philips_running_status()->o2_filter_flash_param.filter0_life = history_p->filter0_life;
    get_philips_running_status()->o2_filter_flash_param.filter0_lastadjustvalue = history_p->filter0_lastadjustvalue;
    get_philips_running_status()->o2_filter_flash_param.filter1_life = history_p->filter1_life;
    get_philips_running_status()->o2_filter_flash_param.filter1_lastadjustvalue = history_p->filter1_lastadjustvalue;
    get_philips_running_status()->o2_filter_flash_param.ms_count = history_p->ms_count;

    mkv_item_set("pcw_passwd", get_philips_running_status()->flash_param.passwd, sizeof(get_philips_running_status()->flash_param.passwd));
    mkv_item_set("pda_aqit", &get_philips_running_status()->flash_param.aqit, sizeof(get_philips_running_status()->flash_param.aqit));
    mkv_item_set(CONFIG_DATA_KV_KEY_O2_FILTER, &get_philips_running_status()->o2_filter_flash_param, sizeof(get_philips_running_status()->o2_filter_flash_param));

    return ;
}

static void philips_59ver_param_adapt(void *p)
{
    philips_config_data_59_t *history_p = (philips_config_data_59_t *)p;

    strncpy(get_philips_running_status()->flash_param.passwd, history_p->passwd, sizeof(get_philips_running_status()->flash_param.passwd));
    get_philips_running_status()->flash_param.aqit = history_p->aqit;
    
    get_philips_running_status()->o2_filter_flash_param.need_transform_filter_life = history_p->need_transform_filter_life;
    get_philips_running_status()->o2_filter_flash_param.filter0_life = history_p->filter0_life;
    get_philips_running_status()->o2_filter_flash_param.filter0_lastadjustvalue = history_p->filter0_lastadjustvalue;
    get_philips_running_status()->o2_filter_flash_param.filter1_life = history_p->filter1_life;
    get_philips_running_status()->o2_filter_flash_param.filter1_lastadjustvalue = history_p->filter1_lastadjustvalue;
    get_philips_running_status()->o2_filter_flash_param.ms_count = history_p->ms_count;

    mkv_item_set("pcw_passwd", get_philips_running_status()->flash_param.passwd, sizeof(get_philips_running_status()->flash_param.passwd));
    mkv_item_set("pda_aqit", &get_philips_running_status()->flash_param.aqit, sizeof(get_philips_running_status()->flash_param.aqit));
    mkv_item_set(CONFIG_DATA_KV_KEY_O2_FILTER, &get_philips_running_status()->o2_filter_flash_param, sizeof(get_philips_running_status()->o2_filter_flash_param));

    return ;
}

static void philips_54ver_param_adapt(void *p)
{
    philips_config_data_54_t *history_p = (philips_config_data_54_t *)p;

    strncpy(get_philips_running_status()->flash_param.passwd, DEFAULT_PASSWD, sizeof(get_philips_running_status()->flash_param.passwd));
    get_philips_running_status()->flash_param.aqit = history_p->aqit;
    
    get_philips_running_status()->o2_filter_flash_param.need_transform_filter_life = history_p->need_transform_filter_life;
    get_philips_running_status()->o2_filter_flash_param.filter0_life = history_p->filter0_life;
    get_philips_running_status()->o2_filter_flash_param.filter0_lastadjustvalue = history_p->filter0_lastadjustvalue;
    get_philips_running_status()->o2_filter_flash_param.filter1_life = history_p->filter1_life;
    get_philips_running_status()->o2_filter_flash_param.filter1_lastadjustvalue = history_p->filter1_lastadjustvalue;
    get_philips_running_status()->o2_filter_flash_param.ms_count = history_p->ms_count;

    mkv_item_set("pcw_passwd", get_philips_running_status()->flash_param.passwd, sizeof(get_philips_running_status()->flash_param.passwd));
    mkv_item_set("pda_aqit", &get_philips_running_status()->flash_param.aqit, sizeof(get_philips_running_status()->flash_param.aqit));
    mkv_item_set(CONFIG_DATA_KV_KEY_O2_FILTER, &get_philips_running_status()->o2_filter_flash_param, sizeof(get_philips_running_status()->o2_filter_flash_param));

    return ;
}

static void philips_53ver_param_adapt(void *p)
{
    philips_config_data_53_t *history_p = (philips_config_data_53_t *)p;

    strncpy(get_philips_running_status()->flash_param.passwd, DEFAULT_PASSWD, sizeof(get_philips_running_status()->flash_param.passwd));
    get_philips_running_status()->flash_param.aqit = history_p->aqit;
    
    get_philips_running_status()->o2_filter_flash_param.need_transform_filter_life = history_p->need_transform_filter_life;
    get_philips_running_status()->o2_filter_flash_param.filter0_life = history_p->filter0_life;
    get_philips_running_status()->o2_filter_flash_param.filter0_lastadjustvalue = history_p->filter0_lastadjustvalue;
    get_philips_running_status()->o2_filter_flash_param.filter1_life = history_p->filter1_life;
    get_philips_running_status()->o2_filter_flash_param.filter1_lastadjustvalue = history_p->filter1_lastadjustvalue;
    get_philips_running_status()->o2_filter_flash_param.ms_count = history_p->ms_count;

    mkv_item_set("pcw_passwd", get_philips_running_status()->flash_param.passwd, sizeof(get_philips_running_status()->flash_param.passwd));
    mkv_item_set("pda_aqit", &get_philips_running_status()->flash_param.aqit, sizeof(get_philips_running_status()->flash_param.aqit));
    mkv_item_set(CONFIG_DATA_KV_KEY_O2_FILTER, &get_philips_running_status()->o2_filter_flash_param, sizeof(get_philips_running_status()->o2_filter_flash_param));

    return ;
}

static void philips_51ver_param_adapt(void *p)
{
    philips_config_data_51_t *history_p = (philips_config_data_51_t *)p;

    strncpy(get_philips_running_status()->flash_param.passwd, DEFAULT_PASSWD, sizeof(get_philips_running_status()->flash_param.passwd));
    get_philips_running_status()->flash_param.aqit = history_p->aqit;
    
    get_philips_running_status()->o2_filter_flash_param.need_transform_filter_life = history_p->need_transform_filter_life;
    get_philips_running_status()->o2_filter_flash_param.filter0_life = history_p->filter0_life;
    get_philips_running_status()->o2_filter_flash_param.filter0_lastadjustvalue = history_p->filter0_lastadjustvalue;
    get_philips_running_status()->o2_filter_flash_param.filter1_life = history_p->filter1_life;
    get_philips_running_status()->o2_filter_flash_param.filter1_lastadjustvalue = history_p->filter1_lastadjustvalue;
    get_philips_running_status()->o2_filter_flash_param.ms_count = history_p->ms_count;

    mkv_item_set("pcw_passwd", get_philips_running_status()->flash_param.passwd, sizeof(get_philips_running_status()->flash_param.passwd));
    mkv_item_set("pda_aqit", &get_philips_running_status()->flash_param.aqit, sizeof(get_philips_running_status()->flash_param.aqit));
    mkv_item_set(CONFIG_DATA_KV_KEY_O2_FILTER, &get_philips_running_status()->o2_filter_flash_param, sizeof(get_philips_running_status()->o2_filter_flash_param));

    return ;
}

static void philips_43ver_param_adapt(void *p)
{
    philips_config_data_43_t *history_p = (philips_config_data_43_t *)p;

    strncpy(get_philips_running_status()->flash_param.passwd, DEFAULT_PASSWD, sizeof(get_philips_running_status()->flash_param.passwd));
    get_philips_running_status()->flash_param.aqit = history_p->aqit;
    
    get_philips_running_status()->o2_filter_flash_param.need_transform_filter_life = history_p->need_transform_filter_life;
    get_philips_running_status()->o2_filter_flash_param.filter0_life = history_p->filter0_life;
    get_philips_running_status()->o2_filter_flash_param.filter0_lastadjustvalue = history_p->filter0_lastadjustvalue;
    get_philips_running_status()->o2_filter_flash_param.filter1_life = history_p->filter1_life;
    get_philips_running_status()->o2_filter_flash_param.filter1_lastadjustvalue = history_p->filter1_lastadjustvalue;
    get_philips_running_status()->o2_filter_flash_param.ms_count = history_p->ms_count;

    mkv_item_set("pcw_passwd", get_philips_running_status()->flash_param.passwd, sizeof(get_philips_running_status()->flash_param.passwd));
    mkv_item_set("pda_aqit", &get_philips_running_status()->flash_param.aqit, sizeof(get_philips_running_status()->flash_param.aqit));
    mkv_item_set(CONFIG_DATA_KV_KEY_O2_FILTER, &get_philips_running_status()->o2_filter_flash_param, sizeof(get_philips_running_status()->o2_filter_flash_param));

    return ;
}

static void philips_39ver_param_adapt(void *p)
{
    philips_config_data_39_t *history_p = (philips_config_data_39_t *)p;

    strncpy(get_philips_running_status()->flash_param.passwd, DEFAULT_PASSWD, sizeof(get_philips_running_status()->flash_param.passwd));
    get_philips_running_status()->flash_param.aqit = history_p->aqit;
    
    mkv_item_set("pcw_passwd", get_philips_running_status()->flash_param.passwd, sizeof(get_philips_running_status()->flash_param.passwd));
    mkv_item_set("pda_aqit", &get_philips_running_status()->flash_param.aqit, sizeof(get_philips_running_status()->flash_param.aqit));

    return ;
}

void philips_user_config_data_adapt_process(void *user_config_data)
{
    merr_t err = kNoErr;
    int len = 0;
    void *p = NULL;
    uint16_t crc16_calc = 0;
    uint16_t crc16_read = 0;

    require((user_config_data != NULL), exit);

    app_log("need flash param old convert new process");

    p = user_config_data;

    // 60.1版本
    crc16_calc = philips_crc_ccitt((uint8_t *)p+2, sizeof(philips_config_data_60_t)-2);
    crc16_read = ((philips_config_data_60_t *)p)->philips_config_crc[0] * 256 + ((philips_config_data_60_t *)p)->philips_config_crc[1];
    if(crc16_calc == crc16_read)
    {
        app_log("======history_param is philips_config_data_60_t======");
        philips_60ver_param_adapt(p);
        goto save;
    }

    // 59版本\58版本\57版本\56.4版本
    crc16_calc = philips_crc_ccitt((uint8_t *)p+2, sizeof(philips_config_data_59_t)-2);
    crc16_read = ((philips_config_data_59_t *)p)->philips_config_crc[0] * 256 + ((philips_config_data_59_t *)p)->philips_config_crc[1];
    if(crc16_calc == crc16_read)
    {
        app_log("======history_param is philips_config_data_59_t======");
        philips_59ver_param_adapt(p);
        goto save;
    }

    // 54.2版本
    crc16_calc = philips_crc_ccitt((uint8_t *)p+2, sizeof(philips_config_data_54_t)-2);
    crc16_read = ((philips_config_data_54_t *)p)->philips_config_crc[0] * 256 + ((philips_config_data_54_t *)p)->philips_config_crc[1];
    if(crc16_calc == crc16_read)
    {
        app_log("======history_param is philips_config_data_54_t======");
        philips_54ver_param_adapt(p);
        goto save;
    }

    // 53版本
    crc16_calc = philips_crc_ccitt((uint8_t *)p+2, sizeof(philips_config_data_53_t)-2);
    crc16_read = ((philips_config_data_53_t *)p)->philips_config_crc[0] * 256 + ((philips_config_data_53_t *)p)->philips_config_crc[1];
    if(crc16_calc == crc16_read)
    {
        app_log("======history_param is philips_config_data_53_t======");
        philips_53ver_param_adapt(p);
        goto save;
    }

    // 51.1版本\47.1版本
    crc16_calc = philips_crc_ccitt((uint8_t *)p+2, sizeof(philips_config_data_51_t)-2);
    crc16_read = ((philips_config_data_51_t *)p)->philips_config_crc[0] * 256 + ((philips_config_data_51_t *)p)->philips_config_crc[1];
    if(crc16_calc == crc16_read)
    {
        app_log("======history_param is philips_config_data_51_t======");
        philips_51ver_param_adapt(p);
        goto save;
    }

    // 43版本\42版本
    crc16_calc = philips_crc_ccitt((uint8_t *)p+2, sizeof(philips_config_data_43_t)-2);
    crc16_read = ((philips_config_data_43_t *)p)->philips_config_crc[0] * 256 + ((philips_config_data_43_t *)p)->philips_config_crc[1];
    if(crc16_calc == crc16_read)
    {
        app_log("======history_param is philips_config_data_43_t======");
        philips_43ver_param_adapt(p);
        goto save;
    }

    // 39版本\35版本\32版本
    crc16_calc = philips_crc_ccitt((uint8_t *)p+2, sizeof(philips_config_data_39_t)-2);
    crc16_read = ((philips_config_data_39_t *)p)->philips_config_crc[0] * 256 + ((philips_config_data_39_t *)p)->philips_config_crc[1];
    if(crc16_calc == crc16_read)
    {
        app_log("======history_param is philips_config_data_39_t======");
        philips_39ver_param_adapt(p);
        goto save;
    }

    app_log("======no history_param or history_param not find suit struct======");

save:
    // 处理结束，KV保存标志位，不进行下次处理
    get_philips_running_status()->flash_param.param_old_convert_new_finish = true;
    mkv_item_set("pocn_finish", &get_philips_running_status()->flash_param.param_old_convert_new_finish, sizeof(get_philips_running_status()->flash_param.param_old_convert_new_finish));

exit:
    return ;
}

#define CRC_SIZE                                ( 2 )
#define SYS_MAGIC_NUMBR                         (0xA43E2165)
#define MAX_HISTORY_USER_FLASH_DATA_LENGTH		(1024 * 5)

typedef struct 
{
    uint32_t start_address;   // the address of the bin saved on flash.
    uint32_t length;          // file real length
    uint8_t version[8];
    uint8_t type;             // B:bootloader, P:boot_table, A:application, D: 8782 driver
    uint8_t upgrade_type;     //u:upgrade, 
    uint16_t crc;
    uint8_t reserved[4];
} old_boot_table_t;

/** 
 *  @brief  Wi-Fi security type enumeration definition.
 */ 
enum wlan_sec_type_e{
   OLD_SECURITY_TYPE_NONE,        /**< Open system. */
   OLD_SECURITY_TYPE_WEP,         /**< Wired Equivalent Privacy. WEP OLD_security. */
   OLD_SECURITY_TYPE_WPA_TKIP,    /**< WPA /w TKIP */
   OLD_SECURITY_TYPE_WPA_AES,     /**< WPA /w AES */
   OLD_SECURITY_TYPE_WPA2_TKIP,   /**< WPA2 /w TKIP */
   OLD_SECURITY_TYPE_WPA2_AES,    /**< WPA2 /w AES */
   OLD_SECURITY_TYPE_WPA2_MIXED,  /**< WPA2 /w AES or TKIP */
   OLD_SECURITY_TYPE_AUTO,        /**< It is used when calling @ref micoWlanStartAdv, MICO read security type from scan result. */
};

typedef uint8_t wlan_sec_type_t;

typedef struct
{
    /*Device identification*/
    char            name[maxNameLen];

    /*Wi-Fi configuration*/
    char            ssid[maxSsidLen];
    char            user_key[maxKeyLen]; 
    int             user_keyLength;
    char            key[maxKeyLen]; 
    int             keyLength;
    char            bssid[6];
    int             channel;
    wlan_sec_type_t security;

    /*Power save configuration*/
    bool            rfPowerSaveEnable;
    bool            mcuPowerSaveEnable;

    /*Local IP configuration*/
    bool            dhcpEnable;
    char            localIp[maxIpLen];
    char            netMask[maxIpLen];
    char            gateWay[maxIpLen];
    char            dnsServer[maxIpLen];

    /*EasyLink configuration*/
    config_state_type_t   configured;
    uint8_t               easyLinkByPass;
    uint32_t              reserved;

    /*Services in MICO system*/
    uint32_t        magic_number;

    /*Update seed number when configuration is changed*/
    int32_t         seed;
} old_mico_sys_config_t;

typedef struct {
    /*OTA options*/
    old_boot_table_t             bootTable;
    /*MICO system core configuration*/
    old_mico_sys_config_t        micoSystemConfig;
} old_system_config_t;

extern system_context_t *sys_context;

static bool is_crc_match( uint16_t crc_1, uint16_t crc_2)
{
    if( crc_1 != crc_2 )
        return false;

    return true;
}

/* Calculate CRC value for parameter1/parameter2. exclude boottable and the last 2 bytes(crc16 result) */
static uint16_t para_crc16(int part)
{
    uint16_t crc_result;
    CRC16_Context crc_context;
    uint32_t offset, len = 1024, end;
    uint8_t *tmp;
    mxos_logic_partition_t *partition; 
    
    if (part != MODULE_PARTITION_INFO)
        return 0;

    tmp = (uint8_t*)malloc(1024);
    if (tmp == NULL)
        return 0;

    offset = sizeof(old_boot_table_t);
    partition = mhal_flash_get_info( part );
    /* Calculate CRC value */
    CRC16_Init( &crc_context );
    end = partition->partition_length - CRC_SIZE;
    while(offset < end) 
    {
        if (offset + len > end)
            len = end - offset;
        mhal_flash_read( part, &offset, tmp, len );
        CRC16_Update( &crc_context, tmp, len );
    }
    CRC16_Final( &crc_context, &crc_result );

    free(tmp);

    return crc_result;
}

merr_t philips_read_configuration(void **user_config_data)
{
    merr_t err = kGeneralErr;

    uint32_t para_offset = 0x0;
    uint32_t crc_offset;
    uint16_t crc_result, crc_target;
    mxos_logic_partition_t *partition; 

    mxos_Context_t *mxos_context = mxos_system_context_get();
    old_system_config_t *flashContentInRam = NULL;

    flashContentInRam = (old_system_config_t *)malloc(sizeof(old_system_config_t));
    require_action( (flashContentInRam != NULL), exit, err = kNoMemoryErr);

    *user_config_data = (void *)malloc(MAX_HISTORY_USER_FLASH_DATA_LENGTH);
    require_action( (*user_config_data != NULL), exit, err = kNoMemoryErr);

    partition = mhal_flash_get_info( MODULE_PARTITION_INFO );
    crc_result = para_crc16( MODULE_PARTITION_INFO );
    app_log( "crc_result = %d", crc_result );
    crc_offset = partition->partition_length - CRC_SIZE;
    err = mhal_flash_read( MODULE_PARTITION_INFO, &crc_offset, (uint8_t *)&crc_target, CRC_SIZE );
    app_log( "crc_target = %d", crc_target);

    if( is_crc_match( crc_result, crc_target ) == true )
    {
        partition = mhal_flash_get_info( MODULE_PARTITION_INFO );
        para_offset = 0x0;
        err = mhal_flash_read( MODULE_PARTITION_INFO, &para_offset, (uint8_t *)flashContentInRam, sizeof( old_system_config_t ) );
        para_offset = sizeof( old_system_config_t );
        err = mhal_flash_read( MODULE_PARTITION_INFO, &para_offset, (uint8_t *)(*user_config_data), MAX_HISTORY_USER_FLASH_DATA_LENGTH );

        if(flashContentInRam->micoSystemConfig.magic_number == SYS_MAGIC_NUMBR)
        {
            strcpy(mxos_context->mxos_config.name, flashContentInRam->micoSystemConfig.name);
            
            strcpy(mxos_context->mxos_config.ssid, flashContentInRam->micoSystemConfig.ssid);
            strcpy(mxos_context->mxos_config.user_key, flashContentInRam->micoSystemConfig.user_key);
            mxos_context->mxos_config.user_keyLength = flashContentInRam->micoSystemConfig.user_keyLength;
            strcpy(mxos_context->mxos_config.key, flashContentInRam->micoSystemConfig.key);
            mxos_context->mxos_config.keyLength = flashContentInRam->micoSystemConfig.keyLength;
            strcpy(mxos_context->mxos_config.bssid, flashContentInRam->micoSystemConfig.bssid);
            mxos_context->mxos_config.channel = flashContentInRam->micoSystemConfig.channel;
            mxos_context->mxos_config.security = flashContentInRam->micoSystemConfig.security;

            mxos_context->mxos_config.rfPowerSaveEnable = flashContentInRam->micoSystemConfig.rfPowerSaveEnable;
            mxos_context->mxos_config.mcuPowerSaveEnable = flashContentInRam->micoSystemConfig.mcuPowerSaveEnable;

            mxos_context->mxos_config.dhcpEnable = flashContentInRam->micoSystemConfig.dhcpEnable;
            strcpy(mxos_context->mxos_config.localIp, flashContentInRam->micoSystemConfig.localIp);
            strcpy(mxos_context->mxos_config.netMask, flashContentInRam->micoSystemConfig.netMask);
            strcpy(mxos_context->mxos_config.gateWay, flashContentInRam->micoSystemConfig.gateWay);
            strcpy(mxos_context->mxos_config.dnsServer, flashContentInRam->micoSystemConfig.dnsServer);

            mxos_context->mxos_config.configured = flashContentInRam->micoSystemConfig.configured;
            mxos_context->mxos_config.reserved = flashContentInRam->micoSystemConfig.reserved;

            mxos_context->mxos_config.magic_number = flashContentInRam->micoSystemConfig.magic_number;

            mxos_context->mxos_config.seed = flashContentInRam->micoSystemConfig.seed;

            mxos_system_context_update(mxos_context);

            system_context_t *inContext = sys_context;
            MXOSReadConfiguration(inContext);

            app_log("parameter1 partition convert kv finish");
            err = kNoErr;
        }
        else
        {
            app_log("Magic number error");
            err = kGeneralErr;
        }
    }   
    else 
    { 
        app_log("parameter1 partition crc check fail");
        err = kGeneralErr;
    }

    // 处理结束，KV保存标志位，不进行下次处理
    get_philips_running_status()->flash_param.parameter1_convert_kv_finish = true;
    mkv_item_set("pck_finish", &get_philips_running_status()->flash_param.parameter1_convert_kv_finish, sizeof(get_philips_running_status()->flash_param.parameter1_convert_kv_finish));

exit:

    if(flashContentInRam)
    {
        free(flashContentInRam);
        flashContentInRam = NULL;
    }
    return err;
}

void philips_history_param_adapt_process(void)
{
    merr_t err = kNoErr;
    merr_t result = kNoErr;
    int len = 0;
    bool parameter_convert_flag = false;
    bool user_data_convert_flag = false;

    void *user_config_data = NULL;

    len = sizeof(get_philips_running_status()->flash_param.parameter1_convert_kv_finish);
    err = mkv_item_get("pck_finish", &get_philips_running_status()->flash_param.parameter1_convert_kv_finish, &len);
    if((err != kNoErr) || (get_philips_running_status()->flash_param.parameter1_convert_kv_finish == false))
    {
        app_log("need convert parameter1 to kv");
        parameter_convert_flag = true;
    }
    else
    {
        app_log("don't need convert parameter1 to kv");
    }

    len = sizeof(get_philips_running_status()->flash_param.param_old_convert_new_finish);
    err = mkv_item_get("pocn_finish", &get_philips_running_status()->flash_param.param_old_convert_new_finish, &len);
    if((err != kNoErr) || (get_philips_running_status()->flash_param.param_old_convert_new_finish == false))
    {
        app_log("need convert user data to kv");
        user_data_convert_flag = true;
    }
    else
    {
        app_log("don't need convert user data to kv");
    }

    if(parameter_convert_flag == false && user_data_convert_flag == false)
    {
        return ;
    }
    else if(parameter_convert_flag == true || user_data_convert_flag == true)
    {
        result = philips_read_configuration(&user_config_data);
    }

    if(user_data_convert_flag == true && result == kNoErr && user_config_data != NULL)
    {
        philips_user_config_data_adapt_process(user_config_data);
    }
    else if(result == kGeneralErr ||get_philips_running_status()->flash_param.param_old_convert_new_finish == false)
    {
        // parameter1分区数据读取校验错误，用户数据不需要处理，KV保存标志位，不进行下次处理
        get_philips_running_status()->flash_param.param_old_convert_new_finish = true;
        mkv_item_set("pocn_finish", &get_philips_running_status()->flash_param.param_old_convert_new_finish, sizeof(get_philips_running_status()->flash_param.param_old_convert_new_finish));
    }

    if(user_config_data)
    {
        free(user_config_data);
        user_config_data = NULL;
    }

    if(get_philips_running_status()->flash_param.param_old_convert_new_finish == true && get_philips_running_status()->flash_param.parameter1_convert_kv_finish == true)
    {
        app_log("erase INFO partition");
        mhal_flash_erase(MODULE_PARTITION_INFO, 0, mhal_flash_get_info(MODULE_PARTITION_INFO)->partition_length);
    }

    return ;
}