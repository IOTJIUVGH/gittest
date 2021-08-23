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

#ifndef __PHILIPS_DEVICE_MODEL_H__
#define __PHILIPS_DEVICE_MODEL_H__

#include "mxos.h"
#include "cJSON.h"

enum _philips_range
{
	PHILIPS_RANGE_NONE,
	PHILIPS_RANGE_O2,
	PHILIPS_RANGE_MicroMario,
	PHILIPS_RANGE_PUMA,
	PHILIPS_RANGE_MicroCube2,
	PHILIPS_RANGE_Thor,
	PHILIPS_RANGE_Loki,
	PHILIPS_RANGE_Comfort,
	PHILIPS_RANGE_Simba,
	PHILIPS_RANGE_Mario,
	PHILIPS_RANGE_MicroCube1,
	PHILIPS_RANGE_Mars,
	PHILIPS_RANGE_MarsLE,
	PHILIPS_RANGE_Marte,
	PHILIPS_RANGE_COMMON,
	PHILIPS_RANGE_AMOUNT,
};

// Comfort
void philips_process_air_properties_Comfort(const uint8_t *data);
void philips_process_cloud_data_Comfort(char *data, int datalen);
void philips_encode_json_Comfort(char **buf, const char *StatusType, const char *ConnectType, bool head);

// Loki
void philips_process_air_properties_Loki(const uint8_t *data);
void philips_process_cloud_data_Loki(char *data, int datalen);
void philips_encode_json_Loki(char **buf, const char *StatusType, const char *ConnectType, bool head);

// Mario
void philips_process_air_properties_Mario(const uint8_t *data);
void philips_process_cloud_data_Mario(char *data, int datalen);
void philips_encode_json_Mario(char **buf, const char *StatusType, const char *ConnectType, bool head);

// Mars
void philips_process_air_properties_Mars(const uint8_t *data);
void philips_process_cloud_data_Mars(char *data, int datalen);
void philips_encode_json_Mars(char **buf, const char *StatusType, const char *ConnectType, bool head);

// MarsLE
void philips_process_air_properties_MarsLE(const uint8_t *data);
void philips_process_cloud_data_MarsLE(char *data, int datalen);
void philips_encode_json_MarsLE(char **buf, const char *StatusType, const char *ConnectType, bool head);

// Marte
void philips_process_air_properties_Marte(const uint8_t *data);
void philips_process_cloud_data_Marte(char *data, int datalen);
void philips_encode_json_Marte(char **buf, const char *StatusType, const char *ConnectType, bool head);

//common  nala..
void philips_process_air_properties_Common(const uint8_t *data);
void philips_process_cloud_data_Common(char *data, int datalen);
void philips_encode_json_Common(char **buf, const char *StatusType, const char *ConnectType, bool head);

// MicroCube1
void philips_process_air_properties_MicroCube1(const uint8_t *data);
void philips_process_cloud_data_MicroCube1(char *data, int datalen);
void philips_encode_json_MicroCube1(char **buf, const char *StatusType, const char *ConnectType, bool head);

// MicroCube2
void philips_process_air_properties_MicroCube2(const uint8_t *data);
void philips_process_cloud_data_MicroCube2(char *data, int datalen);
void philips_encode_json_MicroCube2(char **buf, const char *StatusType, const char *ConnectType, bool head);

// MicroMario
void philips_process_air_properties_MicroMario(const uint8_t *data);
void philips_process_cloud_data_MicroMario(char *data, int datalen);
void philips_encode_json_MicroMario(char **buf, const char *StatusType, const char *ConnectType, bool head);

// O2
void philips_process_air_properties_O2(const uint8_t *data);
void philips_process_cloud_data_O2(char *data, int datalen);
void philips_encode_json_O2(char **buf, const char *StatusType, const char *ConnectType, bool head);

// PUMA
void philips_process_air_properties_PUMA(const uint8_t *data);
void philips_process_cloud_data_PUMA(char *data, int datalen);
void philips_encode_json_PUMA(char **buf, const char *StatusType, const char *ConnectType, bool head);

// Simba
void philips_process_air_properties_Simba(const uint8_t *data);
void philips_process_cloud_data_Simba(char *data, int datalen);
void philips_encode_json_Simba(char **buf, const char *StatusType, const char *ConnectType, bool head);

// Thor
void philips_process_air_properties_Thor(const uint8_t *data);
void philips_process_cloud_data_Thor(char *data, int datalen);
void philips_encode_json_Thor(char **buf, const char *StatusType, const char *ConnectType, bool head);

// COMMON
void philips_process_air_properties(const uint8_t *data);
void philips_process_fltsts_properties(const uint8_t *data);
void philips_process_cloud_data(char *data, int datalen);
void philips_encode_json(char **buf, const char *StatusType, const char *ConnectType, bool head);
merr_t philips_process_cloud_ota_request_info(cJSON *root);

merr_t philips_check_product_range(void);

#endif
