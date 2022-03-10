/******************************************************************************
 *                       IPK - project 1 - Simple HTTP server
 * 
 *                                File: server.c
 *                            Author: Vojtech Dvorak
 * 
 *                                  March 2022
 * 
 * ***************************************************************************/

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#define BACKLOG 3

#define MAX_PORT_NUM 65535

#define HTTP_RESP_HEADER "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\nContent-Type: text/plain\r\n\r\n"
#define NOT_FOUND_HEADER "HTTP/1.1 404 Not Found\r\nContent-Length: 16\r\nContent-Type: text/plain\r\n\r\n404 Not Found!\r\n"
#define BAD_REQUEST_HEADER "HTTP/1.1 400 Bad Request\r\nContent-Length: 18\r\nContent-Type: text/plain\r\n\r\n400 Bad Request!\r\n"

#define BUFFER_SIZE 4096

typedef enum stat_fields {
    USER, NICE, SYSTEM, IDLE, IO_WAIT, IRQ, SOFTIRQ, STEAL, STAT_FIELDS_NUM
} t_stat_fields;

/**
 * @brief Prints usage to stdout
 */
void printUsage() {
    fprintf(stderr, "Usage: .\\hinfosvc <PORT_NUMBER>\n");
}

/**
 * @brief Gets port number from arguments of program and check its validity
 * @param argc From main function
 * @param argv From main function
 * @return int Port number where server should listen
 */
int parse_args(int argc, char **argv) {
    if(argc <= 1) {
        fprintf(stderr, "Port number missing!\n");
        printUsage();

        exit(EXIT_FAILURE);
    }

    char *end;
    unsigned int port_number = strtoul(argv[1], &end, 10);
    if(*end != '\0' || port_number > MAX_PORT_NUM) {
        fprintf(stderr, "Error! Invalid port number\n");
        printUsage();

        exit(EXIT_FAILURE);
    }

    return port_number;
}

/**
 * @brief Create a server socket and assigns it to given port
 * @param port_number Port where should be server socket set
 * @return int socket fd
 */
int create_server_socket(int port_number) {
    int server_socket = socket(AF_INET6, SOCK_STREAM, 0);
    if(server_socket < -1) {
        perror("Cannot create server socket!");
        exit(EXIT_FAILURE);
    }

    int opt = 1;

    //Set options of server socket
    if(setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Unable to set server socket!");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in6 server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_port = htons(port_number);
    server_addr.sin6_addr = in6addr_any;

    if(bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Cannot bind socket to the port!");
        close(server_socket);
        return EXIT_FAILURE;
    }

    return server_socket;
}

/**
 * @brief Marks server socket as passive listening socket
 * @note If it cannot be set to this mode it closes the socket 
 *       and ends program with EXIT_FAILURE
 */
void set_to_listen(int server_socket) {
    if(listen(server_socket, BACKLOG) < 0) {
        perror("Error while setting socket to listening!");
        close(server_socket);
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Reads one line from given file 
 * @param out_buffer Buffer where will be loaded read line
 * @param buffer_size Size of given buffer
 * @param source Source file from which will be read line
 * @return size_t Length of line
 */
size_t read_line(char *out_buffer, size_t buffer_size, FILE *source) {
    char cur_char;
    size_t cur_len = 0;

    do {
        cur_char = fgetc(source);
        if(cur_char != EOF && cur_char != '\n' && cur_len < buffer_size - 1) {
            *(out_buffer + cur_len) = cur_char;
            cur_len++;
        }
    }
    while(cur_char != EOF && cur_char != '\n' && cur_len < buffer_size - 1);

    out_buffer[cur_len] = '\0';

    return cur_len;
}

/**
 * @brief Reads hostname of machine from /etc/hostname
 * @param buffer Points to buffer where is hostname stored
 * @param len Length of hostname
 * @return true If everything was OK
 * @return false If an error occured
 */
bool read_hostname(char *buffer, size_t *len) {
    FILE * hostname_fd = fopen("/etc/hostname", "r");
    if(!hostname_fd) {
        perror("Cannot get the hostname!");
        return false;
    }

    read_line(buffer, BUFFER_SIZE - 1, hostname_fd);
    strcat(buffer, "\n");

    *len = strlen(buffer);

    fclose(hostname_fd);

    return true;
}

/**
 * @brief Reads hostname of machine from /etc/hostname
 * @param buffer Points to buffer where is hostname stored
 * @param len Length of hostname
 * @return true If everything was OK
 * @return false If an error occured
 */
bool read_cpu_name(char *buffer, size_t *len) {
    FILE * cpuinfo_fd = fopen("/proc/cpuinfo", "r");
    if(!cpuinfo_fd) {
        perror("Cannot get info about CPU!");
        return false;
    }

    char *wanted_row = "model name";
    char tmp[BUFFER_SIZE];
    
    do {
        read_line(tmp, BUFFER_SIZE, cpuinfo_fd);
    }
    while(!strstr(tmp, wanted_row));

    strtok(tmp, ":"); //Set strtok function and throw away line start
    char *cpu_name = strtok(NULL, ":");
    if(buffer == NULL) {
        perror("Cannot get info about CPU! Error while reading file!");
        fclose(cpuinfo_fd);
        return false;
    }

    //There is whitespace before cpu name -> cpu_name + 1
    strncpy(buffer, cpu_name + 1, BUFFER_SIZE - 1);
    strcat(buffer, "\n");

    *len = strlen(buffer);

    fclose(cpuinfo_fd);

    return true;
}

/**
 * @brief Reads cpu stats from /proc/stat (it can be used for computing cpu load)
 * @param stat_arr Array that will be filled by values from read file
 * @return true If reading of /proc/stat was succesful
 * @return false If an error occured
 */
bool read_stats(unsigned int *stat_arr) {
    FILE* stat_file_fd = fopen("/proc/stat", "r");
    if(!stat_file_fd) {
        perror("Unable to get CPU status file!");
        return false;
    }

    char tmp[BUFFER_SIZE];
    read_line(tmp, BUFFER_SIZE, stat_file_fd);
    
    strtok(tmp, " ");
    for(int i = 0; i < STAT_FIELDS_NUM; i++) {
        char *str = strtok(NULL, " ");
        if(str == NULL) {
            perror("Unable to read all informations from CPU stat file!");
            fclose(stat_file_fd);
            return false;
        }

        //Converting string to integer (due to source file we can assume that it has numeric format)
        stat_arr[i] = atoi(str); 
    }

    fclose(stat_file_fd);

    return true;
}

/**
 * @brief Returns string buffer with cpu load (in percent)
 * @param buffer Destination buffer with string
 * @param len Length of returned string
 * @return true If computing of load was succesful
 * @return false If an error occured
 */
bool get_load(char *buffer, size_t *len) {
    unsigned int prev_stats[STAT_FIELDS_NUM];
    unsigned int stats[STAT_FIELDS_NUM];

    if(!read_stats(prev_stats)) {
        return false;
    }

    sleep(1);

    if(!read_stats(stats)) {
        return false;
    }
    
    //Computation algorithm of CPU load was inspired by https://stackoverflow.com/questions/23367857/accurate-calculation-of-cpu-usage-given-in-percentage-in-linux
    unsigned int pidle = prev_stats[IDLE] + prev_stats[IO_WAIT];
    unsigned int idle = stats[IDLE] + stats[IO_WAIT];
    unsigned int pnon_idle = prev_stats[USER] + prev_stats[NICE] + prev_stats[SYSTEM] + prev_stats[IRQ] + prev_stats[SOFTIRQ] + prev_stats[STEAL];
    unsigned int non_idle = stats[USER] + stats[NICE] + stats[SYSTEM] + stats[IRQ] + stats[SOFTIRQ] + stats[STEAL];

    unsigned int ptotal = pidle + pnon_idle;
    unsigned int total = idle + non_idle;

    unsigned int total_diff = total - ptotal;
    unsigned int idle_diff = idle - pidle;

    double cpu_load_perc = ((total_diff - idle_diff)/(double)total_diff)*100;
    //

    *len = snprintf(buffer, BUFFER_SIZE, "%.0f%%\n", cpu_load_perc);

    return true;
}

/**
 * @brief Create a response message
 */
char * create_response(char *dest, char *msg, size_t msg_len) {
    int written = snprintf(dest, BUFFER_SIZE, HTTP_RESP_HEADER, msg_len);
    strncat(dest, msg, BUFFER_SIZE - written - 1);

    return dest;
}

/**
 * @brief Get the path from HTTP request and checks its validity (request line and presence of blank line)
 * @param req Pointer to HTTP request message
 * @return char* Pointer to path from HTTP request (or NULL if HTTP request has invalid format)
 * @note it requires at least GET method, path, HTTP version on first line
 */
char * get_path(char *req) {
    if(req == NULL) {
        return NULL;
    }

    char *method = strtok(req, " ");
    if(!method || strcmp(method, "GET") != 0) {
        return NULL;
    }

    char *path = strtok(NULL, " ");
    if(!path) {
        return NULL;
    }

    char *http_version = strtok(NULL, "\r\n");
    if(!http_version) {
        return NULL;
    }

    return path;
}


/**
 * @brief Main function of HTTP server
 */
int main(int argc, char **argv) {
    int port_number = parse_args(argc, argv);
    int server_socket = create_server_socket(port_number);

    set_to_listen(server_socket);

    struct sockaddr_in6 client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char msg_buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    char payload[BUFFER_SIZE];
    while(true) {
        int socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if(socket <= 0) {
            perror("Error while accepting!");
            return EXIT_FAILURE;
        }

        size_t msg_len = read(socket, msg_buffer, BUFFER_SIZE - 1);
        size_t payload_len = 0;
        msg_buffer[msg_len] = '\0'; //Set correct ending to message

        if(msg_len > 0) {
            char *path = get_path(msg_buffer); //Get path from HTTP request

            if(path == NULL) { //Bad format of HTTP request
                memcpy(response, BAD_REQUEST_HEADER, strlen(BAD_REQUEST_HEADER) + 1);
            }
            else if(strcmp(path, "/hostname") == 0) {
                if(!read_hostname(payload, &payload_len)) {
                    close(server_socket);
                    return EXIT_FAILURE;
                }

                create_response(response, payload, payload_len);
            }
            else if(strcmp(path, "/cpu-name") == 0) {
               if(!read_cpu_name(payload, &payload_len)) {
                    close(server_socket);
                    return EXIT_FAILURE;
                }

                create_response(response, payload, payload_len);
            }
            else if(strcmp(path, "/load") == 0) {
                if(!get_load(payload, &payload_len)) {
                    close(server_socket);
                    return EXIT_FAILURE;
                }
                
                create_response(response, payload, payload_len);
            }
            else { //Given path was not recognized
                memcpy(response, NOT_FOUND_HEADER, strlen(NOT_FOUND_HEADER) + 1);
            }
        }

        send(socket, response, strlen(response)*sizeof(char), 0);

        close(socket);
    }


    return EXIT_SUCCESS;
}