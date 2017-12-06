/*

UNIVERSIDADE FEDERAL DE MINAS GERAIS (UFMG)
INSTITUTO DE CIÊNCIAS EXATAS (ICEX)
DEPARTAMENTO DE CIÊNCIA DA COMPUTAÇÃO (DCC)

		REDES DE COMPUTADORES 
		  TRABALHO PRÁTICO 2

CAROLINA COIMBRA VIEIRA - 2014032941

*/

#include <sys/time.h> //gettimeofday()
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h> //printf
#include <string.h> //memset
#include <stdlib.h> //exit(0);

#include "timeout.h"
#include "tp_socket.h"

#define HEADER_LEN 2

/*
CLIENTE:
processa argumentos da linha de comando:
	host_do_servidor (argv[1]) -> tipo char*
	porta_servidor (argv[2]) -> tipo int
	nome_arquivo (argv[3]) -> tipo char*
	tam_buffer (argv[4]) -> tipo int
chama gettimeofday para tempo inicial
faz abertura ativa a host_do_servidor : porta_servidor
envia string com nome do arquivo (terminada em zero)
abre arquivo que vai ser gravado - pode ser fopen(nome,"w+")
loop recv buffer até que receba zero bytes ou valor negativo
	escreve bytes do buffer no arquivo (fwrite)
	atualiza contagem de bytes recebidos
fim_loop
fecha conexão e arquivo
chama gettimeofday para tempo final e calcula tempo gasto
imprime resultado:
"Buffer = \%5u byte(s), \%10.2f kbps (\%u bytes em \%3u.\%06u s)
*/

int main(int argc, char** argv){

	if (tp_init() == 0)
		fprintf(stderr, "tp_init() executado com sucesso!\n");
	else
		error("Erro ao inicializar (tp_init)\n", -1, -1);

    if (argc != 5)
        error("Erro! Quantidade de argumentos errada!\n", -1, -1);

    //Recebimento dos parametros da linha de comando
    char *ip = argv[1];
    int port = atoi(argv[2]);
    char *file_name = argv[3];
    int buffer_len = atoi(argv[4]);

    int client_socket, server_socket;
    int i, code, bytes_s, bytes_r, t1, t2;
	int esperando_ack = 0, server_s = 0, client_s = 0, server_ack = 0, client_ack = 0;

    struct timeval start, finish;
	char *data;
    char *buffer = (char*)malloc(sizeof(char)*(buffer_len+1));
	FILE *file_write;

    so_addr server_address;

    //Cria socket UDP
    client_socket = tp_socket(0);
    if (client_socket < 0)
	error("Erro ao criar o socket do cliente!\n", -1, -1);

	server_socket = tp_build_addr(&server_address, ip, port);
	if (server_socket < 0)
		error("Erro ao construir socket do servidor!\n", -1, -1);
 
    bytes_s = 0;
    bytes_r = 0;
	
    //Chama gettimeofday para tempo inicial
    gettimeofday(&start, NULL);


	fprintf(stderr, "Enviando nome do arquivo...\n"); 
    //Envia mensagem com o nome do arquivo
    code = tp_sendto(client_socket, file_name, strlen(file_name), &server_address);
    if (code < 0)
        error("Erro ao enviar nome do arquivo!\n", -1, client_socket);
    else
        bytes_s += strlen(file_name);

	//Abre o arquivo onde o cliente vai salvar o que recebeu do servidor
	//abre arquivo que vai ser gravado - pode ser fopen(nome,"w+")
	file_write = fopen(strcat(file_name, ".out"), "wb");
	if (file_write == NULL)
		error("Erro ao abrir o arquivo de escrita!\n", -1, client_socket);
	else {
		while (1) {
			//Recebe os dados do servidor 
			code = tp_recvfrom(client_socket, buffer, buffer_len, &server_address);
			if (code < 0)
				error("Erro ao receber mensagem!\n", -1, client_socket);
			else {
				if (code > 0){
					server_s = buffer[0];
					server_ack = buffer[1];
					data = &(buffer[2]);

					fprintf(stderr, "Recebi dados:%s\n", data);
					// O cliente recebeu uma mensagem 
					if (esperando_ack == server_ack){
						if (strcmp(data, "FIM") == 0){
							client_ack = (server_s + 1)%127;
							client_s = server_ack;
							esperando_ack = (server_ack + 1)%127;

							memset(buffer, 0, buffer_len+1);
							buffer[0] = client_s;
							buffer[1] = client_ack;
							buffer[2] = 'A';

							code = tp_sendto(client_socket, buffer, buffer_len, &server_address);
							if (code < 0)
								error("Erro ao enviar mensagem!\n", -1, client_socket);
							break;				
						}

						fwrite(data, sizeof(*data), strlen(data), file_write);

						bytes_r += strlen(data);

						client_ack = (server_s + 1)%127;
						client_s = server_ack;
						esperando_ack = (server_ack + 1)%127;
					}

					memset(buffer, 0, buffer_len+1);
					buffer[0] = client_s;
					buffer[1] = client_ack;
					buffer[2] = 'A';

					fprintf(stderr, "Sequencia: %d, Ack: %d, Esperando: %d, Sequencia servidor: %d, Ack servidor: %d\n", client_s, client_ack, esperando_ack, server_s, server_ack);
					code = tp_sendto(client_socket, buffer, buffer_len, &server_address);
					if (code < 0)
						error("Erro ao enviar mensagem!\n", -1, client_socket);
					else
						bytes_s += 1;	
				}
			}
		}

		//Se chegou aqui é porque o servidor ja enviou FIM!
		fclose(file_write);
	}


    //Chama gettimeofday para tempo final e calcula tempo gasto
    gettimeofday(&finish, NULL);

    t1 = (start.tv_sec*1000000 + start.tv_usec);
    t2 = (finish.tv_sec*1000000 + finish.tv_usec);

    //Imprime resultado: "Buffer = \%5u byte(s), \%10.2f kbps (\%u bytes em \%3u.\%06u s)
    printf("Tempo gasto: %d usec\n", t2-t1);
    printf("Quantidade de dados enviados: %d bytes\n", bytes_s);
    printf("Quantidade de dados recebidos: %d bytes\n", bytes_r);

    close(client_socket);
    free (buffer);

    return 0;
}
