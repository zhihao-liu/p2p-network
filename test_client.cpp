#include <iostream>
#include <sstream>
#include <thread>
#include <unistd.h>
#include "p2p.h"

using namespace std;

int main(int argc, char** argv) {
	signal(SIGALRM, [](int) {});

    int port = atoi(argv[1]);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_addr = *(in_addr*)he->h_addr;
    addr.sin_port = htons(port);

    int sockfd = -1;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    if (connect(sockfd, (sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect");
        exit(EXIT_FAILURE);
    }
    cout << "connected" << endl;

    string line;
    int buf_len = 1024;
    char buf[buf_len];
    while (cin) {
        getline(cin, line);

        line += '\n';
        if (send(sockfd, line.data(), line.size(), 0) == -1) {
            perror("send");
            exit(EXIT_FAILURE);
        }
        cout << "sent: " << line;

        if (line == "get\n") {
            int n_read = recv(sockfd, buf, buf_len, 0);
            if (n_read > 0) cout << string(buf, buf + n_read) << endl;
            continue;
        }
    }
}