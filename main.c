/////////////////////////INCLUDES/////////////////////////////

#include <stdio.h>
#include <stdlib.h>

#include "modification_meta.h"
#include "main.h"
#include "analyse_meta.h"


/////////////////////////Defines/////////////////////////////


#define TAILLE_MAX_FICHIER 50


//Ligne permettant de tranformer les printfs en fonctions permettant d'afficher dans la box GTK (afficher_texte_format garde les mêmes arguments qu'un printf)
#define printf(...) afficher_texte_format(__VA_ARGS__)




void viderbuffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

/////////////////////////Commandes My_String/////////////////////////////


//comparer 2 chaines

int my_strcmp(unsigned char s1[],unsigned char s2[])
{
    int i = 0;
    while (s1[i] == s2[i] && s1[i] != '\0')
    {
        i++;
    }
    return s1[i] - s2[i];
}



//calculer la taille

unsigned long my_strlen(const unsigned char *s) {
    //renvoie la longueur du tableau
unsigned long i = 0;
    while (s[i] != '\0') {
        i++;
    }
    return i;
}

//copie
unsigned char *my_strcpy(unsigned char *dest, const unsigned char *src) {
    unsigned char *ptr = dest;
    while (*src) {
        *ptr = *src;
        ptr++;
        src++;
    }
    *ptr = '\0';
    return dest;
}



//copie mais de n caractère(s)
unsigned char *my_strcpy4(unsigned char *dest, const unsigned char *src, size_t n) {
    for (size_t i = 0; i < n; i++) {
        dest[i] = src[i];
    }
    return dest;
}

// dupliqué
unsigned char *my_strdup(const unsigned char *s) {
    unsigned char *copy = malloc(my_strlen(s) + 1);
    if (copy) my_strcpy(copy, s);
    return copy;
}
//////////////////////////////////////////////FONCTIONS EXTRACTION//////////////////////////////////////////////////:


// recherche une boite spécifique ( recherche) dans le fichier
unsigned long rechercheboite(FILE *fichier,unsigned char recherche[4], unsigned long taille) {
    unsigned long compteur=0;
    unsigned char rlettre ;

    do
    {
        fseek(fichier,compteur,SEEK_SET);
        unsigned char lettre = recherche[0];

        if (fread(&rlettre, 1, 1, fichier) != 1){
            return -1;
        }

        if(rlettre == lettre){
            if (confirmateur(fichier,recherche) == 0){
                return compteur - 4;   //position debut avant taille
            }
        }
        compteur++;
    }
    while(compteur + 4 < taille);

    return -1;
}




int saisie_et_verification_int(int mini, int maxi){
    //controle la saisie de l'utilisater pour s'assurer qu'il emploie uniquement les int qui lui sont demander (pour la version ligne de commande)

 char entrer[3];
 char *fin;

if (fgets(entrer, sizeof(entrer), stdin) == NULL) {
    printf("Erreur de lecture\nveuillez reessayer ");
    viderbuffer();
    return 44;
    }

    int valeur = strtol(entrer, &fin, 10);


if (fin == entrer) {
    printf("rentrer uniquement que des nombres\n");
    if ( entrer[1] != '\n'){
    viderbuffer(); }
    return 44;
}


if ((valeur < mini) || (valeur > maxi)) {
    printf("option non dipsonible\n");
    viderbuffer();
    return 44;
    }

return valeur;
}



void saisie_et_verification_char(char *nomfichier) {

    //controle la saisie de l'utilisater pour s'assurer qu'il emploie uniquement les char qui lui sont demander (pour la version ligne de commande)



int bon =0;

do {
    printf(" quelle fichier voulez-vous charger : ");

    if (fgets(nomfichier,TAILLE_MAX_FICHIER, stdin) == NULL) {
        printf("erreur saisie, veuillez reessayer\n");
        continue;
        }

    for(int i =0; i < TAILLE_MAX_FICHIER;i++){
        if( nomfichier[i] == '\n') {
            nomfichier[i] = '\0';
            bon =1;
            return;
        }
    }

    if ( bon != 1) {
        printf("nom du fichier trop grand veuillez reessayer\n");
        viderbuffer();
        }
} while (bon != 1);
}


//ouverture fichier
FILE *ouverturefichier(char nom[50]) {

    FILE *fichier = fopen(nom, "r+b");
    if (fichier == NULL) {
        printf("Impossible d'ouvrir le fichier\n");
        return NULL;
    }
    return fichier;
}


//lire 32bits
unsigned long lire_u32(unsigned char b[4]) {
    //Lire en big-Endian des chiffres sur 4 octets
    return ((unsigned long)b[0] << 24) |
           ((unsigned long)b[1] << 16) |
           ((unsigned long)b[2] << 8)  |
           ((unsigned long)b[3]);
}


//ecris en big-Endian des chiffres sur 4 octets
void ecrire_u32(unsigned long valeur, unsigned char buf[4])
{
    buf[0] = (valeur >> 24) & 0xFF;
    buf[1] = (valeur >> 16) & 0xFF;
    buf[2] = (valeur >> 8)  & 0xFF;
    buf[3] = valeur & 0xFF;
}

//comparateur 4 caractère
int comparateur4(unsigned char a[4], unsigned char b[4]) {
    for (int i = 0; i < 4; i++) {
        if (a[i] != b[i]) return 0;
    }
    return 1;
}

//Lis le contenu même d'une boîte, jusqu'à la suivant
int lire_box(FILE *fichier, box *b) {
    unsigned char t[4];

    b->position = ftell(fichier);

    if (fread(t, 1, 4, fichier) != 4) return 0;
    b->taille = lire_u32(t);

    if (fread(b->type, 1, 4, fichier) != 4) return 0;
    b->type[4] = '\0';


    if (b->taille == 1) {
        unsigned char grossetaille[8];
        if (fread(grossetaille, 1, 8, fichier) != 8) return 0;
        unsigned long long vraie_taille =
            ((unsigned long long)grossetaille[0] << 56) | ((unsigned long long)grossetaille[1] << 48) |
            ((unsigned long long)grossetaille[2] << 40) | ((unsigned long long)grossetaille[3] << 32) |
            ((unsigned long long)grossetaille[4] << 24) | ((unsigned long long)grossetaille[5] << 16) |
            ((unsigned long long)grossetaille[6] << 8)  | ((unsigned long long)grossetaille[7]);
        b->taille = (unsigned long)vraie_taille;
    }
    return 1;
}



//permet de vérifier que les lettres suivantes sont bien celle qui correspond a la boite recherché

int confirmateur(FILE *fichier,unsigned char mot[]){
    unsigned char suivant[3];

    fread(suivant, 3, 1,fichier);

    for(int i =0; i < 3;i++){
        if(mot[i+1] != suivant[i]){
            return 1;
        }
    }
    return 0;
}


//lire champ

void lire_champ(FILE *fichier, box champ) {
    box data;
    unsigned long taille_texte;

    fseek(fichier, champ.position + 8, SEEK_SET);

    if (!lire_box(fichier, &data)) {
        printf(" Erreur lecture boîte data pour champ %c%c%c%c\n",
               champ.type[0], champ.type[1], champ.type[2], champ.type[3]);
        return;
    }

    if (!comparateur4(data.type, "data")) {
        printf("⚠ Pas de boîte data pour champ %c%c%c%c\n",
               champ.type[0], champ.type[1], champ.type[2], champ.type[3]);
        return;
    }

    fseek(fichier, 8, SEEK_CUR); // type + locale
    taille_texte = data.taille - 16;
    if (taille_texte <= 0) {
        printf("⚠ Champ %c%c%c%c vide\n",
               champ.type[0], champ.type[1], champ.type[2], champ.type[3]);
        return;
    }

     unsigned char *texte_brut = malloc(taille_texte + 1);
    if (!texte_brut) {
        printf("Allocation échouée\n");
        return;
    }

    size_t lu = fread(texte_brut, 1, taille_texte, fichier);
    texte_brut[lu] = 0;

    printf("%c%c%c :",
           champ.type[1], champ.type[2], champ.type[3]);


    for (size_t i = 0; i < lu; i++) {
        if (texte_brut[i] >= 32 && texte_brut[i] <= 126) {
            printf("%c", texte_brut[i]);
        } else {
            printf(".");
        }
    }
    printf("\n\n\n");

    free(texte_brut);
}


//permet de lire la boite ilst
void lire_ilst(FILE *fichier, long pos, unsigned long taille) {
    box champ;
    long fin = pos + taille;

    fseek(fichier, pos + 8, SEEK_SET);



    while (ftell(fichier) < fin && lire_box(fichier, &champ)) {

        printf("-------Boîte %c%c%c (taille: %lu)-------\n\n",
               champ.type[1], champ.type[2], champ.type[3],
               champ.taille);

        lire_champ(fichier, champ);
        fseek(fichier, champ.position + champ.taille, SEEK_SET);
    }
}


//lire boite meta
void lire_meta(FILE *fichier, long pos, unsigned long taille) {
    box b;
    long fin = pos + taille;

    fseek(fichier, pos + 12, SEEK_SET);

    while (ftell(fichier) < fin && lire_box(fichier, &b)) {
        if (comparateur4(b.type, "ilst")) {
            lire_ilst(fichier, b.position, b.taille);
            return;
        }
        fseek(fichier, b.position + b.taille, SEEK_SET);
    }
}


//lire boite udta
void lire_udta(FILE *fichier, long pos, unsigned long taille) {
    box b;
    long fin = pos + taille;

    fseek(fichier, pos + 8, SEEK_SET);

    while (ftell(fichier) < fin && lire_box(fichier, &b)) {
        if (comparateur4(b.type, "meta")) {
            lire_meta(fichier, b.position, b.taille);
            return;
        }
        fseek(fichier, b.position + b.taille, SEEK_SET);
    }
}





//lire la boite xyz qui contient les données sattelite
void lire_xyz(FILE *fichier, long pos, unsigned long taille) {
    unsigned char header[4];


    fseek(fichier, pos + 8, SEEK_SET);


    if (fread(header, 1, 4, fichier) != 4) return;


    unsigned int taille_texte = ((unsigned int)header[0] << 8) | (unsigned int)header[1];



    if (taille_texte == 0 || taille_texte > taille - 12) {
        printf("xyz (GPS) : donnees invalides\n");
        return;
    }


    unsigned char *buf = malloc(taille_texte + 1);
    if (buf == NULL) return;

    if (fread(buf, 1, taille_texte, fichier) == taille_texte) {
        buf[taille_texte] = '\0';
        printf("xyz (GPS) : %s\n", buf);
    }
    free(buf);
}


//lit xyz si il est dans le moov

void lire_xyz_dans_moov(FILE *fichier, long pos_moov, unsigned long taille_moov) {
    box b;
    long fin_moov = pos_moov + taille_moov;

    fseek(fichier, pos_moov + 8, SEEK_SET);

    while (ftell(fichier) < fin_moov && lire_box(fichier, &b)) {
        if (comparateur4(b.type, "\xA9""xyz")) {
            lire_xyz(fichier, b.position, b.taille);
            return;
        }
        if (comparateur4(b.type, "udta")) {
            box c;
            long fin_udta = b.position + b.taille;
            fseek(fichier, b.position + 8, SEEK_SET);

            while (ftell(fichier) < fin_udta && lire_box(fichier, &c)) {
                if (comparateur4(c.type, "\xA9""xyz")) {
                    lire_xyz(fichier, c.position, c.taille);
                    return;
                }
                fseek(fichier, c.position + c.taille, SEEK_SET);
            }
            }
        fseek(fichier, b.position + b.taille, SEEK_SET);
    }
    printf("boite ©xyz introuvable dans moov/udta\n");
}


//xyz menu
void affichage_xyz_menu(FILE *fichier, int taille)
{
    long pos_moov = rechercheboite(fichier, "moov", taille);
    if (pos_moov < 0) {
        printf("boite moov introuvable\n");
    } else {
         unsigned char t[4];
        fseek(fichier, pos_moov, SEEK_SET);
        fread(t, 1, 4, fichier);
        unsigned long taille_moov = lire_u32(t);

        lire_xyz_dans_moov(fichier, pos_moov, taille_moov);
    }

}

///////////////////////////////////////////////////////////////////////////////////
//affiché le contenu de ftyp

void afficher_contenu_ftyp(FILE *fichier, long position) {
     unsigned char taille2[4];
     unsigned char buf[4];
    unsigned long taille_boite;
    fseek(fichier, position, SEEK_SET);

    // taille
    if (fread(taille2, 1, 4, fichier) != 4) {
        printf("erreur lecture taille ftyp\n");
        return;
    }
    taille_boite = ((unsigned long)taille2[0] << 24) | ((unsigned long)taille2[1] << 16) |
                   ((unsigned long)taille2[2] << 8)  | ((unsigned long)taille2[3]);

    // type
    if (fread(buf, 1, 4, fichier) != 4) {
        printf("erreur lecture type ftyp\n");
        return;
    }
    printf("\n[ftyp] taille = %lu octets\n", taille_boite);

    // major brand
    if (fread(buf, 1, 4, fichier) != 4) {
        printf("erreur lecture major_brand\n");
        return;
    }
    printf("major_brand      : %c%c%c%c\n", buf[0], buf[1], buf[2], buf[3]);

    //version mineur
    if (fread(buf, 1, 4, fichier) != 4) {
        printf("erreur lecture minor_version\n");
        return;
    }
    unsigned long minor_version;
    minor_version  = ((unsigned long)buf[0] << 24);
    minor_version |= ((unsigned long)buf[1] << 16);
    minor_version |= ((unsigned long)buf[2] << 8);
    minor_version |= ((unsigned long)buf[3]);
    printf("minor_version    : %lu\n", minor_version);

    //compatible brand
    unsigned long restant;

    if (taille_boite > 16) {
        restant = taille_boite - 16;
    } else {
        restant = 0;
    }

    printf("compatible_brands:\n");

    while (restant >= 4) {
        if (fread(buf, 1, 4, fichier) != 4)
            break;

        printf("  - %c%c%c%c\n", buf[0], buf[1], buf[2], buf[3]);
        restant -= 4;
    }
}


//ftyp menu
void affichage_ftyp_menu(FILE *fichier, int taille)
{

    long position = rechercheboite(fichier, "ftyp", taille);
    if (position < 0) {
        printf("boite ftyp introuvable\n");
    }
    else {
        affichagev1(fichier, position);
        printf("\n");
        afficher_contenu_ftyp(fichier, position);
    }

}



// udata menu
void affichage_udata_menu(FILE *fichier,long taille){

long pos = rechercheboite(fichier, "udta", taille);
    if (pos < 0) {
        printf("udta introuvable\n");
        }
    else {
         unsigned char t[4];
        fseek(fichier, pos, SEEK_SET);
        fread(t, 1, 4, fichier);
        unsigned long taille_udta = lire_u32(t);

        lire_udta(fichier, pos, taille_udta);
    }
}



///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////



// detection type de tracks

int type_track(FILE *fichier, long pos_trak, unsigned long taille_trak, int parametre) {
    box box_mdia;
    long fin_trak = pos_trak + taille_trak;

    fseek(fichier, pos_trak + 8, SEEK_SET);

    while (ftell(fichier) < fin_trak && lire_box(fichier, &box_mdia)) {
        if (comparateur4(box_mdia.type, "mdia")) {
            box sub;
            long fin_mdia = box_mdia.position + box_mdia.taille;
            fseek(fichier, box_mdia.position + 8, SEEK_SET);

            while (ftell(fichier) < fin_mdia && lire_box(fichier, &sub)) {
                if (comparateur4(sub.type, "hdlr")) {
                     unsigned char tmp[12];
                    fseek(fichier, sub.position + 8, SEEK_SET);
                    if (fread(tmp, 1, 12, fichier) != 12) return 0;

                    unsigned char handler[5];
                    handler[0] = tmp[8];
                    handler[1] = tmp[9];
                    handler[2] = tmp[10];
                    handler[3] = tmp[11];
                    handler[4] = '\0';

                    if (comparateur4(handler, "vide")) return 1;
                    if (comparateur4(handler, "soun")) return 2;
                    if (comparateur4(handler, "sbtl") || comparateur4(handler, "text")) {
                        if(parametre == 2){
                            main_recuperation_st(fichier,box_mdia,fin_mdia);
                            return 3;
                            }
                        return 3;
                        }
                    return 0;
                }
                fseek(fichier, sub.position + sub.taille, SEEK_SET);
            }
            return 0;
        }
        fseek(fichier, box_mdia.position + box_mdia.taille, SEEK_SET);
    }
    return 0;
}


//comptage nombre track
void resume_tracks(FILE *fichier, long pos_moov, unsigned long taille_moov, int parametre) {

    box b;
    long fin_moov = pos_moov + taille_moov;
    int total = 0, nb_video = 0, nb_audio = 0, nb_subt = 0;

    fseek(fichier, pos_moov + 8, SEEK_SET);

    while (ftell(fichier) < fin_moov && lire_box(fichier, &b)) {
        if (comparateur4(b.type, "trak")) {
            total++;
            int t = type_track(fichier, b.position, b.taille,parametre);
            if (t == 1) nb_video++;
            else if (t == 2) nb_audio++;
            else if (t == 3) nb_subt++;
        }
        fseek(fichier, b.position + b.taille, SEEK_SET);
    }

    printf("Tracks totaux : %d (video=%d, audio=%d, sous-titres=%d)\n",
           total, nb_video, nb_audio, nb_subt);
}


/////////////////////////////////////////////

void affichagev1(FILE *fichier, long position){

     unsigned char taille2[4];
    unsigned char lettre[5] = {0};

    fseek(fichier,position,SEEK_SET);
    if (fread(taille2, 1, 4, fichier) != 4) {
        fprintf(stderr, "lecture taille échouée\n");
        fclose(fichier);
        return;
    }

    unsigned long taille_boite =((unsigned long)taille2[0] << 24) | ((unsigned long)taille2[1] << 16) |((unsigned long)taille2[2] << 8)  | ((unsigned long)taille2[3] & 0xFF);


    fread(lettre,4, 1, fichier);

    if(taille_boite ==1){
        unsigned char grossetaille[8];
        fread(grossetaille, 1, 8, fichier);
        unsigned long long vraie_taille =((unsigned long long)grossetaille[0] << 56) |((unsigned long long)grossetaille[1] << 48) |((unsigned long long)grossetaille[2] << 40) |((unsigned long long)grossetaille[3] << 32) | ((unsigned long long)grossetaille[4] << 24) | ((unsigned long long)grossetaille[5] << 16) |((unsigned long long)grossetaille[6] << 8)  |((unsigned long long)grossetaille[7]);
        printf("taille : %lu : boite",vraie_taille);
    }
    else{printf("taille : %llu : boite ",taille_boite);}

    for(int i =0; i< 4;i++ ){
        printf("%c",lettre[i]);
    }

}

unsigned long track_affichage_modif_menu(FILE *fichier,unsigned long taille,int parametre) {
        long pos_moov = rechercheboite(fichier, "moov", taille);
        if (pos_moov < 0) {
            printf("boite moov introuvable\n");
        } else {
            unsigned char t[4];
            fseek(fichier, pos_moov, SEEK_SET);
            fread(t, 1, 4, fichier);
            unsigned long taille_moov = lire_u32(t);
            if(parametre == 2){
            }
            resume_tracks(fichier, pos_moov, taille_moov,parametre);
}
}
/////////////////////////////////////////////////////////////////////////////////////////////////////

//affichage tout depuis le fichier gtkMP4

void afficher_tout(char *nomfichier){


FILE *fichier = ouverturefichier(nomfichier);
if (fichier == NULL){
    printf("erreur ouverture fichier\n");
}

    fseek(fichier, 0, SEEK_END);
    long taille = ftell(fichier);

    affichage_udata_menu(fichier,taille);
    affichage_xyz_menu(fichier,taille);
    affichage_ftyp_menu(fichier,taille);
    fclose(fichier);

}



