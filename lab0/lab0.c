//NAME: Weikeng Yang
//EMAIL: weikengyang@gmail.com
//ID: 405346443

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#define INPUT 'i'
#define OUTPUT 'o'
#define SEGFAULT 's'
#define CATCH 'c'

void handle_seg() {
    fprintf(stderr, "Scaught and received SIGSEGV. \n");
    exit(4);
}
void segFault(){
    char* ptr = NULL;
    *ptr = 'a';
    return;
}
// Reference: File Descriptor Management from course material
void redirect_input(char *file){
    int ifd = open(optarg, O_RDONLY);    //Return a file descriptor ; O_RDONLY: Open for reading only.
    if (ifd >= 0)
        //open the file
    {
        close(0);
        dup(ifd);
        close(ifd);
    }
    else
    {
        fprintf(stderr, "INPUT ERROR. Unable to open input file %s: \n", file);
        fprintf(stderr, "%s\n", strerror(errno));
        exit(2);
    }
}
void redirect_output(char *file){
    int ofd = creat(optarg, 0666);
    if (ofd >= 0)
    {
        close(1);
        dup(ofd);
        close(ofd);
    }
    else
    {
        fprintf(stderr, "OUTPUT ERROR. Unable to create output file %s: \n", file);
        fprintf(stderr, "%s\n", strerror(errno));
        exit(3);
    }
}


int main(int argc, char *argv[])
{
    int c;
    while(1){
    int opt_index;
    
    static struct option long_options[] =
    {
        {"input", required_argument,NULL, INPUT},
        {"output", required_argument,NULL, OUTPUT},
        {"segfault", no_argument, NULL, SEGFAULT},
        {"catch", no_argument, NULL, CATCH},
        {0, 0, 0, 0}
    };
    

    c = getopt_long(argc, argv, "i:o:sc", long_options, &opt_index);
    if (c==-1)
        break;
    switch (c){
        case INPUT:
            redirect_input(optarg);
            break;
        case OUTPUT:
            redirect_output(optarg);
            break;
        case SEGFAULT:
            segFault();
            break;
        case CATCH:
            signal(SIGSEGV, handle_seg);
            break;
        default:
            fprintf(stderr, " ERROR: unrecognized option argument.\n");
            exit(1);
        }
    }
    
        const int COUNT = 32;
        char* buffer = (char*) malloc(COUNT);

        ssize_t rd = read(STDIN_FILENO, buffer, COUNT);
        while (rd > 0)
        {
            ssize_t wd = write(STDOUT_FILENO, buffer, rd);
            if (wd < 0)
            {
                fprintf(stderr, "Error writing from output file: %s\n", strerror(errno));
                exit(3);
            }
            rd = read(STDIN_FILENO, buffer, COUNT);
        }

        if (rd < 0)
        {
            fprintf(stderr, "Errror reading from input file: %s\n", strerror(errno));
            exit(2);
        }
        free(buffer);
        exit(0);
    
}
