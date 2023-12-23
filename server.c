#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <errno.h>
#include <sys/select.h>
#include <unistd.h>

#define MAX_FD 10

volatile sig_atomic_t wasSigHup = 0;

void sigHupHandler(int r) {
    wasSigHup = 1;
}

int main() {
    char buffer[256];

    int socket_fd;
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }
    printf("Server socket: %d\n", socket_fd);

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(1234);

    bind(socket_fd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr_in));
    listen(socket_fd, MAX_FD);
    printf("Server listen...\n");

    struct sigaction sa;
    sigaction(SIGHUP, NULL, &sa);
    sa.sa_handler = sigHupHandler;
    sa.sa_flags |= SA_RESTART;
    sigaction(SIGHUP, &sa, NULL);

    sigset_t blockedMask, origMask;
    sigemptyset(&blockedMask);
    sigaddset(&blockedMask, SIGHUP);
    sigprocmask(SIG_BLOCK, &blockedMask, &origMask);

    int clients[MAX_FD];
    memset(clients, 0, sizeof(int) * MAX_FD);
    int cur_fd = 0;
    int res;

    while (1) {
        fd_set work_fds;
        FD_ZERO(&work_fds);
        FD_SET(socket_fd, &work_fds);

        for (int clientIt = 0; clientIt < cur_fd; clientIt++)
            FD_SET(clients[clientIt], &work_fds);

        res = pselect(MAX_FD + 1, &work_fds, NULL, NULL, NULL, &origMask);
        if (errno != EINTR && res == -1) {
            //some actions on receiving the signal
            printf("pselect");
            exit(1);
        }
        if (wasSigHup) {
            printf("Signal received\n");
            wasSigHup = 0;
            break;
        }

        if (FD_ISSET(socket_fd, &work_fds)) {
            int sock = accept(socket_fd, NULL, NULL);
            if (sock < 0) {
                perror("accept");
                exit(3);
            }
            if (cur_fd + 1 > MAX_FD) {
                continue;
            } else {
                clients[cur_fd] = sock;
                cur_fd++;
                printf("Add client. Client socket: %d\n", sock);
            }

        }
        for (int clientIt = 0; clientIt < cur_fd; clientIt++)
            if (FD_ISSET(clients[clientIt], &work_fds)) {
                bzero(buffer, 256);
                int n = read(clients[clientIt], buffer, 255);
                if (n <= 0 | n == 2) {
                    close(clients[clientIt]);
                    for (int i = clientIt + 1; i < cur_fd; i++)
                        clients[i - 1] = clients[i];
                    cur_fd--;
                    printf("Client %d close connection\n", clients[clientIt]);
                    continue;
                }
                printf("Client %d send %d bytes\n", clients[clientIt], n);
            }
    }

    return 0;
};
