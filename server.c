#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>

#define MAX_BUFFER 4096
#define MAX_CLIENTS 10
struct in_addr localInterface;
struct sockaddr_in groupSock;
int sd;
char databuf[MAX_BUFFER] = "A new client has joined";
int datalen = sizeof(databuf);

struct client {
    char name[32];
    int sock;
} clients[MAX_CLIENTS];
int client_count = 0;

void sctp_sender(char* buffer, int sender) 
    { // Function used for sending messages to all clients but one using SCTP

    int i;
    char sender_name[32];
    for (i = 0; i < client_count; i++) {
        if (clients[i].sock == sender) {
            strcpy(sender_name, clients[i].name);
            break;
        }
    }
    char new_buffer[MAX_BUFFER + 1];
    sprintf(new_buffer, "\n[%s]: %s", sender_name, buffer);

    for (i = 0; i < client_count; i++) {
        if (clients[i].sock != sender) {
            struct sctp_sndrcvinfo sndrcvinfo;
            int flags = 0;
            sctp_sendmsg(clients[i].sock, new_buffer, strlen(new_buffer), (struct sockaddr*)NULL, 0,
                0, 0, sndrcvinfo.sinfo_stream, 0, 0);
        }
    }
}


void* client_handler(void* arg) 
    { // Function used for handling client traffic (receive nickname, receive message, send messages)
    int sock = (*(int*)arg);
    char buffer[MAX_BUFFER + 1];
    int in, flags;
    struct sctp_sndrcvinfo sndrcvinfo;

    // Receive client name
    in = sctp_recvmsg(sock, buffer, sizeof(buffer), (struct sockaddr*)NULL, 0, &sndrcvinfo, &flags);
    buffer[in] = '\0';
    strcpy(clients[client_count].name, buffer);
    clients[client_count].sock = sock;

    sctp_sender("A new cleint has joined", sock);

    // Send message through multicast
    if (sendto(sd, databuf, datalen, 0, (struct sockaddr*)&groupSock, sizeof(groupSock)) < 0)
    {
        perror("Sending datagram message error");
    }
    else
    {
        printf("Sending datagram message...OK\n");
    }


    client_count++;

    while (1) {
        // Receive multicast messages
	if(strcmp(buffer, "exit") ==0) {
		printf("Client disconn.\n");
		sctp_sender("Client: %s disconned.\n", sock);
		client_count--;
		break;
	}
	else {
        in = sctp_recvmsg(sock, buffer, sizeof(buffer), (struct sockaddr*)NULL, 0, &sndrcvinfo, &flags);
	}
        if (in < 0) {
 	    exit(EXIT_FAILURE);
            perror("sctp_recvmsg()");
        }
        if (in == 0) {
            printf("Client disconnected.\n");
            client_count--;
            break;
        }

        // Send message to all clients but one
        buffer[in] = '\0';
        multicast(buffer, sock);
    }
    close(sock);
}

void daemonize() {
    pid_t pid, sid;

    // Create a new process
    pid = fork();
    if (pid < 0) {
        // Fork failed, exit with failure status
        exit(EXIT_FAILURE);
    }
    // Exit the original parent process
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    // Change file mask
    umask(0);

    // Create a new session
    sid = setsid();
    if (sid < 0) {
        // Unable to create a new session, exit with failure status
        exit(EXIT_FAILURE);
    }

    // Change working directory to root
    if ((chdir("/")) < 0) {
        // Unable to change the working directory, exit with failure status
        exit(EXIT_FAILURE);
    }

    // Close standard file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // Open logs for daemonization
    openlog("myserver", LOG_PID, LOG_DAEMON);
}

int main(int argc, char* argv[]) {
    daemonize();
    int listen_sock, conn_sock, ret;
    struct sockaddr_in6 server_addr;
    pthread_t thread;

    listen_sock = socket(AF_INET6, SOCK_STREAM, IPPROTO_SCTP);
    if (listen_sock < 0) {
        perror("SCTP socket() error: ");
        exit(EXIT_FAILURE);
    }

    // SCTP cont.
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_addr = in6addr_any;
    server_addr.sin6_port = htons(5555);

    // Create a multicast socket
    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd < 0)
    {
        perror("Opening UDP socket error");
        exit(EXIT_FAILURE);
    }

    // Initialize the group sockaddr structure with a
    // group address of 225.1.1.1 and multicast port 2222
    memset((char*)&groupSock, 0, sizeof(groupSock));
    groupSock.sin_family = AF_INET;
    groupSock.sin_addr.s_addr = inet_addr("225.1.1.1");
    groupSock.sin_port = htons(2222);

    localInterface.s_addr = inet_addr("0.0.0.0");
    if (setsockopt(sd, IPPROTO_IP, IP_MULTICAST_IF, (char*)&localInterface, sizeof(localInterface)) < 0)
    {
        perror("Setting local interface error");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("Setting the local interface...OK\n");
    }

    ret = bind(listen_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (ret < 0) {
        perror("bind()");
        exit(EXIT_FAILURE);
    }
    ret = listen(listen_sock, 5);
    if (ret < 0) {
        perror("listen()");
        exit(EXIT_FAILURE);
    }

    while (1) {
        conn_sock = accept(listen_sock, (struct sockaddr*)NULL, NULL);
        if (conn_sock < 0) {
            perror("accept()");
            exit(EXIT_FAILURE);
        }
        pthread_create(&thread, NULL, client_handler, &conn_sock);
    }

    close(listen_sock);

    return 0;
}

