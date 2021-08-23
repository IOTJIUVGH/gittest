#ifndef __PHILIPS_DEVICE_FLTSTS_PROCESS_H__
#define __PHILIPS_DEVICE_FLTSTS_PROCESS_H__

#include "mxos.h"

void philips_process_fltsts_properties_common(const uint8_t *data);
void philips_process_fltsts_properties_Mars_or_MarsLE(const uint8_t *data);

void start_filter_transform_O2(void);
void philips_o2_filter_adjust(void);

#endif