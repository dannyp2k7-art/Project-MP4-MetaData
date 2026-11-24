  ___  ____       _  _  ____   ___ 
 / __)(    \ ___ ( \/ )(  _ \ / _ \
( (_ \ ) D ((___)/ \/ \ ) __/(__  (
 \___/(____/     \_)(_/(__)    (__/



---BIENVENUE---



Fonctionnalités :

Ouvrir MP4		Sélecteur de fichiers 
MetaData		ftyp, udta, xyz_ complet
Modifier		9 champs (Titre, Auteur, Album...)
Nettoyage		Supprime TOUTES métadonnées
Malware			Scan VirusTotal API
Tracks			Audio/Video sous titre




DEPENDANCE

# Ubuntu/Debian

sudo apt install libcurl4-openssl-dev
sudo apt install libjansson-dev
sudo apt install libssl-dev
sudo apt install libgtk-4-dev pkg-config

# MSYS2 (Windows)
pacman -S mingw-w64-ucrt-x86_64-gtk4 \
  mingw-w64-ucrt-x86_64-curl mingw-w64-ucrt-x86_64-openssl \
  mingw-w64-ucrt-x86_64-jansson mingw-w64-ucrt-x86_64-gcc \
  mingw-w64-ucrt-x86_64-pkg-config make git




compilation :

gcc -o gdmp4 main.c gtkMP4.c Virustotal.c analyse_meta.c modification_meta.c $(pkg-config --cflags --libs gtk4) -lcurl -ljansson -lssl -lcrypto





mp4_analyseur/
├──  gtkMP4.c              # Interface GTK4 principale
├──  modification_meta.c   # Édition métadonnées MP4
├──  modification_meta.h   # Structures Metadata
├──  main.c                # Parsing MP4 (ftyp/udta/ilst)
├──  Virustotal.c          # API VirusTotal
└──  readme.txt            # Ce fichier





G-D MP4 Analyseur - 2026

Auteur : Danny PEREIRA/Gregoire DUPEREY
