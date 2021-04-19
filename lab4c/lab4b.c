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

#else
#include <mraa.h>
#include <mraa/aio.h>
#endif

const char* usage = "Usage: --scale={F(Farenheight), C(Celcius)}; --period=# Temperature] ; --log = filename ";
char scale = 'F'; // F is the temperature in Fahrenheit
sig_atomic_t volatile run_flag = 1;

/* Form slides*/
const int B = 4275; // B value of the thermistor
const int R0 = 100000.0; // R0 = 100k

int period = 1;
int report = 1;
int log_flag = 0;
int log_file;

mraa_aio_context temp_sensor;
mraa_gpio_context button;


void stop() {
    run_flag = 0;
}


void handle_segfault() {
    fprintf(stderr, "ERROR! Segmentation fault! Exiting......\n");
    exit(2);
}

void error_handling(char* message) {
    fprintf(stderr, "%s with error: %s\n", message, strerror(errno));
    exit(1);
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
    else if (strcmp(input, "LOG ") == 0) {
    }
}

int main(int argc, char *argv[]) {

    static const struct option long_options[] = {
        {"period",  required_argument, NULL, 'p'},
        {"scale",  required_argument, NULL, 's'},
        {"log",   required_argument, NULL, 'l'},
        {0,0,0,0}
    };

    int opt;
    while (1) {
        opt = getopt_long(argc, argv, "p:s:l:", long_options, NULL);
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
                log_file = open(optarg, O_WRONLY | O_CREAT, 0666);
                if (log_file < 0) {
                   fprintf(stderr, "Failed to create/open  file: %s\n", strerror(errno));
                   exit(1);
                }
                log_flag = 1;
                break;
            default:
                fprintf(stderr, "ERROR! Invalid arguments.\n");
                fprintf(stderr, "%s\n", usage);
                exit(1);
        }
    }

    
    //Initialize the I/O devices
    // Analog A0/A1 connector as I/O pin #0
    temp_sensor = mraa_aio_init(0);
    if (temp_sensor == NULL) {
        fprintf(stderr, "ERROR! Failed to initialize AIO\n");
        mraa_deinit();
        exit(1);
    }
     // GPIO_115 connector with address 73
    button = mraa_gpio_init(73);
    if (button == NULL) {
        fprintf(stderr, "ERROR! Failed to initialize GPIO %d\n", 73);
        mraa_deinit();
        exit(1);
    }

    mraa_gpio_dir(button, MRAA_GPIO_IN);
    mraa_gpio_isr(button, MRAA_GPIO_EDGE_RISING, &stop, NULL);

    // initialize poll
    struct pollfd poll_input;
    poll_input.fd = STDIN_FILENO;
    poll_input.events = POLLIN | POLLERR | POLLHUP;
    poll_input.revents = 0;

    
    
    //Time stamp
    time_t raw_start;
    time_t rawFinal;
    time(&raw_start);

    while (run_flag) {
        int ret = poll(&poll_input, 1, 0);
        if (ret < 0){
            error_handling("ERROR! Failed to poll for stdin");
        }
        
        if (poll_input.revents & POLLIN) {
            char input[20];
            fgets(input,20,stdin);
            if (log_flag) {
                if (write(log_file, input, strlen(input)) < 0)
                        error_handling("ERROR! Cannot write input to log file");
            }
            strtok(input, "\n");
            
            process_input(input);
        }

        if (poll_input.revents & (POLLHUP|POLLERR))
            error_handling("ERROR: STDIN hangup/err");
        
        
        /* Discussion Slides */
        // Read temperature
        float temperature = mraa_aio_read(temp_sensor);
        float R = (1023.0/ temperature) - 1.0;
        R = R0 * R;
        //Convert scale
        float tempC = 1.0/ (log(R/R0) / B + 1/298.15) - 273.15;
        float tempF = (tempC * 9)/5 + 32;

        time_t raw_end;
        struct tm *timeinfo;
        
        time(&raw_end);
        timeinfo = localtime(&raw_end);

        
        
        if (difftime(raw_end, raw_start) == period) {
            char str[20];
            switch (scale){
                case 'F': //Fahrenherit
                    sprintf(str, "%02d:%02d:%02d %.1f\n", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, tempF);
                    break;
                case 'C':
                    sprintf(str, "%02d:%02d:%02d %.1f\n", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, tempC);
                    break;
                default:
                    break;
            }
            
            if (report) {
                if (write(1, str, strlen(str)) < 0){
                    error_handling("ERROR! Cannot write to output");
                }
                if (log_flag) {
                    if (write(log_file, str, strlen(str)) < 0)
                        error_handling("ERROR! Unable to write to log file");
                }
            }
            raw_start = raw_end;
        }
        
    }

 
    struct tm *timeinfo2;
    time(&rawFinal);
    timeinfo2 = localtime(&rawFinal);
    
    char exitMsg[20];
        sprintf(exitMsg, "%02d:%02d:%02d SHUTDOWN\n", timeinfo2->tm_hour, timeinfo2->tm_min, timeinfo2->tm_sec);
    if (log_flag) {
        if (write(log_file, exitMsg, strlen(exitMsg)) < 0)
            error_handling("ERROR! Cannot write to log file");
    }
    
    
    // close I/O devices
    mraa_aio_close(temp_sensor);
    mraa_gpio_close(button);


    exit(0);
}
