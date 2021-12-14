#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h> //ouverture de fichier 🍔
#define TAILLE_MOT 1000
#define NOMBRE_MAX_PARAM 100

void parse(char commande[], char Parse_commande[][TAILLE_MOT],int *nombre_param);
void play_commande(char Parse_commande[][TAILLE_MOT],int nombre_param);
void erreur(char *erreur);
int ComparaisonDeDeuxChaines(char *chaine1,char *chaine2);

void main() {

    int close = 0;
    char commande[TAILLE_MOT];
    char Parse_commande[NOMBRE_MAX_PARAM][TAILLE_MOT];
    int nombre_param;

    char chemin[TAILLE_MOT] = ".\0";

    printf("GeoGeo SHELL \n \n");

    while(close == 0) {
        printf("%s > ",chemin);
        fgets(commande, TAILLE_MOT, stdin);
        parse(commande,Parse_commande,&nombre_param);
        
        if (ComparaisonDeDeuxChaines(Parse_commande[0],"echo"))
        {
            for (int i = 1; i < nombre_param + 1; i++)
            {
                printf("%s ",Parse_commande[i]);
            }
            printf(" \n");
            
        }
        else if (ComparaisonDeDeuxChaines(Parse_commande[0],"exit"))
        {
            close = 1;
        }
        else if (ComparaisonDeDeuxChaines(Parse_commande[0],"cd"))
        {
            if(ComparaisonDeDeuxChaines(Parse_commande[1],"..")){
                printf("pas encore implementer \n");
            }
            else if (ComparaisonDeDeuxChaines(Parse_commande[1],"-s"))
            {
                strcpy(chemin, "/\0");
            }
            
            else{
                strcat(chemin,Parse_commande[1]);
            }
        }
        else if (ComparaisonDeDeuxChaines(Parse_commande[0],"ls")){
            struct dirent *dir;
            // opendir() renvoie un pointeur de type DIR.

            DIR *d = opendir(chemin); 
            if (d)
            {
                while ((dir = readdir(d)) != NULL)
                {
                    printf("%s\n", dir->d_name);
                }
                closedir(d);
            }
            else
            {
                printf("votre dossier n'existe pas \n");
            }
            
        }        
        else {
            erreur("Votre commande n\'existe pas");
        }
    }    
};

void parse(char commande[], char Parse_commande[][TAILLE_MOT],int *nombre_param){

    char mot_tempon[TAILLE_MOT];
    int j = 0;
    int numero_param = 0;

    // INITIALISATION DE PARSE_COMMANDE
    for (int i = 0; i < NOMBRE_MAX_PARAM; i++)
    {
        for (int k = 0; k < TAILLE_MOT; k++)
        {
            Parse_commande[i][k] = '\0';
        }
    }
    

    for (int i = 0; i < TAILLE_MOT; i++)
    {
        if(commande[i] == '\n' || commande[i] == EOF){  
            *nombre_param = numero_param;    

            for (int k = 0; k < j; k++)
            {
                Parse_commande[numero_param][k] = mot_tempon[k];
            };

            break;
        }
        else{
            if(commande[i] != ' '){
                mot_tempon[j] = commande[i];
                ++j;
            }
            else{

                for (int k = 0; k < j; k++)
                {
                    Parse_commande[numero_param][k] = mot_tempon[k];
                };
                
                ++numero_param;
                j = 0;
            }
        }
    }
};

void erreur(char *erreur){
    printf("ERREUR : %s \n",erreur);
};

int ComparaisonDeDeuxChaines(char *chaine1,char *chaine2){

    int Pareil = 1;
    int i = 0;

    while (Pareil && i < TAILLE_MOT)
    {
        if(chaine1[i] != chaine2[i] && chaine1[i] != '\0'){
            Pareil = 0;
        }
        ++i;
    }

    return Pareil;
}