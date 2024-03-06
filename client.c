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

#define MAX_BUFFER 4096
struct sockaddr_in localSock;
struct ip_mreq group;
int sd;
int datalen;
char databuf[MAX_BUFFER];

void* receive_messages(void* arg) 
    { // Function for receiving SCTP messages
    int sock = (*(int*)arg);
    char buffer[MAX_BUFFER + 1];
    int in, flags;
    struct sctp_sndrcvinfo sndrcvinfo;
    while (1) {
        in = sctp_recvmsg(sock, buffer, sizeof(buffer), (struct sockaddr*)NULL, 0, &sndrcvinfo, &flags);
        if (in < 0) {
            perror("sctp_recvmsg()");
            exit(EXIT_FAILURE);
        }
        if (in == 0) {
            printf("Server disconnected.\n");
            break;
        }

        buffer[in] = '\0';
        printf("%s\n", buffer);
    }
    close(sock);
    return NULL;
}

void* receive_udp_messages(void* arg) 
    { // Function for receiving UDP multicast messages 
    while (1)
    {
        datalen = sizeof(databuf);
        if (read(sd, databuf, datalen) < 0)
        {
            perror("Reading datagram message error:");
            close(sd);
            exit(EXIT_FAILURE);
        }
        else
        {
            printf("Reading datagram message...OK.\n");
            printf("Message from server: \"%s\"\n", databuf);
        }

        return 0;
    }
}



int main(int argc, char* argv[]) {
    int sock, ret;
    struct sockaddr_in6 server_addr;
    pthread_t thread, udp_thread;
    if (argc < 2) {
        printf("Usage: %s <server IP>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Create a SCTP socket
    sock = socket(AF_INET6, SOCK_STREAM, IPPROTO_SCTP);
    if (sock < 0) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    // Create a UDP socket 
    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd < 0)
    {
        perror("Opening UDP socket error:");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("Opening datagram socket....OK.\n");
    }

    // SCTP cont.
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_port = htons(5555);
    ret = inet_pton(AF_INET6, argv[1], &server_addr.sin6_addr);
    if (ret <= 0) {
        perror("inet_pton()");
        exit(EXIT_FAILURE);
    }

    // Enable SO_REUSEADDR to allow multiple instances of this
    // application to receive copies of the multicast datagrams
        int reuse = 1;
        if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse)) < 0)
        {
            perror("Setting SO_REUSEADDR error");
            close(sd);
            exit(EXIT_FAILURE);
        }
        else
        {
            printf("Setting SO_REUSEADDR...OK.\n");
        }
    
        // Bind to the proper port number with the IP address
        // specified as INADDR_ANY
        memset((char*)&localSock, 0, sizeof(localSock));
        localSock.sin_family = AF_INET;
        localSock.sin_port = htons(2222);
        localSock.sin_addr.s_addr = INADDR_ANY;
        if (bind(sd, (struct sockaddr*)&localSock, sizeof(localSock)))
        {
            perror("Binding UDP socket error:");
            close(sd);
            exit(EXIT_FAILURE);
        }
        else
        {
            printf("Binding datagram socket...OK.\n");
        }

        group.imr_multiaddr.s_addr = inet_addr("225.1.1.1");
        group.imr_interface.s_addr = inet_addr("0.0.0.0");
        if (setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&group, sizeof(group)) < 0)
        {
            perror("Adding multicast group error");
            close(sd);
            exit(EXIT_FAILURE);
        }
        else
        {
            printf("Adding multicast group...OK.\n");
        }

    ret = connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (ret < 0) {
        perror("connect()");
        exit(EXIT_FAILURE);
    }

    // Send client name
    char name[32];
    printf("Please enter a name to be identified by: ");
    scanf("%s", name);
    sctp_sendmsg(sock, name, strlen(name), NULL, 0, 0, 0, 0, 0, 0);

    // Crate 2 threads one for receiving SCTP one for UDP
    pthread_create(&udp_thread, NULL, receive_udp_messages, &sd);
    pthread_create(&thread, NULL, receive_messages, &sock);

    // Send SCTP message to server
    char buffer[MAX_BUFFER + 1];
    printf("Enter a message to send: ");
    while (1) {
        fgets(buffer, MAX_BUFFER, stdin);
        sctp_sendmsg(sock, buffer, strlen(buffer), NULL, 0, 0, 0, 0, 0, 0);
    }
    pthread_join(thread, NULL);
    pthread_join(udp_thread, NULL);

    return 0;
}
