#ifndef _TIMEOUT_H_ 
#define _TIMEOUT_H_ 

void error(char *s, int server, int client);

void mysettimer(int milisegundos);

void timer_handler(int signum);

void mysethandler(void);

#endif /* _TIMEOUT_H_ */
