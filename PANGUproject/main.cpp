#include "mainwindow.h"
#include <QApplication>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <iostream>

#include "socket_stuff.h"
#include "pan_protocol_lib.h"

#define SERVER_NAME	"localhost"
#define SERVER_PORT	10363

static unsigned long hostid_to_address(char *);
SOCKET open_pangu_connection(char* name);

SOCKET open_pangu_connection(char* name) {
    long addr;
    SOCKET sock;
    struct sockaddr_in saddr;
    unsigned long saddr_len;

#ifdef _WIN32
    WSAData wsaData;
    if (WSAStartup(MAKEWORD(1, 1), &wsaData))  {
        (void)fprintf(stderr, "Failed to initialise Winsock 1.1\n");
        (void)exit(1);
    }
#endif

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)  {
        (void)fprintf(stderr, "Error: failed to create socket.\n");
        (void)exit(2);
    }

    addr = hostid_to_address(name);
    if (addr == INADDR_NONE)    {
        (void)fprintf(stderr, "Failed to lookup %s.\n", name);
        (void)exit(3);
    }

    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = addr;
    saddr.sin_port = htons(10363);
    saddr_len = sizeof(struct sockaddr_in);

    if (connect(sock, (struct sockaddr*)&saddr, saddr_len) == -1)  {
        (void)fprintf(stderr, "Error: failed to connect.\n");
        (void)exit(4);
    }

    std::cout << "About to start" << std::endl;
    pan_protocol_start(sock);
    return sock;
}

static unsigned long hostid_to_address(char *s) {
    struct hostent *host;

    /* Assume we have a dotted IP address ... */
    long result = inet_addr(s);
    if (result != (long)INADDR_NONE) return result;

    /* That failed so assume DNS will resolve it. */
    host = gethostbyname(s);
    return host ? *((long *)host->h_addr_list[0]) : INADDR_NONE;
}

// beginning of program execution
int main(int argc, char *argv[]) {
    SOCKET sock;                                       // creating the socket object
    sock = open_pangu_connection((char*)SERVER_NAME);  // opening connection
    QApplication a(argc, argv);                        // loading QApplication
    MainWindow w;                                      // creating the mainwindow object
    w.setSock(sock);                                   // setting the socket
    w.show();                                          // displaying the window
    return a.exec();                                   // loop of execution
}
