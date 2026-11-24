#ifndef MODIFICATION_META_H
#define MODIFICATION_META_H

#include <stdio.h>
#include <stdlib.h>

#define MAX_META 16
#define MAX_KEY  5
#define MAX_TEXTE 100

//structupe pour sotocker les métadonnées que l'on veut modifer
typedef struct Metadata{
char nom_meta[4];
char texte[MAX_TEXTE];
unsigned int longueur;
struct Metadata *suivant;
} Metadata;


typedef struct MetadataExistante{
char nom_meta[4];
unsigned long taille_boite;
unsigned long offset_de_ilst;
unsigned long taille_texte;
unsigned long type_data;
unsigned long local;
unsigned char *texte;
struct MetadataExistante *suivant;
}   MetadataExistante;

//////////////////////////////
// Fonctions utilitaires

void saisie_texte(unsigned char *dest, int taille);
unsigned long recherche_placement(FILE *fichier, long position);
void ecrire_u32(unsigned long valeur, unsigned char buf[4]);
unsigned long rechercher_boite_char(unsigned char *moov_temporaire, unsigned char recherche[], unsigned long taille, unsigned long compteur);

//////////////////////////////
// Fonctions sur Metadata

Metadata *creer_noeud_metadata( char *texte, unsigned char *nom_meta, unsigned int longueur);
Metadata *ajout_en_tete_metadata(Metadata* metadata, const char *texte, char *nom_meta, unsigned int longueur);

MetadataExistante *recuperation_metadata_dans_boite_ilst(unsigned char *ilst_avant, unsigned long taille_ilst);
MetadataExistante *creation_nouvelle_metadata(MetadataExistante *liste_metadata_existante, Metadata *temporaire);

unsigned char *reconstruction_ilst(MetadataExistante *liste_metadata_existante, unsigned long *n_taille_ilst);
unsigned char *reconstruction_moov(unsigned char *ilst_nouveau, unsigned char *ancien_moov, unsigned long position_ilst, unsigned long taille_moov, unsigned long n_taille_ilst, unsigned long taille_ilst);
unsigned char *modification_et_ajout_metadata_dans_moov(Metadata *metadata, unsigned char *moov_temporaire, unsigned long taille_moov, unsigned long *n_taille_moov, int nettoyage);

//////////////////////////////
// Fonctions fichier

unsigned char *creation_copie_moov(FILE *fichier, unsigned long taille, unsigned long fin_moov, unsigned long position_moov, unsigned long taille_moov);
int changement_taille_boite(unsigned char boite[], FILE *fichier, unsigned long taille, long difference_taille_moov);
int remplacement_moov_dans_fichier(FILE *fichier, unsigned long taille, unsigned char *nouveau_moov, unsigned long position_moov, unsigned long n_taille_moov, unsigned long taille_moov);

//////////////////////////////
// Fonctions principales

void main_modification_nettoyage(FILE *fichier, unsigned long taille, Metadata *metadata, int nettoyage);
void menu_modification(FILE *fichier, unsigned long taille);

#endif // MODIFICATION_META_H
