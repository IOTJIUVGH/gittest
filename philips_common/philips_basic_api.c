#include "philips_basic_api.h"

/**
 * *****************************************************************************
 * hex数据转换
 * *****************************************************************************
 **/
bool hex2uint8(const char *hexstr, unsigned char *out)
{

	unsigned char res = 0;
	unsigned char temp = 0;
	for ( int i = 0; i < 2; ++i )
	{
		switch ( hexstr[i] )
		{
		case '0':
			temp = 0;
			break;
		case '1':
			temp = 1;
			break;
		case '2':
			temp = 2;
			break;
		case '3':
			temp = 3;
			break;
		case '4':
			temp = 4;
			break;
		case '5':
			temp = 5;
			break;
		case '6':
			temp = 6;
			break;
		case '7':
			temp = 7;
			break;
		case '8':
			temp = 8;
			break;
		case '9':
			temp = 9;
			break;
		case 'A':
			case 'a':
			temp = 10;
			break;
		case 'B':
			case 'b':
			temp = 11;
			break;
		case 'C':
			case 'c':
			temp = 12;
			break;
		case 'D':
			case 'd':
			temp = 13;
			break;
		case 'E':
			case 'e':
			temp = 14;
			break;
		case 'F':
			case 'f':
			temp = 15;
			break;
		default:
			return false;
		}
		res = (res << 4) + temp;
	}

	*out = res;
	return true;
}

bool hex2uint32(const char *hexstr, int len, unsigned int *out)
{
	unsigned int res = 0;
	unsigned int temp = 0;
	for ( int i = 0; i < len; ++i )
	{
		switch ( hexstr[i] )
		{
		case '0':
			temp = 0;
			break;
		case '1':
			temp = 1;
			break;
		case '2':
			temp = 2;
			break;
		case '3':
			temp = 3;
			break;
		case '4':
			temp = 4;
			break;
		case '5':
			temp = 5;
			break;
		case '6':
			temp = 6;
			break;
		case '7':
			temp = 7;
			break;
		case '8':
			temp = 8;
			break;
		case '9':
			temp = 9;
			break;
		case 'A':
			case 'a':
			temp = 10;
			break;
		case 'B':
			case 'b':
			temp = 11;
			break;
		case 'C':
			case 'c':
			temp = 12;
			break;
		case 'D':
			case 'd':
			temp = 13;
			break;
		case 'E':
			case 'e':
			temp = 14;
			break;
		case 'F':
			case 'f':
			temp = 15;
			break;
		default:
			return false;
		}
		res = (res << 4) + temp;
	}

	*out = res;
	return true;
}

uint16_t philips_hex2str(char *str, const uint8_t *hex, uint16_t len)
{
	uint8_t i = 0;
	for (i = 0; i < len; i++)
	{
		sprintf(str + i * 2, "%02X", *(hex + i));
	}
	return len * 2;
}

uint16_t philips_hex2str_lowercase(char *str, const uint8_t *hex, uint16_t len)
{
	uint8_t i = 0;
	for (i = 0; i < len; i++)
	{
		sprintf(str + i * 2, "%02x", *(hex + i));
	}
	return len * 2;
}

void philips_str_lower2upper(char *str, uint16_t len)
{
	uint16_t i = 0;
	for (i = 0; i < len; i++)
	{
		if(str[i] >= 'a' && str[i] <= 'z')
		{
			str[i] -= 0x20;
		}
	}
	return ;
}

void philips_str_upper2lower(char *str, uint16_t len)
{
	uint16_t i = 0;
	for (i = 0; i < len; i++)
	{
		if(str[i] >= 'A' && str[i] <= 'Z')
		{
			str[i] += 0x20;
		}
	}
	return ;
}

uint16_t philips_str2hex(unsigned char *hex, const unsigned char *str)
{
	unsigned int i;		 /* loop iteration variable */
	unsigned int j = 0;  /* current character */
	unsigned int by = 0; /* byte value for conversion */
	unsigned char ch;	/* current character */
	unsigned int len = strlen((char *)str);

	/* process the list of characaters */
	for (i = 0; i < len; i++)
	{
		ch = str[i];
		/* do the conversion */
		if (ch >= '0' && ch <= '9')
			by = (by << 4) + ch - '0';
		else if (ch >= 'A' && ch <= 'F')
			by = (by << 4) + ch - 'A' + 10;
		else if (ch >= 'a' && ch <= 'f')
			by = (by << 4) + ch - 'a' + 10;
		else
		{ /* error if not hexadecimal */
			return 0;
		}

		/* store a byte for each pair of hexadecimal digits */
		if (i & 1)
		{
			j = ((i + 1) / 2) - 1;
			hex[j] = by & 0xff;
		}
	}
	return j + 1;
}

/**
 * *****************************************************************************
 * url encode
 * *****************************************************************************
 **/
/**
 * 16进制数转换成10进制数
 * 如：0xE4=14*16+4=228
 **/
int php_htoi(char *s)
{
    int value;
    int c;

    c = ((unsigned char *)s)[0];
    if (isupper(c))
        c = tolower(c);
    value = (c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10) * 16;

    c = ((unsigned char *)s)[1];
    if (isupper(c))
        c = tolower(c);
    value += c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10;

    return (value);
}

static unsigned char hexchars[] = "0123456789ABCDEF";
char *php_url_encode(char const *s, int len, int *new_length)
{
    register unsigned char c;
    unsigned char *to, *start;
    unsigned char const *from, *end;

    from = (unsigned char *)s;
    end  = (unsigned char *)s + len;
    start = to = (unsigned char *) calloc(1, 3*len+1);

    while (from < end)
    {
        c = *from++;

        if (c == ' ')
        {
            *to++ = '+';
        }
        else if ((c < '0' && c != '-' && c != '.') ||
                 (c < 'A' && c > '9') ||
                 (c > 'Z' && c < 'a' && c != '_') ||
                 (c > 'z'))
        {
            to[0] = '%';
            to[1] = hexchars[c >> 4];//将2进制转换成16进制表示
            to[2] = hexchars[c & 15];//将2进制转换成16进制表示
            to += 3;
        }
        else
        {
            *to++ = c;
        }
    }
    *to = 0;
    if (new_length)
    {
        *new_length = to - start;
    }
    return (char *) start;
}

/**
 * *****************************************************************************
 * base64字符串检测
 * *****************************************************************************
 **/
bool philips_is_base64_char(char c)
{
	bool res = false;

	if ( c >= 'a' && c <= 'z' )
	{
		res = true;
	}
	else if ( c >= 'A' && c <= 'Z' )
	{
		res = true;
	}
	else if ( c >= '0' && c <= '9' )
	{
		res = true;
	}
	else if ( c == '+' || c == '/' || c == '=' )
	{
		res = true;
	}
	else
	{
		res = false;
	}

	return res;
}

/**
 * *****************************************************************************
 * encrypt
 * *****************************************************************************
 **/
word32 philipsAesCbcEncryptPkcs5Padding(Aes* aes, byte* output, const byte* input, word32 sz)
{
	if ( aes == NULL || output == NULL || input == NULL || sz == 0 )
		return 0;

	word32 div = sz / 16;
	word32 sz_align = (div + 1) * 16;
	byte pad_byte = sz_align - sz;

	byte* input_align = malloc(sz_align);
	if ( input_align == NULL )
		return 0;

	memcpy(input_align, input, sz);
	memset(input_align + sz, pad_byte, sz_align - sz);

	AesCbcEncrypt(aes, output, input_align, sz_align);

	free(input_align);
	return sz_align;
}
