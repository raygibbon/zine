#include <stdio.h>
#include <string.h>
#include "../incl/secrete.h"

int
main()
{
	unsigned char key[] = { 0xaa,0xbb,0xcc,0xff,0x01,0xff,0xaf,0x00 };
	unsigned char msg[] = "AAAAAAAAAAAAAAAAAAAAA";
	unsigned char cipher[100] , plain[100];
	bfs test;
	int i;

	bzero(cipher , sizeof(cipher));
	bzero(plain , sizeof(plain));

	bf_keyinit(&test , key , sizeof(key));
	
	printf("encrypting msg \"%s\" with key %s ...\n" , msg , key);
	bf_ecb(ENC , &test , msg , cipher);
	bf_ecb(ENC , &test , msg+8 , cipher+8);	

	for(i=0 ; i < 16 ; i++) printf("%x " , cipher[i]);
	printf("\n");
	

	printf("decrypting ...\n");
	bf_ecb(DEC , &test , cipher , plain);
	bf_ecb(DEC , &test , cipher+8 , plain+8);

	printf("after decryption: %s\n" , plain);

	return 0;
}
	
	
	
	
	
