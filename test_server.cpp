#include <iostream>
#include <sstream>
#include <thread>
#include "p2p.h"

using namespace std;

static void printChatLog() {
    cout << "--------" << endl;
    for (auto& texts : chat_log) {
        for (auto& text : texts) {
            cout << text << "; ";
        }
        cout << endl;
    }
    cout << "--------" << endl;
}

int main(int argc, char** argv) {
    srand(time(nullptr));

    pid = atoi(argv[1]);
    int peer_port = pidToPort(pid);
    int proxy_port = atoi(argv[2]);

    thread peer_thread(listenToPeers, peer_port);
    thread proxy_thread(listenToProxy, proxy_port);

    peer_thread.join();
    proxy_thread.join();
}