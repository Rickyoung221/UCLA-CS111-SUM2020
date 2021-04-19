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

#define CR '\015' //carriage return
#define LF '\012' //line feed
#define ETX '\003' //^C (End of text)
#define EOT '\004' //^D (End of transmission)
#define BUFSIZE 256

struct termios old_mode;
char* filename;
int pipe_child[2];
int pipe_parent[2];
char crlf[2] = {CR, LF};
int pid;
int shell_flag;

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

void restore_terminal(void) {
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

// WHEN PID = 0
void run_child() {
    close(pipe_child[1]);
    close(pipe_parent[0]);

    //Replace STDIN, STDOUT, STDERR
    dup2(pipe_child[0], STDIN_FILENO);
    dup2(pipe_parent[1], STDOUT_FILENO);
    dup2(pipe_parent[1], STDERR_FILENO);

    close(pipe_child[0]);
    close(pipe_parent[1]);

    char* args[2] = {filename, NULL};
    if (execvp(filename, args) == -1) {
        fprintf(stderr, "ERROR! Failed to comile the shell file: %s\n", strerror(errno));
        exit(1);
    }
}

// WHEN PID > 0
void run_parent() {
    close(pipe_child[0]);
    close(pipe_parent[1]);

    struct pollfd file_descriptor[2] = {
        {STDIN_FILENO, POLLIN, 0},
        {pipe_parent[0], POLLIN, 0}
    };
    int ret;

    char buf[BUFSIZE];

    char lf[1] = {LF};
    char temp;
    int num;
    int EOT_flag = 0;
    while (!EOT_flag) {
        ret = poll(file_descriptor, 2, 0);
        if (ret == -1) {
            fprintf(stderr, "ERROR! Failed to poll: %s\n", strerror(errno));
        }

        if (file_descriptor[0].revents & POLLIN) {
            num = read(STDIN_FILENO, &buf, BUFSIZE); //keyboard input which is standard input
            if (num < 0) {
                fprintf(stderr, "ERROR! Failed reading from the keyboard input");
                exit(1);
            }
            int i;
            for (i = 0; i < num; i++) {
                temp = buf[i];
                switch(temp) {
                    case LF:
                    case CR:
                        write(STDOUT_FILENO, &crlf, 2);
                        write(pipe_child[1], &lf, 1);
                        break;
                    case ETX:
                        kill(pid, SIGINT);
                        break;
                    case EOT:
                        close(pipe_child[1]);
                        break;
                    default:
                        write(STDOUT_FILENO, &temp, 1);
                        write(pipe_child[1], &buf, 1);
                        break;
                }
            }
            memset(buf, 0, num);
        }

        if (file_descriptor[1].revents & POLLIN) {
            num = read(pipe_parent[0], &buf, BUFSIZE);
            if (num < 0) {
                fprintf(stderr, "Failed to read from shell output");
                exit(1);
            }
            int j;
            for(j = 0; j < num; j++) {
                temp = buf[j];
                switch(temp) {
                    case LF:
                        write(1, &crlf, 2);
                        break;
                    default:
                        write(1, &temp, 1);
                        break;
                }
            }
            memset(buf, 0, num);
        }
  
        else if (POLLHUP & file_descriptor[1].revents || POLLERR & file_descriptor[1].revents) {

            exit(0);
        }
    }
}


int main(int argc, char **argv) {
    int opt;
    shell_flag = 0;

    while(1) {
        static struct option options[] = {
            {"shell",  optional_argument,  NULL,  's'},
            {0, 0, 0, 0}
        };
        int i = 0;
        opt = getopt_long(argc, argv, "s:", options, &i);
        if (opt == -1)
        break;

        switch(opt) {
            case 's':
                shell_flag = 1;
                filename = optarg;
                break;
            default:
                fprintf(stderr, "Unrecognized argument, only --shell option is permitted.\n");
                exit(1);
        }
  }

  //Set terminal attributes
  set_terminal();

  //Shell option

  if (shell_flag) {
       //Create pipes
      if (pipe(pipe_child) != 0) {
          fprintf(stderr, "ERROR! Failed creating child pipe from terminal to shell: %s\n", strerror(errno));
          exit(1);
      }
      if (pipe(pipe_parent) != 0) {
          fprintf(stderr, "ERROR! Failed creating parent pipe from shell to terminal: %s\n", strerror(errno));
          exit(1);
      }
      
      //fork
      pid = fork();
      if (pid == 0){
          run_child();
      }
      else if (pid > 0){
          run_parent();
      }
      else {
          // Fork fail. Show error message
          fprintf(stderr, "Error while forking: %s\n", strerror(errno));
          exit(1);
      }
  }
    
    
    //NO Shell option
    char buf[BUFSIZE];

    int size;
    char temp;
    while ((size = read(0, &buf, BUFSIZE)) > 0) {
        int i = 0;
        for(i = 0; i < size; i++) {
            temp = buf[i];
            switch(temp) {
                case EOT:
                    exit(0);
                    break;
                case LF:
                case CR:
                    write(STDOUT_FILENO, &crlf, 2);
                    break;
                default:
                    write(STDOUT_FILENO, &temp, 1);
                    break;
            }
        }
        memset(buf, 0, size);
    }
    exit(0);
}
