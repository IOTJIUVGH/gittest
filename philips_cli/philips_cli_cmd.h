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

#ifndef __PHILIPS_CLI_CMD_H__
#define __PHILIPS_CLI_CMD_H__

#include "mxos.h"
#include "mxos_config.h"

#define CHECK_CLI_ARGC(num, usage)				\
do \
{ \
	if ( argc < num ) \
	{ \
		usage; \
		return ; \
	} \
} while ( false )

#define CHECK_CLI_ARGV(num, str)				(strncmp(argv[num], str, strlen(str)) == 0)

void philips_cli_cmd_pause_break(int argc, char **argv);
void philips_cli_cmd_status(int argc, char **argv);
void philips_cli_cmd_delete(int argc, char **argv);
void philips_cli_cmd_log(int argc, char **argv);

#endif
