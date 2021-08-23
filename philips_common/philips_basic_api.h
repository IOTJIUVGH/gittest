#ifndef __PHILIPS_BASIC_API_H__
#define __PHILIPS_BASIC_API_H__

#include "mxos.h"

/**
 * *****************************************************************************
 * hex数据转换
 * *****************************************************************************
 **/
bool hex2uint8(const char *hexstr, unsigned char *out);
bool hex2uint32(const char *hexstr, int len, unsigned int *out);
uint16_t philips_hex2str(char *str, const uint8_t *hex, uint16_t len);
uint16_t philips_hex2str_lowercase(char *str, const uint8_t *hex, uint16_t len);
uint16_t philips_str2hex(unsigned char *hex, const unsigned char *str);
void philips_str_lower2upper(char *str, uint16_t len);
void philips_str_upper2lower(char *str, uint16_t len);

/**
 * *****************************************************************************
 * url encode
 * *****************************************************************************
 **/
int php_htoi(char *s);
char *php_url_encode(char const *s, int len, int *new_length);

/**
 * *****************************************************************************
 * base64字符串检测
 * *****************************************************************************
 **/
bool philips_is_base64_char(char c);

/**
 *******************************************************************************
 * encrypt
 *******************************************************************************
 **/
word32 philipsAesCbcEncryptPkcs5Padding(Aes* aes, byte* output, const byte* input, word32 sz);

#endif