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

#include "philips_cli_cmd.h"

#include "philips_param.h"

#include "philips_log.h"
#define app_log(M, ...)             			philips_custom_log("", M, ##__VA_ARGS__)

#define USAGE_PHILIPS_status					\
do \
{ \
	app_log("USAGE:"); \
	app_log("philips" " " "status" " " "list: dump running status"); \
	app_log("philips" " " "status" " " "list cert: dump certificate and privatekey"); \
	app_log("philips" " " "status" " " "list flash: dump flash param"); \
	app_log("philips" " " "status" " " "list fogid: dump fogid"); \
	app_log("philips" " " "status" " " "list otau: dump otau param"); \
	app_log(" "); \
} while ( false )

void philips_cli_cmd_status(int argc, char **argv)
{
	CHECK_CLI_ARGC(3, USAGE_PHILIPS_status);

	if ( CHECK_CLI_ARGV(2, "list") )
	{
		if ( argc == 3 )
		{
			philips_param_dump();
			app_log(" ");
			USAGE_PHILIPS_status;
		}
		else if ( argc == 4 )
		{
			if ( CHECK_CLI_ARGV(3, "cert") )
			{
				philips_param_dump_cert();
				app_log(" ");
				USAGE_PHILIPS_status;
			}
			else if ( CHECK_CLI_ARGV(3, "flash") )
			{
				philips_param_dump_flash();
				app_log(" ");
				USAGE_PHILIPS_status;
			}
			else if ( CHECK_CLI_ARGV(3, "fogid") )
			{
				philips_param_dump_fogid();
				app_log(" ");
				USAGE_PHILIPS_status;
			}
			else if ( CHECK_CLI_ARGV(3, "otau") )
			{
				philips_param_dump_otau();
				app_log(" ");
				USAGE_PHILIPS_status;
			}
			else
			{
				USAGE_PHILIPS_status;
			}
			
		}
		else
		{
			philips_param_dump();
			app_log(" ");
			USAGE_PHILIPS_status;
		}
	}
	else
	{
		USAGE_PHILIPS_status;
	}
}
