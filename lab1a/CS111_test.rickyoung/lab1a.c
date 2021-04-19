// NAME: Weikeng Yang
// EMAIL: weikengyang@gmail.com
// ID: 405346443

#include <stdio.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <getopt.h>
#include <poll.h>
#include <signal.h>
#include <sys/wait.h>
#define CTRL_D 0x04
#define CTRL_C 0x03
#define KEYBUF_SIZE 64
#define SHELLBUF_SIZE 512

const char* progName = "lab1a";
void killProg(const char* errMsg);
void dlog(const char* msg);

// separate (global) termios struct to restore in atexit()
struct termios initSettings;
void restoreAttr();

int main(int argc, char** argv) {
    // first get the current termios settings
    if (tcgetattr(STDIN_FILENO, &initSettings) == -1) {
        killProg("Error getting initial termios attributes");
    }
    if (atexit(&restoreAttr) != 0) {
        killProg("Unable to register atexit() function");
    }

    struct termios newSettings = initSettings;
    newSettings.c_iflag = ISTRIP;
    newSettings.c_oflag = 0;
    newSettings.c_lflag = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &newSettings);

    int useShell = 0;
    char* shell;
    int debug = 0;

    struct option options[] = {
            {"shell", optional_argument, NULL, 's'},
            {"debug", no_argument, NULL, 'd'},
            {0,0,0,0}
    };

    int optIndex;
    int ch = getopt_long(argc, argv, "s::d", options, &optIndex);
    while (ch != -1) {
        switch (ch) {
            case 's':
                useShell = 1;
                shell = (optarg == 0) ? "/bin/bash" : optarg;
                break;
            case 'd':
                debug = 1;
                break;
            default:
                killProg("Unrecognized option argument");
        }

        ch = getopt_long(argc, argv, "s::d", options, &optIndex);
    }

    int pipe1[2];
    int pipe2[2];
    // dummy values to shut my IDE up
    int pRead = -1;
    int pWrite = -1;
    int pid = -1;
    if (useShell) {
        if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
            killProg("Error creating pipe in parent process");
        }

        pid = fork();
        if (pid < 0) {
            killProg("Error in forking into two processes");
        }
        else if (pid == 0) {
            // child process
            // read through pipe2[0], write through pipe1[1]
            if (close(pipe1[0]) == -1 || close(pipe2[1]) == -1) {
                killProg("Error closing pipes in child process");
            }
            // easier names, pipe-read and pipe-write
            int childRead = pipe2[0];
            int childWrite = pipe1[1];

            // make the read end of the pipe be the shell's standard input
            if (dup2(childRead, STDIN_FILENO) == -1 || close(childRead) == -1) {
                killProg("Error redirecting pipe to stdin in child process");
            }
            // make stderr/stdout of the shell go to the write end
            if (dup2(childWrite, STDOUT_FILENO) == -1) {
                killProg("Error redirecting stdout to pipe in child process");
            }
            if (dup2(childWrite, STDERR_FILENO) == -1 || close(childWrite) == -1) {
                killProg("Error redirecting stderr to pipe in child process");
            }
            if (execl("/bin/bash", "/bin/bash", (char *) NULL) == -1) {
                fprintf(stderr, "%s: Error executing shell %s: %s", progName, shell, strerror(errno));
                exit(1);
            }
        }
        else {
            // parent process
            // read through pipe1[0], write through pipe2[1]
            if (close(pipe2[0]) == -1 || close(pipe1[1]) == -1) {
                killProg("Error closing pipes in parent process");
            }
            // if a shell is specified, pRead will read from the shell and pWrite will write to the shell.
            pRead = pipe1[0];
            pWrite = pipe2[1];
        }
    }

    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    int nfds = 1;
    if (useShell) {
        fds[1].fd = pRead;
        fds[1].events = POLLIN;
        fds[1].revents = 0;
        nfds++;
    }

    while (1) {
        int ret = poll(fds, nfds, 0);
        if (ret == -1) {
            killProg("Error in poll() syscall");
        }

        // deal with keyboard input
        if (fds[0].revents & POLLERR) {
            killProg("Received POLLERR from poll() call on keyboard");
        }
        if (fds[0].revents & POLLIN) {
            // got input from keyboard, deal with it
            char keyBuf[KEYBUF_SIZE];
            int charsRead = read(STDIN_FILENO, keyBuf, KEYBUF_SIZE);
            if (charsRead == -1) {
                killProg("Error in read() call on keyboard");
            }

            // process all chars read from keyBuf
            for (int i = 0; i < charsRead; i++) {
                char curChar = keyBuf[i];
                // output <lr> or <cr> as <cr><lf>
                if (curChar == '\r' || curChar == '\n') {
                    if (write(STDOUT_FILENO, "\r\n", 2) == -1) {
                        killProg("Error writing \\r\\n to stdout from keyboard");
                    }
                    if (useShell) {
                        if (debug) {
                            dlog("char to shell");
                        }
                        if (write(pWrite, "\n", 1) == -1) {
                            killProg("Error writing \\n to shell");
                        }
                    }
                }
                else if (curChar == CTRL_D) {
                    if (debug) {
                        fprintf(stderr, "^D");
                    }

                    if (useShell) {
                        // sends EOF to the terminal
                        if (close(pWrite) == -1) {
                            killProg("Unable to close pipe from keyboard to shell");
                        }
                    }
                    else {
                        exit(0);
                    }
                }
                else if (curChar == CTRL_C) {
                    if (debug) {
                        fprintf(stderr, "^C");
                    }
                    if (useShell) {
                        int killRet = kill(pid, SIGINT);
                        if (killRet == -1) {
                            killProg("Error sending SIGINT to shell");
                        }
                        if (debug) {
                            dlog("SIGINT sent to shell");
                        }
                        break;
                    }
                }
                else {
                    if (write(STDOUT_FILENO, keyBuf + i, 1) == -1) {
                        killProg("Error writing character to stdout from keyboard");
                    }
                    if (useShell) {
                        if (debug) {
                            dlog("char to shell");
                        }
                        if (write(pWrite, keyBuf + i, 1) == -1) {
                            killProg("Error writing character to shell");
                        }
                    }
                }
            }
        }

        // deal with output from shell process
        if (useShell) {
            if (fds[1].revents & POLLERR) {
                killProg("Received POLLERR from poll() call on shell");
            }
            if (fds[1].revents & POLLIN) {
                char shellBuf[SHELLBUF_SIZE];
                int charsRead = read(pRead, shellBuf, SHELLBUF_SIZE);
                if (charsRead == -1) {
                    killProg("Error in read() call on shell");
                }
                for (int i = 0; i < charsRead; i++) {
                    char curChar = shellBuf[i];
                    // output <lf> as <cr><lf>
                    if (curChar == '\n') {
                        if (write(STDOUT_FILENO, "\r\n", 2) == -1) {
                            killProg("Error writing \\r\\n to stdout from shell process");
                        }
                    }
                    else {
                        if (write(STDOUT_FILENO, shellBuf + i, 1) == -1) {
                            killProg("Error writing character to stdout from shell process");
                        }
                    }
                }
            }
            // shell has hung up its end
            if (fds[1].revents & POLLHUP) {
                if (debug) {
                    dlog("POLLHUP from shell");
                }

                int status;
                int waitRet = waitpid(pid, &status, 0);
                if (waitRet == -1) {
                    killProg("Error in waitpid() for shell");
                }
                int sig = status & 0x7f;
                int stat = status >> 8;
                fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", sig, stat);
                break;
            }
        }
        fds[0].revents = 0;
        fds[1].revents = 0;
    }

    return 0;
}

void killProg(const char* errMsg) {
    fprintf(stderr, "%s: %s: %s", progName, errMsg, strerror(errno));
    exit(1);
}

void dlog(const char* msg) {
    fprintf(stderr, "%s", msg);
}

void restoreAttr() {
    if (tcsetattr(STDIN_FILENO, TCSANOW, &initSettings) == -1) {
        killProg("Unable to restore initial termios settings");
    }
}
