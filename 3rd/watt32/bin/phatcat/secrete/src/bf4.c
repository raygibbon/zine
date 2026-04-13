/*
 *
 * blowfish implementation by thokky
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../incl/bf4.h"

#define	BLOCKL(d,s)	d = (s[3] << 24|s[2] << 16|s[1] << 8 | s[0])
#define	BLOCKR(d,s)	d = (s[7] << 24|s[6] << 16|s[5] << 8 | s[4])

void
bf_keyinit(bfs * session, unsigned char *key, unsigned int keylen)
{
	int i;
	bf_data temp;
	register unsigned long *p;

	p = session->parray;

	if (keylen >= 56)
		keylen = 56;

	memcpy(session->sbox0, sbox0, sizeof (sbox0));
	memcpy(session->sbox1, sbox1, sizeof (sbox1));
	memcpy(session->sbox2, sbox2, sizeof (sbox2));
	memcpy(session->sbox3, sbox3, sizeof (sbox3));
	memcpy(session->parray, parray, sizeof (parray));

	for (i = 0; i < (18 * 4); i += 4) {

		*p++ ^= (key[i % keylen] << 24) |
		    (key[(i + 1) % keylen] << 16) |
		    (key[(i + 2) % keylen] << 8) | (key[(i + 3) % keylen]);

	}

	/* init parray[] */

	temp.xL = 0x00000000;
	temp.xR = 0x00000000;

	for (i = 0; i < 18; i += 2) {
		temp = bf_encrypt(session, temp.xL, temp.xR);
		session->parray[i] = temp.xL;
		session->parray[i + 1] = temp.xR;
	}

	/* init sboxes */

	for (i = 0; i < 256; i += 2) {
		temp = bf_encrypt(session, temp.xL, temp.xR);
		session->sbox0[i] = temp.xL;
		session->sbox0[i + 1] = temp.xR;
	}

	for (i = 0; i < 256; i += 2) {
		temp = bf_encrypt(session, temp.xL, temp.xR);
		session->sbox1[i] = temp.xL;
		session->sbox1[i + 1] = temp.xR;

	}

	for (i = 0; i < 256; i += 2) {
		temp = bf_encrypt(session, temp.xL, temp.xR);
		session->sbox2[i] = temp.xL;
		session->sbox2[i + 1] = temp.xR;

	}

	for (i = 0; i < 256; i += 2) {
		temp = bf_encrypt(session, temp.xL, temp.xR);
		session->sbox3[i] = temp.xL;
		session->sbox3[i + 1] = temp.xR;

	}

}

int
bf_ecb(int mode, bfs * session, unsigned char *in, unsigned char *out)
{
	unsigned long left, right;
	static unsigned char tmp[7];
	bf_data output;

/*
	left = (in[3] << 24|in[2] << 16|in[1] << 8 | in[0]);
	right = (in[7] << 24|in[6] << 16|in[5] << 8 | in[4]);
*/

	BLOCKL(left, in);
	BLOCKR(right, in);

	if (mode == ENC) {

		output = bf_encrypt(session, left, right);

	} else if (mode == DEC) {

		output = bf_decrypt(session, left, right);

	} else {
		return -1;
	}

	tmp[3] = output.xL >> 24;
	tmp[2] = output.xL >> 16;
	tmp[1] = output.xL >> 8;
	tmp[0] = output.xL;

	tmp[7] = output.xR >> 24;
	tmp[6] = output.xR >> 16;
	tmp[5] = output.xR >> 8;
	tmp[4] = output.xR;

	memcpy(out, tmp, 8);

	return 0;
}

bf_data
bf_encrypt(bfs * session, unsigned long xl, unsigned long xr)
{

	int i;
	unsigned long temp, xL, xR;
	bf_data retdata;

	xL = xl;
	xR = xr;

	for (i = 0; i < 16; ++i) {

		xL = xL ^ session->parray[i];
		xR = F(session, xL) ^ xR;

		temp = xL;
		xL = xR;
		xR = temp;

	}

	temp = xL;
	xL = xR;
	xR = temp;

	xR = xR ^ session->parray[16];
	xL = xL ^ session->parray[17];

	retdata.xL = xL;
	retdata.xR = xR;

	return retdata;
}

bf_data
bf_decrypt(bfs * session, unsigned long xl, unsigned long xr)
{

	int i;
	unsigned long temp, xL, xR;
	bf_data retdata;

	xL = xl;
	xR = xr;

	for (i = 17; i > 1; --i) {

		xL = xL ^ session->parray[i];
		xR = F(session, xL) ^ xR;

		temp = xL;
		xL = xR;
		xR = temp;
	}

	temp = xL;
	xL = xR;
	xR = temp;

	xR = xR ^ session->parray[1];
	xL = xL ^ session->parray[0];

	retdata.xL = xL;
	retdata.xR = xR;

	return retdata;
}

unsigned long
F(bfs * session, unsigned long x)
{
	unsigned char a;
	unsigned char b;
	unsigned char c;
	unsigned char d;
	unsigned long y;

	a = x >> 24;
	b = x >> 16;
	c = x >> 8;
	d = x;

	y = session->sbox0[a] + session->sbox1[b];
	y = y ^ session->sbox2[c];
	y = y + session->sbox3[d];

	return y;
}
