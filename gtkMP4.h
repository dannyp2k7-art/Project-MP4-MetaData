//
// Created by danny on 24/01/2026.
//
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
//#include "gtkMP4.c"

#ifndef GTKMP4_H

static void printf_hex_simple(const char *format, ...);

void afficher_texte_format(const char *format, ...);
int lancerGTK(int argc, char **argv);
void open_callback(GtkNativeDialog *native, int response);
void on_open_clicked(GtkButton *button, gpointer data);
void on_side_btn_clicked(GtkButton *button, gpointer data);
void activate(GtkApplication *app, gpointer user_data);



#define GTKMP4_H

#endif //MAIN_C_GTKMP4_H