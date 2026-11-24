#define MAX_META 16
#define MAX_KEY  5
#define MAX_TEXTE 100

#include <stdio.h>
#include <stdlib.h>

#include "main.h"
#include "modification_meta.h"

//Ligne permettant de tranformer les printfs en fonctions permettant d'afficher dans la box GTK (afficher_texte_format garde les mêmes arguments qu'un printf)
#define printf(...) afficher_texte_format(__VA_ARGS__)


unsigned long recherche_placement(FILE *fichier, long position) {
    unsigned char t[4];
    fseek(fichier, position, SEEK_SET);
    if (fread(t, 1, 4, fichier) != 4) return 0;
    return lire_u32(t);
}
/*
//ecrire 32bits
void ecrire_u32(unsigned long valeur, unsigned char buf[4])
{
    buf[0] = (valeur >> 24) & 0xFF;
    buf[1] = (valeur >> 16) & 0xFF;
    buf[2] = (valeur >> 8)  & 0xFF;
    buf[3] = valeur & 0xFF;
}
*/

unsigned long rechercher_boite_char(unsigned char *moov_temporaire,unsigned char recherche[4], unsigned long taille, unsigned long compteur) {   // passer fin du fichier en paramètre ?


while (compteur+3 < taille) {

    if(moov_temporaire[compteur] == recherche[0]){

        if(moov_temporaire[compteur+1] == recherche[1]){

            if(moov_temporaire[compteur+2] == recherche[2]){

                if(moov_temporaire[compteur+3] == recherche[3]){
                    return compteur;

                }
            }
        }
    }
compteur++;
}
return 0;
}



// création noeud pour les métadonnées qu'on veut modifier
Metadata *creer_noeud_metadata( char *texte, unsigned char *nom_meta, unsigned int longueur){
//Test d'allocation
    Metadata *metadata = malloc(sizeof(Metadata));
    if ( metadata == NULL){
        printf("erreur allocation\n");
        return NULL;
        }
//cpy des infos de la structure métadata
    my_strcpy4(metadata->nom_meta,nom_meta,4);
    my_strcpy(metadata->texte,texte);
    metadata->longueur = longueur;
    metadata->suivant = NULL;
    return metadata;
}


///// ajout neoud sur la liste des métadonnées qu'on veut modifier
Metadata *ajout_en_tete_metadata(Metadata* metadata,const char *texte,char *nom_meta, unsigned int longueur){

    Metadata *nouveau=creer_noeud_metadata(texte,nom_meta,longueur);
    if (nouveau == NULL){
        printf("erreur ajout\n");
        return metadata;
    }

    if(metadata == NULL){
    return nouveau;
    }

    nouveau->suivant = metadata;
    return nouveau;
 }


// crée une copie du moov à partir du fichier pour pouvoir ensuite le modifier
unsigned char *creation_copie_moov(FILE *fichier, unsigned long taille, unsigned long fin_moov, unsigned long position_moov, unsigned long taille_moov){

    unsigned char *moov_temporaire = malloc(taille_moov);
    if(moov_temporaire == NULL){
    printf("erreur malloc\n");
    return NULL;
    }

    fseek(fichier, position_moov, SEEK_SET);

    fread(moov_temporaire,taille_moov,1,fichier);
    return moov_temporaire;


}




// on recupère les metadonnées présente dans la boîte ilst pour les stocker dans une liste chainé qu'on renvoie 
MetadataExistante *recuperation_metadata_dans_boite_ilst(unsigned char *ilst_avant,unsigned long taille_ilst){

unsigned long curseur = 8 ;
MetadataExistante *liste_metadata = NULL;
MetadataExistante *dernier = NULL;
unsigned char recuperation[4];


while(curseur +8 <= taille_ilst) {

//Test d'allocation de mémoire
    MetadataExistante *meta = malloc(sizeof(MetadataExistante));
    if(meta == NULL) {
        printf("erreur allocation noeud metadata");
        return liste_metadata; }

    meta->suivant = NULL;

    // taille boite
    meta->taille_boite =0;
    for(int i =0; i < 4; i++){
        recuperation[i] = ilst_avant[curseur+i];
    }

    meta->taille_boite = lire_u32(recuperation);
    if(meta->taille_boite < 8){
    printf("erreur recuperation taille\n");
        free(meta);
        break;
    }

    //nom de la métadonnée  ex @nam
    for(int i =0; i < 4; i++){
        meta->nom_meta[i] = ilst_avant[curseur +4+i];
    }
    //offset de ilst
    meta->offset_de_ilst = curseur;

    //fin boite
    unsigned long curseur_temporaire = curseur +8; //ajoute un curseur temporaire pour s'assurer du bon suivi des boites
    unsigned long fin_boite = curseur + meta->taille_boite;

    while( curseur_temporaire +16 <= fin_boite) {
        //utilation du curseur tmp tant qu'utile au sein de la boucle
        for(int i =0; i < 4; i++){
            recuperation[i] = ilst_avant[curseur_temporaire+i];
        }
        unsigned long taille_data = lire_u32(recuperation);

        if (ilst_avant[curseur_temporaire + 4] == 'd' && ilst_avant[curseur_temporaire + 5] == 'a' && ilst_avant[curseur_temporaire + 6] == 't' && ilst_avant[curseur_temporaire + 7] == 'a') {

            // type data

            for (int i = 0; i < 4; i++) {
            recuperation[i] = ilst_avant[curseur_temporaire + 8 + i]; }
            meta->type_data = lire_u32(recuperation);

            //local data
            for (int i = 0; i < 4; i++){
                recuperation[i] = ilst_avant[curseur_temporaire + 12 + i]; }
            meta->local = lire_u32(recuperation);



                meta->taille_texte = taille_data - 16;

                meta->texte = malloc(meta->taille_texte + 1);  // ici enlever le 1
                if (meta->texte == NULL) {
                    free(meta);
                    return liste_metadata;
                }
        for (unsigned long i = 0; i < meta->taille_texte; i++) {
            meta->texte[i] = ilst_avant[curseur_temporaire + 16 + i];
            }
      //  meta->texte[meta->taille_texte] = '\0';
        break;
        }
        curseur_temporaire = curseur_temporaire +taille_data;
        }

if (liste_metadata == NULL) {
    liste_metadata = meta;
    dernier = meta;
    //si la liste est nulle on s'arrête là
} else {
    dernier->suivant = meta;
    dernier = meta; //sinon on la parcours tant qu'on y est pas
}
    curseur = curseur + meta->taille_boite;
    }
return liste_metadata; //retourne la liste des boite à changer
    }



//ajout en tête d'une metadonée dans le cas où elle ne serait pas déjà présente (lors de la modification de la liste
//qu'on veut utiliser pour reconstruire ilst 
MetadataExistante *creation_nouvelle_metadata( MetadataExistante *liste_metadata_existante,Metadata *temporaire){


MetadataExistante *nouveau = malloc(sizeof(MetadataExistante));
//Test d'allocution
if (nouveau == NULL){
    printf("erreur allocation nouvelle metatdata\n");
    return liste_metadata_existante;
}
//copy le contenu du nom de la metadata vers le nouveau temporaire
my_strcpy4(nouveau->nom_meta,temporaire->nom_meta,4);

nouveau->taille_boite = temporaire->longueur + 24;
nouveau->taille_texte = temporaire->longueur;

nouveau->texte = malloc(temporaire->longueur);
    //Test d'allocution sur le nouveau texte
if (nouveau->texte == NULL){
    printf("erreur allocation malloc pour texte nouvelle meta\n");
    free(nouveau);
    return liste_metadata_existante;
}
    //copy du texte vers le temporaire du texte sur la longueur du texte
my_strcpy4(nouveau->texte,temporaire->texte,temporaire->longueur);
nouveau->offset_de_ilst =0;
nouveau->type_data = 1;
nouveau->local = 0;


nouveau->suivant = liste_metadata_existante;
return nouveau;


}




/// réécriture de la boîte ilst ( dans un buffer) à l'aide de la liste des métadonnées existantes mise à jour 
unsigned char *reconstruction_ilst(MetadataExistante *liste_metadata_existante,unsigned long *n_taille_ilst){
  

    MetadataExistante *temporaire = liste_metadata_existante;
    unsigned long nouvelle_taille = 8;
    unsigned long curseur =0;
 

while(temporaire != NULL){
    nouvelle_taille += 24 + temporaire->taille_texte;;
    temporaire = temporaire->suivant;
}


    unsigned char *ilst_nouveau = malloc(nouvelle_taille);
    if (ilst_nouveau == NULL){
        printf("erreur allcoation ilst_nouveau\n");
        return NULL;
    }

    ecrire_u32(nouvelle_taille, &ilst_nouveau[curseur]); //pour récrire la taille de la ilst en bigendian
    curseur+=4;
    my_strcpy4((unsigned char*)&ilst_nouveau[curseur], "ilst", 4);  //réecriture du nom ilst

    curseur+=4;

    temporaire = liste_metadata_existante;

while(temporaire != NULL) {
//réecriture de toutes les infos de la ilist
        ecrire_u32(temporaire->taille_texte + 24, &ilst_nouveau[curseur]); curseur += 4;
        my_strcpy4((unsigned char*)&ilst_nouveau[curseur], temporaire->nom_meta, 4); curseur += 4;

        ecrire_u32(temporaire->taille_texte + 16, &ilst_nouveau[curseur]); curseur += 4;
        my_strcpy4((unsigned char*)&ilst_nouveau[curseur], "data", 4); curseur += 4;

        ecrire_u32(temporaire->type_data, &ilst_nouveau[curseur]); curseur += 4;
        ecrire_u32(temporaire->local, &ilst_nouveau[curseur]); curseur += 4;

        my_strcpy4((unsigned char*)&ilst_nouveau[curseur], (unsigned char*)temporaire->texte, temporaire->taille_texte);
        curseur += temporaire->taille_texte;

        temporaire = temporaire->suivant;
    }
    *n_taille_ilst = nouvelle_taille;


    // nettoyage / free de la liste
    MetadataExistante *temporaire2;

    while(liste_metadata_existante != NULL) {
        temporaire2 = liste_metadata_existante->suivant;

        if(liste_metadata_existante->texte){
            free(liste_metadata_existante->texte);}

        free(liste_metadata_existante);
        liste_metadata_existante = temporaire2;
    }

    return ilst_nouveau;//renvoie de la nouvelle ilst
}




// reconstruction du moov ( dans un buffer ) a l'aide du nouveau ilst et de l'ancien moov ( pour les données qu'on ne change pas)
unsigned char *reconstruction_moov(unsigned char *ilst_nouveau, unsigned char * ancien_moov,unsigned long position_ilst, unsigned long taille_moov,unsigned long n_taille_ilst, unsigned long taille_ilst){

    unsigned long curseur =0;

    unsigned long n_taille_moov = taille_moov -taille_ilst +n_taille_ilst;

    unsigned char *nouveau_moov = malloc(n_taille_moov);
    if (nouveau_moov == NULL){
        printf("erreur allocation nouveau_moov");
        return NULL;
    }

    my_strcpy4(&nouveau_moov[curseur],&ancien_moov[0],position_ilst);
    curseur = position_ilst;
    my_strcpy4(&nouveau_moov[curseur],&ilst_nouveau[0],n_taille_ilst);
    curseur  += n_taille_ilst;
    my_strcpy4(&nouveau_moov[curseur],&ancien_moov[position_ilst + taille_ilst],taille_moov- (position_ilst + taille_ilst));
    ecrire_u32(n_taille_moov, nouveau_moov);

    return nouveau_moov;
}




// la fonction gère la modification et l'ajout métadonnée en partant de la recuperation de celles existantes dans ilst à la création du nouveau moov dans un buffer
unsigned char *modification_et_ajout_metadata_dans_moov(Metadata *metadata,unsigned char *moov_temporaire,unsigned long taille_moov, unsigned long *n_taille_moov, int nettoyage){

    unsigned long curseur =0;
    unsigned long taille_ilst =0;
    unsigned long position_ilst =0;
    unsigned char recuperation[4];

     curseur = rechercher_boite_char(moov_temporaire,"meta",taille_moov,curseur);
    if (curseur == 0) {
        printf("erreur meta introuvable");
        return NULL;
    }
    
curseur = rechercher_boite_char(moov_temporaire,"ilst",taille_moov,curseur);
    if (curseur == 0) {
        printf("erreur ilst introuvable");
        return NULL;
    }


    position_ilst = curseur;
    curseur = curseur-4;
    for(int i =0; i < 4 ; i++) {
        recuperation[i] = moov_temporaire[curseur+i];
    }

    taille_ilst = lire_u32(recuperation);

    
    position_ilst = curseur;

    if( nettoyage == 1){

        unsigned long n_taille_ilst =8;
        unsigned char *ilst_vide = malloc(8);

        if(ilst_vide == NULL){
            return NULL;
        }

        ecrire_u32(8,ilst_vide);
        my_strcpy4(ilst_vide+4,"ilst",4);
        unsigned char *nouveau_moov = reconstruction_moov(ilst_vide,moov_temporaire, position_ilst, taille_moov,8,taille_ilst);
        if(nouveau_moov == NULL){
        printf("erreur reconstruction moov");
        return NULL;
        }
        free(ilst_vide);
        *n_taille_moov = taille_moov - taille_ilst +8;
        return nouveau_moov;

    }



    unsigned char *ilst_avant = malloc(taille_ilst);
    if (ilst_avant == NULL){
        printf("erreur allocation ilst");
        return NULL;
    }


        
    for (unsigned long i = 0; i < taille_ilst; i++) {
        ilst_avant[i] = moov_temporaire[position_ilst + i];
    }
        

    MetadataExistante *liste_metadata_existante = NULL;
    if(taille_ilst != 8){
    liste_metadata_existante = recuperation_metadata_dans_boite_ilst(ilst_avant,taille_ilst);
    if(liste_metadata_existante == NULL){
        printf("erreur création liste métadonnée existante\n");
        return NULL;
    }
        
}
    Metadata *temporaire = metadata;

    while(temporaire != NULL){
        MetadataExistante *existant_temporaire = liste_metadata_existante;
        int trouve = 0;
        while(existant_temporaire != NULL){
        
         if (existant_temporaire->nom_meta[0] == temporaire->nom_meta[0] &&   existant_temporaire->nom_meta[1] == temporaire->nom_meta[1] &&   existant_temporaire->nom_meta[2] == temporaire->nom_meta[2] && existant_temporaire->nom_meta[3] == temporaire->nom_meta[3 ]) {

              if (existant_temporaire->taille_texte < temporaire->longueur) {
                   unsigned char *securite = realloc(existant_temporaire->texte, temporaire->longueur);
                   if (securite == NULL){
                    return NULL;}
                   existant_temporaire->texte = securite;
              }

    my_strcpy4(existant_temporaire->texte, temporaire->texte, temporaire->longueur);

    existant_temporaire->taille_texte = temporaire->longueur;
    existant_temporaire->taille_boite = 24 + temporaire->longueur;
        trouve = 1;
        break;
        }
        existant_temporaire = existant_temporaire->suivant;
        }

        if (trouve == 0){
            liste_metadata_existante = creation_nouvelle_metadata(liste_metadata_existante,temporaire);
                    
        }

    temporaire = temporaire->suivant;
    }
    
    unsigned long n_taille_ilst=0;
    unsigned char *ilst_nouveau = reconstruction_ilst(liste_metadata_existante,&n_taille_ilst);


    if (ilst_nouveau == NULL){
        printf("erreur nouveau ilst");
        return NULL;
    }
    
    unsigned char *nouveau_moov = reconstruction_moov(ilst_nouveau, moov_temporaire,position_ilst,taille_moov,n_taille_ilst,taille_ilst);
    free(ilst_nouveau);
    if( nouveau_moov == NULL){
        printf("erreur moov\n");
        return NULL;
    }


 // on met à jour les tailles parent   
 long difference = (long)n_taille_ilst - (long)taille_ilst;

// ici meta
unsigned long pos_meta = rechercher_boite_char((unsigned char*)nouveau_moov, "meta", *n_taille_moov, 0);
if(pos_meta >= 4) {
    unsigned long t_meta = lire_u32(&nouveau_moov[pos_meta-4]);
    ecrire_u32(t_meta + difference, &nouveau_moov[pos_meta-4]);
}

// ici udta
unsigned long pos_udta = rechercher_boite_char((unsigned char*)nouveau_moov, "udta", *n_taille_moov, 0);
if(pos_udta >= 4) {
    unsigned long t_udta = lire_u32(&nouveau_moov[pos_udta-4]);
    ecrire_u32(t_udta + difference, &nouveau_moov[pos_udta-4]);
}

 *n_taille_moov = taille_moov -taille_ilst +n_taille_ilst;
  free(ilst_avant);
return nouveau_moov;
}


/////////////////////////////////////////////////////////////////////
/*
int changement_taille_boite(unsigned char boite[],FILE *fichier,unsigned long taille,long difference_taille_moov){
    unsigned long nouvelle_taille=0;
    unsigned char recuperation[4];
    long position_fichier_boite = rechercheboite(fichier,boite,taille);
    if (position_fichier_boite == -1){
        printf("erreur position boite");
        return -1;

    }
    fseek(fichier, position_fichier_boite, SEEK_SET);
    fread(recuperation,4,1,fichier);

    nouvelle_taille = lire_u32(recuperation);
    nouvelle_taille += (difference_taille_moov);

    ecrire_u32(nouvelle_taille,recuperation);
    fseek(fichier,position_fichier_boite,SEEK_SET);
    fwrite(recuperation,4,1,fichier);
}
*/

// ici on vient remplacer le moov dans le fichier ( qu'il soit avant mdat ou après mdta) in gère aussi le "nettoyage" pour que la vidéo soit toujours utilisable 
int remplacement_moov_dans_fichier(FILE *fichier, unsigned long taille, unsigned char *nouveau_moov, unsigned long position_moov, unsigned long n_taille_moov, unsigned long taille_moov) {

    long position_mdat = rechercheboite(fichier, "mdat", taille);
    if (position_mdat < 0) {
        printf("Erreur : position mdat introuvable\n");
        return -1;
    }


    // cas où moioù est avant mdat
    if (position_moov > (unsigned long)position_mdat) {
        if (fseek(fichier, position_moov, SEEK_SET) != 0) {
            printf("Erreur : fseek vers moov impossible\n");
            return -1;
        }

        size_t ecrit = fwrite(nouveau_moov, 1, n_taille_moov, fichier);
        if (ecrit != n_taille_moov) {
            return -1;
        }

       // cas où le moov est plus petit on remplit avec une boîte free ( vide)
        if (n_taille_moov < taille_moov) {
            unsigned long reste = taille_moov - n_taille_moov;
            if (reste >= 8) {
                unsigned char free_box[8];
                ecrire_u32(reste, free_box);
                my_strcpy4((unsigned char*)&free_box[4], "free", 4);
                fwrite(free_box, 1, 8, fichier);

                unsigned char zero = 0;
                for(unsigned long i = 0; i < (reste - 8); i++) {
                    fwrite(&zero, 1, 1, fichier);
                }
            }
        }
        printf("Moov remplacé avec succès\n");

    }
    
    //  cas ou le moov est avant le mdat
    else {
        //on remplacé l'ancien moov par free qu'on remplit de 0 ( comme avant)
        fseek(fichier, position_moov, SEEK_SET);
        unsigned char free_box[8];
        ecrire_u32(taille_moov, free_box);
        my_strcpy4((unsigned char*)&free_box[4], "free", 4);
        fwrite(free_box, 1, 8, fichier);

        unsigned char buffer_zero[4096] = {0};
        unsigned long reste_remplissage = taille_moov - 8;

        while (reste_remplissage > 0) {
            unsigned long a_ecrire;
            if (reste_remplissage > 4096) {
                a_ecrire = 4096;
            }
            else {
                a_ecrire = reste_remplissage;
            }
            fwrite(buffer_zero, 1, a_ecrire, fichier);
            reste_remplissage -= a_ecrire;
        }

        // on écrit le.moov à la fin
        fseek(fichier, 0, SEEK_END);
        size_t ecrit = fwrite(nouveau_moov, 1, n_taille_moov, fichier);
        if (ecrit != n_taille_moov) {
            return -1;
        }
        printf("Moov déplacé à la fin du fichier\n");
    }

    free(nouveau_moov);
    return 0;
}




//fonction qui est appelé depuis gtk et fait office de main pour la modification 
void main_modification_nettoyage(FILE *fichier,unsigned long taille, Metadata *metadata, int nettoyage){

if( metadata == NULL && nettoyage == 0){
    printf("erreur aucune donnée a modifier\n");
    return;
}

// recuperation info moov
long position_moov = rechercheboite(fichier, "moov", taille);
    if (position_moov < 0) {
        printf("boite moov introuvable\n");
        return;
        printf("moov trouvé\n");
    }
    unsigned char recuperation[4];
    fseek(fichier, position_moov, SEEK_SET);
    fread(recuperation, 1, 4, fichier);
    unsigned long taille_moov = lire_u32(recuperation);
    if ( taille_moov > taille){
    printf("erreur taille moov\n");
    return;
    }
    unsigned long fin_moov =  position_moov + taille_moov;

    unsigned char *moov_temporaire = creation_copie_moov(fichier,taille,fin_moov,position_moov,taille_moov);
    if(moov_temporaire == NULL){
    return;
    }

    unsigned long n_taille_moov;


    unsigned char *nouveau_moov = modification_et_ajout_metadata_dans_moov(metadata,moov_temporaire,taille_moov,&n_taille_moov,nettoyage);
                

if(nouveau_moov == NULL){
    return;
}
   remplacement_moov_dans_fichier(fichier,taille,nouveau_moov,position_moov,n_taille_moov,taille_moov);

free(moov_temporaire);
}
