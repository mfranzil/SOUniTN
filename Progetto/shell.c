#include "shell.h"
#include "util.h"

// CENTRALINA = 0
// BULB = 1
// FRIDGE = 2
// WINDOW = 3

int main(int argc, char *argv[]) {
    char(*buf)[MAX_BUF_SIZE] = malloc(MAX_BUF_SIZE * sizeof(char *));  // array che conterrà i comandi da eseguire

    char ch;   // carattere usato per la lettura dei comandi
    int ch_i;  // indice del carattere corrente

    int cmd_n;  // numero di comandi disponibili

    int j;

    int device_i = 0;        // indice progressivo dei dispositivi
    int children_pids[100];  // array contenenti i PID dei figli

    for (j = 0; j < MAX_CHILDREN; j++) {
        children_pids[j] = -1;  // se è -1 non contiene nulla
    }

    system("clear");
    char *name = getUserName();

    /*//PID del launcher
    int ppid = atoi(argv[1]);*/

    //Creo FIFO da shell a launcher.
   /* char *shpm = "/tmp/myshpm";
    mkfifo(shpm, 0666);
    int fd = open(shpm, O_WRONLY);

    //Scrivo il pid della centralina, dato che non è figlia diretta di program manager, sulla pipe.
    char str[16];
    sprintf(str, "%d", (int)getpid());
    write(fd, str, 16);
    close(fd);*/
    ////FINE MODIFICAAAAAAAAAAAAAAAAAAAAAAA

    while (1) {
        printf("\e[92m%s\e[39m:\e[31mCentralina\033[0m$ ", name);

        // Parser dei comandi
        ch = ' ';
        ch_i = -1;
        cmd_n = 0;
        while (ch != EOF && ch != '\n') {
            ch = getchar();
            if (ch == ' ') {
                buf[cmd_n++][++ch_i] = '\0';
                ch_i = -1;
            } else {
                buf[cmd_n][++ch_i] = ch;
            }
        }
        buf[cmd_n][ch_i] = '\0';

        //   for (int k = cmd_n; k >= 0; k--) {
        //       printf(buf[k]);
        //   }

        if (strcmp(buf[0], "exit") == 0) {  // supponiamo che l'utente scriva solo "exit" per uscire
            break;
        } else if (strcmp(buf[0], "\0") == 0) {  // a capo a vuoto
            //return; Hai scritto Return, credo sia un continue, sbaglio?
            continue;
        } else if (strcmp(buf[0], "help") == 0) {  // guida
            printf("%s", HELP_STRING);
        } else if (strcmp(buf[0], "list") == 0) {
            printf("Info su list\n");
        } else if (strcmp(buf[0], "info") == 0) {
            if (cmd_n != 1) {
                printf("Sintassi: info <device>\n");
            } else {
                info(buf, children_pids);
            }
        } else if (strcmp(buf[0], "switch") == 0) {
            if (cmd_n != 3) {
                printf(SWITCH_STRING);
            } else {
                __switch(buf, children_pids);
            }
        } else if (strcmp(buf[0], "add") == 0) {
            if (cmd_n != 1) {
                printf(ADD_STRING);
            } else {
                add(buf, &device_i, children_pids);
            }
        } else if (strcmp(buf[0], "restart") == 0) {
            char *const args[] = {NULL};
            int pid = fork();
            if (pid == 0) {
                execvp("make", args);
                exit(0);
            } else {
                wait(NULL);
                execvp("./shell", args);
            }
            exit(0);
        }
        else {  //tutto il resto
            printf("Comando non riconosciuto. Usa help per visualizzare i comandi disponibili\n");
        }
    }
    free(buf);
    return 0;
}

char *pipename(int pid) {
    char *pipe_str = malloc(4 * sizeof(char));
    sprintf(pipe_str, "/tmp/%i", pid);
    return pipe_str;
}

int get_by_index(int in, int *children_pids) {
    if (in >= MAX_CHILDREN || in < 0) return -1;
    return children_pids[in] == -1 ? -1 : children_pids[in];
}

void info(char buf[][MAX_BUF_SIZE], int *children_pids) {
    int pid = get_by_index(atoi(buf[1]), children_pids);

    if (pid == -1) {
        printf("Errore! Non esiste questo dispositivo.\n");
        return;
    }

    char *pipe_str = pipename(pid);
    char tmp[MAX_BUF_SIZE];  // dove ci piazzo l'output della pipe

    // apertura della pipe fallita
    if (kill(pid, SIGUSR1) != 0) {
        printf("Errore! Sistema: codice errore %i\n", errno);
        return;
    }

    int fd = open(pipe_str, O_RDONLY);
    read(fd, tmp, MAX_BUF_SIZE);
    close(fd);

    if (strncmp(tmp, "1", 1) == 0) {  // Lampadina
        char **vars = split(tmp, 5);
        // parametri: tipo, stato, tempo di accensione, pid, indice

        printf("Oggetto: Lampadina\nStato: %s\nTempo di accensione: %s\nPID: %s\nIndice: %s\n",
               atoi(vars[1]) ? "ON" : "OFF", vars[2], vars[3], vars[4]);
        free(vars);
    } else if (strncmp(tmp, "2", 1) == 0) {  // Frigo
        char **vars = split(tmp, 9);
        // parametri: tipo, stato, tempo di apertura, pid, indice, delay
        // percentuale riempimento, temperatura interna

        printf("Oggetto: Frigorifero\n");

        if (vars[8] != NULL) {
            printf("[!!] Messaggio di log: <%s>\n", vars[8]);
        }
        printf("Stato: %s\nTempo di apertura: %s sec\nPID: %s\nIndice: %s\n",
               atoi(vars[1]) ? "Aperto" : "Chiuso", vars[2], vars[3], vars[4]);
        printf("Delay richiusura: %s sec\nPercentuale riempimento: %s\nTemperatura: %s°C\n",
               vars[5], vars[6], vars[7]);
        free(vars);
    } else if (strncmp(tmp, "3", 1) == 0) {  // Finestra
        char **vars = split(tmp, 5);

        // parametri: tipo, stato, tempo di accensione, pid, indice
        printf("Oggetto: Finestra\nStato: %s\nTempo di apertura: %s sec\nPID: %s\nIndice: %s\n",
               atoi(vars[1]) ? "Aperto" : "Chiuso", vars[2], vars[3], vars[4]);
        free(vars);
    } else {
        printf("Dispositivo non supportato.\n");
    }

    free(pipe_str);
}

void __switch(char buf[][MAX_BUF_SIZE], int *children_pids) {
    int pid = get_by_index(atoi(buf[1]), children_pids);

    if (pid == -1) {
        printf("Errore! Non esiste questo dispositivo.\n");
        return;
    }

    char *pipe_str = pipename(pid);
    char tmp[MAX_BUF_SIZE];  // dove ci piazzo l'output della pipe

    // apertura della pipe fallita
    if (kill(pid, SIGUSR1) != 0) {
        printf("Errore! Impossibile notificare il dispositivo. Errno: %i\n", errno);
        return;
    }

    int fd = open(pipe_str, O_RDWR);
    read(fd, tmp, MAX_BUF_SIZE);

    if (strncmp(tmp, "1", 1) == 0) {  // Lampadina
    close(fd);

        if (strcmp(buf[2], "accensione") != 0) {
            printf("Operazione non permessa su una lampadina!\nOperazioni permesse: accensione\n");
            return;
        }

        char **vars = split(tmp, 5);  // parametri: tipo, stato, tempo di accensione, pid, indice
        int status = atoi(vars[1]);
        if (strcmp(buf[3], "on") == 0 && status == 0) {
            kill(pid, SIGUSR2);
            printf("Lampadina accesa.\n");
        } else if (strcmp(buf[3], "off") == 0 && status == 1) {
            kill(pid, SIGUSR2);
            printf("Lampadina spenta.\n");
        } else if (strcmp(buf[3], "off") == 0 && status == 0) {  // Spengo una lampadina spenta
            printf("Stai provando a spegnere una lampadina spenta!\n");
        } else if (strcmp(buf[3], "on") == 0 && status == 1) {  // Spengo una lampadina accesa
            printf("Stai provando a accendere una lampadina accesa!\n");
        } else {
            printf("Sintassi non corretta. Sintassi: switch <bulb> accensione <on/off>\n");
        }
        free(vars);
    } else if (strncmp(tmp, "2", 1) == 0) {  // Fridge
        if (strcmp(buf[2], "apertura") == 0) {
            char **vars = split(tmp, 8);  // parametri: tipo, stato, tempo di accensione, pid, indice
            int status = atoi(vars[1]);

            printf("%d", fd);
            fflush(stdout);

            char* tmp2 = "0|0";
            int out = write(fd, tmp2, sizeof(tmp2));
            close(fd);

            if (strcmp(buf[3], "on") == 0 && status == 0) {
                kill(pid, SIGUSR2);
                printf("Frigorifero aperto.\n");
            } else if (strcmp(buf[3], "off") == 0 && status == 1) {
                kill(pid, SIGUSR2);
                printf("Frigorifero chiuso.\n");
            } else if (strcmp(buf[3], "off") == 0 && status == 0) {  // Chiudo frigo già chiuso
                printf("Stai provando a chiudere un frigorigero già chiuso.\n");
            } else if (strcmp(buf[3], "on") == 0 && status == 1) {  // Apro frigo già aperto
                printf("Stai provando a aprire un frigorifero già aperto.\n");
            } else {
                printf("Sintassi non corretta. Sintassi: switch <fridge> apertura <on/off>\n");
            }

            free(vars);
        } else if (strcmp(buf[2], "temperatura") == 0) {
    close(fd);

        } else {
    close(fd);

            printf("Operazione non permessa su un frigorifero! Operazioni permesse: <temperatura/apertura>");
        }

        
    } else if (strncmp(tmp, "3", 1) == 0) {  // Window
    close(fd);

        if (((strcmp(buf[2], "apertura") != 0) || (strcmp(buf[2], "apertura") == 0 && strcmp(buf[3], "off") == 0)) &&
            ((strcmp(buf[2], "chiusura") != 0) || (strcmp(buf[2], "chiusura") == 0 && strcmp(buf[3], "off") == 0))) {
            printf("Operazione non permessa: i pulsanti sono solo attivi!\n");
            return;
        }
        // se off non permetto
        char **vars = split(tmp, 5);  // parametri: tipo, stato, tempo di accensione, pid, indice
        int status = atoi(vars[1]);
        if (strcmp(buf[2], "apertura") == 0 && status == 0) {
            kill(pid, SIGUSR2);
            printf("Finestra aperta.\n");
        } else if (strcmp(buf[2], "chiusura") == 0 && status == 1) {
            kill(pid, SIGUSR2);
            printf("Finestra chiusa.\n");
        } else {
            printf("Operazione non permessa: pulsante già premuto.\n");
        }
        free(vars);

    } else {  // tutti gli altri dispositivi
        printf("Dispositivo non supportato.\n");
    }
}

void add(char buf[][MAX_BUF_SIZE], int *device_i, int *children_pids) {
    if (strcmp(buf[1], "bulb") == 0 || strcmp(buf[1], "fridge") == 0 || strcmp(buf[1], "window") == 0) {
        // Aumento l'indice progressivo dei dispositivi
        (*device_i)++;

        pid_t pid = fork();
        if (pid == 0) {  // Figlio
            // Apro una pipe per padre-figlio
            char *pipe_str = pipename(getpid());
            mkfifo(pipe_str, 0666);

            // Conversione a stringa dell'indice
            char *index_str = malloc(4 * sizeof(char));
            sprintf(index_str, "%d", *device_i);

            char program_name[MAX_BUF_SIZE / 4];
            sprintf(program_name, "./%s%s", "bin/", buf[1]);

            // Metto gli argomenti in un array e faccio exec
            char *const args[] = {program_name, index_str, pipe_str, NULL};
            execvp(args[0], args);

            exit(0);
        } else {  // Padre
            printf("Aggiunta una %s con PID %i e indice %i\n", buf[1], pid, *device_i);
            children_pids[*device_i] = pid;
            return;
        }
    } else {
        printf("Dispositivo non ancora supportato\n");
    }
}