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
#define espera 1000
 
/*
SERVIDOR:
processa argumentos da linha de comando:
	porta_servidor (argv[1]) -> tipo int
	tam_buffer (argv[2]) -> tipo int
faz abertura passiva e aguarda conexão
recebe o string com nome do arquivo 
abre arquivo que vai ser lido -- pode ser fopen(nome,"r")
se deu erro, fecha conexão e termina
loop lê o arquivo, um buffer por vez até fread retornar zero
	envia o buffer lido
	se quiser, contabiliza bytes enviados
fim_loop
fecha conexão e arquivo
chama gettimeofday para tempo final e calcula tempo gasto
se quiser, imprime nome arquivo e no. de bytes enviados
fim_servidor.
*/

int main(int argc, char** argv){

	if (tp_init() == 0)
		fprintf(stderr, "tp_init() executado com sucesso!\n");
	else
		error("Erro ao inicializar (tp_init)\n", -1, -1);

    if (argc != 3)
        error("Erro! Quantidade de argumentos errada!\n", -1, -1);

    //Recebimento dos parametros da linha de comando
    int port = atoi(argv[1]);
    int buffer_len = atoi(argv[2]);
    int data_len = buffer_len-HEADER_LEN;

    int client_socket, server_socket;
    int i, code, bytes_s, bytes_r, t1, t2;
	int esperando_ack = 0, server_s = 0, client_s = 0, server_ack = 0, client_ack = 0;

    struct timeval start, finish;
    char *buffer = (char*)malloc(sizeof(char)*(buffer_len+1));
	char *data = (char*)malloc(sizeof(char)*(data_len+1));

	FILE *file_read;

    so_addr client_address;

    //Cria socket UDP
    server_socket = tp_socket(port);
    if (server_socket < 0)
        error("Erro ao criar o socket do servidor!\n", -1, -1);
     
    bytes_s = 0;
    bytes_r = 0;
	
    //Chama gettimeofday para tempo inicial
    gettimeofday(&start, NULL);
     
    while(1){
        fprintf(stderr, "Esperando clientes...\n");
         
        //Espera receber o nome do arquivo
        code = tp_recvfrom_secure(server_socket, buffer, buffer_len, &client_address);
        if (code < 0)
            error("Erro ao receber nome do arquivo!\n", server_socket, -1);
        else {
            bytes_r += code;

			//abre arquivo que vai ser lido -- pode ser fopen(nome,"r")
			//se deu erro, fecha conexão e termina
			file_read = fopen(buffer, "r");
			if (file_read == NULL)
				error("Erro ao abrir o arquivo de leitura!\n", server_socket, -1);
			else 
				break;
		}
    }

	//Inicializa o temporizador
	mysethandler(); 
    mysettimer(espera);

	//Começa a ler o arquivo
	if (!feof(file_read)){
		memset(buffer, 0, buffer_len+1);
		memset(data, 0, data_len+1);

		code = fread(data, sizeof(*data), data_len, file_read);
		if (code <= 0)
			error("Erro ao ler arquivo!\n", server_socket, -1);

		else {
			buffer[0] = server_s;
			buffer[1] = server_ack;
			memcpy (buffer+2, data, code); 
			esperando_ack = (server_s + 1)%127;

			fprintf(stderr, "Enviei dados:\n%s\n", data);
			//Envia o primeiro buffer do arquivo para o cliente
			code = tp_sendto(server_socket, buffer, buffer_len, &client_address);
			if (code < 0)
			    error("Erro ao enviar mensagem!\n", server_socket, -1);
			else 
			    bytes_s += strlen(data);

			//Inicia o temporizador
			pause();
			fprintf(stderr, "Sequencia: %d, Ack: %d, Esperando: %d, Dados enviados: %d\n", server_s, server_ack, esperando_ack, strlen(data));
		}
	}
	else {
		error("Erro! Arquivo vazio!\n", server_socket, -1);
	}

    while (1) {
    	//Recebe o ACK do cliente
		code = tp_recvfrom(server_socket, buffer, buffer_len, &client_address);

        if (code < 0)
            error("Erro ao receber mensagem!\n", server_socket, -1);

		else {
			if (code == 0) {
				//Envia o buffer do arquivo para o cliente
				code = tp_sendto(server_socket, buffer, buffer_len, &client_address);
				if (code < 0)
			    	error("Erro ao enviar mensagem!\n", server_socket, -1);

			    //Inicia o temporizador
				pause();
        	} 

        	else {
        		client_s = buffer[0];
				client_ack = buffer[1];

				if (esperando_ack == client_ack){
					bytes_r += 1;

					if (!feof(file_read)){
						memset(buffer, 0, buffer_len+1);
						memset(data, 0, data_len+1);

						code = fread(data, sizeof(*data), data_len, file_read);
						if (code <= 0)
							error("Erro ao ler arquivo!\n", server_socket, -1);

						else {
							server_ack = (client_s + 1)%127; 
							server_s = client_ack;
							buffer[0] = server_s;
							buffer[1] = server_ack;
							memcpy (buffer+2, data, code); 
							esperando_ack = (client_ack + 1)%127;

							fprintf(stderr, "Enviei dados:\n%s\n", data);
							//Envia o proximo buffer do arquivo para o cliente
							code = tp_sendto(server_socket, buffer, buffer_len, &client_address);
							if (code < 0)
							    error("Erro ao enviar mensagem!\n", server_socket, -1);
							else 
							    bytes_s += strlen(data);

							//Inicia temporizador
							pause();
							fprintf(stderr, "Sequencia: %d, Ack: %d, Esperando: %d, Dados enviados: %d\n", server_s, server_ack, esperando_ack, strlen(data));
						}
					}
					//Sai do while quando o arquivo que o servidor esta enviando acaba!
					else {
						//Fecha o arquivo
						fclose(file_read);
						break;
					}
				}
			}
        }
    }

	//Fim do envio do arquivo
	//Envia mensagem com "FIM" para o cliente
	memset(buffer, 0, buffer_len+1);
	server_ack = (client_s + 1)%127; 
	server_s = client_ack;
	esperando_ack = (client_ack + 1)%127;

	buffer[0] = server_s;
	buffer[1] = server_ack;
	memcpy (buffer+2, "FIM", strlen("FIM"));
	buffer[5] = '\0';

    code = tp_sendto(server_socket, buffer, buffer_len, &client_address);
    if (code < 0)
        error("Erro ao enviar mensagem!\n", -1, client_socket);
    //Inicia temporizador
    pause();

    //O servidor fica esperando o ack do cliente de fim de arquivo
    while (1){
    	//Recebe o ACK do cliente
		code = tp_recvfrom_secure(server_socket, buffer, buffer_len, &client_address);
        if (code < 0)
            error("Erro ao receber mensagem!\n", server_socket, -1);

		else {
			if (code == 0) {
				//Envia o buffer do arquivo para o cliente
				code = tp_sendto(server_socket, buffer, buffer_len, &client_address);
				if (code < 0)
			    	error("Erro ao enviar mensagem!\n", server_socket, -1);

			    //Inicia o temporizador
				pause();
        	} 

        	else {
				client_ack = buffer[1];
				
        		if (esperando_ack == client_ack)
        			break;
        	}
        }
    }

    //Chama gettimeofday para tempo final e calcula tempo gasto
    gettimeofday(&finish, NULL);

    t1 = (start.tv_sec*1000000 + start.tv_usec);
    t2 = (finish.tv_sec*1000000 + finish.tv_usec);

    //Imprime resultado: "Buffer = \%5u byte(s), \%10.2f kbps (\%u bytes em \%3u.\%06u s)
    printf("Tempo gasto: %d usec\n", t2-t1);
    printf("Quantidade de dados enviados: %d bytes\n", bytes_s);
    printf("Quantidade de dados recebidos: %d bytes\n", bytes_r);
 
    close(server_socket);
    free(buffer);

    return 0;
}
