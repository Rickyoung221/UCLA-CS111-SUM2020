// NAME: Weikeng Yang
// EMAIL: weikengyang@gmail.com
// ID: 405346443

#include <termios.h> // termios(3), tcgetattr(3), tcsetattr(3)
#include <sys/types.h> // waitpid(2): wait for process to change state; fork(2): creates a new process by duplicating the calling process
#include <unistd.h> // termios(3), tcgetattr(3), tcsetattr(3), fork(2), read(2), write(2), exec(3), pipe(2), getopt_long(3)
#include <stdlib.h> // atexit(3)
#include <stdio.h> // fprintf(3)
#include <string.h> // strerror(3)
#include <errno.h> // errno(3)
#include <getopt.h> // getopt_long(3)
#include <sys/wait.h> // waitpid(2)
#include <signal.h> // kill(3): send a signal to a process or a group of proecesses specified by pid
#include <poll.h> // poll(2): wait for some event on a file descriptor
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <zlib.h>

#define CR '\015' //Carriage Return
#define LF '\012' //Line Feed
#define ETX '\003' //^C (End of text)
#define EOT '\004' //^D (End of transmission)
#define BUFSIZE 256
#define DEBUFSIZE 1024

struct termios old_mode;
char* filename;
int pipe_child[2];
int pipe_parent[2];
char crlf[2] = {CR, LF};
int pid;
int shell_flag;
int sockfd;
int portNo;
int size;
int log_file;

z_stream client_to_server;
z_stream server_to_client;


void shell_exit_status(){
    int status;
    int wait_ret = waitpid(pid, &status, 0) ;
    if (wait_ret != pid) {
        fprintf(stderr, "Waitpid error: %s\n", strerror(errno));
        exit(1);
    }
    if (WIFEXITED(status)) {
        fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WTERMSIG(status), WEXITSTATUS(status));
        exit(0);
    }
}

//the function that will be called upon normal process termination
void restore_terminal(void) {
    close(sockfd);
    close(log_file);
    tcsetattr(STDIN_FILENO, TCSANOW, &old_mode);
    if (shell_flag) {
        shell_exit_status();
    }
}

void set_terminal() {
    struct termios new_mode;
    if (!isatty(STDIN_FILENO)) {
        fprintf(stderr, "ERROR! The standard input does not refer to a terminal.\n");
        exit(1);
    }

    if (tcgetattr(STDIN_FILENO, &old_mode) != 0 || tcgetattr(STDIN_FILENO, &new_mode) != 0) {
        fprintf(stderr, "ERROR! unrecognized argument. \n");
        exit(1);
    }
    atexit(restore_terminal);
    tcgetattr(STDIN_FILENO, &new_mode);
    
    /* Rather, it is suggested that you get the current terminal modes, save them for restoration, and then make a copy with only the following changes: */
    new_mode.c_iflag &= ISTRIP;  /* only lower 7 bits    */
    new_mode.c_oflag = 0;        /* no processing    */
    new_mode.c_lflag = 0;        /* no processing    */

    tcsetattr(STDIN_FILENO, TCSANOW, &new_mode);
}

//Build connection
void connect_server(){
    struct sockaddr_in server_address;
    struct hostent* server;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "ERROR! Failed to create a socket: %s\n", strerror(errno));
        exit(1);
    }
    server = gethostbyname("localhost");
    bzero((char * ) &server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    memcpy((char *) &server_address.sin_addr.s_addr, (char *) server->h_addr,server->h_length);
    server_address.sin_port = htons(portNo);
    int connect_flag = connect(sockfd, (struct sockaddr *) &server_address, sizeof(server_address));
    if ( connect_flag < 0) {
        fprintf(stderr, "ERROR! Failed to connect to socket.\n");
        exit(1);
    }
}

void stdin_compress(int bytes, char *buffer, char *compression_buf){
    client_to_server.avail_in = bytes;
    client_to_server.next_in = (Bytef *) buffer;
    client_to_server.avail_out = BUFSIZE;
    client_to_server.next_out = (Bytef *) compression_buf;

    while (client_to_server.avail_in > 0) {
        deflate(&client_to_server, Z_SYNC_FLUSH);
    }
}
void socket_decompress(int bytes, char *buffer, char *decompression_buf){
     server_to_client.avail_in = bytes;
     server_to_client.next_in = (Bytef *) buffer;
     server_to_client.avail_out = DEBUFSIZE;
     server_to_client.next_out = (Bytef *) decompression_buf;
    
     while (server_to_client.avail_in > 0) {
         inflate(&server_to_client, Z_SYNC_FLUSH);
     }
}

// Clean the compression
void clean_up(){
    inflateEnd(&server_to_client);
    deflateEnd(&client_to_server);
}

void write_sent(int size, char *temp, char *compression_buf){
    char sent_pre[20] = "SENT ";
    char sent_end[20] = " bytes: ";

    sprintf(temp, "%d", size);
    write(log_file, sent_pre, strlen(sent_pre));
    write(log_file, temp, strlen(temp));
    write(log_file, sent_end, strlen(sent_end));
    write(log_file, compression_buf, BUFSIZE - client_to_server.avail_out);
    write(log_file, &crlf[1], 1);
}

int main(int argc, char **argv) {
    portNo = 0;
    int log_flag = 0;
    int port_flag = 0;
    int compress_flag = 0;
    log_file = -1;
    
    while(true){
        static struct option options[] = {
            {"port", required_argument, NULL, 'p'},
            {"log", required_argument, NULL, 'l'},
            {"compress", no_argument, NULL, 'c'},
            {0, 0, 0, 0}
        };
        int opt = getopt_long(argc, argv, "p:", options, NULL);
        if (opt == -1)
            break;
        switch (opt) {
            case 'p':
                portNo = atoi(optarg);
                port_flag = 1;
                break;
            case 'l':
                log_flag = 1;
                log_file = creat(optarg, 0666);
                if (log_file == -1) {
                    fprintf(stderr, "Failure to create/write to file.\n");
                }
                break;
            case 'c':
                compress_flag = true;
                break;
            default:
                fprintf(stderr, "ERROR! Unrecognized arguments.\n");
                exit(1);
        }
    }
    //set up compression session
    if (compress_flag) {
        //Setup compression
        int dret;
        dret = deflateInit(&client_to_server, Z_DEFAULT_COMPRESSION);
        client_to_server.zalloc = Z_NULL;
        client_to_server.zfree = Z_NULL;
        client_to_server.opaque = Z_NULL;
        if ( dret != Z_OK ) {
            fprintf(stderr, "ERROR! Failed to deflateInit. \n");
            exit(1);
        }
        
        server_to_client.zalloc = Z_NULL;
        server_to_client.zfree = Z_NULL;
        server_to_client.opaque = Z_NULL;
        dret = inflateInit(&server_to_client);
        if (dret != Z_OK ) {
            fprintf(stderr, "ERROR! Failed to inflateInit. \n");
            exit(1);
        }
        
    }
    
    if (!port_flag) {
        fprintf(stderr, "--port=NUM option is required.\n");
        exit(1);
    }
    
    //Set up terminal attributes
    set_terminal();
    // Build connection
    connect_server();

    //run
    
    int ret;

    struct pollfd file_descriptors[2] = {
        {STDIN_FILENO, POLLIN | POLLHUP | POLLERR, 0}, //standard input
        {sockfd, POLLIN | POLLHUP | POLLERR, 0} //server output
    };

    while (true) {
        ret = poll(file_descriptors, 2, 0);
        if (ret > 0) {
            //stdin
            //compression
            if (file_descriptors[0].revents & POLLIN) {
                char input[BUFSIZE];
                int bytes = read(STDIN_FILENO, &input, BUFSIZE);
                int i;
                for (i = 0; i < bytes; i++) {
                    if (input[i] == CR || input[i] == LF) {
                        write(STDOUT_FILENO, crlf, 2);
                    } else {
                        write(STDOUT_FILENO, (input + i), 1);
                    }
                }
                if (compress_flag) {
                    char compression_buf[BUFSIZE];
                    stdin_compress(bytes, input, compression_buf);
                    size = BUFSIZE - client_to_server.avail_out;
                    write(sockfd, compression_buf, size);
                    if (log_flag) {
                        char temp[20];
                        write_sent(size, temp, compression_buf);
                    }
                } else {
                    write(sockfd, input, bytes);
                    if (log_flag) {
                        char temp[20];
                        write_sent(bytes, temp, input);
                    }
                }
            } else if (file_descriptors[0].revents & POLLERR) {
                fprintf(stderr, "ERROR! Fail to poll with STDIN.\n");
                exit(1);
            }
            
            char receive_pre[20] = "RECEIVED ";
            char receive_end[20] = " bytes: ";

            //socket
            if (file_descriptors[1].revents & POLLIN) {
                char input[BUFSIZE];
                int bytes = read(sockfd, &input, BUFSIZE);
                
                if (bytes == 0) {
                    break;
                }
 
                if (log_flag) {
                    char temp[20];
                    sprintf(temp, "%d", bytes);
                    write(log_file, receive_pre, strlen(receive_pre));
                    write(log_file, temp, strlen(temp));
                    write(log_file, receive_end, strlen(receive_end));
                    write(log_file, input, bytes);
                    write(log_file, &crlf[1], 1);
                }
                
                if (!compress_flag) {
                    write(STDOUT_FILENO, input, bytes);
                 } else {
                    char decompression_buf[DEBUFSIZE];
                    socket_decompress(bytes, input, decompression_buf);
                    size = DEBUFSIZE - server_to_client.avail_out;
                    write(STDOUT_FILENO, decompression_buf, size);
                 }
                
                
                //done
            }
            else if ( POLLERR & file_descriptors[1].revents ||  POLLHUP & file_descriptors[1].revents) { //polling error
                exit(0);
            }
        }
    }

    if (compress_flag) {
        clean_up();
    }
    
    exit(0);
}
