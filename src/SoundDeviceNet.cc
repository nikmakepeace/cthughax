/*
 * Read sound via network from Cthugha server
 */

#include "cthugha.h"
#include "Sound.h"
#include "network.h"
#include "cth_buffer.h"

int REQ_PORT = 5555;
int CLT_PORT = 5556;
int SRV_PORT = 5555;

#if WITH_NETWORK == 1

#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

char SoundDeviceNet::sound_hostname[256] = "";

/*
 * create data-socket
 */
SoundDeviceNet::SoundDeviceNet()
    : SoundDevice() {
    CTH_INFO("Initializing net-sound...\n");

    // sound data is transmitted as 8bit unsigned, stereo
    soundFormat.setValue(SF_u8);
    soundChannels.setValue(2);

    // create socket to receive data
    if ((handle = make_socket(SOCK_DGRAM, CLT_PORT)) < 0) {
        error = 1;
        return;
    }
    // set to non-blocking mode
    fcntl(handle, F_SETFL, O_NONBLOCK);

    // send request to server
    net_request(0);

    tmpSize = 2048;
}

/*
 * send a request over the net
 * request: 0: connect
 *          1: disconnect
 */
void SoundDeviceNet::net_request(int request) {
    struct sockaddr_in my_s_addr;
    struct hostent* hostinfo;
    int request_socket;
    char req[65];

    /* create request-string */
    switch (request) {
    case 0: /* connect */
        /* request connection from host "sound_hostname" */
        sprintf(req, "connect %d", CLT_PORT);
        break;
    case 1:
        /* request for disconnect on exit */
        sprintf(req, "disconnect %d", CLT_PORT);
        break;
    default:
        /* unknown request */
        error = 1;
        return;
    }
    CTH_INFO("  Requesting: `%s'.\n", req);

    /* create socket for request */
    if ((request_socket = make_socket(SOCK_STREAM, CLT_PORT2)) < 0)
        printfee("Can not create request socket.");

    /* create address of server */
    my_s_addr.sin_family = AF_INET;
    my_s_addr.sin_port = htons(SRV_PORT);
    hostinfo = gethostbyname(sound_hostname);
    if (hostinfo == NULL) {
        printfee("Could not find host `%s'.", sound_hostname);
        close(request_socket);
        error = 1;
        return;
    }
    my_s_addr.sin_addr = *(struct in_addr*)hostinfo->h_addr;

    /* connect to server */
    if (connect(request_socket, (struct sockaddr*)&my_s_addr, sizeof(struct sockaddr_in)) < 0) {
        printfee("Can not connect to server `%s'.", sound_hostname);
        close(request_socket);
        return;
    }

    /* sending request */
    CTH_INFO("  Sending request `%s'\n", req);
    strcat(req, "\n");
    if (send(request_socket, req, 64, 0) <= 0) {
        printfee("Can not send request.");
    }

    sleep(1);

    /* closing socket */
    if (shutdown(request_socket, 2))
        printfee("Can not shutdown request-socket.");

    if (close(request_socket))
        printfee("Can not close request-socket.");
}

/*
 * read sound from network
 */
int SoundDeviceNet::read() {
    int r, rmax = 0;

    // read a much data as possible
    while ((r = recv(handle, tmpData, 1024, 0)) >= 0)
        if (r > rmax)
            rmax = r;

    return rmax / 2;
}

void SoundDeviceNet::update() {
    soundFormat.setValue(SF_u8);
    soundChannels.setValue(2);

    net_request(0);
}

/*
 * sent disconnect-request
 */
SoundDeviceNet::~SoundDeviceNet() {
    net_request(1);
    close(handle);
}

#else

//
// network support disables
//

SoundDeviceNet::SoundDeviceNet() {
    CTH_ERROR("Network code was disabled at compile time.\n");
    error = 1;
}

char SoundDeviceNet::sound_hostname[256] = "";
SoundDeviceNet::~SoundDeviceNet() { }
int SoundDeviceNet::read() { return 0; }
void SoundDeviceNet::update() { }
void SoundDeviceNet::net_request(int) { }

#endif
