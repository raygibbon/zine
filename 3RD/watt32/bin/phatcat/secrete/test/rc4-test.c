#include <stdio.h>
#include <string.h>
#include "../incl/secrete.h"

int main()
{
	unsigned char key[] = "f9s8dfsdf9df8";
	unsigned char msg[] = "hello ladies!";
	unsigned char cip[20] , plain[20];
	rc4s test , test2;
	int i;

	bzero(cip , sizeof(cip));
	bzero(plain , sizeof(plain));

	rc4_keyinit(&test , key , sizeof(key));
	rc4_keyinit(&test2 , key , sizeof(key));
	
	printf("encrypting msg \" %s \" with key \"%s\" \n" , msg , key);
	rc4(&test , msg , cip , strlen(msg));
	printf("ciphertext: ");
	
	for(i=0 ; i < sizeof(msg) ; i++) printf("%X ", cip[i]);

	printf("\n");
	
	printf("decrypting...\n");

	rc4(&test2 , cip, plain , strlen(msg));

	printf("after: %s\n" , plain);

	return 0;
}
