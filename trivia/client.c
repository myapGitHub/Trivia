#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>

int main(int argc, char* argv[]) { 
    int    server_fd;
    struct sockaddr_in server_addr;
    socklen_t addr_size = sizeof(server_addr);
    int opt;
    char* IPAddress = "127.0.0.1";
    int portNumber = 25555;

    while ((opt = getopt(argc, argv, "i:p:h")) != -1) { // Parse thru args, setting specified fields if neededa
        switch (opt) {
        case 'i':
            IPAddress = optarg;
            break;
        case 'p':
            portNumber = atoi(optarg);
            break;
        case 'h':
            printf("Usage: %s [-i IP_address] [-p port_number] [-h]\n", argv[0]);
            printf("-i IP_address       Default to \"127.0.0.1\";\n");
            printf("-p port_number      Default to 25555;\n");
            printf("-h                  Display this help info.\n");
            exit(EXIT_SUCCESS);
        default:
            fprintf(stderr, "Error: Unknown option '-%c' received.\n", optopt);
            exit(EXIT_FAILURE);
        }
    }
    /* STEP 1:
    Create a socket to talk to the server;
    */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(25555);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    /* STEP 2:
    Try to connect to the server.
    */
    connect(server_fd,
        (struct sockaddr*)&server_addr,
        addr_size);

    char buffer[1024];
    fd_set client_fds;
    int max_fd = server_fd;

    while (1) {

        FD_SET(server_fd, &client_fds);
        FD_SET(STDIN_FILENO, &client_fds); 

        select(max_fd + 1, &client_fds, NULL, NULL, NULL);

        if (FD_ISSET(server_fd, &client_fds)) {  
            int recvbytes = read(server_fd, buffer, 1024);
            if (recvbytes == 0) break;
            buffer[recvbytes] = '\0';
            printf("%s", buffer); fflush(stdout);
        }

        if (FD_ISSET(STDIN_FILENO, &client_fds)) {  
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\n")] = '\0'; // Trim newline characterfas
            if (strlen(buffer) > 0) {
                send(server_fd, buffer, strlen(buffer), 0);
            }
        }
    }

    close(server_fd);
    return 0;
}
