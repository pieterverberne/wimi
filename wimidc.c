/* wimi - What Is My Ip
 * connect with telnet client to this service. It will return the client's ip address.
 * cc -o wimi -lpthread wimidc.
 * 
 * Contains duplicate code
 */


#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <syslog.h>
#include <stdarg.h>
#include <unistd.h>

#define MAXPORT 65535
#define MSGSIZE 100
#define NROFCONN 3

char msg[] = "Your ip address is: ";
int port = 23;
int sockfd, sock6fd;

void * inet4srv(void *param);
void * inet6srv(void *param);

int closesockets(const char* msg, int err)
{
	perror(msg);
	close(sockfd);
	close(sock6fd);
	exit(err);
}

int main(int argc, char *argv[]) 
{
	struct sockaddr_in serv_addr;
	struct sockaddr_in6 serv_addr6;
	int  n;
	int ch;

	while ((ch = getopt(argc, argv, "p:")) != -1) {
		switch (ch) {
			case 'p':
				port = atoi(optarg);
				if (port < 1 || port > MAXPORT) {
					printf("Invalid port number\n");
					exit(1);
				} else {
					printf("Binding on port %d\n", port);
				}
		}
	}

	pthread_t inet4t, inet6t;

	pledge("stdio inet", NULL);

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		closesockets("socket() AF_INET", 1);
	}
	if ((sock6fd = socket(AF_INET6, SOCK_STREAM, 0)) == -1) {
		closesockets("socket() AF_INET6: ", 1);
	}
	int so_reuse = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &so_reuse,  sizeof(int)) != 0) {
		closesockets("socketopt() AF_INET", 1);
	}
	if (setsockopt(sock6fd, SOL_SOCKET, SO_REUSEADDR, &so_reuse,  sizeof(int)) != 0) {
		closesockets("socketopt() AF_INET6", 1);
	}
	bzero((char *) &serv_addr, sizeof(serv_addr));
	bzero((char *) &serv_addr6, sizeof(serv_addr6));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);

	serv_addr6.sin6_family = AF_INET6;
	serv_addr6.sin6_addr = in6addr_any;
	serv_addr6.sin6_port = htons(port);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		closesockets("bind() AF_INET", 1);
	}

	if (bind(sock6fd, (struct sockaddr *) &serv_addr6, sizeof(serv_addr6)) < 0) {
		closesockets("bind() AF_INET6", 1);
	}

	listen(sockfd, NROFCONN);
	listen(sock6fd, NROFCONN);
	
	pthread_create(&inet4t, NULL, inet4srv, NULL);
	pthread_create(&inet6t, NULL, inet6srv, NULL);
	pthread_join(inet4t, NULL);
	pthread_join(inet6t, NULL);
	return 0;
}

void * inet4srv(void *param)
{
	int n;
	int newsockfd = 0;
	struct sockaddr_in cli_addr;
	char clientaddr[INET_ADDRSTRLEN];
	int cliaddrlen;
	for(;;) {
		bzero(&cli_addr, sizeof(cli_addr));
		cliaddrlen = sizeof(cli_addr);

		if ((newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &cliaddrlen)) <= 0) {
			closesockets("accept() AF_INET", 1);
		}

		bzero(clientaddr, sizeof(clientaddr));
		inet_ntop(AF_INET, &cli_addr.sin_addr, clientaddr, INET_ADDRSTRLEN);

		write(newsockfd, msg, sizeof(msg));
		n = write(newsockfd, clientaddr, INET_ADDRSTRLEN);
		write(newsockfd, "\n", 1);

		if (n < 0) {
			perror("write(): ");
			exit(1);
		}
		close(newsockfd);
	}
}

void * inet6srv(void *param)
{
	int n;
	int newsockfd = 0;
	struct sockaddr_in6 cli_addr;
	char clientaddr[INET6_ADDRSTRLEN];
	int cliaddrlen;
	for(;;) {
		bzero(&cli_addr, sizeof(cli_addr));
		cliaddrlen = sizeof(cli_addr);

		if ((newsockfd = accept(sock6fd, (struct sockaddr *)&cli_addr, &cliaddrlen)) <= 0) {
			closesockets("accept() AF_INET6", 1);
		}

		bzero(clientaddr, INET6_ADDRSTRLEN);
		inet_ntop(AF_INET6, &cli_addr.sin6_addr, clientaddr, INET6_ADDRSTRLEN);

		write(newsockfd, msg, sizeof(msg));
		n = write(newsockfd, clientaddr, INET6_ADDRSTRLEN);
		write(newsockfd, "\n", 1);

		if (n < 0) {
			perror("write(): ");
			exit(1);
		}
		close(newsockfd);
	}
}
