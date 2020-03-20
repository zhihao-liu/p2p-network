#include <iostream>
#include <thread>
#include <unistd.h>
#include "p2p.h"

using namespace std;

int main(int argc, char** argv) {
    srand(time(nullptr));

    pid = atoi(argv[1]);
    int peer_port = pidToPort(pid);
    int proxy_port = atoi(argv[3]);
    vector<thread> threads;

    setUpPeerSocket(peer_port);
    setUpProxySocket(proxy_port);

    threads.emplace_back(listenToPeers);

    // intial sync after restart
    syncWithPeers(); 
    sleep(SYNC_TIMEOUT);

    threads.emplace_back(listenToProxy);
    threads.emplace_back(startAntientropy);

    for (auto& t : threads) t.join();
}
