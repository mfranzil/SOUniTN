#include "actions.h"

/* wrapper per switch */
void __switch_index(int index, char *action, char *position, int *children_pids) {
    int pid;
    char *device_info;
    pid = get_device_pid(index, children_pids, &device_info);
    __switch(pid, action, position, device_info);
}

/* imposta un interruttore ACTION su POSITION di un certo DEVICE */
void __switch(int pid, char *action, char *position, char *device_info) {
    int status;
    char **vars;
    char buffer[MAX_BUF_SIZE]; /* buffer per la pipe */
    int __index;
    key_t key;
    int msgid;
    char switch_names[6][MAX_BUF_SIZE] = {"-", "accensione", "apertura", "apertura", "accensione", "accensione"};

    if (pid == -1) {
        printf("Errore! Non esiste questo dispositivo.\n");
        return;
    }

    if (device_info == NULL) {
        printf("Errore di connessione (PID %d)\n", pid);
        return;
    }

    vars = split(device_info);
    __index = atoi(vars[2]);

    if (strcmp(action, "generic_on_off") == 0) {
         action = switch_names[atoi(vars[0])];
    }

    key = ftok("/tmp/ipc/mqueues", pid);
    msgid = msgget(key, 0666 | IPC_CREAT);
    message.mesg_type = 1;

    if (strcmp(vars[0], BULB_S) == 0) { /* Lampadina */
        if (strcmp(action, "accensione") == 0) {
            status = atoi(vars[3]);
            sprintf(buffer, "0|0");
            if (strcmp(position, "on") == 0 && status == 0) {
                sprintf(message.mesg_text, "%s", buffer);
                msgsnd(msgid, &message, sizeof(message), 0);
                kill(pid, SIGUSR2);
                printf("Lampadina %d accesa.\n", __index);
            } else if (strcmp(position, "off") == 0 && status == 1) {
                sprintf(message.mesg_text, "%s", buffer);
                msgsnd(msgid, &message, sizeof(message), 0);
                kill(pid, SIGUSR2); /* sleep(1) */
                printf("Lampadina %d spenta.\n", __index);
            } else if (strcmp(position, "off") == 0 && status == 0) { /* Spengo una lampadina spenta */
                printf("La lampadina %d è già spenta.\n", __index);
            } else if (strcmp(position, "on") == 0 && status == 1) { /* Accendo una lampadina accesa */
                printf("La lampadina %d è già accesa.\n", __index);
            } else {
                printf("Sintassi non corretta. Sintassi: switch <bulb> accensione <on/off>\n");
            }
        } else {
            printf("Operazione non permessa su una lampadina!\nOperazioni permesse: accensione\n");
        }
    } else if (strcmp(vars[0], FRIDGE_S) == 0) { /* Fridge */
        if (strcmp(action, "apertura") == 0) {
            status = atoi(vars[3]);
            sprintf(buffer, "0|0");
            if (strcmp(position, "on") == 0 && status == 0) {
                sprintf(message.mesg_text, "%s", buffer);
                msgsnd(msgid, &message, sizeof(message), 0); /* invio sulla message queue */
                kill(pid, SIGUSR2);
                printf("Frigorifero %d aperto.\n", __index);
            } else if (strcmp(position, "off") == 0 && status == 1) {
                sprintf(message.mesg_text, "%s", buffer);
                msgsnd(msgid, &message, sizeof(message), 0); /* invio sulla message queue */
                kill(pid, SIGUSR2);
                printf("Frigorifero %d chiuso.\n", __index);
            } else if (strcmp(position, "off") == 0 && status == 0) { /* Chiudo frigo già chiuso */
                printf("Il frigorifero %d è già chiuso.\n", __index);
            } else if (strcmp(position, "on") == 0 && status == 1) { /* Apro frigo già aperto */
                printf("Il frigorifero %d è già aperto.\n", __index);
            } else {
                printf("Sintassi non corretta. Sintassi: switch <fridge> apertura <on/off>\n");
            }
        } else if (strcmp(action, "temperatura") == 0) {
            if ((atoi(position) >= -10 && atoi(position) < 0) || (atoi(position) > 0 && atoi(position) <= 15) || strcmp(position, "0") == 0) {
                sprintf(buffer, "1|%s", position);
                sprintf(message.mesg_text, "%s", buffer);
                msgsnd(msgid, &message, sizeof(message), 0); /* invio sulla message queue */
                kill(pid, SIGUSR2);
                printf("Temperatura del frigo %d modificata con successo a %s°C.\n", __index, position);
            } else {
                printf("Sintassi non corretta. Sintassi: switch <fridge> temperatura <-10 - 15>\n");
            }
        } else if (strcmp(action, "delay") == 0) {
            if ((atoi(position) > 0 && atoi(position) <= (60 * 5)) || strcmp(position, "0") == 0) { /* Massimo 5 minuti */
                sprintf(buffer, "2|%s", position);
                sprintf(message.mesg_text, "%s", buffer);
                msgsnd(msgid, &message, sizeof(message), 0); /* invio sulla message queue */
                kill(pid, SIGUSR2);
                printf("Tempo di richiusura automatico del frigo %d modificato con successo a %s secondi.\n", __index, position);
            } else {
                printf("Sintassi non corretta. Sintassi: switch <fridge> delay <0-300>.\n");
            }
        } else if (strcmp(action, "riempimento") == 0) { /* Possibile solo manualmente (launcher) */
            if ((atoi(position) > 0 && atoi(position) <= 100) || strcmp(position, "0") == 0) {
                sprintf(buffer, "3|%s", position);
                sprintf(message.mesg_text, "%s", buffer);
                msgsnd(msgid, &message, sizeof(message), 0); /* invio sulla message queue */
                kill(pid, SIGUSR2);
                printf("Percentuale di riempimento del frigo %d modificato con successo a %s. \n", __index, position);
            } else {
                printf("Sintassi non corretta. Sintassi: switch <fridge> riempimento <0-100>.\n");
            }
        } else {
            printf("Operazione non permessa su un frigorifero! Operazioni permesse: <temperatura/apertura/delay/riempimento>\n");
        }

    } else if (strcmp(vars[0], WINDOW_S) == 0) { /* Window */
        if (strcmp(action, "apertura") == 0 || strcmp(action, "chiusura") == 0) {
            if (((strcmp(action, "apertura") != 0) || (strcmp(action, "apertura") == 0 && strcmp(position, "off") == 0)) &&
                ((strcmp(action, "chiusura") != 0) || (strcmp(action, "chiusura") == 0 && strcmp(position, "off") == 0))) {
                printf("Operazione non permessa: i pulsanti sono solo attivi!\n");
                /* se posizione è off non permetto alcuna azione */
                return;
            }
            status = atoi(vars[3]);
            sprintf(buffer, "0|0");

            if (strcmp(action, "apertura") == 0 && status == 0) {
                sprintf(message.mesg_text, "%s", buffer);
                msgsnd(msgid, &message, sizeof(message), 0); /* invio sulla message queue */
                kill(pid, SIGUSR2);
                printf("Finestra %d aperta.\n", __index);
            } else if (strcmp(action, "chiusura") == 0 && status == 1) {
                sprintf(message.mesg_text, "%s", buffer);
                msgsnd(msgid, &message, sizeof(message), 0); /* invio sulla message queue */
                kill(pid, SIGUSR2);
                printf("Finestra %d chiusa.\n", __index);
            } else {
                printf("Operazione non permessa: pulsante già premuto.\n");
            }
        } else {
            printf("Operazione non permessa su una finestra! Operazioni permesse: <apertura/chiusura>.");
        }
    } else if (strcmp(vars[0], HUB_S) == 0) { /* Hub */
        if (strcmp(action, "accensione") == 0) {
            status = atoi(vars[3]);
            sprintf(buffer, "0|0");

            if (strcmp(position, "on") == 0 && (status == 0 || status == 3)) {
                sprintf(message.mesg_text, "%s", buffer);
                msgsnd(msgid, &message, sizeof(message), 0); /* invio sulla message queue */
                kill(pid, SIGUSR2);
                printf("Hub %d acceso.\n", __index);
            } else if (strcmp(position, "off") == 0 && (status == 1 || status == 2)) {
                sprintf(message.mesg_text, "%s", buffer);
                msgsnd(msgid, &message, sizeof(message), 0); /* invio sulla message queue */
                kill(pid, SIGUSR2);
                printf("Hub %d spento.\n", __index);
            } else if (strcmp(position, "off") == 0 && (status == 0 || status == 3)) { /* Spengo un hub spento */
                printf("Hub %d già spento.\n", __index);
            } else if (strcmp(position, "on") == 0 && (status == 1 || status == 2)) { /* Accendo un hub acceso */
                printf("Hub %d già acceso.\n", __index);
            } else {
                printf("Sintassi non corretta. Sintassi: switch <hub> accensione <on/off>\n");
            }
        } else {
            printf("Operazione non permessa su un hub!\nOperazioni permesse: accensione\n");
        }
    } else if (strcmp(vars[0], TIMER_S) == 0) {
        if (strcmp(action, "accensione") == 0) {
            status = atoi(vars[3]);
            sprintf(buffer, "0|0");

            if (strcmp(position, "on") == 0 && (status == 0 || status == 3)) {
                sprintf(message.mesg_text, "%s", buffer);
                msgsnd(msgid, &message, sizeof(message), 0); /* invio sulla message queue */
                kill(pid, SIGUSR2);
                printf("Timer %d acceso.\n", __index);
            } else if (strcmp(position, "off") == 0 && (status == 1 || status == 2)) {
                sprintf(message.mesg_text, "%s", buffer);
                msgsnd(msgid, &message, sizeof(message), 0);
                kill(pid, SIGUSR2); /* sleep(1) */
                printf("Timer %d spento.\n", __index);
            } else if (strcmp(position, "off") == 0 && (status == 0 || status == 3)) { /* Spengo un timer spento */
                printf("Timer %d già spento.\n", __index);
            } else if (strcmp(position, "on") == 0 && (status == 1 || status == 2)) { /* Accendo un timer acceso */
                printf("Timer %d già acceso.\n", __index);
            } else {
                printf("Sintassi non corretta. Sintassi: switch <timer> accensione <on/off>\n");
            }
        } else if (strcmp(action, "orario") == 0) {
            int h_start, m_start, h_end, m_end;
            /* Aggiungere controlli sugli orari */
            int scan = sscanf(position, "%d:%d->%d:%d", &h_start, &m_start, &h_end, &m_end);
            if (scan != 4 || h_start < 0 || h_start > 23 || h_end < 0 || h_end > 59 || h_start > h_end || m_start > m_end) {
                printf("Formattazione degli orari sbagliata. Formato (24 ore): \"HH:MM -> HH:MM\"\n");
            } else {
                sprintf(buffer, "3|%d|%d|%d|%d", h_start, m_start, h_end, m_end);
                sprintf(message.mesg_text, "%s", buffer);
                msgsnd(msgid, &message, sizeof(message), 0);
                kill(pid, SIGUSR2);
                printf("Timer %d settato dalle ore %d:%d alle ore %d:%d\n", __index, h_start, m_start, h_end, m_end);
            }
        } else {
            printf("Operazione non permessa su un timer!\nOperazioni permesse: <accensione/orario>\n");
        }
    } else { /* tutti gli altri dispositivi */
        printf("Dispositivo non supportato.\n");
    }
    free(vars);
}

/* chiede le informazioni ai device in base al'id */
void __info(int index, int *children_pids) {
    char *info = NULL;
    int pid;
    char **info_p = NULL;

    pid = get_device_pid(index, children_pids, &info);

    if (pid == -1) {
        printf("Errore! Non esiste questo dispositivo.\n");
        return;
    }

    if (info == NULL) {
        printf("Errore di connessione (PID %d)\n", pid);
        return;
    }

    if (strncmp(info, HUB_S, 1) != 0 && strncmp(info, TIMER_S, 1) != 0) {
        info_p = split(info);
        __print(info_p);
    } else {
        info = get_raw_device_info(pid);

        if (strncmp(info, HUB_S, 1) == 0) {
            hub_tree_parser(info);
        } else {
            info_p = split(info);
            __print(info_p);
        }
    }

    free(info);
}

/* stampa le informazioni */
void __print(char **vars) {
    if (strcmp(vars[0], BULB_S) == 0) {
        /* Lampadina - parametri: tipo, pid, indice, stato, tempo di accensione */
        printf("Lampadina (PID %s, Indice %s)\nStato: %s\nTempo di accensione: %s\n",
               vars[1], vars[2], atoi(vars[3]) ? "ON" : "OFF", vars[4]);
    } else if (strcmp(vars[0], FRIDGE_S) == 0) {
        /* Frigo -  parametri: tipo, pid, indice, stato, tempo di apertura, delay */
        /* percentuale riempimento, temperatura interna */
        printf("Frigorifero (PID %s, Indice %s)\nMessaggio di log: <%s>\n", vars[1], vars[2], vars[8]);
        printf("Stato: %s\nTempo di apertura: %s sec\n", atoi(vars[3]) ? "Aperto" : "Chiuso", vars[4]);
        printf("Delay richiusura: %s sec\nPercentuale riempimento: %s\nTemperatura: %s°C\n",
               vars[5], vars[6], vars[7]);
    } else if (strcmp(vars[0], WINDOW_S) == 0) {
        /* Finestra - parametri: tipo, pid, indice, stato, tempo di accensione */
        printf("Finestra (PID %s, Indice %s)\nStato: %s\nTempo di apertura: %s sec\n",
               vars[1], vars[2], atoi(vars[3]) ? "Aperto" : "Chiuso", vars[4]);
    } else if (strcmp(vars[0], HUB_S) == 0) {
        printf("Hub (PID %s, Indice %s)\nStato: %s\nDispositivi collegati: %s\n",
               vars[1], vars[2], (atoi(vars[3]) == 1 || atoi(vars[3]) == 2) ? "Acceso" : "Spento", vars[4]);
    } else if (strcmp(vars[0], TIMER_S) == 0) {
        printf(
            "Timer (PID %s, Indice %s)%s\nStato: %s\nFascia oraria: %s%s:%s%s -> %s%s:%s%s \
        \nDispositivi collegati: %s\n",
            vars[1], vars[2], (atoi(vars[3]) == 2 || atoi(vars[3]) == 3) ? "  -> Override" : " ",
            (atoi(vars[3]) == 1 || atoi(vars[3]) == 2) ? "Acceso" : "Spento",
            atoi(vars[4]) < 10 ? "0" : "", vars[4],
            atoi(vars[5]) < 10 ? "0" : "", vars[5],
            atoi(vars[6]) < 10 ? "0" : "", vars[6],
            atoi(vars[7]) < 10 ? "0" : "", vars[7],
            vars[8]);
    } else {
        printf("Dispositivo non supportato.\n");
    }
}

/* aggiungo */
int __add(char *device, int device_index, int *children_pids, const int devices_number, char *__out_buf) {
    int actual_index = -1;
    pid_t pid;
    int i; /* del ciclo */
    char pipe_str[MAX_BUF_SIZE];
    char index_str[MAX_BUF_SIZE / 4];
    char program_name[MAX_BUF_SIZE / 4];
    char device_name[MAX_BUF_SIZE];

    for (i = 0; i < devices_number; i++) {
        if (children_pids[i] == -1) {
            actual_index = i;
            break;
        }
    }

    if (i == devices_number) {
        sprintf(__out_buf, "Non c'è più spazio! Rimuovi qualche dispositivo.\n");
        return 0;
    }

    pid = fork();
    if (pid == 0) { /* Figlio */
        /* Apro una pipe per padre-figlio */
        get_pipe_name(getpid(), pipe_str);
        mkfifo(pipe_str, 0666);
        /* Conversione a stringa dell'indice */
        sprintf(index_str, "%d", device_index);
        sprintf(program_name, "./%s%s", DEVICES_POSITIONS, device);
        execlp(program_name, program_name, index_str, pipe_str, NULL);
        exit(0);
    } else { /* Padre */
        children_pids[actual_index] = pid;
        key_t key = ftok("/tmp/ipc/mqueues", device_index);
        int msgid = msgget(key, 0666 | IPC_CREAT);
        if (msgid = -1) {
            msgrcv(msgid, &message, sizeof(message), 1, IPC_NOWAIT);
        }
        get_device_name_str(device, device_name);
        sprintf(__out_buf, "Aggiunto un dispositivo di tipo %s con PID %d e indice %d\n",
                device_name, pid, device_index);
    }
    return 1;
}

void __list(int *children_pids) {
    int i;
    char **vars = NULL;
    char *tmp;
    int children_pid;
    char device_name[MAX_BUF_SIZE];

    /* Centralina */
    printf("Centralina, (PID %d, Indice 0)\n", (int)getpid());

    for (i = 0; i < MAX_CHILDREN; i++) { /* l'indice i è logicamente indipendente dal nome/indice del dispositivo */
        children_pid = children_pids[i];
        if (children_pid != -1) {
            printf("∟ ");
            tmp = get_raw_device_info(children_pid);

            if (tmp == NULL) {
                printf("Errore di comunicazione (PID %d)\n", children_pid);
            } else {
                if (strncmp(tmp, HUB_S, 1) == 0 || strncmp(tmp, TIMER_S, 1) == 0) {
                    hub_tree_parser(tmp);
                } else {
                    vars = split(tmp);
                    get_device_name(atoi(vars[0]), device_name);
                    device_name[0] += 'A' - 'a';

                    printf("%s, (PID %s, Indice %s)\n", device_name, vars[1], vars[2]);
                    /* Pulizia */
                    free(vars);
                }
            }
        }
    }
}

void __del(int index, int *children_pids, char *__out_buf) {
    char *raw_device_info = NULL;
    char **vars;
    int pid;
    char pipe_str[MAX_BUF_SIZE];
    char device_name[MAX_BUF_SIZE];
    int i;

    pid = get_device_pid(index, children_pids, &raw_device_info);
    //raw_device_info = get_raw_device_info(pid);

    if (pid == -1) {
        sprintf(__out_buf, "Errore! Non esiste questo dispositivo.\n");
        return;
    }

    vars = split(raw_device_info);

    get_pipe_name(pid, pipe_str);

    get_device_name(atoi(vars[0]), device_name);
    device_name[0] += 'A' - 'a';

    sprintf(__out_buf, "Dispositivo di tipo %s con PID %s e indice %s rimosso.\n",
            device_name, vars[1], vars[2]);

    free(vars);

    kill(pid, SIGTERM);
    remove(pipe_str); /* RIP pipe */

    for (i = 0; i < MAX_CHILDREN; i++) {
        if (children_pids[i] == pid) {
            children_pids[i] = -1;
            return;
        }
    }
}

void __link(int index, int controller, int *children_pids) {
    char *raw_device_info = NULL;
    char *raw_controller_info = NULL;
    int device_pid, controller_pid; /*, fd;*/
    char buf[MAX_BUF_SIZE];
    char __out_buf[MAX_BUF_SIZE];

    key_t key;
    int msgid;

    int t;

    device_pid = get_device_pid(index, children_pids, &raw_device_info);

    if (device_pid == -1) {
        printf("Errore! Non esiste il dispositivo %d.\n", index);
        return;
    }

    if (raw_device_info == NULL) {
        printf("Errore in lettura del dispositivo.\n");
        return;
    }

    controller_pid = get_device_pid(controller, children_pids, &raw_controller_info);

    if (controller_pid == -1) {
        printf("Errore! Non esiste il dispositivo %d.\n", controller);
        return;
    }

    if (device_pid == controller_pid) {
        printf("Errore! Non puoi collegarti a te stesso.\n");
        return;
    }

    printf("Spostamento in corso...\n");
    sleep(1);

    if (is_controller(controller_pid, raw_controller_info)) {
        if (!controller_is_full(controller_pid, raw_controller_info)) {
            sprintf(buf, "1|");
            strcat(buf, raw_device_info);

            __del(index, children_pids, __out_buf);


            key = ftok("/tmp/ipc/mqueues", controller_pid);
            msgid = msgget(key, 0666 | IPC_CREAT);
            message.mesg_type = 1;
            sprintf(message.mesg_text, "%s", buf);
            msgsnd(msgid, &message, sizeof(message), 0);
            kill(controller_pid, SIGUSR2);

            printf("Spostato l'oggetto %d sotto l'oggetto %d\n", index, controller);
            printf("Controllo dello stato dei dispositivi figli...\n");
            sleep(2);

            free(raw_device_info);
            free(raw_controller_info);
        } else {
            printf("Operazione non permessa. Il dispositivo %d è già pieno. Eliminare qualche figlio.\n", controller);
        }
    } else {
        printf("Configurazione dei dispositvi non valida. Sintassi: link <device> to <hub/timer>\n");
    }
}

/* collego figli dopo una link */
int __add_ex(char **vars, int *children_pids, const int devices_number) {
    char __out_buf[MAX_BUF_SIZE];
    if (strcmp(vars[0], BULB_S) == 0) { /* Lampadina */
        return __add("bulb", atoi(vars[2]), children_pids, devices_number, __out_buf);
        /* Chiaramente minchia posso replicare il time_on o lo stato... */
        /* se scollego una lampadina quella si spegne */
    } else if (strcmp(vars[0], FRIDGE_S) == 0) { /* Frigo */
        return __add("fridge", atoi(vars[2]), children_pids, devices_number, __out_buf);
        /*    .... aggiungere la temperatura eventualmente riempimento etc */
    } else if (strcmp(vars[0], WINDOW_S) == 0) { /* Frigo */
        return __add("window", atoi(vars[2]), children_pids, devices_number, __out_buf);
    } else if (strcmp(vars[0], HUB_S) == 0) {
        return __add("hub", atoi(vars[2]), children_pids, devices_number, __out_buf);
    } else if (strcmp(vars[0], TIMER_S) == 0) {
        return __add("timer", atoi(vars[2]), children_pids, devices_number, __out_buf);
    } else {
        printf("Da implementare...");
        return 0;
    }
}

int hub_tree_constructor(char *__buf, int *children_pids, const int devices_number) {
    char *tokenizer = strtok(__buf, "|");
    char *old = NULL;

    char **vars;
    int i = 0;
    int to_be_added = 1;

    vars = malloc((FRIDGE_PARAMETERS + 4) * sizeof(*vars));
    while (tokenizer != NULL) {
        if (strcmp(tokenizer, "<!") == 0) {
            to_be_added += atoi(old) - 1;
            i = 0;
            __add_ex(vars, children_pids, devices_number);
        } else if (strcmp(tokenizer, "!>") == 0) {
            if (strcmp(old, "<!") != 0 && strcmp(old, "!") != 0 && to_be_added > 0) {
                i = 0;

                __add_ex(vars, children_pids, devices_number);
                to_be_added--;
            }
        } else if (strcmp(tokenizer, "!") == 0) {
            if (strcmp(old, "!>") != 0 && to_be_added > 0) {
                i = 0;

                __add_ex(vars, children_pids, devices_number);
                to_be_added--;
            }
        } else {
            vars[i++] = tokenizer;
        }
        old = tokenizer;
        tokenizer = strtok(NULL, "|");
    }
    free(vars);
    return -1;
}

void del_direct(int index, int *children_pids, char *__out_buf) {
    char *device_info;
    int pid;
    char device_name[MAX_BUF_SIZE];
    char **vars;

    pid = get_device_pid(index, children_pids, &device_info);
    vars = split(device_info);
    get_device_name(atoi(vars[0]), device_name);
    

    sprintf(__out_buf, "Dispositivo di tipo %s con PID %s e indice %s rimosso.\n",
            device_name, vars[1], vars[2]);

    kill(pid, SIGINT);
    int i;
    for (i = 0; i < MAX_CHILDREN; i++) {
        if (children_pids[i] == pid) {
            children_pids[i] = -1;
        }
    }
}

