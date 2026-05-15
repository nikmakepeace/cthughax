#include "cthugha.h"
#include "SoundServer.h"
#include "SoundDevice.h"
#include "network.h"
#include "Interface.h"
#include "DisplayDevice.h"

OptionTime srv_wait_time("srv-wait-time", 0); /* time to wait after send */

SoundServer* soundServer = NULL;

#if WITH_NETWORK == 1

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

SoundServer::SoundServer()
    : nClients(0)
    , bcast_socket(-1)
    , request_socket(-1) {
    struct sockaddr_in my_s_addr;

    // create socket for broadcast
    if ((bcast_socket = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
        printfee("Can not open socket");
        exit(0);
    }
    fcntl(bcast_socket, F_SETFL, O_NONBLOCK);

    /* create request-socket */
    if ((request_socket = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        printfee("Can not open request socket");
        exit(0);
    }
    /* bind */
    my_s_addr.sin_family = AF_INET;
    my_s_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    my_s_addr.sin_port = htons(REQ_PORT);
    if (bind(request_socket, (struct sockaddr*)&my_s_addr, sizeof(my_s_addr)) < 0) {
        printfee("Can not bind to request socket.");
        exit(0);
    }
    /* change to non-blocking */
    fcntl(request_socket, F_SETFL, O_NONBLOCK);

    /* listen */
    if (listen(request_socket, 1) < 0) {
        printfee("Can not listen.\n");
        exit(0);
    }
}

SoundServer::~SoundServer() {

    if (bcast_socket != -1) {
        close(bcast_socket);
    }
    if (request_socket != -1) {
        shutdown(request_socket, 2);
        close(request_socket);
    }
}

void SoundServer::operator()() {

    // breadcast sound
    for (int i = 0; i < nClients; i++) {

        if (sendto(bcast_socket, soundDevice->data, soundDevice->rawSize, 0, &(clientAddrs[i]),
                clientSizes[i])
            == -1) {
            if (errno != 111)
                printfee("Can not write to client.");
            else
                fprintf(stderr, "x");
        }
    }

    // accept new clients
    struct sockaddr my_s_addr;
    int size = sizeof(struct sockaddr);
    memset(&my_s_addr, 0, size);
    int acc_socket;
    if ((acc_socket = accept(request_socket, (struct sockaddr*)&my_s_addr, (socklen_t*)&size))
        >= 0) {

        if (my_s_addr.sa_family == AF_INET) {
            struct sockaddr_in* ai = (sockaddr_in*)&my_s_addr;
            CTH_TRACE("accept from %x:%d. addr size: %d\n", ntohl(ai->sin_addr.s_addr),
                ntohs(ai->sin_port), size);

            char data[512];
            int nr_read = recv(acc_socket, data, 64, 0);

            CTH_TRACE("received %d bytes: %s\n", nr_read, data);

            int port;
            if (sscanf(data, "connect %d", &port) > 0) {
                ((struct sockaddr_in*)&my_s_addr)->sin_port = ntohs(port);
                add_client(my_s_addr, size);

            } else if (sscanf(data, "disconnect %d", &port) > 0) {
                ((struct sockaddr_in*)&my_s_addr)->sin_port = ntohs(port);
                remove_client(my_s_addr, size);

            } else
                CTH_ERROR("Unknown request `%s'\n", data);

        } else {
            CTH_ERROR("Only AF_INET connections are supported currently.\n");
        }

        close(acc_socket);
    } else if (errno != EWOULDBLOCK)
        printfee("Can not accept clients.");
}

/*
 * include a new client in the list
 */
int SoundServer::add_client(struct sockaddr my_s_addr, int size) {

    if (my_s_addr.sa_family == AF_INET) {
        struct sockaddr_in* ai = (sockaddr_in*)&my_s_addr;
        CTH_TRACE("adding %x:%d\n", ntohl(ai->sin_addr.s_addr), ntohs(ai->sin_port));
    }

    remove_client(my_s_addr, size); // remove old entry

    /* check for full memory */
    if (nClients >= MAX_CLIENTS) {
        CTH_ERROR("Maximum number of clients (%d) reached.\n", MAX_CLIENTS);
        return 1;
    }

    /* append new entry to list */
    memcpy(&(clientAddrs[nClients]), &my_s_addr, size);
    clientSizes[nClients] = size;

    nClients++;

    return 0;
}

/*
 * remove a client from the list
 */
int SoundServer::remove_client(struct sockaddr my_s_addr, int /*size*/) {

    if (my_s_addr.sa_family == AF_INET) {
        struct sockaddr_in* ai = (sockaddr_in*)&my_s_addr;
        CTH_TRACE("removing %x:%d\n", ntohl(ai->sin_addr.s_addr), ntohs(ai->sin_port));

        for (int i = 0; i < nClients; i++) {
            struct sockaddr_in* ci = (sockaddr_in*)&(clientAddrs[i]);
            if ((ai->sin_addr.s_addr == ci->sin_addr.s_addr) && (ai->sin_port == ci->sin_port)) {

                CTH_TRACE("Removing entry %d\n", i);
                memcpy(&(clientAddrs[i]), &(clientAddrs[i + 1]),
                    sizeof(struct sockaddr) * (nClients - i));
                memcpy(&(clientSizes[i]), &(clientSizes[i + 1]), sizeof(int) * (nClients - i));
                nClients--;
            }
        }
    }

    return 0;
}

class InterfaceServer : public Interface {
public:
    InterfaceServer()
        : Interface("server", "Sound Server", NULL) { }
    virtual void display() {

        Interface::display(); // display standard elements (title lines)

        // print a list of the clients
        for (int i = 0; i < soundServer->nClients; i++) {
            char str[256];
            int addr = ntohl(((struct sockaddr_in*)&soundServer->clientAddrs[i])->sin_addr.s_addr);
            sprintf(str, "Client %d: %d.%d.%d.%d:%d\n", i, (addr >> 24) & 0xff, (addr >> 16) & 0xff,
                (addr >> 8) & 0xff, addr & 0xff,
                ntohs(((struct sockaddr_in*)&soundServer->clientAddrs[i])->sin_port));
            displayDevice->print(str, i + 2, 'l', TEXT_COLOR_NORMAL);
        }
    }

} interfaceServer;

#else

int init_server() {
    CTH_ERROR("Sound server was disabled at compile time.\n");
    return 1;
}
int exit_server() { return 0; }

// InterfaceElement * serverElements[] = {
//     new InterfaceElementTitle("Server was disabled at compile time"),
//     new InterfaceElementText("----------------------------------------")
// };
// Interface  interfaceServer = new Interface(serverElements, 2, 7);

#endif
