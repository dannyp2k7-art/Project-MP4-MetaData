#ifndef RECUPERATION_ST_H
#define RECUPERATION_ST_H

#include <stdio.h>
#include <stdlib.h>
#include "main.h"
#include "modification_meta.h"

/* --- Structures --- */

// Structure pour stocker les informations d'un chunk
typedef struct {
    unsigned int premier_chunk;
    unsigned int nb_sample;
    unsigned int type;
} Structure_chunk;

// Structure pour la durée des samples (corrigée par rapport au .c)
typedef struct {
    unsigned long nb_sample;
    unsigned long duree;
} Structure_duree;

/* --- Prototypes des fonctions --- */

// Utilitaire de concaténation de buffer
void buffer_concat(char *buffer, const char *ajout, size_t taille_ajout, size_t taille_max);

// Récupération du format de sous-titre (stsd)
void recuperation_format_st(FILE *fichier, box box_stbl, long fin_stbl, char *format);

// Récupération des tailles de samples multiples (stsz)
unsigned int *recuperation_taille_sample_multiple(FILE *fichier, unsigned long nb_sample, box box_stsz);

// Récupération du nombre et de la taille des samples
unsigned int *recuperation_taille_et_nombre_sample(FILE *fichier, box box_stbl, long fin_stbl, unsigned long *nb_sample, unsigned long *taille_sample);

// Récupération des offsets des chunks (stco)
unsigned int *recuperation_offset_chunk(FILE *fichier, box box_stbl, long fin_stbl, unsigned long *nb_chunk);

// Récupération de la relation entre samples et chunks (stsc)
Structure_chunk *recuperation_relation_sample_dans_chunk(FILE *fichier, box box_stbl, long fin_stbl, unsigned long *nb_structure);

// Extraction du texte brut des sous-titres
char* recuperation_sous_titre_brute(FILE *fichier, char format[5], unsigned int *tableau_sample, unsigned int nb_sample, unsigned int *tableau_offset_chunk, unsigned int nb_chunk, Structure_chunk *structure_chunk, unsigned int nb_structure);

// Fonction principale de gestion des sous-titres
void main_recuperation_st(FILE *fichier, box box_mdia, long fin_mdia);

#endif /* RECUPERATION_ST_H */
