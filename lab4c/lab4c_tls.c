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
#include <sys/stat.h>
#include <mraa/gpio.h>
#include <mraa/aio.h>
#include <sys/socket.h>
#include <netdb.h> //gethostbyname()
#include <netinet/in.h>
#include <ctype.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
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
const int BUF_SIZE = 256;
const int SIZE = 32;

int period = 1;
int report = 0;
int run_flag = 1;
char* id = "";
int log_fd;
int port = -1;


/* Host */
char *hostname = "";
struct hostent *server_host;
struct sockaddr_in server_address;
int sock;

/* SSL structures */
SSL_CTX *newContext = NULL;
SSL *sslClient = NULL;

void error_handling(char* message) {
    fprintf(stderr, "%s\n", message);
    exit(2);
}


void initialize_sensors(){
    temp_sensor = mraa_aio_init(1);
    if(temp_sensor == NULL){
        mraa_deinit();
        exit(2);
    }
}

void ssl_init() {
    SSL_library_init();
    //Initialize the error message
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    //TLS version: v1, one context per server.
    newContext = SSL_CTX_new(TLSv1_client_method());
    if (newContext == NULL){
        error_handling("ERROR! Cannot get SSL context.\n");
    }
}

void ssl_clean_client(SSL* client) {
    SSL_shutdown(client);
    SSL_free(client);
}

double get_temp(){
    /* Discussion Slides */
    // Read temperature
    float temperature = mraa_aio_read(temp_sensor);
    float R = (1023.0/ temperature) - 1.0;
    R = R0*R;
    //Convert scale
    float tempC = 1.0/ (log(R/R0) / B + 1/298.15) - 273.15;
    float tempF = (tempC * 9)/5 + 32;
    float ret;
    if (scale == 'F'){
        ret = tempF;
    }
    if (scale == 'C'){
        ret = tempC;
    }
    return ret;
    
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
        run_flag = 0;
    }
    else if (strcmp(input, "LOG") == 0) {
    }
}

int main(int argc, char *argv[]) {

    static const struct option long_options[] = {
        {"period",  required_argument, NULL, 'p'},
        {"scale",   required_argument, NULL, 's'},
        {"log",   required_argument, NULL, 'l'},
        {"id",   required_argument, NULL, 'i'},
        {"host", required_argument, NULL, 'h'},
        {0,0,0,0}
    };

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
                log_fd = open(optarg, O_WRONLY | O_CREAT, 0666);
                if (log_fd < 0) {
                   fprintf(stderr, "ERROR! Failed to create/open file: %s\n", strerror(errno));
                   exit(1);
                }
                break;
            case 'i':
                id = optarg;
                if (strlen(id) != 9) {
                    fprintf(stderr, "ID must be a 9 digit number.\n");
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
    
    if (argc < 4){
        fprintf(stderr, "ERROR! Not enough arguments.\n");
        fprintf(stderr, "%s\n", usage);
        exit(1);
    }

    // Port
    if (optind < argc){
        port = atoi(argv[optind]);
        if (port <= 0){
            fprintf(stderr, "Invalid port.\n");
            exit(1);
        }
    }
    
    if (strlen(hostname) == 0) {
        fprintf(stderr, "ERROR! Host argument is REQUIRED.\n");
        exit(1);
    }
    
    
    //Create context
    //TLS: APIs, Initialization.
    ssl_init();

    // Create socket and find host
     if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
         fprintf(stderr, "ERROR! Failure to create socket in client program.\n");
         exit(2);
     }
     if ((server_host = gethostbyname(hostname)) == NULL) {
         fprintf(stderr, "ERROR! Cannot not get host.\n");
         exit(2);
     }
    


    int address_size = sizeof(server_address);
    memset((void *) &server_address, 0, address_size);
    server_address.sin_family = AF_INET;
    memcpy((char *) &server_address.sin_addr.s_addr, (char *) server_host->h_addr, server_host->h_length);
    server_address.sin_port = htons(port);
    if (connect(sock, (struct sockaddr *) &server_address, address_size) < 0) {
              fprintf(stderr, "ERROR! Cannot connect on client side.\n");
              exit(2);
    }
    
    // Attach the SSL to a socket
    sslClient = SSL_new(newContext);
    if (!sslClient) {
        error_handling("ERROR! Cannot complete SSL setup.\n");
    }
    if (!SSL_set_fd(sslClient, sock)) {
        error_handling("ERROR! Cannot set SSL file descriptor.\n");
    }
    if (SSL_connect(sslClient) != 1) {
        error_handling("ERROR! SSL Connection rejected.\n");
    }
    
    // Send Id
    char id_out[SIZE];
    sprintf(id_out, "ID=%s\n", id); //terminated with newline
    if (SSL_write(sslClient, id_out, strlen(id_out)) <= 0){
        error_handling("ERROR! Cannot write to the socket.\n");
    }

    if (write(log_fd, id_out, strlen(id_out)) < 0){
        error_handling("ERROR! Cannot to write to log file.\n");
    }

    
    // Initialize I/O
    initialize_sensors();
    

    // initialize poll
    struct pollfd poll_input;
    poll_input.fd = sock;
    poll_input.events = POLLIN | POLLERR | POLLHUP;
    poll_input.revents = 0;

    //Time stamp
    time_t raw_start;
    time_t raw_finish;
    time(&raw_start);

    while (1) {
        if (!run_flag)
            break;

        int ret = poll(&poll_input,1,0);
        if (ret < 0){
            error_handling("ERROR! Failed to poll for stdin.\n");
        }
        
        
        if (poll_input.revents & POLLIN) {
            char input[BUF_SIZE];
            memset(input, 0, BUF_SIZE);
            //Check read
            int rd = SSL_read(sslClient, input, BUF_SIZE);
            if (rd <= 0){
                error_handling("ERROR: Unable to read from socket");
            }
            if (write(log_fd, input, strlen(input)) < 0){
                error_handling("ERROR: Unable to write input command to log file");
            }
            strtok(input, "\n");
            process_input(input);
        }

 
        time_t raw_pause;
        struct tm *timeinfo;
        time(&raw_pause);
        timeinfo = localtime(&raw_pause);

        if (difftime(raw_pause, raw_start) == period) {
            char buffer[SIZE];
            double temp = get_temp();

            sprintf(buffer, "%02d:%02d:%02d %.1f\n", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, temp);
            

            if (report) {
                if (SSL_write(sslClient, buffer, strlen(buffer)) < 0){
                    error_handling("ERROR! Cannkt write to socket.\n");
                }
                if (write(log_fd, buffer, strlen(buffer)) < 0){
                    error_handling("ERROR! Cannot write to log file.\n");
                }
            }
            raw_start = raw_pause;
        }
    }


    struct tm *timeinfo_2;
    time(&raw_finish);
    timeinfo_2 = localtime(&raw_finish);
    char exit_info[SIZE];
    sprintf(exit_info, "%02d:%02d:%02d SHUTDOWN\n", timeinfo_2 ->tm_hour, timeinfo_2->tm_min, timeinfo_2->tm_sec);
    if (SSL_write(sslClient, exit_info, strlen(exit_info)) < 0){
        error_handling("ERROR! Cannot write to socket");
    }
    if (write(log_fd, exit_info, strlen(exit_info)) < 0){
        error_handling("ERROR! Cannot write to log file");
    }
    
    // close I/O devices
    mraa_aio_close(temp_sensor);
    //Clean up
    ssl_clean_client(sslClient);
    exit(0);
}
