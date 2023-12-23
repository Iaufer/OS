#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stddef.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>

#define PORT "31337"
#define MAX_PENDING 50

volatile sig_atomic_t was_stopped = 0;

void signal_handler(int signal) {
    was_stopped = 1;
}

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int start_server() {
    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int rv;
    struct addrinfo *ai, *p;
    getaddrinfo(NULL, PORT, &hints, &ai);
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }

    int sockfd = 0;
    for (p = ai; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd < 0) {
            continue;
        }

        int yes = 1;
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) < 0) {
            close(sockfd);
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }

    freeaddrinfo(ai);

    if (listen(sockfd, MAX_PENDING) == -1) {
        perror("listen");
        exit(1);
    }

    return sockfd;
}

void register_handlers(sigset_t* sigmask) {
    struct sigaction sa;
    sigaction(SIGHUP, NULL, &sa);
    sa.sa_handler = signal_handler;
    sa.sa_flags |= SA_RESTART;
    sigaction(SIGHUP, &sa, NULL);

    sigaction(SIGTERM, NULL, &sa);
    sa.sa_handler = signal_handler;
    sa.sa_flags |= SA_RESTART;
    sigaction(SIGTERM, &sa, NULL);

    sigaction(SIGQUIT, NULL, &sa);
    sa.sa_handler = signal_handler;
    sa.sa_flags |= SA_RESTART;
    sigaction(SIGQUIT, &sa, NULL);

    sigset_t blockedMask;
    sigemptyset(&blockedMask);
    sigaddset(&blockedMask, SIGHUP);
    sigaddset(&blockedMask, SIGTERM);
    sigaddset(&blockedMask, SIGSTOP);
    sigprocmask(SIG_BLOCK, &blockedMask, sigmask);
}

int find_client(int* clients, int active, int fd) {
    for (int i = 0; i < active; i++) {
        if (clients[i] == fd) {
            return i;
        }
    }
    return -1;
}

int main() {
    int sockfd = start_server();
    int maxfd = sockfd;

    fd_set master;
    fd_set temp_fd;

    FD_ZERO(&temp_fd);
    FD_ZERO(&master);
    FD_SET(sockfd, &master);

    sigset_t sigmask;
    register_handlers(&sigmask);

    // NOTE: can be dynamicly allocated to prevent overflow
    int clients[50];
    int active = 0;

    for (;;) {
        if (was_stopped) {
            puts("Stopping server...");
            exit(0);
        }

        temp_fd = master;

        maxfd = sockfd;
        for (int i = 0; i < active; i++) {
            if (clients[i] > maxfd) {
                maxfd = clients[i];
            }
        }

        printf("Active clients: %d, maxfd: %d\n", active, maxfd);
        fflush(stdout);

        if (pselect(maxfd+1, &temp_fd, NULL, NULL, NULL, &sigmask) == -1) {
            if (errno == EINTR) {
                puts("Stopping server...");
                close(sockfd);
                exit(0);
            }
            perror("select");
            exit(1);
        }

        for (int fd = 0; fd <= maxfd; fd++) {
            if (!FD_ISSET(fd, &temp_fd)) {
                continue;
            }

            // handle new connection
            if (fd == sockfd) {
                struct sockaddr_storage remote_addr;
                socklen_t addr_size = sizeof remote_addr;
                int newfd = accept(sockfd, (struct sockaddr *)&remote_addr, &addr_size);
                if (newfd == -1) {
                    perror("accept");
                    continue;
                }
                
                FD_SET(newfd, &master);
                clients[active] = newfd;
                active += 1;

                char remoteIP[INET6_ADDRSTRLEN];
                printf("selectserver: new connection from %s on "
                            "socket %d\n",
                            inet_ntop(remote_addr.ss_family,
                                get_in_addr((struct sockaddr*)&remote_addr),
                                remoteIP, INET6_ADDRSTRLEN),
                            newfd);
                continue;
            }

            // handle data from a client
            char buf[256];
            int n = recv(fd, buf, sizeof buf - 1, 0);
            if (n <= 0) {
                if (n == 0) {
                    printf("selectserver: socket %d hung up\n", fd);
                } else {
                    perror("recv");
                }
                close(fd);
                FD_CLR(fd, &master);

                int idx = find_client(clients, active, fd);
                clients[idx] = clients[active - 1];
                active -= 1;

                continue;
            }
            buf[n] = '\0';

            printf("got data from socket %d: %s", fd, buf);
            if (buf[n-1] != '\n') {
                puts("");
            }
        }
    }
}
