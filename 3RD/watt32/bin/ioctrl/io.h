/*
 * servidor_io.h - Acesso a portos de Hardware
 * 
 * 
 * (c) 1998 Nuno Sucena Almeida <slug@student.dee.uc.pt>
 * 
 */

/* Definicao das variaveis para enviar ou nao os valores para os portos
 * de saida
 */
#define ENVIA 1
#define NAO_ENVIA 2

extern int output_bit_to_port ( int port , int *port_value , int bit , int bit_value );
extern void show_bits ( int valor , int nbits);
extern int execute_command ( void *command_list , void *port_list , char *command , char *value );
extern int init_ports ( void *command_list , void *port_list );

extern int send_for_real;
