#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h>
#include "readcmd.h"
#include <signal.h>
#include <fcntl.h>

/* Etats d'un processus */
enum ETATS {RUNNING, SUSPENDED};

//list pour gérer les processus en arrière plan
typedef struct cell_ps cell_ps;

/*Définition de la struture de cellule */
struct cell_ps {
    int id;
    int pid;
    int etat;
    char cmd[50]; //nom de la commande
    cell_ps *next;
};

/* déinition de la structure de liste */
typedef struct list_ps  {
    int lenght;
    cell_ps *head;
} list_ps;

/*création d'une nouvelle liste */
list_ps * new_list() {
    return malloc(sizeof(list_ps));
}

list_ps *list;

/*création d'une nouvelle cellule*/
cell_ps * new_cell(int id, int pid, char cmd[50]) {
    cell_ps *cell = malloc(sizeof(cell_ps));
    cell->id = id;
    cell->pid = pid;
    cell->etat = RUNNING;
    strcpy(cell->cmd, cmd);
    cell->next = NULL;

    return cell;
}

/*ajout d'une nouvelle cellule dans une liste*/
void add(list_ps * list,int id, int pid, char * cmd) {
    cell_ps * n_cell;
    n_cell = new_cell(id, pid, cmd);
    cell_ps *head;
    head = list->head;
    if (head == NULL) {
        list->head = n_cell;
    }
    else {
        cell_ps *cell_suivant;
        cell_suivant = head;
        while (cell_suivant->next != NULL) {
            cell_suivant = cell_suivant->next;
        }
        cell_suivant->next = n_cell;
    }
    list->lenght++;
}

/* supprimer la cellule avec le pid correspondant*/
void suppr(list_ps * list, int pid) {
    if(list->head != NULL) {
        if (list->head->pid == pid) {
            list->head = list->head->next;
        }
        else {
            cell_ps *cell_suivant;
            cell_suivant = list->head;
            while (cell_suivant->next != NULL && cell_suivant->next->pid != pid) {
                cell_suivant = cell_suivant->next;
            }
            if (cell_suivant->next != NULL) {
                cell_suivant->next = cell_suivant->next->next;
            }
            else {
                cell_suivant->next = NULL;
            }
        }
    }
}

/*changer l'état de la cellule avec le pid associé contenu dans la list*/
void changer_etat(list_ps * list, int pid, int etat) {
    cell_ps *cell_suivant;
    cell_suivant = list->head;
    
    while (cell_suivant->pid != pid) {
        cell_suivant = cell_suivant->next;
    }
    cell_suivant->etat = etat;
}

/*afficher les processus de la list*/
void afficher(list_ps *list) {
    cell_ps *cell;
    cell = list->head;

    printf("ID      PID STATE CMD\n");

    while (cell != NULL) {
        printf(" %d    %d %-5d %s\n",cell->id, cell->pid, cell->etat, cell->cmd);
        cell = cell->next;
    }
}

/*renvoie la cellule avec l'id associé*/
cell_ps * find(list_ps * list, int id) {
    cell_ps *cell_suivant;
    cell_suivant = list->head;
    
    while (cell_suivant != NULL && cell_suivant->id != id) {
        cell_suivant = cell_suivant->next;
    }
    
    if  (cell_suivant != NULL) {
        return cell_suivant;
    }
    else {
        return NULL;
    }
}

//initialisation variable globale

/*si idCourant != -1 alors idCourant représente le id du processus en avant plan 
et si idCourant = -1 il n'y a pas de processus en avant plan */
int idCourant = -1; 
pid_t idFils = 0;
int desc;

//handler SIGCHLD

void handler_SIGCHLD(int sig) {
    int pidFils, etat_fils;

    do {
        pidFils = (int) waitpid(-1, &etat_fils, WNOHANG | WUNTRACED | WCONTINUED);

        if (pidFils > 0) {
            if (WIFEXITED(etat_fils)) {
                suppr(list, pidFils);
            }
            else if (WIFCONTINUED(etat_fils)){
                changer_etat(list, pidFils, RUNNING);
            }
            else if (WIFSTOPPED(etat_fils)) {
                changer_etat(list, pidFils, SUSPENDED);
            }
        }

    } while( pidFils > 0);    
}

// Handler SIGTSTP

void handler_SIGTSTP(int sig) {
    afficher(list);
    if(idCourant == 0) { //idCourant == 0 -> on veut suspendre le processus le minishell
        kill(getpid(),SIGSTOP);
    }
    else {
        if (find(list,idCourant) != NULL) {
            if (find(list,idCourant)->etat == 0) {
                printf("\n");
                printf("\n [%d] Arrété  %s\n",idCourant, find(list,idCourant)->cmd);
            }
            kill(find(list,idCourant)->pid,SIGSTOP);
        }
    }
    
}

// Handler SIGINT

void handler_SIGINT(int sig) {
    if(find(list,idCourant) != NULL) {
        suppr(list,find(list,idCourant)->pid);
        kill(idCourant,SIGINT);
    }
}

// MAIN
int main() {

    struct cmdline *commande;
    //int codeTerm;
    int pidFils;
    int desc_stdout = dup(1);
    int desc_stdin = dup(0);
// f
    //initialisation des listes de processus
    list = new_list();

    //initialisation du handler de SIGCHLD

    struct sigaction handler_SIGCHLD_sig;
    handler_SIGCHLD_sig.sa_handler = handler_SIGCHLD;
    handler_SIGCHLD_sig.sa_flags = SA_RESTART;

    sigemptyset(&(handler_SIGCHLD_sig.sa_mask));
    sigaction(SIGCHLD, &handler_SIGCHLD_sig, NULL);

    //initialisation du handler de SIGTSTP

    struct sigaction handler_SIGTSTP_sig;
    handler_SIGTSTP_sig.sa_handler = handler_SIGTSTP;
    //handler_SIGTSTP_sig.sa_flags = SA_RESTART;

    sigemptyset(&(handler_SIGTSTP_sig.sa_mask));
    sigaction(SIGTSTP, &handler_SIGTSTP_sig, NULL);

    //initialisation du handler de SIGINT

    struct sigaction handler_SIGINT_sig;
    handler_SIGINT_sig.sa_handler = handler_SIGINT;
    //handler_SIGTSTP_sig.sa_flags = SA_RESTART;

    sigemptyset(&(handler_SIGINT_sig.sa_mask));
    sigaction(SIGINT, &handler_SIGINT_sig, NULL);

    //Boucle du minishell

    while (true) {
        if(dup2(desc_stdout, 1) < 0) {
            perror("Erreur de dup2");
        }

        if(dup2(desc_stdin, 0) < 0) {
            perror("Erreur de dup2");
        }

        char pwd[1024];
        getcwd(pwd, sizeof(pwd));
        printf("%s$ ",pwd);

        commande = readcmd();

        if (commande == NULL) {
            //printf("\n--- Erreur da la fonction de saisie ou EOF - CtrlD\n");
            printf("\n");
        }
        else {
            if (commande->err != NULL) {
                printf("--- Erreur de structure de la commande : %s\n", commande->err);
            } 
            else {

                if (commande->in != NULL) {
                        if ((desc = open(commande->in, O_RDONLY)) >= 0) {
                            if(dup2(desc, 0) >= 0) {
                                close(desc);
                            }
                            else {
                                perror("dup2 n'a pas fonctionné");
                            }
                        }
                        else {
                            perror("Open n'a pas fonctionné");
                        }
                    }
                    

                int i = 0;

                //calcul de la taille de la commande
                int taille_commande = 0;
                while(commande->seq[++taille_commande] != NULL);

                int p[taille_commande][2];
                for(int j = 0; j < taille_commande; j++) {
                    if(pipe(p[j]) > 0) {
                        perror("Erreur pipe");
                    };
                }

                while (i < taille_commande) {

                    //     else {
                    //         dup2(p[1], 1);
                    //         dup2(p[0], 0);
                    //         close(p[1]);
                    //         close(p[0]);
                    //     }
                    // }
                    // if (commande->in != NULL) {
                    //     printf("=== Redirection de l'entrée : %s\n", commande->in);
                    // }
                    // if (commande->out != NULL) {
                    //     printf("=== Redirection de la sortie : %s\n", commande->out);
                    // }
                    // if (commande->backgrounded != NULL) {
                    //     printf("=== Commande en tache de fond\n");
                    // }
                    /* commande->seq[i] est accessible seulement si :
                        commande != NULL && command->err == NULL
                    */
                    

                    if (commande->seq[i] != NULL) {

                        if (strncmp(commande->seq[i][0],"exit",strlen("exit")) == 0) { //commande exit
                            //printf("Salut\n");
                            exit(0);
                        } else if ((strcmp(commande->seq[i][0], "cd")) == 0) { //commande cd

                            if (commande->seq[i][1] != NULL) {
                                if (strcmp(commande->seq[i][1], "~") == 0) {
                                    chdir(getenv("HOME"));
                                }
                                else{
                                    if (chdir(commande->seq[i][1]) != 0){
                                        printf("bash: cd: %s: Aucun fichier ou dossier de ce type \n",commande->seq[i][1]);
                                    };
                                }
                            }
                            else {
                                chdir(getenv("HOME"));
                            }
                        }
                        else if ((strcmp(commande->seq[i][0], "lj")) == 0) { //commande lj
                            afficher(list);
                        }
                        else if ((strcmp(commande->seq[i][0], "sj")) == 0) { //commande sj
                            if (commande->seq[i][1] != NULL) {
                                if (find(list,atoi(commande->seq[i][1])) != NULL) {
                                    
                                    if (kill(find(list,atoi(commande->seq[i][1]))->pid, SIGSTOP) < 0) {
                                        printf("erreur \n");
                                    };
                                }
                                else {
                                    printf("Ce processus n'existe pas \n");
                                }                        

                            }
                            else {
                                printf("Il manque l'id \n");
                            }
                        
                        }
                        else if ((strcmp(commande->seq[i][0], "bg")) == 0) { //commande bg
                            if (commande->seq[i][1] != NULL) {
                                if (find(list,atoi(commande->seq[i][1])) != NULL) {
                                    
                                    if (kill(find(list,atoi(commande->seq[i][1]))->pid, SIGCONT) < 0) {
                                        printf("Erreur \n");
                                    };
                                }
                                else {
                                    printf("Ce processus n'existe pas \n");
                                }                        

                            }
                            else {
                                printf("Il manque l'id \n");
                            }
                        
                        }
                        else if ((strcmp(commande->seq[i][0], "fg")) == 0) { //commande fg
                            if (commande->seq[i][1] != NULL) {
                                if (find(list,atoi(commande->seq[i][1])) != NULL) {
                                    int etat_fils;
                                    idCourant = atoi(commande->seq[i][1]);
                                    int pidCourant = find(list,idCourant)->pid;

                                    if (kill(find(list,atoi(commande->seq[i][1]))->pid, SIGCONT) < 0) {
                                        printf("Erreur \n");
                                    };
                                    //pidFils = (int) waitpid(find(list,atoi(commande->seq[i][1])), &etat_fils, 0);
                                    if ((pidFils = (int) waitpid(pidCourant, &etat_fils, 0)) > 0) {
                                        suppr(list, pidFils);
                                    };
                                }
                                else {
                                    printf("Ce processus n'existe pas \n");
                                }                        

                            }
                            else {
                                printf("Il manque l'id \n");
                            }
                        
                        }
                        else if ((strcmp(commande->seq[i][0], "susp")) == 0) { //commande susp
                            idCourant = 0;
                            kill(getpid(),SIGTSTP);
                        }
                        else {
                            switch (pidFils = fork()) { //création du fils
                            case -1: //echec
                                printf("Erreur fork\n");
                                exit(1);
                                break;
                            case 0: //fils
                                setsid();
                                if(i == taille_commande - 1) {
                                    dup2(p[i-1][0], 0);
                                    if (commande->out != NULL) {
                                        if ((desc = open(commande->out, O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU)) >= 0) {
                                            if(dup2(desc, 1) >= 0) {
                                                close(desc);
                                            }
                                            else {
                                                perror("dup2 n'a pas fonctionné");
                                            }
                                        }
                                        else {
                                            perror("Open n'a pas fonctionné");
                                        }
                                    } else {
                                        dup2(desc_stdout, 1);
                                    }
                                }
                                else if (i == 0) {
                                    dup2(desc_stdin, 0);
                                    dup2(p[i][1],1);
                                }
                                else {
                                    dup2(p[i][1], 1);
                                    dup2(p[i-1][0], 0);
                                }

                                for(int j = 0; j < taille_commande; j++) {
                                    close(p[j][0]);
                                    close(p[j][1]);
                                }

                                close(desc_stdout);
                                close(desc_stdin);

                                //add(list, ++idFils, pidFils, commande->seq[i][0]);
                                execvp(commande->seq[i][0], commande->seq[i]);
                                printf("%s : commande introuvable\n",commande->seq[i][0]);
                                exit(1);
                                break;
                            default: //pere
                                //printf(commande->seq[i][0]);

                                if (i == taille_commande - 1) {

                                     for(int j = 0; j < taille_commande; j++) {
                                        if(j != i) {
                                            close(p[j][0]);
                                            close(p[j][1]);
                                        }
                                    }

                                    add(list, ++idFils, pidFils, commande->seq[i][0]);

                                    if(commande->backgrounded == NULL) {
                                        
                                        idCourant = idFils;
                                        if(waitpid(pidFils, NULL, 0) > 0) {
                                            suppr(list, pidFils);
                                        }

                                        //while((waitpid(pidFils, NULL, 0) < 0)) {}
                                        //pause();
                                    }
                                    else {
                                        //idFils = waitpid(-1, &codeTerm, WNOHANG | WUNTRACED | WCONTINUED);
                                        printf("[%d] %d\n", pidFils, idFils);
                                    }

                                    close(p[i][0]);
                                    close(p[i][1]);

                                    
                                    

                                    // if (pidFils == -1) {
                                    //         perror("wait ");
                                    //         exit(2);
                                    //     }
                                }
                            }
                        }
                    }
                    i++;
                }
            }
        }
    }
}
