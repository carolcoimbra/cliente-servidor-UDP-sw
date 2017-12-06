all: servidor cliente

clean: 
	@rm -f *.o 

servidor: servidor.c tp_socket.o timeout.o 
	gcc servidor.c tp_socket.o timeout.o -o servidorFTP

cliente: cliente.c tp_socket.o timeout.o
	gcc cliente.c tp_socket.o timeout.o -o clienteFTP

tp_socket.o: tp_socket.c tp_socket.h

timeout.o: timeout.c timeout.h
