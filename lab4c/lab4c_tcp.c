// NAME: Weikeng Yang
// EMAIL: weikengyang@gmail.com
// UID: 405346443
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <time.h>
#include <stdio.h>
#include <getopt.h>
#include <poll.h>
#include <sys/time.h>
#include <fcntl.h>
#include <math.h>
#include <getopt.h>
#include <mraa/gpio.h>
#include <mraa/aio.h>
#include <sys/socket.h>
#include <netdb.h> //gethostbyname()
#include <netinet/in.h>
#include <ctype.h>
#include <arpa/inet.h> // htons(3)

/*
#else
#include <mraa.h>
#include <mraa/aio.h>
#endif
*/

/*  Global variable*/
const char* usage = "Usage: --scale={F(Farenheight), C(Celcius)}; ‘--period=# Temperature]’ ; ‘--log = filename’; '-id=[9-digit-number]' '--host=[name/address] [port#]' ";
char scale = 'F'; // F is the temperature in Fahrenheit

mraa_aio_context temp_sensor;

/* Form slides*/
const int B = 4275;
const int R0 = 100000.0;
const int BUF_SIZE = 32;
int period = 1;
int report = 1;
int log_flag = 0;
int log_fd;
int ifstop = 0;
char* id = "";
int port = -1;

/* Host */
char *hostname = "";
long val;
char *next;
struct hostent *server_host;
struct sockaddr_in server_address;
int sock;



void error_handling(char* message) {
    fprintf(stderr, "%s with error: %s\n", message, strerror(errno));
    exit(2);
}


void process_input(const char *input) {
    if (strcmp(input,"SCALE=F") == 0) {
        scale = 'F';
    }
    else if (strcmp(input,"SCALE=C") == 0){
        scale = 'C';
    }
    else if (strcmp(input,"STOP") == 0){
        report = 0;
    }
    else if (strcmp(input,"START") == 0){
        report = 1;
    }
    else if (strncmp(input,"PERIOD=", strlen("PERIOD=")) == 0){
        period = atoi (input + 7);
    }
    else if (strcmp(input,"OFF") == 0){
        ifstop = 1;
    }
    else if (strcmp(input, "LOG") == 0) {
    }
}



void initialize_sensors(){
    temp_sensor = mraa_aio_init(1);
    if(temp_sensor == NULL){
        mraa_deinit();
        exit(2);
    }

}


int main(int argc, char *argv[]) {

    static const struct option long_options[] = {
        {"period",  required_argument, NULL, 'p'},
        {"scale",  required_argument, NULL, 's'},
        {"log",   required_argument, NULL, 'l'},
        {"id",  required_argument, NULL, 'i'},
        {"host", required_argument, NULL, 'h'},
        {0,0,0,0}
    };
    // If the arguments are not enough
    if (argc < 4){
        fprintf(stderr, "ERROR! Not enough arguments.\n");
        fprintf(stderr, "%s\n", usage);
        exit(1);
    }
    int opt;
    while (1) {
        opt = getopt_long(argc, argv, "p:s:l:i:h:", long_options, NULL);
        if (opt == -1)
            break;
        switch (opt) {
            case 'p':
                period = atoi(optarg);
                break;
            case 's':
                if (optarg[0] == 'F' || optarg[0] == 'C') {
                    scale = optarg[0];
                }
                break;
            case 'l':

                // initialize log file
                log_fd = open(optarg, O_WRONLY | O_CREAT, 0666);
                if (log_fd < 0) {
                   fprintf(stderr, "ERROR! Failed to create/open file: %s\n", strerror(errno));
                   exit(2);
                }
                log_flag = 1;
                break;
            case 'i':
                id = optarg;
                if (strlen(id) != 9) {
                    fprintf(stderr, "ID must be a 9 digit number\n");
                    exit(1);
                }
                break;
            case 'h':
                hostname = optarg;
                break;
            default:
                fprintf(stderr, "ERROR! Invalid arguments.\n");
                fprintf(stderr, "%s\n", usage);
                exit(1);
        }
    }
    


    if (optind < argc){
        port = atoi(argv[optind]);
        if (port <= 0){
            fprintf(stderr, "Invalid port.\n");
            exit(1);
        }
    }

    // Create socket and find host
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "ERROR! Failure to create socket in client program.\n");
        exit(1);
    }
    if ((server_host = gethostbyname(hostname)) == NULL) {
        fprintf(stderr, "ERROR! Cannot not get host. \n");
        exit(2);
    }

    
    memset((void *) &server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    memcpy((char *) &server_address.sin_addr.s_addr, (char *) server_host->h_addr, server_host->h_length);
    server_address.sin_port = htons(port);
    if (connect(sock, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
              fprintf(stderr, "ERROR! Cannot connect on client side.\n");
              exit(2);
      }
    
    
    // Sent ID
    char ID_out[50];
    sprintf(ID_out, "ID=%s\n", id);
    
    if (write(sock, ID_out, strlen(ID_out)) < 0){
        error_handling("ERROR! Cannot write to the socket");
    }
    if (write(log_fd, ID_out, strlen(ID_out)) < 0){
        error_handling("ERROR! Cannot write to the log file");
    }

    // Initialize I/O
    initialize_sensors();

    
    // Initialize poll
    struct pollfd poll_input;
    poll_input.fd = sock;
    poll_input.events = POLLIN | POLLERR | POLLHUP;
    poll_input.revents = 0;
    
    //Time stamp
    time_t raw_start;
    time_t raw_finish;
    time(&raw_start);

    while (1) {
        int ret = poll(&poll_input, 1, 0);
        if (ret < 0){
            fprintf(stderr, "ERROR! Cannot poll for the socket. \n");
            exit(2);
        }
        if (poll_input.revents & (POLLHUP|POLLERR)){
            close(sock);
            error_handling("ERROR! STDIN hangup. \n");
        }
        
        FILE* file = fdopen(sock, "r");
        if (poll_input.revents & POLLIN) {
            char input[BUF_SIZE];
            fgets(input, BUF_SIZE, file);
     
            if (write(log_fd, input, strlen(input)) < 0){
                fprintf(stderr, "ERROR! Cannot write input to log file \n");
                exit(2);
            }
            strtok(input, "\n");
            process_input(input);
        }

        if (ifstop == 1){
            break;
        }
       
        /* Discussion Slides */
        // Read temperature
        float temperature = mraa_aio_read(temp_sensor);
        float R = (1023.0/ temperature) - 1.0;
        R = R0 * R;
        //Convert scale
        float tempC = 1.0/ (log(R/R0) / B + 1/298.15) - 273.15;
        float tempF = (tempC * 9)/5 + 32;
        
        // Get time stamp
        time_t raw_time;
        struct tm *timeinfo;
        
        time(&raw_time);
        timeinfo = localtime(&raw_time);
        
        if (difftime(raw_time, raw_start) == period) {
            char buffer[BUF_SIZE];
            if (scale == 'F'){ //when Fahrenheit
                sprintf(buffer, "%02d:%02d:%02d %.1f\n", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, tempF);
            }
            if (scale == 'C'){ //when
                sprintf(buffer, "%02d:%02d:%02d %.1f\n", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, tempC);
            }
            //
            if (report) {
                if (write(log_fd, buffer, strlen(buffer)) < 0){
                    error_handling("ERROR! Cannot write to log file. \n");
                }
                if (write(sock, buffer, strlen(buffer)) < 0){
                    error_handling("ERROR! Cannot write to output. \n");
                }
            }
            raw_start = raw_time;
        }
    }

    struct tm *timeinfo2;
    time(&raw_finish);
    timeinfo2 = localtime(&raw_finish);
    char exit_info[BUF_SIZE];
        sprintf(exit_info, "%02d:%02d:%02d SHUTDOWN\n", timeinfo2->tm_hour, timeinfo2->tm_min, timeinfo2->tm_sec);
    if (write(sock, exit_info, strlen(exit_info)) < 0){
        error_handling("ERROR! Cannot write to socket");
    }
    if (write(log_fd, exit_info, strlen(exit_info)) < 0){
        error_handling("ERROR! Cannot write to log file");
    }
    

    // Close the I/O devices
    mraa_aio_close(temp_sensor);


    exit(0);
}
 
