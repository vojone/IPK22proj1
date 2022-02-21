#include <sys/socket.h>
#include <sys/types.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#define BACKLOG 3

void printUsage() {
    fprintf(stderr, "Usage: .\\hinfosvc <PORT_NUMBER>\n");
}

int main(int argc, char **argv) {
    if(argc <= 1) {
        fprintf(stderr, "Port number missing!\n");
        printUsage();
        return EXIT_FAILURE;
    }

    char *end;
    unsigned int port_number = strtoul(argv[1], &end, 10);
    if(*end != '\0') {
        fprintf(stderr, "Error! Invalid port number\n");
        printUsage();
        return EXIT_FAILURE;
    }

    int server = socket(AF_INET6, SOCK_STREAM, 0);
    if(server < -1) {
        perror("Cannot create server socket!");
        return EXIT_FAILURE;
    }

    int opt = 1;
    if(setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Unable to set server socket!");
        return EXIT_FAILURE;
    }

    struct sockaddr_in6 server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_port = htons(port_number);
    server_addr.sin6_addr = in6addr_any;

    if(bind(server, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Cannot bind socket to the port!");
        return EXIT_FAILURE;
    }

    if(listen(server, BACKLOG) < 0) {
        perror("Error while setting socket to listen mode!");
        return EXIT_FAILURE;
    }

    struct sockaddr_in6 client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char msg_buffer[1024];
    char cpu_name[1024];
    memset(cpu_name, '\0', 1024);
    while(true) {
        int socket = accept(server, (struct sockaddr *)&client_addr, &client_addr_len);
        if(socket <= 0) {
            perror("Error while accepting!");
            return EXIT_FAILURE;
        }

        size_t msg_len = read(socket, msg_buffer, 1024);
        char msg[1024];
        memset(msg, '\0', 1024);
        char *tmp1;
        if(msg_len > 0) {
            char *method = strtok(msg_buffer, " ");
            (void)method;

            char *path = strtok(NULL, " ");
            //fprintf(stderr, "%s\n", path);

            if(strcmp(path, "/hostname") == 0) {
                tmp1 = "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\nContent-Type: text/plain\r\n\r\n";

                char tmp[128];
                if(gethostname(tmp, 128) < 0) {
                    perror("Cannot get the hostname!");
                    return EXIT_FAILURE;
                }

                snprintf(msg, 1024, tmp1, (strlen(tmp) + 1));
                strcat(msg, tmp);
                strcat(msg, "\n");
            }
            else if(strcmp(path, "/cpu-name") == 0) {
                tmp1 = "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\nContent-Type: text/plain\r\n\r\n";

                FILE* cpu_info_file = fopen("/proc/cpuinfo", "r");
                if(!cpu_info_file) {
                    perror("Unable to get CPU info!");
                }

                char tmp[1024];
                memset(msg, '\0', 1024);
                if(strcmp(cpu_name, "") == 0) {
                    char *needed_info = "model name";
                    char *info;
                    char cur_char;

                    while((cur_char = fgetc(cpu_info_file)) != EOF) {
                        if(cur_char != '\n') {
                            tmp[strlen(tmp)] = cur_char;
                            tmp[strlen(tmp)] = '\0';
                        }
                        else {
                            if(strstr(tmp, needed_info)) {
                                strtok(tmp, ":");
                                info = strtok(NULL, ":");

                                break;
                            }
                            else {
                                memset(tmp, '\0', 1024);
                            }
                        }
                    }
                    if(cur_char == EOF) {
                        fprintf(stderr, "Cannot find needed row in info file");
                    }

                    fclose(cpu_info_file);

                    info = info + 1;
                    memcpy(cpu_name, info, 1024);
                }
            

                snprintf(msg, 1024, tmp1, (strlen(cpu_name) + 1));
                strcat(msg, cpu_name);
                strcat(msg, "\n");
            }
            else if(strcmp(path, "/load") == 0) {
                tmp1 = "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\nContent-Type: text/plain\r\n\r\n";

                FILE* cpu_stat_file = fopen("/proc/stat", "r");
                if(!cpu_stat_file) {
                    perror("Unable to get CPU status!");
                }

                char tmp[1024];
                memset(tmp, '\0', 1024);
                if(fgets(tmp, 1024, cpu_stat_file) == NULL) {
                    break;
                }

                char* start = strtok(tmp, " ");
                if(!strstr(start, "cpu")) {
                    break;
                }


                char* user_str = strtok(NULL, " ");
                char* nice_str = strtok(NULL, " ");
                char* system_str = strtok(NULL, " ");
                char* idle_str = strtok(NULL, " ");
                char* io_wait_str = strtok(NULL, " ");
                char* irq_str = strtok(NULL, " ");
                char* softirq_str = strtok(NULL, " ");
                char* steal_str = strtok(NULL, " ");

                unsigned int puser = atoi(user_str);
                unsigned int pnice = atoi(nice_str);
                unsigned int psystem = atoi(system_str);
                unsigned int pidle = atoi(idle_str);
                unsigned int pio_wait = atoi(io_wait_str);
                unsigned int pirq = atoi(irq_str);
                unsigned int psoftirq = atoi(softirq_str);
                unsigned int psteal = atoi(steal_str);

                fseek(cpu_stat_file, 0, SEEK_SET);

                sleep(1);

                 if(fgets(tmp, 1024, cpu_stat_file) == NULL) {
                    break;
                }

                start = strtok(tmp, " ");
                user_str = strtok(NULL, " ");
                nice_str = strtok(NULL, " ");
                system_str = strtok(NULL, " ");
                idle_str = strtok(NULL, " ");
                io_wait_str = strtok(NULL, " ");
                irq_str = strtok(NULL, " ");
                softirq_str = strtok(NULL, " ");
                steal_str = strtok(NULL, " ");

                unsigned int user = atoi(user_str);
                unsigned int nice = atoi(nice_str);
                unsigned int system = atoi(system_str);
                unsigned int idle = atoi(idle_str);
                unsigned int io_wait = atoi(io_wait_str);
                unsigned int irq = atoi(irq_str);
                unsigned int softirq = atoi(softirq_str);
                unsigned int steal = atoi(steal_str);


                pidle = pidle + pio_wait;
                idle = idle + io_wait;

                //Computation of CPU load was taken from https://stackoverflow.com/questions/23367857/accurate-calculation-of-cpu-usage-given-in-percentage-in-linux
                unsigned int pnon_idle = puser + pnice + psystem + pirq + psoftirq + psteal;
                unsigned int non_idle = user + nice + system + irq + softirq + steal;

                unsigned int ptotal = pidle + pnon_idle;
                unsigned int total = idle + non_idle;

                unsigned int total_diff = total - ptotal;
                unsigned int idle_diff = idle - pidle;

                double cpu_load_perc = ((total_diff - idle_diff)/(double)total_diff)*100;

                snprintf(tmp, 1024, "%f", cpu_load_perc);
                snprintf(msg, 1024, tmp1, (strlen(tmp) + 1));
                strcat(msg, tmp);
                strcat(msg, "\n");
            }
            else {
                tmp1 = "HTTP/1.1 404 Not Found\r\n";
                memcpy(msg, tmp1, strlen(tmp1) + 1);
            }

        }

        send(socket, msg, (strlen(msg) + 1)*sizeof(char), 0);

        close(socket);
    }


    return EXIT_SUCCESS;
}