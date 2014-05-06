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

int unescape_meta(char *src, char **dest, int len) 
{
        int i;
        *dest = malloc(strlen(src) * 2 + 1 + len);
        for (i = 0; src[i]; i++) {
                (*dest)[i] = '\\';
                (*dest)[i+1] = src[i];
        }
        return 0;
}

int regrecomp_stdin_nchar(regex_t *preg, int feedn)
{
        char *pbuf = NULL;
        size_t pbuflen = feedn + 1;
        int readed = 0;
        char *pattern = NULL;
        int regerr;
        char errbuf[BUFSIZ];
        char fmtbuf[BUFSIZ];

        if ((pbuf = malloc(pbuflen)) == NULL) {
                perror("malloc");
                return -1;
        }
        memset(pbuf, 0, pbuflen);
        snprintf(fmtbuf, sizeof(fmtbuf), "%%%ds ", feedn);
        fscanf(stdin, fmtbuf, pbuf);
        /*        
                if ((readed != fread(pbuf, 1, feedn, stdin)) == feedn) {
                perror("fread");
                return -1;
                }
        */
        regfree(preg);
        unescape_meta(pbuf, &pattern, 2);
        free(pbuf);
        strcat(pattern, " $");
        /*        printf("prompt regex [%s]\n", pbuf);  */
        if ((regerr = regcomp(preg, pattern, 0))) {
                regerror(regerr, preg, errbuf, sizeof(errbuf));
                return -1;
        }
        free(pattern);
        return 0;
}
int loop(int master, regex_t *preg, int feedn) 
{
        char buf[1000];
        char inbuf[1000];
        int buflen;
        char *promptbuf = NULL;
        fd_set fdrs;
        fd_set fdws;
        fd_set fdes;
        int prompt = 0;
        int change_prompt = 1;

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
                                        change_prompt = 1;
                                } else {
                                        return -1;
                                }
                        }
                } else {
                        if (change_prompt && 
                            FD_ISSET(STDIN_FILENO, &fdrs) && feedn > 0) {
                                if (regrecomp_stdin_nchar(preg, feedn)) {
                                        return -1;
                                }
                                change_prompt = 0;
                        }
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
        printf("-f len : len chars ahead each line for prompt )\n");
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
        char *optstring = "+p:f:";
        char *optdef = "[-p prompt | -f n]command [args...]";
        regex_t preg;
        int r = 0;
        char *const *av;
        char *cust_prompt = NULL;
        char *prompt = "\\$ $";
        int regerr;
        char errbuf[BUFSIZ];
        int feedn = 0;
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
                case 'f':
                        feedn = atoi(optarg);
                        printf("aaa %d\n", feedn);
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
        prompt = NULL;
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
                        perror("execpv");
                        exit(1);
                }
        }
        loop(master, &preg, feedn);
        regfree(&preg);
        kill(pid, SIGTERM);
        close(master);
        exit(EXIT_SUCCESS);
}
