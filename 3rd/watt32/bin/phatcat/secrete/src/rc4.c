#include <stdio.h>
#include "../incl/secrete.h"

void
rc4_keyinit(rc4s * session, unsigned char *key, int keylen)
{
	int j, i, a;

	session->i = session->j = session->a = session->t = 0;

	for (i = 0; i < 256; i++)
		session->sbox[i] = i;
	for (i = 0; i < 256; i++)
		session->kbox[i] = key[i % keylen];

	j = 0;

	for (i = 0; i < 256; i++) {
		j = (j + session->sbox[i] + session->kbox[i]) % 256;
		a = session->sbox[i];
		session->sbox[i] = session->sbox[j];
		session->sbox[j] = a;
	}
}

/* -- */

void
rc4(rc4s * session, unsigned char *in, unsigned char *out, unsigned int inlen)
{

	char k;
	unsigned int loopy;

	for (loopy = 0; loopy < inlen; loopy++) {

		session->i = (session->i + 1) % 256;
		session->j = (session->j + session->sbox[session->i]) % 256;
		session->a = session->sbox[session->i];
		session->sbox[session->i] = session->sbox[session->j];
		session->sbox[session->j] = session->a;

		session->t =
		    (session->sbox[session->i] +
		     session->sbox[session->j]) % 256;
		k = session->sbox[session->t];

		out[loopy] = (in[loopy] ^ k);

	}

}
