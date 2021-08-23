#ifndef __PHILIPS_UART_H__
#define __PHILIPS_UART_H__

#include "mxos.h"
#include "philips_param.h"

#define UARTRECV_BUFFER_LENGTH					256
#define UARTSEND_BUFFER_LENGTH					256

#define PHILIPS_MEMORY_DUMP_TIMEOUT				1000 // ms

#define UART_QUEUE_NUM_MAX                      10

#define UART_BUFFER_LENGTH						2048

merr_t philips_uart_start(void);

merr_t philips_uart_param_init(void);
merr_t philips_uart_queue_push(queue_message_t *msg);
merr_t philips_uart_queue_push_history_last(void);

uint16_t philips_crc_ccitt(uint8_t *pDataIn, uint32_t iLenIn);
void philips_hex_dump(const char *from, const char *to, uint8_t *data, uint32_t len, const char *filename, const int linenum);

mxos_time_t philips_get_uart_recv_msg_time(void);
mxos_time_t philips_get_uart_send_msg_time(void);
mxos_time_t philips_get_memory_dump_time(void);

#endif