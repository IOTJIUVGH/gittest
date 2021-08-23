#include "philips_uart.h"

#include "philips_param.h"
#include "philips_device.h"

#include "philips_log.h"
#define app_log(M, ...)							philips_custom_log("", M, ##__VA_ARGS__)

static queue_message_t f_last_queue_msg = {MT_UART_SEND, NULL, 0x00};

static mos_queue_id_t uart_send_msg_queue = NULL;

static mos_thread_id_t uart_process_thread_handle = NULL;

static mxos_time_t recv_msg_time = 0;
static mxos_time_t send_msg_time = 0;
static mxos_time_t last_memory_dump_time = 0;

/**
 * external
 * guduyl_crc_ccitt
 * CRC CCITT (0x1021), initial value (0xFFFF)
 * @param pDataIn: a pointer to the head of payload
 * @param iLenIn: length of payload
 * @return: uint16_t crc result
 */
uint16_t philips_crc_ccitt(uint8_t *pDataIn, uint32_t iLenIn)
{
	unsigned char const CrcOffSet = 0;
	uint16_t temp = 0;
	uint16_t crc = 0xffff;
	uint16_t const POLY16 = 0x1021;
	uint32_t i, j;
	for ( i = 0; i < iLenIn; ++i )
	{
		for ( j = 0; j < 8; ++j )
		{
			temp = ((pDataIn[i + CrcOffSet] << j) & 0x80) ^ ((crc & 0x8000) >> 8);
			crc <<= 1;
			if ( temp != 0 )
			{
				crc ^= POLY16;
			}
		}
	}
	return crc;
}

/**
 * philips_hex_dump
 * @param data
 * @param len
 * @param filename
 * @param linenum
 */
extern mos_mutex_id_t stdio_tx_mutex;

void philips_hex_dump(const char *from, const char *to, uint8_t *data, uint32_t len, const char *filename, const int linenum)
{
#ifndef PHILIPS_SECURITY
	mos_mutex_lock(stdio_tx_mutex);
	printf("[%ld][%s:%4d] %s--->%s [", mos_time(), filename, linenum, from, to);
	for ( uint32_t i = 0; i < len; ++i )
	{
		printf("%02X ", *(data + i));
	}
	printf("]\r\n");
	mos_mutex_unlock(stdio_tx_mutex);
#endif
}

bool check_tail(uint8_t* inBuf, uint32_t inBufLen)
{
#ifndef PHILIPS_SECURITY
	if(get_philips_running_status()->uart_log_switch)
	{
		philips_hex_dump("MCU", "WIFI", inBuf, inBufLen, SHORT_FILE, __LINE__);
	}
#endif

	uint16_t crc = philips_crc_ccitt(inBuf + 5, inBufLen - 7);

	/* check */
	uint16_t recvcrc = inBuf[inBufLen - 1] + (inBuf[inBufLen - 2] << 8);
    // app_log( "%s check tail, recv: %04X, crc: %04X", "======", recvcrc, crc );

	return crc == recvcrc ? true : false;
}

bool set_tail(uint8_t *inBuf, uint32_t inBuflen)
{
    uint16_t crc = philips_crc_ccitt(inBuf + 5, inBuflen - 7);
	inBuf[inBuflen - 2] = (crc >> 8) & 0xFF;
	inBuf[inBuflen - 1] = (crc >> 0) & 0xFF;
	//保存最后一条串口指令
	f_last_queue_msg.data_len = inBuflen;
	f_last_queue_msg.data = (uint8_t *)realloc(f_last_queue_msg.data, f_last_queue_msg.data_len);
	memcpy(f_last_queue_msg.data, inBuf, f_last_queue_msg.data_len);

#ifndef PHILIPS_SECURITY
	if(get_philips_running_status()->uart_log_switch)
	{
		philips_hex_dump("WIFI", "MCU", inBuf, inBuflen, SHORT_FILE, __LINE__);
	}
#endif

	return true;
}

merr_t get_uart_packet(uint8_t *buf, int buflen, uint32_t timeout1, uint32_t timeout2)
{
	merr_t err = kNoErr;
	int datalen = 0;
	uint8_t *p = buf;
    uint32_t read_len = 0;

	while ( 1 )
	{
		/* start */
        read_len = 1;
        err = mhal_uart_read(MXOS_APP_UART, p, &read_len, timeout1);
        if(err != kNoErr || *p != 0xFE)
            goto exit;
        ++p;

        read_len = 1;
        err = mhal_uart_read(MXOS_APP_UART, p, &read_len, timeout2);
        if(err != kNoErr || *p != 0xFF)
            goto exit;
        ++p;

        /* id */
        read_len = 1;
        err = mhal_uart_read(MXOS_APP_UART, p, &read_len, timeout2);
        if(err != kNoErr)
            goto exit;
        ++p;

        /* length */
        read_len = 1;
		err = mhal_uart_read(MXOS_APP_UART, p, &read_len, timeout2);
		if ( err != kNoErr )
			goto exit;
		datalen = *(p) << 8;
		++p;

        read_len = 1;
		err = mhal_uart_read(MXOS_APP_UART, p, &read_len, timeout2);
		if ( err != kNoErr )
			goto exit;
		datalen += *(p);
		++p;

        /* payload & crc */
		for ( int i = datalen + 2; i > 0; --i )
		{
            read_len = 1;
			err = mhal_uart_read(MXOS_APP_UART, p, &read_len, timeout2);
			if ( err != kNoErr )
				goto exit;
			++p;
		}

		return datalen + 7;
	}

exit:
//	app_log("ERROR: %02x, datalen %d", *p, datalen);
	return -1;
}

void uart_process_thread(void *arg)
{
	merr_t err = kNoErr;
    int recv_len = -1;
    uint8_t *recv_buf = NULL;
    queue_message_t queue_msg;
    mxos_time_t time_now = 0;

	recv_buf = (uint8_t *)malloc(UARTRECV_BUFFER_LENGTH);
	require_string(recv_buf, exit, "malloc uart recv buff fail");

    // 串口看门狗复位
	err = mxos_system_monitor_register(&get_philips_running_status()->uart_monitor, PHILIPS_SOFT_WATCHDOG_TIMEOUT);
	require_noerr_string(err, exit, "uart watchdog register fail");

    while(1)
    {
        //接收串口数据
        memset(recv_buf, 0, UARTRECV_BUFFER_LENGTH);
        recv_len = get_uart_packet(recv_buf, UARTRECV_BUFFER_LENGTH, 50, 50);
        if(recv_len > 0)
        {
            if(check_tail(recv_buf, recv_len))
            {
                recv_msg_time = mos_time();
                uart_data_process(recv_buf, recv_len);
            }
        }

        //获取当前系统时间
        time_now = mos_time();

		// 串口线程看门狗喂狗检测
		if(time_now - get_philips_running_status()->uart_feed_dog_time >= PHILIPS_SOFT_FEEDDOG_PERIOD)
		{
			mxos_system_monitor_update(&get_philips_running_status()->uart_monitor, PHILIPS_SOFT_WATCHDOG_TIMEOUT);
			get_philips_running_status()->uart_feed_dog_time = time_now;
			app_log("feed uart watchdog");
		}

        if(time_now > last_memory_dump_time + PHILIPS_MEMORY_DUMP_TIMEOUT)
        {
            app_log("free_memory = %d, mqtt_connected = %s", mos_mallinfo()->free, ((get_philips_running_status()->wifi_status == WRS_wifiui_connect_cloud) ? "true" : "false") );
            last_memory_dump_time = time_now;
        }

        //uart yield处理
		uart_yield( send_msg_time, recv_msg_time );

        //控制串口发送间隔为50MS以上
        if(time_now < send_msg_time + 50)
        {
            continue;
        }

        //判断串口发送队列是否为空，发送数据。
        memset(&queue_msg, 0, sizeof(queue_message_t));
        if(mos_queue_get_total(uart_send_msg_queue) == 0)
        {
            continue;
        }
        if(mos_queue_pop(uart_send_msg_queue, &queue_msg, 200) == kNoErr)
        {
            if(queue_msg.data_len == 0 || queue_msg.data == NULL)
            {
                app_log("ERROR: invaild message in queue!");
            }
            else
            {
                set_tail(queue_msg.data, queue_msg.data_len);
				if (queue_msg.data[2] == 0x03 && (queue_msg.data[5] == 0x01 || queue_msg.data[5] == 0x03)) //ID=0x03 && PORT=0x01 or 0x03控制指令下发
                    get_philips_running_status()->device_upload_manage.app_control_upload = true;
                mhal_uart_write(MXOS_APP_UART, queue_msg.data, queue_msg.data_len);
                send_msg_time = mos_time();
            }

            if(queue_msg.data)
            {
                free(queue_msg.data);
                queue_msg.data = NULL;
            }
        }
    }

exit:
	if(recv_buf)
		free(recv_buf);

    mos_thread_delete(NULL);
}

merr_t philips_uart_start(void)
{
    merr_t err = kNoErr;

    mhal_uart_config_t uart_config;
    mhal_uart_pinmux_t uart_pinmux;

	uart_config.baudrate = 115200;
	uart_config.data_width = DATA_WIDTH_8BIT;
	uart_config.parity = NO_PARITY;
	uart_config.stop_bits = STOP_BITS_1;
	uart_config.flow_control = FLOW_CONTROL_DISABLED;
	uart_config.buffersize = UART_BUFFER_LENGTH;

    uart_pinmux.tx = MXOS_APP_UART_TXD;
    uart_pinmux.rx = MXOS_APP_UART_RXD;
    uart_pinmux.cts = MXOS_APP_UART_CTS;
    uart_pinmux.rts = MXOS_APP_UART_RTS;

    err = mhal_uart_open( MXOS_APP_UART, &uart_config, &uart_pinmux);
    require_noerr(err, exit);

    uart_process_thread_handle = mos_thread_new(MOS_APPLICATION_PRIORITY, "UART PROCESS", uart_process_thread, 1024*4, NULL);
    require_action( (uart_process_thread_handle != NULL), exit, err = kGeneralErr);

exit:
    return err;
}

merr_t philips_uart_param_init(void)
{
	merr_t err = kNoErr;

	uart_send_msg_queue = mos_queue_new(sizeof(queue_message_t), 8);
    require_action( (uart_send_msg_queue != NULL), exit, err = kGeneralErr);
exit:
	return err;
}

merr_t philips_uart_queue_push(queue_message_t *msg)
{
    merr_t err = kNoErr;

    require_action(msg, exit, err = kParamErr);

    err = mos_queue_push(uart_send_msg_queue, msg, 100);
    require_noerr(err, exit);
exit:
    return err;
}

merr_t philips_uart_queue_push_history_last(void)
{
    merr_t err = kNoErr;
	queue_message_t msg = {MT_UART_SEND, NULL, 0x00};

    require_action(f_last_queue_msg.data, exit, err = kParamErr);

	msg.data_len = f_last_queue_msg.data_len;
	msg.data = (uint8_t *)malloc(msg.data_len);
	require_action( (msg.data != NULL), exit, err = kGeneralErr );
	memset(msg.data, 0, msg.data_len);
	memcpy(msg.data, f_last_queue_msg.data, msg.data_len);

    err = mos_queue_push(uart_send_msg_queue, &msg, 100);
    require_noerr(err, exit);

exit:
	if(err != kNoErr)
	{
		if(msg.data)
		{
			free(msg.data);
			msg.data = NULL;
		}
	}
    return err;
}

mxos_time_t philips_get_uart_recv_msg_time(void)
{
	return recv_msg_time;
}

mxos_time_t philips_get_uart_send_msg_time(void)
{
	return send_msg_time;
}

mxos_time_t philips_get_memory_dump_time(void)
{
	return last_memory_dump_time;
}
