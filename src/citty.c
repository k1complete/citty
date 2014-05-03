#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <util.h>
#include <signal.h>
#include <regex.h>
#include <string.h>
#include <errno.h>
#include "config.h"
static int sigchilded = 0;

int loop(int master, regex_t *preg) 
{
        char buf[1000];
        char inbuf[1000];
        fd_set fdrs;
        fd_set fdws;
        fd_set fdes;
        int prompt = 0;
        int buflen = 0;
        while (1) {
                memset(buf, 0, sizeof(buf));
                memset(inbuf, 0, sizeof(inbuf));
                FD_ZERO(&fdrs);
                FD_ZERO(&fdws);
                FD_ZERO(&fdes);
                FD_SET(master, &fdws);
                FD_SET(master, &fdrs);
                FD_SET(STDIN_FILENO, &fdrs);
                FD_SET(master, &fdws);
                FD_SET(master, &fdes);
                if (select(FD_SETSIZE, &fdrs, &fdws, &fdes, NULL) < 0) {
                        perror("select");
                        return -1;
                }
                if (sigchilded) {
                        return 0;
                }
                if (prompt) {
                        if (FD_ISSET(STDIN_FILENO, &fdrs)) {
                                if (feof(stdin)) {
                                        printf("eof\n");
                                        return -1;
                                }
                                if (fgets(inbuf, sizeof(inbuf), stdin)) {
                                        write(master, inbuf, strlen(inbuf));
                                        prompt = 0;
                                } else {
                                        return -1;
                                }
                        }
                } else {
                        if (FD_ISSET(master, &fdrs) || 
                            FD_ISSET(master, &fdes)) {
                                if ((buflen = read(master, buf, sizeof(buf))) < 0) {
                                        perror("read");
                                        return -1;
                                }
                                write(STDOUT_FILENO, buf, buflen);
                                if (regexec(preg, buf, 0, NULL, 0) == 0) {
                                        prompt = 1;
                                }
                        }
                }
        }
        return 0;
}
int usage(const char *desc)
{
        printf("usage: %s %s\n", PACKAGE_NAME, desc);
        printf("-p regexp : prompt regexp(default '\\$ $' for bash)\n");
        return 0;
}
void sigchild(int no)
{
        sigchilded = 1;
}
int main(int argc, char * const argv[], char * const env[])
{
        int master;
        pid_t pid;
        int longindex = 0;
        char *optstring = "+p:";
        char *optdef = "-p prompt command [args...]";
        regex_t preg;
        int r = 0;
        char *const *av;
        char *cust_prompt = NULL;
        char *prompt = "\\$ $";
        int regerr;
        char errbuf[BUFSIZ];
        struct sigaction  sigact;
        memset(&sigact, 0, sizeof(sigact));
        sigact.sa_handler = sigchild;
        if (sigaction(SIGCHLD, &sigact, NULL)) {
                perror("sigaction");
                exit(EXIT_FAILURE);
        }
        while ((r = getopt(argc, argv, optstring)) > 0) {
                switch (r) {
                case 'p':
                        cust_prompt = strdup(optarg);
                        break;
                default:
                        printf("%d\n", r);
                }
        }
        if (cust_prompt) {
                prompt = cust_prompt;
        }
        if ((regerr = regcomp(&preg, prompt, 0))) {
                regerror(regerr, &preg, errbuf, sizeof(errbuf));
                fprintf(stderr, "regcomp: %s\n", errbuf);
                exit(EXIT_FAILURE);
        }
        free(cust_prompt);
        if (optind == argc) {
                usage(optdef);
                exit(EXIT_FAILURE);
        }
        pid = forkpty(&master, NULL, NULL, NULL);
        if (pid < 0) {
                perror("forkpty");
                exit(EXIT_FAILURE);
        }
        if (pid == 0) { /* child */
                av = &argv[optind];
                if (execvp(av[0], av) < 0) {
                        perror("execpve");
                        exit(1);
                }
        }
        loop(master, &preg);
        regfree(&preg);
        close(master);
               
        kill(pid, SIGTERM);
        exit(EXIT_SUCCESS);
}
