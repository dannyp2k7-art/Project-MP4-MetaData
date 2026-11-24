#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

typedef struct {
    unsigned long taille;
    char type[5];
    long position;
} box;


void afficher_tout(char *nomfichier);

unsigned long my_strlen(const unsigned char *s);
unsigned char *my_strcpy(unsigned char *dest, const unsigned char *src);
unsigned char *my_strcpy4(unsigned char *dest, const  unsigned char *src, size_t n);

void viderbuffer();
void lire_udta(FILE *fichier, long pos, unsigned long taille);
unsigned char *my_strdup(const unsigned char *s);
void affichagev1(FILE *fichier, long position);

int comparateur4(char unsigned a[4], char unsigned b[4]);

int saisie_et_verification_int(int mini, int maxi);

void saisie_et_verification_char(char *nomfichier);

void afficher_contenu_ftyp(FILE *fichier, long position);

void affichage_ftyp_menu(FILE *fichier, int taille);

FILE *ouverturefichier(char nom[50]);

unsigned long lire_u32(unsigned char b[4]);

int egal4(unsigned char a[4],unsigned char b[4]);

int lire_box(FILE *f, box *b);

int confirmateur(FILE *fichier,unsigned char mot[]);

void lire_champ(FILE *f, box champ);

unsigned long rechercheboite(FILE *fichier,unsigned char recherche[], unsigned long taille);

void lire_ilst(FILE *f, long pos, unsigned long taille);
void lire_meta(FILE *f, long pos, unsigned long taille);
void lire_udta(FILE *f, long pos, unsigned long taille);

long rechercher_boite(FILE *f, unsigned char nom[4], long taille_fichier);
void afficher_texte_format(const char *format, ...);
void affichage_xyz_menu(FILE *fichier, int taille);
void affichage_udata_menu(FILE *fichier,long taille);
unsigned long track_affichage_modif_menu(FILE *fichier,unsigned long taille,int parametre);
void menu(FILE *fichier);

#endif // MAIN_H_INCLUDED
