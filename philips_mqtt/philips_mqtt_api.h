#ifndef __PHILIPS_MQTT_H__
#define __PHILIPS_MQTT_H__

#include "mxos.h"
#include "philips_param.h"

merr_t philips_mqtt_param_init(void);
merr_t philips_mqtt_msg_push_queue(queue_message_t *msg);
merr_t philips_mqtt_client_start(void);
merr_t philips_mqtt_client_close(void);
bool philips_mqtt_client_is_running(void);

merr_t philips_mqtt_public_status(const char *status);
merr_t philips_mqtt_public_notice(const char *code);
merr_t philips_mqtt_public_notice_json_str(const char *code);
merr_t philips_mqtt_public_data(const char *status);
merr_t philips_mqtt_publish_notice_wifi_log(void);

#endif