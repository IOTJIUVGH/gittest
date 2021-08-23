#include "mxos.h"
#include "mkv.h"

#include "philips_cli_cmd.h"
#include "philips_param.h"

#include "philips_log.h"
#define app_log(M, ...)             			philips_custom_log("", M, ##__VA_ARGS__)

#define USAGE_PHILIPS_delete					\
do \
{ \
	app_log("USAGE:"); \
	app_log("philips delete all: delete all params"); \
	app_log("philips delete active: delete active params"); \
	app_log("philips delete cert: delete certificate"); \
	app_log("philips delete pfx: delete privatekey"); \
	app_log("philips delete mqtt: delete mqtt param"); \
    app_log("philips delete kv key_name: kv del kv value"); \
	app_log(" "); \
} while ( false )

void philips_cli_cmd_delete(int argc, char **argv)
{
    CHECK_CLI_ARGC(3, USAGE_PHILIPS_delete);

    if( CHECK_CLI_ARGV(2, "all") )
    {
        philips_clear_flash_param_for_all();
        philips_device_reset();
    }
    else if( CHECK_CLI_ARGV(2, "active") )
    {
        philips_clear_flash_param_for_reactive();
    }
    else if( CHECK_CLI_ARGV(2, "cert") )
    {
        philips_clear_flash_param_for_certificate();
    }
    else if( CHECK_CLI_ARGV(2, "pfx") )
    {
        philips_clear_flash_param_for_privatekey();
    }
    else if( CHECK_CLI_ARGV(2, "mqtt") )
    {
        
    }
    else if( CHECK_CLI_ARGV(2, "kv") )
    {
        if( argc == 4 )
        {
            mkv_item_delete(argv[3]);
        }
        else
        {
            USAGE_PHILIPS_delete;
        }
    }
    else
    {
        USAGE_PHILIPS_delete;
    }

    return;
}