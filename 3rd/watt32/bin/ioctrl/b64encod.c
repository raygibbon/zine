#include	"b64encod.h"

static char		basis_64[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static char		buffer[256];

/*
 *	Ersatz crypt for IO256. Very flimsy security.
 *	But then if you are sending passwords in the clear over
 *	the network, that is a weakness too.
 *	Encodes to a static buffer. Limit of 256 characters.
 */

char *crypt(char *key, char *salt)
{
	int		c1, c2, c3;
	char		*out;

	for (out = buffer;  out < &buffer[sizeof(buffer)] && *key != '\0'; )
	{
		c1 = *key++;
		if (*key == '\0')
			c2 = c3 = 0;
		else
		{
			c2 = *key++;
			if (*key == '\0')
				c3 = 0;
			else
				c3 = *key++;
		}
		*out++ = basis_64[c1>>2];
		*out++ = basis_64[((c1 & 0x3) << 4) | ((c2 & 0xF0) >> 4)];
		if (c2 == 0 && c3 == 0)
		{
			*out++ = '=';
			*out++ = '=';
		}
		else if (c3 == 0)
		{
			*out++ = basis_64[((c2 & 0xF) << 2) | ((c3 & 0xC0) >> 6)];
			*out++ = '=';
		}
		else
		{
			*out++ = basis_64[((c2 & 0xF) << 2) | ((c3 & 0xC0) >> 6)];
			*out++ = basis_64[c3 & 0x3F];
		}
	}
	*out = '\0';
	return (buffer);
}
