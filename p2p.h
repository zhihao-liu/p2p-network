#ifndef P2P_H
#define P2P_H

#include <vector>
#include <mutex>
#include <netdb.h>

using namespace std;

int constexpr N_SERVERS = 4;
int constexpr MAX_TEXT_LEN = 200;
int constexpr SYNC_TIMEOUT = 2;

enum MessageType {
    STATUS = 0,
    RUMOR
};

struct Message {
    MessageType type;
    uint16_t text_len;
    uint16_t sender;
    uint16_t origin;
    uint16_t seqnum;
    uint16_t vector_clock[N_SERVERS];
    char text[MAX_TEXT_LEN];
};

extern mutex m;
extern int peer_sockfd, proxy_sockfd;
extern hostent* he;
extern uint16_t pid;
extern uint16_t vector_clock[N_SERVERS];
extern vector<vector<string>> chat_log;

int pidToPort(int pid);
void setUpProxySocket(int port);
void setUpPeerSocket(int port);
void listenToProxy();
void listenToPeers();
void syncWithPeers();
void startAntientropy();

#endif