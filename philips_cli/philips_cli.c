/**
 ******************************************************************************
 * @file
 * @author  Howie Zhao
 * @version
 * @date
 * @brief
 ******************************************************************************
 *
 *  The MIT License
 *  Copyright (c) 2016 MXCHIP Inc.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is furnished
 *  to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 *  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 ******************************************************************************
 */

#include "mxos.h"
#include "mxos_config.h"

#include "philips_cli.h"
#include "philips_cli_cmd.h"

#include "philips_log.h"
#define app_log(format, ...)					philips_custom_log("", format, ##__VA_ARGS__)


#define USAGE_PHILIPS							\
do \
{ \
	app_log("USAGE:"); \
	app_log("philips" " {" "help" " | " "status" " | " "pause" " | "  "delete" " | "  "log" "}"); \
	app_log(" "); \
} while ( false )

#define USAGE_PHILIPS_HELP						\
do \
{ \
	app_log("USAGE:"); \
	app_log("philips"); \
	app_log("    " "help" ": " "dump usage of command \"philips\""); \
	app_log("    " "pause" ": " ""); \
	app_log("    " "status" ": " "process running status"); \
	app_log("    " "delete" ": " "delete philips param"); \
	app_log("    " "log" ": " "debug log on/off"); \
	app_log(" "); \
} while ( false )

static void philips_cmd(int argc, char **argv)
{
	CHECK_CLI_ARGC(2, USAGE_PHILIPS);

	if ( CHECK_CLI_ARGV(1, "help") )
	{
		USAGE_PHILIPS_HELP;
	}
	else if ( CHECK_CLI_ARGV(1, "pause") )
	{
		philips_cli_cmd_pause_break(argc, argv);
	}
	else if ( CHECK_CLI_ARGV(1, "status") )
	{
		philips_cli_cmd_status(argc, argv);
	}
	else if ( CHECK_CLI_ARGV(1, "delete") )
	{
		philips_cli_cmd_delete(argc, argv);
	}
	else if ( CHECK_CLI_ARGV(1, "log") )
	{
		philips_cli_cmd_log(argc, argv);
	}
	else
	{
		USAGE_PHILIPS;
	}
}

static mcli_cmd_t philips_cli[] = 
{
	{"philips", "philips cli cmd", philips_cmd},
};

merr_t philips_cli_init(void)
{
    return mcli_cmds_add(philips_cli, sizeof(philips_cli) / sizeof(mcli_cmd_t));
}
