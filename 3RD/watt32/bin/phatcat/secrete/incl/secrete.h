#define	ENC	1
#define	DEC	0

typedef struct {

	unsigned long sbox0[257];
	unsigned long sbox1[257];
	unsigned long sbox2[257];
	unsigned long sbox3[257];
	unsigned long parray[19];
	unsigned long IV[2];
} bfs;

void bf_keyinit(bfs * session, unsigned char *key, unsigned int keylen);
int bf_ecb(int mode, bfs * session, unsigned char *in, unsigned char *out);

/* rc4 related stuff */

struct rc4_session {
	unsigned char sbox[255];
	unsigned char kbox[255];
	int i, j, a, t;
};

typedef struct rc4_session rc4s;

void rc4_keyinit(rc4s * session, unsigned char *key, int keylen);
char get_rc4byte(rc4s * session);
void rc4(rc4s * session, unsigned char *in, unsigned char *out,
	 unsigned int inlen);
