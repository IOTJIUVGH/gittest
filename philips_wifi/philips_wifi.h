#ifndef __PHILIPS_WIFI_H__
#define __PHILIPS_WIFI_H__

merr_t philips_wifi_config(void);
void philips_start_udp(void);

bool philips_set_device_token(uint8_t *arg1, uint32_t arg1len, uint8_t *arg2, uint32_t arg2len);
char *philips_get_device_token(bool autogenerate);
char *generate_bind_token(void);

void fog_wifi_config_stop(void);

#endif
