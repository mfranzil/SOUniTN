#include "../util.h"

/* BULB = 1 */

int shellpid;

/* Registri della lampadina */
int fd;           /* file descriptor della pipe verso il padre */
int pid, __index; /* variabili di stato */
int status = 0;   /* interruttore accensione */
time_t start, time_on;
int changed = 1;

key_t key;
int msgid;

volatile int flag_usr1 = 0;
volatile int flag_usr2 = 0;
volatile int flag_term = 0;

void sighandler_int(int sig) {
    if (sig == SIGUSR1) {
        flag_usr1 = 1;
    }
    if (sig == SIGUSR2) {
        flag_usr2 = 1;
    }
    if (sig == SIGTERM) {
        flag_term = 1;
    }
}

int main(int argc, char* argv[]) {
    /* argv = [./bulb, indice, /tmp/indice]; */
    char tmp[MAX_BUF_SIZE]; /* Buffer per le pipe*/
    /*char ppid_pipe[MAX_BUF_SIZE];*/
    char* this_pipe = NULL; /* nome della pipe */

    char** vars = NULL;
    /*int ppid, ppid_pipe_fd; */

    this_pipe = argv[2];
    pid = getpid();
    __index = atoi(argv[1]);
    fd = open(this_pipe, O_RDWR);

    shellpid = get_shell_pid();

    signal(SIGTERM, sighandler_int);
    signal(SIGUSR1, sighandler_int);
    signal(SIGUSR2, sighandler_int);

    key = ftok("/tmp/ipc/mqueues", pid);
    msgid = msgget(key, 0666 | IPC_CREAT);

    while (1) {
        if (flag_usr1) {
            flag_usr1 = 0;
            /*printf("bulb usr1: %d - %d\n", pid, flag_usr1);*/
            if (status) {
                time_on = (time(NULL) - start);
            } else {
                time_on = 0;
            }

            sprintf(tmp, "1|%d|%d|%d|%d",
                    pid, __index, status, (int)time_on);
            message.mesg_type = 1;
            sprintf(message.mesg_text, "%s", tmp);
            /*printf("Sending Message...\n");*/
            /*int rc = */
            msgsnd(msgid, &message, sizeof(message), 0);
            /*printf("Message Sent: %s\n", message.mesg_text);*/

            /*write(fd, tmp, MAX_BUF_SIZE);*/
        }
        if (flag_usr2) {
            flag_usr2 = 0;
            /* La finestra apre la pipe in lettura e ottiene cosa deve fare. */
            /* 0|... -> accendi/spegni lampadina */

            /*printf("ho ricevuto un messaggio pid: %d\n", pid); */

            read(fd, tmp, MAX_BUF_SIZE);
            vars = split_fixed(tmp, 2);
            if (atoi(vars[0]) == 0) {
                if (!status) {
                    status = 1;
                    start = time(NULL);
                } else {
                    status = 0;
                    start = 0;
                }
            }
        }
        if (flag_term) {

            int ret = msgrcv(msgid, &message, sizeof(message), 1, IPC_NOWAIT);
            if(ret !=-1 ){
                if(strcmp(message.mesg_text, "SENDPID")==0){
                    printf("MANDO MESSAGGIO\n");
                    int ppid = (int)getppid();
                    if(ppid != shellpid){
                        int key_ppid = ftok("/tmp/ipc/mqueues", ppid);
                        int msgid_ppid = msgget(key_ppid, 0666 | IPC_CREAT);
                        message.mesg_type = 1;
                        sprintf(message.mesg_text, "%d", pid);
                        msgsnd(msgid_ppid, &message, sizeof(message), 0);
                        kill(ppid, SIGCONT);
                    }
                }
            }

            msgctl(msgid, IPC_RMID, NULL);
            exit(0);
        }
        sleep(10);
    }

    return 0;
}
