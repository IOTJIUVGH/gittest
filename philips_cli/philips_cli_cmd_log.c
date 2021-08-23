#include "mxos.h"
#include "mkv.h"

#include "philips_cli_cmd.h"
#include "philips_param.h"

#include "philips_log.h"
#define app_log(M, ...)             			philips_custom_log("", M, ##__VA_ARGS__)

#define USAGE_PHILIPS_log					\
do \
{ \
	app_log("USAGE:"); \
	app_log("philips log uart on/off: uart communicate log on/off"); \
	app_log("philips log ssl on/off: ssl communicate log on/off"); \
	app_log(" "); \
} while ( false )

void philips_cli_cmd_log(int argc, char **argv)
{
    int len = 0;
    CHECK_CLI_ARGC(4, USAGE_PHILIPS_log);

    if( CHECK_CLI_ARGV(2, "uart") )
    {
        if( CHECK_CLI_ARGV(3, "on") )
        {
            get_philips_running_status()->uart_log_switch = 1;
            len = sizeof(get_philips_running_status()->uart_log_switch);
            mkv_item_set("LOGSW_uart", &get_philips_running_status()->uart_log_switch, len);
        }
        else if( CHECK_CLI_ARGV(3, "off") )
        {
            get_philips_running_status()->uart_log_switch = 0;
            len = sizeof(get_philips_running_status()->uart_log_switch);
            mkv_item_set("LOGSW_uart", &get_philips_running_status()->uart_log_switch, len);
        }   
        else
        {
            USAGE_PHILIPS_log;
        }
    }
    else if( CHECK_CLI_ARGV(2, "ssl") )
    {
        if( CHECK_CLI_ARGV(3, "on") )
        {
            get_philips_running_status()->ssl_log_switch = 1;
            len = sizeof(get_philips_running_status()->ssl_log_switch);
            mkv_item_set("LOGSW_ssl", &get_philips_running_status()->ssl_log_switch, len);
        }
        else if( CHECK_CLI_ARGV(3, "off") )
        {
            get_philips_running_status()->ssl_log_switch = 0;
            len = sizeof(get_philips_running_status()->ssl_log_switch);
            mkv_item_set("LOGSW_ssl", &get_philips_running_status()->ssl_log_switch, len);
        }
        else
        {
            USAGE_PHILIPS_log;
        }
    }
    else
    {
        USAGE_PHILIPS_log;
    }

    return ;
}