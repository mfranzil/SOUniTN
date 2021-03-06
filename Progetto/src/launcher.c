#include "launcher.h"

pid_t shell_pid = -1;
int n_devices = 0;
int emergencyid;
int emergencyid2;
int shell_on = 0;

int main(int argc, char *argv[]) {

    /* Assegnazione Handler a relative funzioni */
    signal(SIGTERM, handle_sig);
    signal(SIGINT, handle_sigint);

    
    char(*buf)[MAX_BUF_SIZE];       /*buf: andrà a contenere la stringa dei comandi*/
    int cmd_n;                     /* numero di comandi disponibili */
    int device_pids[MAX_CHILDREN]; /* array contenenti i PID dei figli */
    char *name;                     /*Necessario per la stampa*/
    int j, i;

    key_t key; /*Chiave messagequeue per comunicazione tra shell e launcher*/
    int msgid;   /*msgid relativo*/

    key_t key_sh; /*Chiave messagequeue per comunicazione del pid di shell tra tutti i dspositivi*/
    int msgid_sh;  /*msgid relativo*/

    char c;
    name = get_shell_text();
    buf = malloc(MAX_BUF_SIZE * sizeof(char *)); /* array che conterrà i comandi da eseguire */

    for (j = 0; j < MAX_CHILDREN; j++) {
        /* Inizializzo l'array dei figli */
        device_pids[j] = -1; /* se è -1 non contiene nulla */
    }

    if (argc != 2 || strcmp(argv[1], "--no-clear") != 0) {
        /* Parametro opzionale */
        system("clear");
    }
    /* Creo message queue tra shell e launcher. */
    key = ftok("/tmp", 1000);
    msgid = msgget(key, 0666 | IPC_CREAT);
    /* Ripulisco inizialmente per evitare errori. */
    msgrcv(msgid, &message, sizeof(message), 1, IPC_NOWAIT);
    printf("Message launcher: %s\n", message.mesg_text);
    emergencyid = msgid;

    /*Creo message queue per comunicare shellpid */
    key_sh = ftok("/tmp", 2000);
    msgid_sh = msgget(key_sh, 0666 | IPC_CREAT);
    message.mesg_type = 1;
    emergencyid2 = msgid_sh;

    while (1) {
        /*Leggo il numero di devices presenti e i rispettivi id, solo se centralina creata (Ma anche se momentaneamente spenta). */
        if (shell_pid > 0) {
            read_msgqueue(msgid, device_pids);
        } else {
            n_devices = 0;
        }

        /* Stampa del nome utente */
        printf("\033[0;32m%s\033[0m:\033[0;34mLauncher\033[0m$ ", name);
        cmd_n = parse(buf, cmd_n);

        if (strcmp(buf[0], "exit") == 0) {
            /* supponiamo che l'utente scriva solo "exit" per uscire */
            /*Se la shell è aperta => shell_pid != -1 mando un SIGTERM per chiuderla. */
            c = '\0';
            printf("Sei sicuro di voler uscire dal launcher? Tutte le modifiche verranno perse!");

            while (1) {
                printf(" [s/n]: ");
                if (scanf(" %c", &c) != 1) {
                    printf("Errore durante la lettura.\n");
                    continue;
                }

                if (c == 's' || c == 'S') {
                    /*Eliminazione messagequeue verso shell. */
                    msgctl(msgid, IPC_RMID, NULL);
                    msgctl(msgid_sh, IPC_RMID, NULL);

                    free(buf);
                    if (shell_pid != -1) {
                        kill(shell_pid, SIGTERM);
                    }
                    return 0;
                } else if (c == 'n' || c == 'N') {
                    break;
                } else {
                    printf("Inserisci [s]ì o [n]o.\n");
                }
            }
        } else if (strcmp(buf[0], "\0") == 0) { /*a capo a vuoto */
            continue;
        } else if (strcmp(buf[0], "help") == 0) { /* guida */
            printf(HELP_STRING_LAUNCHER);
        } else if (strcmp(buf[0], "info") == 0) { /* info su dispositivo */
            if (cmd_n != 1) {
                printf(INFO_STRING);
            } else {
                /* Chiamata a metodo ad hoc*/
                info_launcher(buf, msgid, device_pids);
            }
        } else if (strcmp(buf[0], "switch") == 0) {
            if (cmd_n != 3) {
                printf(SWITCH_STRING);
            } else {
                /* Chiamata a metodo ad hoc*/
                switch_launcher(buf, msgid, device_pids);
            }
        } else if (strcmp(buf[0], "user") == 0) { /*I comandi da launcher fuorchè per help ed exit devono cominciare per user. */
            if (cmd_n != 3) {                     /*Controllo correttezza nel conteggio degli argomenti. */
                printf(USER_STRING);
            } else {
                /* Chiamata a metodo ad hoc*/
                user_launcher(buf, msgid, device_pids, msgid_sh);
            }
        } else { /* tutto i casi non considerati precedentemente */
            printf("Comando non riconosciuto. Usa help per visualizzare i comandi disponibili\n");
        }
    }

    /* to destroy the message queue */
    msgctl(msgid, IPC_RMID, NULL);
    msgctl(msgid_sh, IPC_RMID, NULL);
    free(buf);
    return 0;
}

/* Handler di SIGTERM, chiamato alla chiusura di Shell */
void handle_sig(int signal) {
    system("clear");
    printf("La centralina è stata chiusa, Premere Invio per proseguire\n");
    shell_pid = -1;
    shell_on = 0;
}

/* Handler SIGINT, cancella le 2 messagequeue e spegne la shell*/
void handle_sigint(int signal) {
    key_t key;
    int msgid;

    msgctl(emergencyid, IPC_RMID, NULL);
    msgctl(emergencyid2, IPC_RMID, NULL);

    if (shell_pid != -1) {
        kill(shell_pid, SIGTERM);
    }
    exit(0);
}

/* Funzione wrapper della chiamata a __switch_index di actions.c */
void switch_launcher(char buf[][MAX_BUF_SIZE], int msgid, int *device_pids) {
    if (shell_pid > 0 && shell_on) { /* Solo se centralina attiva */
        /* Lettura della messagequeue in caso alcuni dispositivi siano stati eliminati o spostati nel frattempo da shell*/
        read_msgqueue(msgid, device_pids);
        if (atoi(buf[1]) <= n_devices) {
            /* Chiamata a actions.c */
            __switch_index(atoi(buf[1]), buf[2], buf[3], device_pids);
        } else {
            printf("ID non presente\n");
        }
    } else {
        printf("Azione non disponibile a centralina spenta!\n");
    }
}

/* Funzione necessaria alla lettura della messagequeue tra shell e launcher in cui
 * sono comunicati i PID dei dispositivi presenti ed il numero di devices presenti */
void read_msgqueue(int msgid, int *device_pids) {
    int ret, q, __count, j;
    char n_dev_str[100];
    char tmp_buf[MAX_BUF_SIZE];
    char **vars;

    /* valore di ritorno della lettura se == -1 la lettura non è andata a buon fine e lo stato non varia */
    ret = msgrcv(msgid, &message, sizeof(message), 1, IPC_NOWAIT);
    if (ret != -1) {
        /* Cerco il primo parametro che dice il numero di devices presenti:
         * N.B.
         * Se un device è stato aggiunto e poi spostato questo comunque sara conteggiato dato che è stato in shell. */
        q = 0;
        while (!(message.mesg_text[q] == '|')) {
            /* Alla prima "|" ottengo il numero di devices */
            n_dev_str[q] = message.mesg_text[q];
            q++;
        }
        n_dev_str[q] = '\0';
        n_devices = atoi(n_dev_str);
        if (n_devices > 0) {
            /* se sono più d'uno splitto e mi salvo i relativi pids in device_pids */
            __count = n_devices;
            sprintf(tmp_buf, "%s", message.mesg_text);
            vars = NULL;
            /* chiamata a util.c che spezza n_valori separati da "|" */
            vars = split_fixed(tmp_buf, __count);
            j = 0;
            while (j <= __count) {
                if (j >= 1) {
                    device_pids[j - 1] = atoi(vars[j]);
                }
                j++;
            }
        }
    }
}

/* Funzione Wrapper che chiama __info di action */
void info_launcher(char buf[][MAX_BUF_SIZE], int msgid, int *device_pids) {
    if (shell_pid > 0 && shell_on) {
        /* Lettura della messagequeue in caso alcuni dispositivi siano stati eliminati o spostati nel frattempo da shell*/
        read_msgqueue(msgid, device_pids);
        /* Chiamata alla funzione __info di actions.c */
        __info(atoi(buf[1]), device_pids);
    } else {
        printf("La centralina è spenta\n");
    }
}


/* Gestisce comandi in input e crea la shell
 * buf: comandi,
 * msgid: id della messagqueue con shell,
 * device_pids: array dei pid
 * msgid_sh: msgid della coda in cui si passa il pid della shell */
void user_launcher(char buf[][MAX_BUF_SIZE], int msgid, int *device_pids, int msgid_sh) {
    pid_t pid;
    int ppid;
    char tmp[50] = "./bin/shell "; /* percorso e nome dell'eseguibile della shell */
    char stringpid[6]; /* stringa in cui sarà passato il pid di Launcher (che differisce dal ppid della shell)*/

    if (strcmp(buf[1], "turn") != 0 || strcmp(buf[2], "shell") != 0) {
        printf("Sintassi: user turn shell <pos>\n");
    }

    if (strcmp(buf[3], "on") == 0 && shell_pid == -1) { /*Se non è ancora accesa => shell_pid == -1 */
        pid = fork();
        if (pid < 0) {
            printf("Errore durante il fork.\n");
            exit(1);
        }

        /* Pulizia coda per shellpid nel caso vi fossero messaggi latenti */
        msgrcv(msgid_sh, &message, sizeof(message), 1, IPC_NOWAIT);

        if (pid == 0) { /*Processo figlio che aprirà terminale e lancerà la shell. */
            /*Sarà passato per argomento alla shell. */
            ppid = (int)getppid();

            /*Eseguibili sono in bin apro terminale parallelo. */
            sprintf(stringpid, "%d", ppid);
            strcat(tmp, stringpid);
            /* N.B. Opzione -e garantisce compatibilità con Ubuntu 16.04 */
            if (execl("/usr/bin/gnome-terminal", "gnome-terminal", "-e", tmp, NULL) == -1) {
                /* In caso di errori in apertura scriverò errore nella messagequeue */
                sprintf(message.mesg_text, "%s", "Errore");
                msgsnd(msgid_sh, &message, MAX_BUF_SIZE, 0);
            }
        } else if (pid > 0) {
            /*Legge il contenuto della messagequeue => Se = "Errore" la finestra è stata aperta. */
            msgrcv(msgid_sh, &message, sizeof(message), 1, 0); /* 0 => Sincronizzata */
            if (strcmp(message.mesg_text, "Errore") == 0) {
                printf("Errore nell'apertura della shell. Codice di errore: %d\n", errno);
            } else {
                /* se il messaggio non è d'errore */
                shell_pid = atoi(message.mesg_text);
                /* mando nuovamente il messaggio sulla queue dedicata al passaggio di shellpid */
                sprintf(message.mesg_text, "%d", shell_pid);
                shell_on = 1;
                message.mesg_type = 1;
                msgsnd(msgid_sh, &message, MAX_BUF_SIZE, 0);
            }
            system("clear");
            printf("Centralina aperta.\n");
            return;
        }
    } else if (strcmp(buf[3], "off") == 0 && shell_pid != -1) { /* != -1 => è stata aperta */
        if (shell_on) { /* Se la centralina è accesa */
            kill(shell_pid, SIGUSR1); /* Manderà un SIGINT, che bloccherà la shell, non permettendo alcun comando su di essa */
            shell_on = 0; /* setta l'intero che segnala la situazione a 0 */
        } else {
            printf("Centralina già spenta.\n");
        }
        return;
    } else if (strcmp(buf[3], "on") == 0 && shell_pid != -1) {  /* != -1 => è stata aperta */
        if (shell_on == 0) {
            kill(shell_pid, SIGUSR1); /* Accende la shell se spenta */
            shell_on = 1;
        } else {
            printf("Centralina già accesa.\n");
        }
    } else if (strcmp(buf[3], "off") == 0 && shell_pid == -1) {  /* Cerca di spegnere una centralina mai stata accesa */
        printf("Centralina già spenta.\n");
    } else {
        printf("Comando non riconosciuto.\n");
    }
}
