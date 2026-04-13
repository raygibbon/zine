#include <stdlib.h>
#include <stdio.h>

static char basis_64[] =
     "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char *b64encode(char *key, char *buffer, int size)
{
	int		c1, c2, c3;
	char		*out;

	for (out = buffer;  out < &buffer[size] && *key != '\0'; )
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

int main(int argc, char *argv[])
{
	char		buffer[256];

	if (argc < 4)
	{
		fprintf(stderr, "Usage: mkpasswd name password fullname\n");
		exit(1);
	}
	printf("%s:%s:%s\n", argv[1], b64encode(argv[2], buffer, sizeof(buffer)), argv[3]);
	exit(0);
}
