#include "mxos.h"
#include "mxos_config.h"
#include "CoAPExport.h"
#include "CoAPPlatform.h"
#include "LinkListUtils.h"

#include "philips_coap_internal.h"

#include "philips_param.h"
#include "philips_basic_api.h"
#include "philips_wifi.h"

#include "philips_log.h"

#ifdef PHILIPS_COAP_DEBUG_ENABLE
#define app_log(M, ...)							philips_custom_log("FOG", M, ##__VA_ARGS__)
#else
#define app_log(M, ...)
#endif

#define MULTICAST_ADDR							"224.0.1.187"
#define COAP_PORT								5683

#define DEV_INFO_URI							"/sys/dev/info"
#define DEV_INFO_ENCRYPTION_URI					"/sys/dev/info/encryption"
#define DEV_STATUS_URL							"/sys/dev/status"
#define DEV_CONTROL_URL							"/sys/dev/control"
#define DEV_SYNC_URL							"/sys/dev/sync"

#define COAP_RESP_OK							"{\"status\":\"success\"}"
#define COAP_RESP_FAILED						"{\"status\":\"failed\"}"

#define CONST_ENCRYPT_KEY						"JiangPan"
#define MESSAGE_LOST_PACKETS					10
#define MESSAGE_MIN_ID							1
#define MESSAGE_MAX_ID							2000000000

static CoAPContext *coap_context = NULL;

typedef struct _client_IP_MsgID
{
	NetworkAddr ClientInfo;
	unsigned int ClientMsgID;
	unsigned int ServerMsgID;
} client_IP_MsgID_t;
static linked_list_t client_IP_MsgID_list;

#define DEVICEINFO_SIZE							1024
static unsigned char *f_deviceinfo = NULL;

/***********************************************
 *
 ***********************************************/

static bool client_node_compare(linked_list_node_t *node_to_compare, void *user_data)
{
	client_IP_MsgID_t *node = (client_IP_MsgID_t *)(node_to_compare->data);
	client_IP_MsgID_t *user = (client_IP_MsgID_t *)user_data;
	if ( memcmp(node->ClientInfo.addr, user->ClientInfo.addr, sizeof(user->ClientInfo.addr)) == 0 )
	{
		return true;
	}
	else
	{
		return false;
	}
}

/***********************************************
 * encrypt
 ***********************************************/

static merr_t philips_add_SHA256(uint8_t *data, unsigned int *len)
{
	merr_t err = kNoErr;

	USHAContext usercontext;
	uint8_t hash[SHA256HashSize];
	USHAReset(&usercontext, SHA256);
	USHAInput(&usercontext, data, *len);
	USHAResult(&usercontext, hash);

	for ( int i = 0, j = *len; i < SHA256HashSize; ++i, j += 2 )
	{
		sprintf((char *)(data + j), "%02X", hash[i]);
	}

	*len += 64;

	return err;
}

static merr_t philips_encrypt_message(CoAPContext *context, NetworkAddr* remote, CoAPLenString *src, CoAPLenString *dest)
{
	merr_t err = kNoErr;
	uint8_t *ciphertext = NULL;

	client_IP_MsgID_t *client = (client_IP_MsgID_t *)malloc(sizeof(client_IP_MsgID_t));
	if ( client == NULL )
	{
		err = kNoMemoryErr;
		goto exit;
	}
	memcpy(&client->ClientInfo, remote, sizeof(client_IP_MsgID_t));
	linked_list_node_t *node = NULL;
	err = linked_list_find_node(&client_IP_MsgID_list, client_node_compare, client, &node);
	if ( err != kNoErr )
	{
		app_log("ENCRYPT === client is not in the list, find res = %d", err);
		goto exit;
	}
	client_IP_MsgID_t *found_client = (client_IP_MsgID_t *)(node->data);
	if ( found_client->ClientMsgID == MESSAGE_MAX_ID )
		found_client->ClientMsgID = MESSAGE_MIN_ID;
	else
		++found_client->ClientMsgID;

	dest->data = (unsigned char *)malloc(2048);
	if ( dest->data == NULL )
	{
		err = kNoMemoryErr;
		goto exit;
	}
	memset(dest->data, 0x00, 2048);
	dest->len = sprintf((char *)(dest->data), "%02X%02X%02X%02X",
						(found_client->ClientMsgID >> 24) & 0xFF,
						(found_client->ClientMsgID >> 16) & 0xFF,
						(found_client->ClientMsgID >> 8) & 0xFF,
						(found_client->ClientMsgID >> 0) & 0xFF);

	uint8_t pre_key[AES_BLOCK_SIZE + 1] = {0};
	uint8_t md5_pre_key[AES_BLOCK_SIZE] = {0};
	memcpy(pre_key, CONST_ENCRYPT_KEY, 8);
	memcpy(pre_key + 8, dest->data, 8);
	md5_context ctx;
	InitMd5(&ctx);
	Md5Update(&ctx, pre_key, AES_BLOCK_SIZE);
	Md5Final(&ctx, md5_pre_key);

	uint8_t key[AES_BLOCK_SIZE] = {0};
	sprintf((char *)pre_key, "%02X%02X%02X%02X%02X%02X%02X%02X", md5_pre_key[0], md5_pre_key[1], md5_pre_key[2], md5_pre_key[3], md5_pre_key[4], md5_pre_key[5], md5_pre_key[6], md5_pre_key[7]);
	memcpy(key, pre_key, AES_BLOCK_SIZE);
	uint8_t iv[AES_BLOCK_SIZE] = {0};
	sprintf((char *)pre_key, "%02X%02X%02X%02X%02X%02X%02X%02X", md5_pre_key[8], md5_pre_key[9], md5_pre_key[10], md5_pre_key[11], md5_pre_key[12], md5_pre_key[13], md5_pre_key[14], md5_pre_key[15]);
	memcpy(iv, pre_key, AES_BLOCK_SIZE);
	Aes enc;
	AesSetKey(&enc, key, AES_BLOCK_SIZE, iv, AES_ENCRYPTION);
	ciphertext = (unsigned char *)malloc(1024);
	if ( ciphertext == NULL )
	{
		err = kNoMemoryErr;
		goto exit;
	}
	memset(ciphertext, 0x00, 1024);
	int aescbclen = philipsAesCbcEncryptPkcs5Padding(&enc, ciphertext, src->data, src->len);
	if ( aescbclen == 0 )
	{
		err = kAuthenticationErr;
		goto exit;
	}
	for ( int i = 0, j = dest->len; i < aescbclen; ++i, j += 2 )
	{
		sprintf((char *)(dest->data + j), "%02X", ciphertext[i]);
	}
	dest->len += (aescbclen << 1);

	philips_add_SHA256(dest->data, (unsigned int *)&(dest->len));

#ifdef PHILIPS_COAP_TRC_ENABLE
	app_log("ENCRYPT === message len = %u, message = %s to %s:%d", dest->len, (char *)(dest->data), remote->addr, remote->port);
#endif

	exit:
	if ( client )
	{
		free(client);
		client = NULL;
	}
	if ( ciphertext )
	{
		free(ciphertext);
		ciphertext = NULL;
	}
	if ( err != kNoErr )
	{
		free(dest->data);
		dest->data = NULL;
	}
	return err;
}

/***********************************************
 * decrypt
 ***********************************************/

static merr_t philips_check_MsgID(NetworkAddr *remote, const char *data, client_IP_MsgID_t *found_client, unsigned int *remoteID)
{
	merr_t err = kNoErr;

	if ( !hex2uint32(data, 8, remoteID) )
	{
		err = kDateErr;
		goto exit;
	}

	unsigned int need_min_id = found_client->ServerMsgID + 1 > MESSAGE_MAX_ID ? MESSAGE_MIN_ID : found_client->ServerMsgID + 1;
	unsigned int need_max_id = found_client->ServerMsgID + MESSAGE_LOST_PACKETS > MESSAGE_MAX_ID ? MESSAGE_MIN_ID + MESSAGE_LOST_PACKETS - (MESSAGE_MAX_ID - found_client->ServerMsgID) : found_client->ServerMsgID + MESSAGE_LOST_PACKETS;
	if ( need_min_id < need_max_id )
	{
		if ( *remoteID < need_min_id || *remoteID > need_max_id )
		{
			err = kIDErr;
			goto exit;
		}
	}
	else
	{
		if ( (*remoteID > need_max_id && *remoteID < need_min_id) || *remoteID < MESSAGE_MIN_ID || *remoteID > MESSAGE_MAX_ID )
		{
			err = kIDErr;
			goto exit;
		}
	}

	exit:
	return err;
}

static merr_t philips_check_SHA256(const uint8_t *data, unsigned int len)
{
	merr_t err = kNoErr;

	USHAContext usercontext;
	uint8_t hash[SHA256HashSize];
	USHAReset(&usercontext, SHA256);
	USHAInput(&usercontext, data, len - 64);
	USHAResult(&usercontext, hash);

	uint8_t cal_hash[(SHA256HashSize << 2) + 1];
	memset(cal_hash, 0x00, (SHA256HashSize << 2) + 1);
	for ( int i = 0, j = 0; i < SHA256HashSize; ++i, j += 2 )
	{
		sprintf((char *)(cal_hash + j), "%02X", hash[i]);
	}

	if ( memcmp(cal_hash, data + len - 64, SHA256HashSize << 2) != 0 )
		err = kChecksumErr;

	return err;
}

static word32 philips_AesCbcDecryptPkcs5Padding(Aes* aes, byte* output, const byte* input, word32 sz)
{
	if ( aes == NULL || output == NULL || input == NULL || sz < 16 || sz % 16 > 0 )
		return 0;

	byte* output_align = malloc(sz);
	if ( output_align == NULL )
		return 0;

	AesCbcDecrypt(aes, output_align, input, sz);

	byte pad_byte = output_align[sz - 1];
	if ( pad_byte >= sz )
		return 0;

	word32 act_size = sz - pad_byte;

	byte pad_cmp[16];
	memset(pad_cmp, pad_byte, 16);
	if ( memcmp(output_align + act_size, pad_cmp, pad_byte) )
		return 0;

	memcpy(output, output_align, act_size);

	free(output_align);

	return act_size;
}

static merr_t philips_decrypt_message(const char *in, unsigned int inlen, NetworkAddr *remote, char *out, unsigned int *outlen)
{
	merr_t err = kNoErr;
	client_IP_MsgID_t *client = NULL;
	uint8_t *plaintext = NULL;

	if ( in == NULL || remote == NULL || out == NULL || outlen == NULL )
	{
		err = kParamErr;
		goto exit;
	}

#ifdef PHILIPS_COAP_TRC_ENABLE
	app_log("CONTROL === message len = %u, message = %s from %s:%d", inlen, in, remote->addr, remote->port);
#endif

	client = (client_IP_MsgID_t *)malloc(sizeof(client_IP_MsgID_t));
	if ( client == NULL )
	{
		err = kNoMemoryErr;
		goto exit;
	}
	memset(client, 0x00, sizeof(client_IP_MsgID_t));
	memcpy(&client->ClientInfo, remote, sizeof(client_IP_MsgID_t));
	linked_list_node_t *node = NULL;
	err = linked_list_find_node(&client_IP_MsgID_list, client_node_compare, client, &node);
	if ( err != kNoErr )
	{
		app_log("CONTROL === client is not in the list, find res = %d", err);
		goto exit;
	}
	client_IP_MsgID_t *found_client = (client_IP_MsgID_t *)(node->data);
	unsigned int remoteID = 0;

	if ( (err = philips_check_MsgID(remote, in, found_client, &remoteID)) != kNoErr )
	{
		goto exit;
	}

	if ( (err = philips_check_SHA256((const uint8_t *)in, inlen)) != kNoErr )
	{
		goto exit;
	}

	uint8_t pre_key[AES_BLOCK_SIZE + 1] = {0};
	uint8_t md5_pre_key[AES_BLOCK_SIZE] = {0};
	memcpy(pre_key, CONST_ENCRYPT_KEY, 8);
	memcpy(pre_key + 8, in, 8);
	md5_context ctx;
	InitMd5(&ctx);
	Md5Update(&ctx, pre_key, AES_BLOCK_SIZE);
	Md5Final(&ctx, md5_pre_key);
	uint8_t key[AES_BLOCK_SIZE] = {0};
	sprintf((char *)pre_key, "%02X%02X%02X%02X%02X%02X%02X%02X", md5_pre_key[0], md5_pre_key[1], md5_pre_key[2], md5_pre_key[3], md5_pre_key[4], md5_pre_key[5], md5_pre_key[6], md5_pre_key[7]);
	memcpy(key, pre_key, AES_BLOCK_SIZE);
	uint8_t iv[AES_BLOCK_SIZE] = {0};
	sprintf((char *)pre_key, "%02X%02X%02X%02X%02X%02X%02X%02X", md5_pre_key[8], md5_pre_key[9], md5_pre_key[10], md5_pre_key[11], md5_pre_key[12], md5_pre_key[13], md5_pre_key[14], md5_pre_key[15]);
	memcpy(iv, pre_key, AES_BLOCK_SIZE);
	Aes enc;
	AesSetKey(&enc, key, AES_BLOCK_SIZE, iv, AES_DECRYPTION);
	plaintext = (unsigned char *)malloc(1024);
	if ( plaintext == NULL )
	{
		err = kNoMemoryErr;
		goto exit;
	}
	memset(plaintext, 0x00, 1024);
	int i = 0, j = 0, k = inlen - 64;
	for ( i = 8, j = 0; i < k; i += 2, ++j )
	{
		hex2uint8(in + i, plaintext + j);
	}
	int aescbclen = philips_AesCbcDecryptPkcs5Padding(&enc, (uint8_t *)out, plaintext, j);
	if ( aescbclen == 0 )
	{
		err = kAuthenticationErr;
		goto exit;
	}

	*outlen = aescbclen;
	out[*outlen] = '\0';
	found_client->ServerMsgID = remoteID;

	exit:
	if ( client )
		free(client);
	if ( plaintext )
		free(plaintext);
	return err;
}

/***********************************************
 * callbacks
 ***********************************************/

static void device_info_callback(CoAPContext *context, const char *paths, NetworkAddr *remote, CoAPMessage *request)
{
	char *dev_info = "{\"product_id\":\"%s\",\"device_id\":\"%s\",\"name\":\"%s\",\"type\":\"%s\",\"modelid\":\"%s\",\"swversion\":\"%s\",\"option\":\"1\"}";
	CoAPMessage response;

	if ( get_philips_running_status()->flash_param.device_id[0] == '\0' )
		return;

	if ( f_deviceinfo == NULL )
	{
		f_deviceinfo = (unsigned char *)malloc(DEVICEINFO_SIZE);
		if ( f_deviceinfo == NULL )
		{
//			err = kNoMemoryErr;
//			goto exit;
			return;
		}
	}
	memset(f_deviceinfo, 0x00, DEVICEINFO_SIZE);

	CoAPMessage_init(&response);
	CoAPMessageType_set(&response, COAP_MESSAGE_TYPE_NON);
	CoAPMessageCode_set(&response, COAP_MSG_CODE_205_CONTENT);
	CoAPMessageId_set(&response, request->header.msgid);
	CoAPMessageToken_set(&response, request->token, request->header.tokenlen);

	CoAPUintOption_add(&response, COAP_OPTION_CONTENT_FORMAT, COAP_CT_APP_JSON);
	snprintf((char *)f_deviceinfo, DEVICEINFO_SIZE, dev_info,
			 get_philips_running_status()->flash_param.product_id,
			 get_philips_running_status()->flash_param.device_id,
			 get_philips_running_status()->device_name,
			 get_philips_running_status()->device_type, 
			 get_philips_running_status()->flash_param.device_modelid, 
			 get_philips_running_status()->device_swversion);
	CoAPMessagePayload_set(&response, f_deviceinfo, strlen((char *)f_deviceinfo));

	mos_msleep(rand() % 1000);
	CoAPMessage_send(context, remote, &response);
	app_log("INFO === COAP send device info %s to %s:%d", f_deviceinfo, remote->addr, remote->port);

	mos_msleep(rand() % 1000);
	remote->port = 5683;
	CoAPMessage_send(context, remote, &response);

	CoAPMessage_destory(&response);
}

static void device_info_encryption_callback(CoAPContext *context, const char *paths, NetworkAddr *remote, CoAPMessage *request)
{
	merr_t err = kGeneralErr;
	char *dev_info = "{\"product_id\":\"%s\",\"didt\":\"%s\",\"name\":\"%s\",\"type\":\"%s\",\"modelid\":\"%s\",\"swversion\":\"%s\",\"option\":\"3\"}";
	CoAPMessage response;
	uint8_t *didt_key = NULL, *didt_iv = NULL;
	md5_context ctx;
	Aes enc;
	char *textbuf = NULL;
	uint32_t textbuflen = 0;
	uint8_t *ciphertext = NULL;
	uint8_t md5_res[16];

	if ( get_philips_running_status()->flash_param.device_id[0] == '\0' )
		return;

	if ( f_deviceinfo == NULL )
	{
		f_deviceinfo = (unsigned char *)malloc(DEVICEINFO_SIZE);
		if ( f_deviceinfo == NULL )
		{
//			err = kNoMemoryErr;
//			goto exit;
			return;
		}
	}
	memset(f_deviceinfo, 0x00, DEVICEINFO_SIZE);

	CoAPMessage_init(&response);
	CoAPMessageType_set(&response, COAP_MESSAGE_TYPE_NON);
	CoAPMessageCode_set(&response, COAP_MSG_CODE_205_CONTENT);
	CoAPMessageId_set(&response, request->header.msgid);
	CoAPMessageToken_set(&response, request->token, request->header.tokenlen);

	// malloc key and iv
	if ( didt_key == NULL )
	{
		didt_key = (uint8_t *)malloc(AES_BLOCK_SIZE + 1);
	}
	if ( didt_iv == NULL )
	{
		didt_iv = (uint8_t *)malloc(AES_BLOCK_SIZE + 1);
	}
	require_action(didt_key != NULL && didt_iv != NULL, exit, err = kNoMemoryErr);

	// generate key and iv
	InitMd5(&ctx);
	Md5Update(&ctx, (uint8_t *)CONST_ENCRYPT_KEY, strlen(CONST_ENCRYPT_KEY));
	Md5Update(&ctx, (uint8_t *)remote->addr, strlen(remote->addr));
	Md5Final(&ctx, md5_res);
	snprintf((char *)didt_key, AES_BLOCK_SIZE + 1, "%02X%02X%02X%02X%02X%02X%02X%02X", md5_res[0], md5_res[1], md5_res[2], md5_res[3], md5_res[4], md5_res[5], md5_res[6], md5_res[7]);
	snprintf((char *)didt_iv, AES_BLOCK_SIZE + 1, "%02X%02X%02X%02X%02X%02X%02X%02X", md5_res[8], md5_res[9], md5_res[10], md5_res[11], md5_res[12], md5_res[13], md5_res[14], md5_res[15]);

	// set key and iv
	AesSetKey(&enc, didt_key, AES_BLOCK_SIZE, didt_iv, AES_ENCRYPTION);

	// generate plaintext
	textbuf = (char *)malloc(1024);
	require_action(textbuf != NULL, exit, err = kNoMemoryErr);
	memset(textbuf, 0x00, 1024);
	textbuflen = sprintf(textbuf, "%s&%s", get_philips_running_status()->flash_param.device_id, philips_get_device_token(false));

	// generate ciphertext
	ciphertext = (uint8_t *)malloc(1024);
	require_action(ciphertext != NULL, exit, err = kNoMemoryErr);
	memset(ciphertext, 0x00, 1024);
	int aescbclen = philipsAesCbcEncryptPkcs5Padding(&enc, ciphertext, (uint8_t *)textbuf, textbuflen);
	if ( aescbclen == 0 )
	{
		err = kAuthenticationErr;
		goto exit;
	}

	// transform ciphertext to hex string
	memset(textbuf, 0x00, 1024);
	textbuflen = 0;
	for ( int i = 0; i < aescbclen; ++i )
	{
		sprintf(textbuf + (i << 1), "%02X", ciphertext[i]);
	}

	CoAPUintOption_add(&response, COAP_OPTION_CONTENT_FORMAT, COAP_CT_APP_JSON);
	snprintf((char *)f_deviceinfo, DEVICEINFO_SIZE, dev_info,
			 get_philips_running_status()->flash_param.product_id,
			 textbuf,
			 get_philips_running_status()->device_name,
			 get_philips_running_status()->device_type, 
			 get_philips_running_status()->flash_param.device_modelid, 
			 get_philips_running_status()->device_swversion);
	CoAPMessagePayload_set(&response, f_deviceinfo, strlen((char *)f_deviceinfo));

	mos_msleep(rand() % 1000);
	CoAPMessage_send(context, remote, &response);
	app_log("INFO === COAP send device info %s to %s:%d", f_deviceinfo, remote->addr, remote->port);

	mos_msleep(rand() % 1000);
	remote->port = 5683;
	CoAPMessage_send(context, remote, &response);

	err = kNoErr;
	exit:
	if ( err != kNoErr )
	{
		app_log("device_info_encryption_callback exit with error %d", err);
	}
	CoAPMessage_destory(&response);
	if ( didt_key != NULL )
	{
		free(didt_key);
		didt_key = NULL;
	}
	if ( didt_iv != NULL )
	{
		free(didt_iv);
		didt_iv = NULL;
	}
	if ( textbuf != NULL )
	{
		free(textbuf);
		textbuf = NULL;
	}
	if ( ciphertext != NULL )
	{
		free(ciphertext);
		ciphertext = NULL;
	}
}

static void device_status_callback(CoAPContext *context, const char *paths, NetworkAddr *remote, CoAPMessage *request)
{
	int ret = COAP_SUCCESS;
	merr_t err = kNoErr;
	CoAPMessage response;
	unsigned int observe = 0;
	client_IP_MsgID_t *client = (client_IP_MsgID_t *)malloc(sizeof(client_IP_MsgID_t));

	ret = CoAPUintOption_get(request, COAP_OPTION_OBSERVE, &observe);
	if ( COAP_SUCCESS == ret && 0 == observe )
	{
		memcpy(&client->ClientInfo, remote, sizeof(client_IP_MsgID_t));
		linked_list_node_t *node = NULL;
		err = linked_list_find_node(&client_IP_MsgID_list, client_node_compare, client, &node);
		if ( err != kNoErr )
		{
			app_log("STATUS === client %s:%d is not in the list, find res = %d", client->ClientInfo.addr, client->ClientInfo.port, err);
		}
		else
		{
			app_log("STATUS === client %s:%d has found", client->ClientInfo.addr, client->ClientInfo.port);

			CoAPMessage_init(&response);
			CoAPMessageType_set(&response, COAP_MESSAGE_TYPE_NON);
			CoAPMessageCode_set(&response, COAP_MSG_CODE_205_CONTENT);
			CoAPMessageId_set(&response, request->header.msgid);
			CoAPMessageToken_set(&response, request->token, request->header.tokenlen);
			CoAPObsServer_add(context, paths, remote, request);
			CoAPUintOption_add(&response, COAP_OPTION_OBSERVE, 0);
			CoAPUintOption_add(&response, COAP_OPTION_CONTENT_FORMAT, COAP_CT_APP_JSON);
			CoAPMessagePayload_set(&response, (unsigned char *)COAP_RESP_OK, strlen(COAP_RESP_OK));
//			CoAPMessage_send(context, remote, &response);
			CoAPMessage_destory(&response);

			task_queue_push(NULL, 0, MSG_TYPE_LOCAL_CMD);
		}
	}

	if ( client )
		free(client);
}

static void device_control_callback(CoAPContext *context, const char *paths, NetworkAddr *remote, CoAPMessage *request)
{
	merr_t err = kNoErr;

	unsigned int outlen = 1024;
	char *out = (char *)malloc(outlen);
	memset(out, 0x00, 1024);
	if ( (err = philips_decrypt_message((char *)request->payload, request->payloadlen, remote, out, &outlen)) != kNoErr )
	{
		app_log("CONTROL === decrypt message failed, res = %d", err);

		CoAPMessage response;
		CoAPMessage_init(&response);
		CoAPMessageType_set(&response, COAP_MESSAGE_TYPE_NON);
		CoAPMessageCode_set(&response, COAP_MSG_CODE_205_CONTENT);
		CoAPMessageId_set(&response, request->header.msgid);
		CoAPMessageToken_set(&response, request->token, request->header.tokenlen);
		CoAPUintOption_add(&response, COAP_OPTION_CONTENT_FORMAT, COAP_CT_APP_JSON);
		CoAPMessagePayload_set(&response, (unsigned char *)COAP_RESP_FAILED, strlen(COAP_RESP_FAILED));
		CoAPMessage_send(context, remote, &response);
		CoAPMessage_destory(&response);
	}
	else
	{
		CoAPMessage response;
		CoAPMessage_init(&response);
		CoAPMessageType_set(&response, COAP_MESSAGE_TYPE_NON);
		CoAPMessageCode_set(&response, COAP_MSG_CODE_205_CONTENT);
		CoAPMessageId_set(&response, request->header.msgid);
		CoAPMessageToken_set(&response, request->token, request->header.tokenlen);
		CoAPUintOption_add(&response, COAP_OPTION_CONTENT_FORMAT, COAP_CT_APP_JSON);
		CoAPMessagePayload_set(&response, (unsigned char *)COAP_RESP_OK, strlen(COAP_RESP_OK));
		CoAPMessage_send(context, remote, &response);
		CoAPMessage_destory(&response);

		task_queue_push(out, outlen, MSG_TYPE_LOCAL_CTL);
	}

	if ( out )
		free(out);
}

static void device_sync_callback(CoAPContext *context, const char *paths, NetworkAddr *remote, CoAPMessage *request)
{
	merr_t err = kNoErr;

	client_IP_MsgID_t *client = (client_IP_MsgID_t *)malloc(sizeof(client_IP_MsgID_t));
	memcpy(&client->ClientInfo, remote, sizeof(client_IP_MsgID_t));
	if ( !hex2uint32((char *)(request->payload), 8, &client->ClientMsgID) )
	{
		err = kDateErr;
		goto exit;
	}
	client->ServerMsgID = rand() % (MESSAGE_MAX_ID - MESSAGE_MIN_ID + 1) + MESSAGE_MIN_ID;

	linked_list_node_t *node = NULL;
	err = linked_list_find_node(&client_IP_MsgID_list, client_node_compare, client, &node);
	if ( err != kNoErr )
	{
		linked_list_node_t *newnode = (linked_list_node_t *)malloc(sizeof(linked_list_node_t));
		newnode->data = client;
		linked_list_insert_node_at_rear(&client_IP_MsgID_list, newnode);
		app_log("SYNC === new client: info = %s:%d, ClientMsgID = %08X, ServerMsgID = %08X", client->ClientInfo.addr, client->ClientInfo.port, client->ClientMsgID, client->ServerMsgID);
	}
	else
	{
		client_IP_MsgID_t *found_client = (client_IP_MsgID_t *)(node->data);
		found_client->ClientInfo.port = remote->port;
		found_client->ClientMsgID = client->ClientMsgID;
		found_client->ServerMsgID = client->ServerMsgID;
		app_log("SYNC === client found: info = %s:%d, ClientMsgID = %08X, ServerMsgID = %08X", found_client->ClientInfo.addr, found_client->ClientInfo.port, found_client->ClientMsgID, found_client->ServerMsgID);
	}

	CoAPMessage response;
	CoAPMessage_init(&response);
	CoAPMessageType_set(&response, COAP_MESSAGE_TYPE_NON);
	CoAPMessageCode_set(&response, COAP_MSG_CODE_205_CONTENT);
	CoAPMessageId_set(&response, request->header.msgid);
	CoAPMessageToken_set(&response, request->token, request->header.tokenlen);
	CoAPUintOption_add(&response, COAP_OPTION_CONTENT_FORMAT, COAP_CT_TEXT_PLAIN);

	uint8_t payload[10] = {0};
	int len = sprintf((char *)payload, "%02X%02X%02X%02X",
						(client->ServerMsgID >> 24) & 0xFF,
						(client->ServerMsgID >> 16) & 0xFF,
						(client->ServerMsgID >> 8) & 0xFF,
						(client->ServerMsgID >> 0) & 0xFF);
	CoAPMessagePayload_set(&response, payload, len);

	CoAPMessage_send(context, remote, &response);
	CoAPMessage_destory(&response);

	exit:
	if ( err == kNoErr )
		free(client);
	else
		app_log("SYNC === err = %d", err);
}

void fog_locol_report(char *data, uint32_t len)
{
	if ( (coap_context == NULL) || (data == NULL) || (len == 0) )
		return;

	CoAPObsServer_notify(coap_context, DEV_STATUS_URL, (unsigned char *)data, len, philips_encrypt_message);
}

static void coap_server_thread(void *arg)
{
	CoAPInitParam param;

	app_log("coap server init");

	param.appdata = NULL;
	param.group = MULTICAST_ADDR;
	param.notifier = NULL;
	param.obs_maxcount = 16;
	param.res_maxcount = 32;
	param.port = COAP_PORT;
	param.send_maxcount = 16;
	param.waittime = 2000;

	coap_context = CoAPContext_create(&param);

	linked_list_init(&client_IP_MsgID_list);

	CoAPResource_register(coap_context, DEV_INFO_URI, COAP_PERM_GET, COAP_CT_APP_JSON, 60, device_info_callback);
	CoAPResource_register(coap_context, DEV_INFO_ENCRYPTION_URI, COAP_PERM_GET, COAP_CT_APP_JSON, 60, device_info_encryption_callback);
	CoAPResource_register(coap_context, DEV_STATUS_URL, COAP_PERM_GET, COAP_CT_APP_JSON, 60, device_status_callback);
	CoAPResource_register(coap_context, DEV_CONTROL_URL, COAP_PERM_POST, COAP_CT_APP_JSON, 60, device_control_callback);
	CoAPResource_register(coap_context, DEV_SYNC_URL, COAP_PERM_POST, COAP_CT_APP_JSON, 60, device_sync_callback);

	while ( 1 )
	{
		CoAPMessage_cycle(coap_context);
		mos_msleep(50);
	}
}

merr_t coap_server_start(void)
{
	merr_t err = kNoErr;
	mos_thread_id_t thread = NULL;

	thread = mos_thread_new(MOS_APPLICATION_PRIORITY, "coap", coap_server_thread, 0xA00, NULL);
	require_action( (thread != NULL), exit, err = kGeneralErr);
exit:
	return err;
}
