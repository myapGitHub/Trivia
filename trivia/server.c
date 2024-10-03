//Matthew Yap I pledge my honor that I have abided by the Stevens Honor System
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>

#define MAX_CONN 2

struct Entry {
    char prompt[1024];
    char options[3][50];
    int answer_idx;
};

struct Player {
    int fd;
    int score;
    char name[128];
    int is_ready;
};

int all_players_ready(struct Player players[MAX_CONN], int player_count) {
    if (player_count < MAX_CONN) {
        return 0; // Not enough players
    }
    for (int i = 0; i < player_count; i++) {
        if (!players[i].is_ready) {
            return 0; // At least one player is not ready
        }
    }
    return 1; // All players are ready
}

void send_question_to_all(struct Player players[MAX_CONN], struct Entry question, int player_count) { // Sends questions to all clients/players
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "Question: %s\n1: %s\n2: %s\n3: %s\nChoose (1-3): ", question.prompt, question.options[0], question.options[1], question.options[2]); // Prints out options
    printf("%s\n", buffer);
    for (int i = 0; i < player_count; i++) {
        if (players[i].fd != -1) {
            send(players[i].fd, buffer, strlen(buffer), 0);
        }
    }
}

int read_questions(struct Entry* arr, char* filename) { // Reads the question file
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("Failed to open file");
        return -1;
    }

    char buffer[1024];
    int count = 0;

    while (fgets(buffer, sizeof(buffer), file) && count < 50) { // Use count < 50 to ensure no more than 50 questions are read
        buffer[strcspn(buffer, "\n")] = 0; // Trim newline character from buffer

        if (strlen(buffer) == 0) continue;  // Skip empty lines

        // Read the question prompt
        strcpy(arr[count].prompt, buffer);

        // Read options
        if (fgets(buffer, sizeof(buffer), file)) {
            buffer[strcspn(buffer, "\n")] = 0; // Trim newline character
            sscanf(buffer, "%49s %49s %49s", arr[count].options[0], arr[count].options[1], arr[count].options[2]);
        }

        // Read the answer
        if (fgets(buffer, sizeof(buffer), file)) {
            buffer[strcspn(buffer, "\n")] = 0; // Trim newline character
            // Find out which option matches the answer
            for (int i = 0; i < 3; i++) {
                if (strcmp(arr[count].options[i], buffer) == 0) {
                    arr[count].answer_idx = i;
                    break;
                }
            }
        }

        count++;
    }

    fclose(file); // CLOSE FILE AFTER DONE

    return count; // Return the number of entries read
}

int main(int argc, char* argv[]) {
    int server_fd;
    int client_fd;
    struct sockaddr_in server_addr;
    struct sockaddr_in in_addr;
    socklen_t addr_size = sizeof(in_addr);

    int current_question = 0;
    int question_answered = 0;

    char introMessage[] = "Welcome to the 392 trivia game!\n"
        "Once all players have been logged in, an example will pop up.\n"
        "Select numbers one through three to select an answer.\n"
        "Once an answer, right or wrong, has been submitted by anyone\n"
        "the client will show whether it is right or wrong, wait 1 second,\n"
        "and then proceed to the next question. Have fun! The first example is below:\n\n"; // I LOVE CHEESE

    int opt; //default dance
    char* questionFile = "questions.txt";
    char* IPAddress = "127.0.0.1";
    int portNumber = 25555;

    while ((opt = getopt(argc, argv, "f:i:p:h")) != -1) { // Parse thru args, setting specified fields if needed
        switch (opt) {
        case 'f':
            questionFile = optarg;
            break;
        case 'i':
            IPAddress = optarg;
            break;
        case 'p':
            portNumber = atoi(optarg);
            break;
        case 'h':
            printf("Usage: %s [-f question_file] [-i IP_address] [-p port_number] [-h]\n", argv[0]);
            printf("-f question_file    Default to \"questions.txt\";\n");
            printf("-i IP_address       Default to \"127.0.0.1\";\n");
            printf("-p port_number      Default to 25555;\n");
            printf("-h                  Display this help info.\n");
            exit(EXIT_SUCCESS);
        default:
            fprintf(stderr, "Error: Unknown option '-%c' received.\n", optopt);
            exit(EXIT_FAILURE);
        }
    }

    /* STEP 1
    Create and set up a socket
    */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd == -1) {
        perror("Server");
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(25555);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    /* STEP 2
    Bind the file descriptor with address structure
    so that clients can find the address
    */

    int i =
        bind(server_fd,
            (struct sockaddr*)&server_addr,
            sizeof(server_addr));

    if (i < 0) { perror("No Connections"); exit(1); }
    /* STEP 3
    Listen to at most 2 incoming connections
    */

    if (listen(server_fd, MAX_CONN) == 0)
        printf("Welcome to 392 Trivia!\n\n");
    else
        perror("listen");

    // Initialize question Entry array to hold questions
    struct Entry questions[50];

    int num_questions = read_questions(questions, questionFile);

    if (num_questions <= 0) {
        fprintf(stderr, "Failed to read questions from specified file\n");
        return(EXIT_FAILURE);
    }

    /* STEP 4
    Accept connections from clientsp
    to enable communication
    */

    struct Player players[MAX_CONN]; // Create arr for players
    int player_count = 0;

    fd_set myset;
    FD_SET(server_fd, &myset);
    int max_fd = server_fd;
    int n_conn = 0;
    for (int i = 0; i < MAX_CONN; i++) players[i].fd = -1;

    char* receipt = "Read\n";
    int recvbytes = 0;
    char buffer[1024];

    for (int i = 0; i < MAX_CONN; i++) { // Initialize said arr for players, up to 2
        players[i].fd = -1;
        players[i].score = 0;
        players[i].is_ready = 0;
        memset(players[i].name, 0, sizeof(players[i].name));
    }

    while (1) {

        FD_SET(server_fd, &myset);
        max_fd = server_fd;
        for (int i = 0; i < MAX_CONN; i++) {
            if (players[i].fd != -1) {
                FD_SET(players[i].fd, &myset);
                if (players[i].fd > max_fd) max_fd = players[i].fd;
            }
        }
        select(max_fd + 1, &myset, NULL, NULL, NULL);

        if (FD_ISSET(server_fd, &myset)) {
            client_fd = accept(server_fd,
                (struct sockaddr*)&in_addr,
                &addr_size);
            if (client_fd == -1) {// Error check accepting
                perror("Accept");
                continue;
            }
            if (n_conn < MAX_CONN) {
                printf("New connection detected!\n");
                const char* welcome_msg = "Please type your name: \n";
                send(client_fd, welcome_msg, strlen(welcome_msg), 0);
                n_conn++;
                for (int i = 0; i < MAX_CONN; i++) { // INitialize players elements to play
                    if (players[i].fd == -1) {
                        players[i].fd = client_fd;
                        players[i].score = 0;
                        memset(players[i].name, 0, sizeof(players[i].name));
                        break;
                    }
                }
            }
            else {
                printf("Max connection reached!\n");
                close(client_fd);
                break;
            }
        }

        for (int i = 0; i < MAX_CONN; i++) { // Ask for names and get them in the game
            if (players[i].fd != -1 && FD_ISSET(players[i].fd, &myset)) {
                recvbytes = read(players[i].fd, buffer, 1024);
                buffer[recvbytes] = '\0';
                if (recvbytes < 0) {
                    perror("recv");
                }
                else if (recvbytes > 0) {
                    if (strlen(players[i].name) == 0) {
                        strncpy(players[i].name, buffer, sizeof(players[i].name) - 1);
                        players[i].is_ready = 1;
                        printf("Hi %s!\n", players[i].name);
                        send(players[i].fd, introMessage, strlen(introMessage), 0);
                    }
                    else {
                        printf("[%s] %s\n", players[i].name, buffer);
                    }
                }
                else {
                    printf("Connection lost!\n");
                    close(players[i].fd);
                    players[i].fd = -1;
                    n_conn--;
                }
            }
        }

        int ready = all_players_ready(players, MAX_CONN);
        //while (current_question < num_questions && ready player one== 1) {
        if (current_question < num_questions && ready == 1) {
            if (current_question == 0) { // Check if this is the start of the game
                printf("%s", introMessage);
                printf("The game starts now!\n");
            }


            
            if (current_question < num_questions && all_players_ready(players, MAX_CONN)) {
                if (current_question == 0) {  // Check if the question has not been answered yet
                    send_question_to_all(players, questions[current_question], MAX_CONN);
                }

                for (int i = 0; i < MAX_CONN; i++) {
                    if (players[i].fd != -1 && FD_ISSET(players[i].fd, &myset) && !question_answered) { // if no one dc and question is not ans

 
                        if (recvbytes == 0) { // Print msg if player dc
                        
                            printf("Connection Lost!\n");
                            close(players[i].fd);
                            players[i].fd = -1;
                            n_conn--;
                            break;
                            exit(EXIT_FAILURE);
                        }
                        if (recvbytes > 0) { // bytes receive
                            buffer[recvbytes] = '\0'; // bytes receieve
                            int player_answer = atoi(buffer) - 1;  // Convert answer to index

                            char result_message[256];
                            if (player_answer == questions[current_question].answer_idx) {
                                players[i].score += 1;  // Increase score for correct answer
                                snprintf(result_message, sizeof(result_message), "Correct! %s was the right answer.\n", questions[current_question].options[player_answer]);
                            }
                            else {
                                players[i].score -= 1; // Decrease score for incorrect answer
                                snprintf(result_message, sizeof(result_message), "Wrong! The correct answer was %s.\n", questions[current_question].options[questions[current_question].answer_idx]);
                            }

                            printf("%s", result_message);
                            // Broadcast the result to all players
                            for (int j = 0; j < MAX_CONN; j++) {
                                if (players[j].fd != -1) {
                                    send(players[j].fd, result_message, strlen(result_message), 0);
                                }
                            }

                            question_answered = 1;  // Mark the question as answered
                        }
                    }
                }

                //if (question_answered) {
                //    // Move to the next question after a short delay or instantly based on your game design
                //    current_question++;
                //    question_answered = 0;  // Reset for the next question
                //}

                if (question_answered) {
                    // Brief pause before next question
                    sleep(1);  // Pause for 1 second
                    current_question++;
                    // Check if there are more questions to ask
                    if (current_question < num_questions) {
                        send_question_to_all(players, questions[current_question], MAX_CONN);
                        question_answered = 0;  // Reset for the next question
                    }
                }
            }
        }

        if (current_question >= num_questions) {
            int max_score = -1; 
            int winner_index = -1;
            char winner_message[256];

            // Determine the winner
            for (int i = 0; i < MAX_CONN; i++) {
                if (players[i].fd != -1 && players[i].score > max_score) {
                    max_score = players[i].score;
                    winner_index = i;
                }
            }

            snprintf(winner_message, sizeof(winner_message), "Congrats, %s!\n", players[winner_index].name);
      

            // send winner message to loser and server
            for (int i = 0; i < MAX_CONN; i++) {
                if (players[i].fd != -1) {
                    send(players[i].fd, winner_message, strlen(winner_message), 0);
                    close(players[i].fd); // Close the connection
                }
            }

            printf("%s", winner_message); 
            close(server_fd); 
            exit(EXIT_SUCCESS); 
        }

    }//While loop end

    return 0;
}
