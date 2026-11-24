#include <stdio.h>
#include <stdlib.h>
#include "main.h"
#include "modification_meta.h"

//Ligne permettant de tranformer les printfs en fonctions permettant d'afficher dans la box GTK (afficher_texte_format garde les mêmes arguments qu'un printf)
#define printf(...) afficher_texte_format(__VA_ARGS__)

//structure pour stocker toutes les infos d'un chunk pour recupéré les données brutes
typedef struct {
unsigned int premier_chunk;
unsigned int nb_sample;
unsigned int type;
}Structure_chunk;


typedef struct {
unsigned long nb_sample;
unsigned long duree;
}Structure_timecode;
// fonction strinf concat
void buffer_concat(char *buffer, const char *ajout, size_t taille_ajout, size_t taille_max) {

    size_t i = 0;
    while (i < taille_max && buffer[i] != '\0') {
        i++;
    }

    size_t j = 0;
    while (i < taille_max - 1 && j < taille_ajout) {
        buffer[i] = ajout[j];
        i++;
        j++;
    }
    buffer[i] = '\0';
}







//////////////////////////////////////////////////////////////////////////////////////////////

//recuperation sous-titre.

//////////////////////////////////////////////////////////////////////////////////////////////

// recuperation format du sous-titre
void recuperation_format_st(FILE *fichier,box box_stbl, long fin_stbl,char *format){

    fseek(fichier, box_stbl.position + 8, SEEK_SET);
    box box_stsd;


    while (ftell(fichier) < fin_stbl && lire_box(fichier, &box_stsd)) {
        if (comparateur4(box_stsd.type, "stsd")) {
            fseek(fichier,box_stsd.position+20,SEEK_SET);
            fread(format,1,4,fichier);
            printf("format :%c%c%c%c\n",format[0],format[1],format[2],format[3]);
            return;
            }
       fseek(fichier, box_stsd.position + box_stsd.taille, SEEK_SET);
        }

    printf("erreur, pas trouver la boite stsd\n");
}


//////////////////////////////////////////////////////////////////////////////////////////////

// recuperation taille des samples

//code si la taille n'est pas la même pour tout les sample

unsigned int *recuperation_taille_sample_multiple(FILE *fichier, unsigned long nb_sample, box box_stsz){

    unsigned int *tableau_sample = malloc(sizeof(unsigned int) * nb_sample);
    if (tableau_sample == NULL){  //teste malloc
        printf("erreur allocatation malloc\n");
        return NULL;
    }
    char recuperation[5];

    fseek(fichier,box_stsz.position+20,SEEK_SET);

    for( int i =0 ; i < nb_sample; i++ ) {
        fread(recuperation,4,1,fichier);
        tableau_sample[i] = lire_u32(recuperation);
    }
    return tableau_sample;
}



//////////////////////////////////////////////////////////////////////////////////////////////

// code principal pour la recuperation de la taille et du nombre de sample
unsigned int *recuperation_taille_et_nombre_sample(FILE *fichier,box box_stbl, long fin_stbl, unsigned long *nb_sample, unsigned long *taille_sample){

    box box_stsz;
    char recuperation[5];
    unsigned int *tableau_sample = NULL;
    fseek(fichier, box_stbl.position + 8, SEEK_SET);

    while (ftell(fichier) < fin_stbl && lire_box(fichier, &box_stsz)) {
        if (comparateur4(box_stsz.type, "stsz")) {



            //nombre sample
        fseek(fichier,box_stsz.position+16,SEEK_SET);
        fread(recuperation,4,1,fichier);
        *nb_sample =lire_u32(recuperation);
        printf("nombre de sample : %lu\n",*nb_sample);

        //taille	sample
        fseek(fichier,box_stsz.position+12,SEEK_SET);
        fread(recuperation,4,1,fichier);
        *taille_sample =lire_u32(recuperation);

	// cas ou la taille est la même partout
        if(*taille_sample != 0){
            tableau_sample = malloc(sizeof(unsigned int) * (*nb_sample));
            if (tableau_sample == NULL){
             return NULL;}

            for (unsigned int i = 0; i < *nb_sample; i++) {
                tableau_sample[i] = (unsigned int)*taille_sample;
            }
            return tableau_sample;
        }
        else {
		// cas on ce n'est pas le cas
                tableau_sample = recuperation_taille_sample_multiple(fichier,*nb_sample,box_stsz);
                return tableau_sample;
        }

        }


fseek(fichier, box_stsz.position + box_stsz.taille, SEEK_SET);
}
printf("erreur, pas trouver la boite stsz\n");
}

//////////////////////////////////////////////////////////////////////////////////////////////

// code recuperation OFFSET des chunk
unsigned int *recuperation_offset_chunk(FILE *fichier,box box_stbl,long fin_stbl, unsigned long *nb_chunk){

fseek(fichier, box_stbl.position + 8, SEEK_SET);
box box_stco;
char recuperation[5];


while (ftell(fichier) < fin_stbl && lire_box(fichier, &box_stco)) {
    if (comparateur4(box_stco.type, "stco")) {

        fseek(fichier,box_stco.position+12,SEEK_SET);

        fread(recuperation,4,1,fichier);
        *nb_chunk = lire_u32(recuperation);



        unsigned int *tableau_offset_chunk = malloc(sizeof(unsigned int) * *nb_chunk);

        if (tableau_offset_chunk == NULL){
        printf("erreur allocatation malloc\n");
        return NULL;
        }

        fseek(fichier,box_stco.position+16,SEEK_SET);

        for( int i =0 ; i < *nb_chunk; i++ ) {
            fread(recuperation,4,1,fichier);
            tableau_offset_chunk[i] = lire_u32(recuperation);
        }
        return tableau_offset_chunk;
    }
fseek(fichier, box_stco.position + box_stco.taille, SEEK_SET);
}
printf("erreur, pas trouver la boite stsd\n");
}


//////////////////////////////////////////////////////////////////////////////////////////////

// code recuperation strucuture chunk / sample (position, nombre sample dans chunk,...)

Structure_chunk *recuperation_relation_sample_dans_chunk(FILE *fichier,box box_stbl,long fin_stbl, unsigned long *nb_structure){

fseek(fichier, box_stbl.position + 8, SEEK_SET);

box box_stsc;
char recuperation[5];


while (ftell(fichier) < fin_stbl && lire_box(fichier, &box_stsc)) {
    if (comparateur4(box_stsc.type, "stsc")) {

        fseek(fichier,box_stsc.position+12,SEEK_SET);

        fread(recuperation,4,1,fichier);
        *nb_structure = lire_u32(recuperation);

        Structure_chunk *structure_chunk = malloc(sizeof(Structure_chunk) * *nb_structure);

        if (structure_chunk == NULL){
        printf("erreur allocatation malloc\n");
        return NULL;
        }

        for( int i =0 ; i < *nb_structure; i++ ) {

	    //offset 1er chunk
            fread(recuperation,4,1,fichier);
            structure_chunk[i].premier_chunk = lire_u32(recuperation);

	     //nombre de sample dans ce chunk
            fread(recuperation,4,1,fichier);
            structure_chunk[i].nb_sample = lire_u32(recuperation);

	    //type de sample
            fread(recuperation,4,1,fichier);
            structure_chunk[i].type = lire_u32(recuperation);
        }
        return structure_chunk;
    }
fseek(fichier, box_stsc.position + box_stsc.taille, SEEK_SET);

}
printf("erreur, pas trouver la boite stsc\n");
return NULL;
}


//////////////////////////////////////////////////////////////////////////////////////////////

// convertit un temps (en unités time_scale) en heures, minutes, secondes, millisecondes
void convert_time(unsigned long long time, unsigned int time_scale,
                  unsigned int *hh, unsigned int *mm, unsigned int *ss, unsigned int *ms) {
    unsigned long long total_ms = (time * 1000) / time_scale;
    *hh = total_ms / 3600000;
    *mm = (total_ms / 60000) % 60;
    *ss = (total_ms / 1000) % 60;
    *ms = total_ms % 1000;
}

// concatène un timecode au format [HH:MM:SS,mmm --> HH:MM:SS,mmm] dans le buffer
void concat_timecode(char *buffer, size_t buffer_max,
                     unsigned long long start_time, unsigned long long end_time, unsigned int time_scale) {

    unsigned int sh, sm, ss, sms;
    unsigned int eh, em, es, ems;

    convert_time(start_time, time_scale, &sh, &sm, &ss, &sms);
    convert_time(end_time, time_scale, &eh, &em, &es, &ems);

    // on ajoute caractère par caractère pour éviter sprintf
    char temp[32]; // assez grand pour contenir le timecode
    int pos = 0;

    // format [HH:MM:SS,mmm --> HH:MM:SS,mmm]
    temp[pos++] = '[';
    temp[pos++] = '0' + (sh / 10); temp[pos++] = '0' + (sh % 10);
    temp[pos++] = ':';
    temp[pos++] = '0' + (sm / 10); temp[pos++] = '0' + (sm % 10);
    temp[pos++] = ':';
    temp[pos++] = '0' + (ss / 10); temp[pos++] = '0' + (ss % 10);
    temp[pos++] = ',';
    temp[pos++] = '0' + (sms / 100); temp[pos++] = '0' + ((sms / 10) % 10); temp[pos++] = '0' + (sms % 10);
    temp[pos++] = ' ';
    temp[pos++] = '-'; temp[pos++] = '>'; temp[pos++] = ' ';
    temp[pos++] = '0' + (eh / 10); temp[pos++] = '0' + (eh % 10);
    temp[pos++] = ':';
    temp[pos++] = '0' + (em / 10); temp[pos++] = '0' + (em % 10);
    temp[pos++] = ':';
    temp[pos++] = '0' + (es / 10); temp[pos++] = '0' + (es % 10);
    temp[pos++] = ',';
    temp[pos++] = '0' + (ems / 100); temp[pos++] = '0' + ((ems / 10) % 10); temp[pos++] = '0' + (ems % 10);
    temp[pos++] = ']';
    temp[pos++] = ' ';

    // ajoute dans le buffer avec ta fonction buffer_concat
    buffer_concat(buffer, temp, pos, buffer_max);
}



//////////////////////////////////////////////////////////////////////////////////////////////
char* recuperation_sous_titre_brute(FILE *fichier, char format[5],
    unsigned int *tableau_sample, unsigned int nb_sample,
    unsigned int *tableau_offset_chunk, unsigned int nb_chunk,
    Structure_chunk *structure_chunk, unsigned int nb_structure,
    Structure_timecode *structure_timecode, unsigned int time_scale,
    unsigned long nb_timecode) {

    // Calculer la taille totale approximative du buffer
    unsigned long taille_totale_texte = 0;
    for (unsigned int i = 0; i < nb_sample; i++) {
        if (tableau_sample[i] > 2)
            taille_totale_texte += (tableau_sample[i] - 2) + 50; // +50 pour timecode et numéro
    }

    char *buffer = malloc(taille_totale_texte + 1);
    if (!buffer) return NULL;
    buffer[0] = '\0';

    unsigned long index_global_sample = 0;
    unsigned long index_timecode = 0;
    unsigned long time_acc = 0; // temps courant en unités time_scale

    unsigned int subtitle_number = 1;

    for (unsigned int i = 0; i < nb_chunk; i++) {
        unsigned int num_chunk_actuel = i + 1;
        unsigned int nb_samples_dans_ce_chunk = 0;

        // Déterminer combien de samples dans ce chunk
        for (unsigned int s = 0; s < nb_structure; s++) {
            if (s + 1 == nb_structure || (num_chunk_actuel >= structure_chunk[s].premier_chunk &&
                                          num_chunk_actuel < structure_chunk[s + 1].premier_chunk)) {
                nb_samples_dans_ce_chunk = structure_chunk[s].nb_sample;
                break;
            }
        }

        fseek(fichier, tableau_offset_chunk[i], SEEK_SET);

        for (unsigned int j = 0; j < nb_samples_dans_ce_chunk; j++) {
            if (index_global_sample >= nb_sample || index_timecode >= nb_timecode) break;

            unsigned int taille_actuelle = tableau_sample[index_global_sample];

            if (taille_actuelle > 2) {
                char *temp = malloc(taille_actuelle);
                if (temp) {
                    fread(temp, 1, taille_actuelle, fichier);

                    // Ajouter numéro de sous-titre
                    char numbuf[12];
                    int pos = 0;
                    unsigned int num = subtitle_number++;
                    while (num > 0) {
                        numbuf[pos++] = '0' + (num % 10);
                        num /= 10;
                    }
                    // inverser les chiffres
                    for (int k = pos - 1; k >= 0; k--) buffer_concat(buffer, &numbuf[k], 1, taille_totale_texte + 1);
                    buffer_concat(buffer, "\n", 1, taille_totale_texte + 1);

                    // Ajouter timecode
                    unsigned long long start_time = time_acc;
                    unsigned long long end_time = time_acc + structure_timecode[index_timecode].duree;
                    concat_timecode(buffer, taille_totale_texte + 1, start_time, end_time, time_scale);

                    // Ajouter texte
                    buffer_concat(buffer, temp + 2, taille_actuelle - 2, taille_totale_texte + 1);
                    buffer_concat(buffer, "\n\n", 2, taille_totale_texte + 1);

                    time_acc = end_time; // avancer le temps
                    index_timecode++;
                    free(temp);
                }
            } else {
                fseek(fichier, taille_actuelle, SEEK_CUR);
            }

            index_global_sample++;
        }
    }

    return buffer;
}


Structure_timecode *recuperation_timecode(FILE *fichier,box box_stbl, long fin_stbl,unsigned long * nb_timecode){
fseek(fichier, box_stbl.position + 8, SEEK_SET);

box box_stts;
char recuperation[4];


while (ftell(fichier) < fin_stbl && lire_box(fichier, &box_stts)) {
    if (comparateur4(box_stts.type, "stts")) {

        fseek(fichier,box_stts.position+12,SEEK_SET);

        fread(recuperation,4,1,fichier);
        *nb_timecode = lire_u32(recuperation);

Structure_timecode *structure_timecode = malloc(sizeof(Structure_timecode) * (*nb_timecode));
if (structure_timecode == NULL){
        printf("erreur allocatation malloc\n");
        return NULL;
        }


        for( int i =0 ; i < *nb_timecode; i++ ) {

	     //nombre de sample avec la meme durée
            fread(recuperation,4,1,fichier);
            structure_timecode[i].nb_sample = lire_u32(recuperation);

        //duree timecode
            fread(recuperation,4,1,fichier);
            structure_timecode[i].duree = lire_u32(recuperation);
        }
        return structure_timecode;
    }
fseek(fichier, box_stts.position + box_stts.taille, SEEK_SET);

}
printf("erreur, pas trouver la boite stsc\n");
return NULL;
}

/////////////////////////////////

//recupérer le timescal
unsigned int recuperation_time_scale(FILE *fichier, box box_mdia, long fin_mdia) {
    box box_mdhd;
    char buffer[5];
    unsigned int time_scale = 1000; // Valeur par défaut au cas où

    fseek(fichier, box_mdia.position + 8, SEEK_SET);

    while (ftell(fichier) < fin_mdia && lire_box(fichier, &box_mdhd)) {
        if (comparateur4(box_mdhd.type, "mdhd")) {

            fseek(fichier, box_mdhd.position + 8, SEEK_SET);
            unsigned char version;
            fread(&version, 1, 1, fichier);

            int offset_time_scale = (version == 1) ? 20 : 12;

            fseek(fichier, box_mdhd.position + 8 + offset_time_scale, SEEK_SET);
            fread(buffer, 4, 1, fichier);
            time_scale = lire_u32(buffer);

            printf("TimeScale extrait : %u\n", time_scale);
            return time_scale;
        }
        fseek(fichier, box_mdhd.position + box_mdhd.taille, SEEK_SET);
    }

    printf("Attention : mdhd non trouvé, utilise TimeScale par défaut (1000)\n");
    return time_scale;
}




void extraction_sous_titre(char *soustitre){

FILE *extraction = fopen("extraction_sous-titre","a");
if(extraction == NULL){
    printf("erreur création fichier");
    return;
}
unsigned long taille = my_strlen(soustitre);
fwrite(soustitre,1,taille,extraction);
fclose(extraction);
}

//////////////////////////////////////////////////////////////////////////////////////////////

//code qui fait office de main pour recupéré les sous-titre
void main_recuperation_st(FILE *fichier,box box_mdia, long fin_mdia) {

box box_minf;
box box_stbl;
char format[5];
unsigned long nb_sample = 0;
unsigned long taille = 0;

unsigned int *tableau_sample = NULL;
unsigned int *tableau_offset_chunk = NULL;

unsigned long nb_timecode;
unsigned long nb_chunk = 0;
unsigned long nb_structure = 0;
Structure_chunk *structure_chunk = NULL;
Structure_timecode *structure_timecode = NULL;

unsigned int timecode = recuperation_time_scale(fichier,box_mdia,fin_mdia);


    fseek(fichier, box_mdia.position + 8, SEEK_SET);

while (ftell(fichier) < fin_mdia && lire_box(fichier, &box_minf)) {
    if (comparateur4(box_minf.type, "minf")) {
        long fin_minf = box_minf.position + box_minf.taille;

        while (ftell(fichier) < fin_minf && lire_box(fichier, &box_stbl)) {
            if (comparateur4(box_stbl.type, "stbl")) {
                long fin_stbl = box_stbl.position + box_stbl.taille;


                recuperation_format_st(fichier,box_stbl, fin_stbl,format); //recuperation format



                tableau_sample = recuperation_taille_et_nombre_sample(fichier,box_stbl, fin_stbl, &nb_sample, &taille);
                if (tableau_sample == NULL){
                    printf("erreur recupération position sample\n");
                    return;
                }


                tableau_offset_chunk = recuperation_offset_chunk(fichier, box_stbl, fin_stbl,&nb_chunk);
                if (tableau_offset_chunk == NULL){
                    printf("erreur recupération offset chunk\n");
                    return;


                }
                structure_chunk = recuperation_relation_sample_dans_chunk(fichier, box_stbl, fin_stbl,&nb_structure);
                if (structure_chunk == NULL){
                    printf("erreur recupération structure sample chunk\n");
                    return;
                }

                structure_timecode = recuperation_timecode(fichier, box_stbl, fin_stbl,&nb_timecode);
                if (structure_timecode == NULL){
                    printf("erreur recupération structure sample timecode\n");
                    return;
                }


                break;
            }
        }
    }
}

char *soustitre = recuperation_sous_titre_brute(fichier,format,tableau_sample,nb_sample,tableau_offset_chunk,nb_chunk,structure_chunk,nb_structure,structure_timecode,timecode,nb_timecode);
printf("%s\n",soustitre);
extraction_sous_titre(soustitre);

free(tableau_sample);
free(tableau_offset_chunk);
free(structure_chunk);
free(structure_timecode);

}




