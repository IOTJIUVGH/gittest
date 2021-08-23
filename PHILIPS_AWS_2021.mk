############################################################################### 
#
#  The MIT License
#  Copyright (c) 2016 MXCHIP Inc.
#
#  Permission is hereby granted, free of charge, to any person obtaining a copy 
#  of this software and associated documentation files (the "Software"), to deal
#  in the Software without restriction, including without limitation the rights 
#  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
#  copies of the Software, and to permit persons to whom the Software is furnished
#  to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice shall be included in
#  all copies or substantial portions of the Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
#  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
#  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR 
#  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
############################################################################### 


NAME := App_PHILIPS_AWS_2021

ifeq ($(SERVER), STG)
GLOBAL_DEFINES +=ENVIRONMENT_STAGING
else ifeq ($(SERVER), PRO)
GLOBAL_DEFINES +=ENVIRONMENT_PRODUCTION
else
$(error SERVER option ERROR or NO)
endif

ifeq ($(SECURITY), ON)
GLOBAL_DEFINES +=PHILIPS_SECURITY
endif

$(NAME)_SOURCES += main.c

$(NAME)_SOURCES += philips_cli/philips_cli_cmd_delete.c
$(NAME)_SOURCES += philips_cli/philips_cli_cmd_log.c
$(NAME)_SOURCES += philips_cli/philips_cli_cmd_pause_break.c
$(NAME)_SOURCES += philips_cli/philips_cli_cmd_status.c
$(NAME)_SOURCES += philips_cli/philips_cli.c

$(NAME)_SOURCES += philips_coap/philips_coap_process.c
$(NAME)_SOURCES += philips_coap/philips_coap_task.c

$(NAME)_SOURCES += philips_common/philips_basic_api.c
$(NAME)_SOURCES += philips_common/philips_param.c
$(NAME)_SOURCES += philips_common/philips_history_param_adapt.c

$(NAME)_SOURCES += philips_device/philips_device_factory.c
$(NAME)_SOURCES += philips_device/philips_device_fltsts_process.c
$(NAME)_SOURCES += philips_device/philips_device_model.c
$(NAME)_SOURCES += philips_device/philips_device_ota.c
$(NAME)_SOURCES += philips_device/philips_device_upload_manage.c
$(NAME)_SOURCES += philips_device/philips_device.c

$(NAME)_SOURCES += philips_device/philips_device_model/philips_Comfort.c
$(NAME)_SOURCES += philips_device/philips_device_model/philips_Loki.c
$(NAME)_SOURCES += philips_device/philips_device_model/philips_Mario.c
$(NAME)_SOURCES += philips_device/philips_device_model/philips_Mars.c
$(NAME)_SOURCES += philips_device/philips_device_model/philips_MarsLE.c
$(NAME)_SOURCES += philips_device/philips_device_model/philips_Marte.c
$(NAME)_SOURCES += philips_device/philips_device_model/philips_MicroCube1.c
$(NAME)_SOURCES += philips_device/philips_device_model/philips_MicroCube2.c
$(NAME)_SOURCES += philips_device/philips_device_model/philips_MicroMario.c
$(NAME)_SOURCES += philips_device/philips_device_model/philips_O2.c
$(NAME)_SOURCES += philips_device/philips_device_model/philips_PUMA.c
$(NAME)_SOURCES += philips_device/philips_device_model/philips_Simba.c
$(NAME)_SOURCES += philips_device/philips_device_model/philips_Thor.c
$(NAME)_SOURCES += philips_device/philips_device_model/philips_common.c

$(NAME)_SOURCES += philips_https_request/device_https_request.c
$(NAME)_SOURCES += philips_https_request/device_certification.c
$(NAME)_SOURCES += philips_https_request/device_activation.c
$(NAME)_SOURCES += philips_https_request/device_download_certificate.c
$(NAME)_SOURCES += philips_https_request/device_download_privatekey.c
$(NAME)_SOURCES += philips_https_request/device_bind.c
$(NAME)_SOURCES += philips_https_request/device_factory.c
$(NAME)_SOURCES += philips_https_request/device_ota.c
$(NAME)_SOURCES += philips_https_request/device_wifi_log.c
$(NAME)_SOURCES += ../mxos/MXOS/security/SHAUtils/sha1.c

$(NAME)_SOURCES += philips_uart/philips_uart.c

$(NAME)_SOURCES += philips_wifi/philips_wifi.c
$(NAME)_SOURCES += philips_wifi/philips_wifi_api.c
$(NAME)_SOURCES += philips_wifi/philips_device_discovery.c

$(NAME)_SOURCES += philips_mqtt/mqtt_main.c
$(NAME)_SOURCES += philips_mqtt/philips_mqtt_api.c


$(NAME)_INCLUDES += philips_cli
$(NAME)_INCLUDES += philips_coap
$(NAME)_INCLUDES += philips_https_request
$(NAME)_INCLUDES += philips_uart
$(NAME)_INCLUDES += philips_wifi
$(NAME)_INCLUDES += philips_common
$(NAME)_INCLUDES += philips_device
$(NAME)_INCLUDES += philips_device/philips_device_serial
$(NAME)_INCLUDES += philips_mqtt


$(NAME)_COMPONENTS += lib_cJSON
$(NAME)_COMPONENTS += lib_http_file_download
$(NAME)_COMPONENTS += lib_http_short_connection
$(NAME)_COMPONENTS += lib_linkcoap
$(NAME)_COMPONENTS += lib_mqtt
$(NAME)_COMPONENTS += backoffAlgorithm
$(NAME)_COMPONENTS += PHILIPS_AWS_2021/philips_mqtt/logging-stack


GLOBAL_INCLUDES += philips_mqtt
GLOBAL_INCLUDES += philips_mqtt/logging-stack

GLOBAL_DEFINES += LIBRARY_LOG_LEVEL=LOG_NONE
# GLOBAL_DEFINES += LIBRARY_LOG_LEVEL=LOG_ERROR