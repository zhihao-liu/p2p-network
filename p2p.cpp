#include "p2p.h"

#include <iostream>
#include <thread>
#include <string>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>

#define DEBUG

using namespace std;

static int constexpr ANTIENTROPY_INTERVAL = 10;
static int constexpr BUF_LEN = 1024;

hostent* he = gethostbyname("localhost");
int peer_sockfd = -1, proxy_sockfd = -1;
uint16_t pid = -1;
uint16_t vector_clock[N_SERVERS] = {};
vector<vector<string>> chat_log(N_SERVERS, vector<string>(1)); // place holders for sequence number 0
mutex m;

static void sendTo(uint16_t to_pid, Message const& msg) {
    sockaddr_in addr;
    memset(&addr, 0, sizeof(sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_addr = *(in_addr*)he->h_addr;
    addr.sin_port = htons(pidToPort(to_pid));

    if (sendto(peer_sockfd, &msg, sizeof(msg), 0, (sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("sendto");
        exit(EXIT_FAILURE);
    }
}

static Message createStatus() {
    Message msg = { .type = STATUS, .sender = pid };
    copy(begin(vector_clock), end(vector_clock), begin(msg.vector_clock));
    return msg;
}

static Message createRumor(uint16_t origin, uint16_t seqnum, string const& text) {
    Message msg = { RUMOR, (uint16_t)text.size(), pid, origin, seqnum };
    copy(text.begin(), text.end(), begin(msg.text));
    return msg;
}

static string getChatLog() {
    string rsp = "chatLog ";
    for (auto& peer_msgs: chat_log) {
        for (auto it = peer_msgs.begin() + 1; it != peer_msgs.end(); ++it) {
            rsp += *it;
            rsp += ',';
        }
    }

    if (rsp.back() == ',') rsp.pop_back();
    return rsp;
}

static void handleRumor(Message const& msg) {
    // handle only one rumor at a time
    lock_guard<mutex> guard(m);
    
    if (msg.seqnum > vector_clock[msg.origin] + 1) {
        // message gap detected, reply status
        auto status_msg = createStatus();
        sendTo(msg.sender, status_msg);
    } else if (msg.seqnum == vector_clock[msg.origin] + 1) {
        // valid message received, update chat log and forward the message
        ++vector_clock[msg.origin];
        chat_log[msg.origin].emplace_back(msg.text, msg.text + msg.text_len);

        auto fwd_msg = msg;
        fwd_msg.sender = pid;

        int r = (rand() % 2 == 0 ? 1 : -1); // randomly choose a peer
        sendTo(pid + r, fwd_msg);
        if (rand() % 2 == 0) return; // randomly choose to stop or forward to another peer
        sendTo(pid - r, fwd_msg);
    }
}

static void handleStatus(Message const& msg) {
    bool status_replied = false;
    for (uint16_t i = 0; i < N_SERVERS; ++i) {
        if (!status_replied && vector_clock[i] < msg.vector_clock[i]) {
            status_replied = true;
            auto status_msg = createStatus();
            sendTo(msg.sender, status_msg);

        } else if (vector_clock[i] > msg.vector_clock[i]) {
            for (uint16_t j = msg.vector_clock[i] + 1; j <= vector_clock[i]; ++j) {
                auto rumor_msg = createRumor(i, j, chat_log[i][j]);
                sendTo(msg.sender, rumor_msg);
            }
        }
    }
}

static void handlePeerMessage(Message const& msg) {
    msg.type == RUMOR ? handleRumor(msg) : handleStatus(msg);
}

static void handleProxyMessage(string const& line) {
    vector<string> elems;
    stringstream ss(line);
    while (ss) {
        elems.emplace_back();
        ss >> elems.back();
    }

    auto& cmd = elems[0];
    if (cmd == "msg") {
        auto& text = elems[2];
        auto new_msg = createRumor(pid, vector_clock[pid] + 1, text);
        handleRumor(new_msg);

        #ifdef DEBUG
        cout << "DEBUG: " << pid << " received message: " << text << endl;
        #endif

    } else if (cmd == "get") {
        string rsp = getChatLog();
        rsp += '\n';
        send(proxy_sockfd, rsp.data(), rsp.size(), 0);

    } else if (cmd == "crash") {
        #ifdef DEBUG
        cout << "DEBUG: " << pid << " crashed" << endl;
        #endif

        _exit(EXIT_FAILURE);
    }
}

int pidToPort(int pid) {
    return pid + 20000;
}

void setUpProxySocket(int port) {
    int listen_sockfd;
    if ((listen_sockfd = ::socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(listen_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    sockaddr_in listen_addr;
	memset(&listen_addr, 0, sizeof(sockaddr_in));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    listen_addr.sin_port = htons(port);

    if (::bind(listen_sockfd, (sockaddr*)&listen_addr, sizeof(listen_addr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    if (listen(listen_sockfd, 8) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    sockaddr proxy_addr;
    socklen_t proxy_socklen = sizeof(proxy_addr);
    if ((proxy_sockfd = accept(listen_sockfd, &proxy_addr, &proxy_socklen)) == -1) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    #ifdef DEBUG
    cout << "DEBUG: " << pid << " connected" << endl;
    #endif
}

void setUpPeerSocket(int port) {
    sockaddr_in addr;
	memset(&addr, 0, sizeof(sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);

    if ((peer_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    if (::bind(peer_sockfd, (sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
}

void listenToProxy() {
    string buffer;
    char buf[BUF_LEN] = {};
    while (true) {
        // keep receiving until a line break appears
        int pos;
        while ((pos = buffer.find('\n')) == string::npos) {
            int n_read = recv(proxy_sockfd, buf, BUF_LEN, 0);
            buffer.append(buf, buf + n_read);
        }

        // extract a line
        string line = buffer.substr(0, pos);
        buffer = buffer.substr(pos + 1);

        // handle proxy messages synchronously to ensure ordering
        handleProxyMessage(line);
    }
}

void listenToPeers() {
    Message msg;
    while (true) {
        if (recv(peer_sockfd, &msg, sizeof(msg), 0) == -1) {
            perror("recv");
            exit(EXIT_FAILURE);
        }

        // handle peer messages asynchronously since they are not ordered
        thread(handlePeerMessage, msg).detach();
    }
}

void syncWithPeers() {
    auto status_msg = createStatus();
    sendTo(pid + 1, status_msg);
    sendTo(pid - 1, status_msg);
}

void startAntientropy() {
    while (true) {
        sleep(ANTIENTROPY_INTERVAL);

        int r = (rand() % 2 == 0 ? 1 : -1);
        auto status_msg = createStatus();
        sendTo(pid + r, status_msg);
    }
}