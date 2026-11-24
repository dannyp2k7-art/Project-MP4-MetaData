#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include "Virustotal.h"
#include "main.h"
#include "modification_meta.h"
#include "analyse_meta.h"

// --- VARIABLES GLOBALES ---
static GtkTextView *g_text_view = NULL;
static GtkStack *g_stack = NULL;
static char g_fichier_path[256] = {0};
static GtkWidget *g_entries[9]; // Widgets GtkEntry
static char *g_entries_text[9] = {NULL}; // Textes extraits du fichier

// --- FONCTION EXTRACTION MÉTADONNÉES ---
void lire_metadata_pour_form(FILE *fichier, long taille) {
    // Libérer anciennes allocations
    for (int i = 0; i < 9; i++) {
        if (g_entries_text[i]) {
            free(g_entries_text[i]);
            g_entries_text[i] = NULL;
        }
    }

    long pos_udta = rechercheboite(fichier, "udta", taille);
    if (pos_udta < 0) {
        return;
    }

    unsigned char t[4];
    fseek(fichier, pos_udta, SEEK_SET);
    fread(t, 1, 4, fichier);
    unsigned long taille_udta = lire_u32(t);

    // Codes des boîtes
    unsigned char codes[9][4] = {
        {'\xA9', 'n', 'a', 'm'},  // Titre
        {'\xA9', 'A', 'R', 'T'},  // Auteur
        {'\xA9', 'a', 'l', 'b'},  // Album
        {'\xA9', 'c', 'm', 't'},  // Commentaire
        {'\xA9', 'w', 'r', 't'},  // Compositeur
        {'d', 'e', 's', 'c'},     // Description
        {'l', 'd', 'e', 's'},     // Synopsis
        {'\xA9', 'g', 'e', 'n'},  // Genre
        {'\xA9', 'c', 'p', 'y'}   // Copyright
    };

    fseek(fichier, pos_udta + 8, SEEK_SET);
    box b;
    long fin_udta = pos_udta + taille_udta;

    while (ftell(fichier) < fin_udta && lire_box(fichier, &b)) {
        if (comparateur4(b.type, "meta")) {
            fseek(fichier, b.position + 12, SEEK_SET);
            box ilst_box;
            long fin_meta = b.position + b.taille;

            while (ftell(fichier) < fin_meta && lire_box(fichier, &ilst_box)) {
                if (comparateur4(ilst_box.type, "ilst")) {
                    fseek(fichier, ilst_box.position + 8, SEEK_SET);
                    box champ;
                    long fin_ilst = ilst_box.position + ilst_box.taille;

                    while (ftell(fichier) < fin_ilst && lire_box(fichier, &champ)) {
                        fseek(fichier, champ.position + 8, SEEK_SET);
                        box data;

                        if (lire_box(fichier, &data) && comparateur4(data.type, "data")) {
                            // CORRECTION ICI : Sauter 8 octets (Flags + Locale)
                            fseek(fichier, 8, SEEK_CUR);
                            unsigned long taille_texte = data.taille > 16 ? data.taille - 16 : 0;

                            if (taille_texte > 0) {
                                unsigned char *texte = malloc(taille_texte + 1);
                                if (texte) {
                                    size_t lu = fread(texte, 1, taille_texte, fichier);
                                    texte[lu] = '\0';

                                    char *clean_texte = malloc(lu + 1);
                                    if (clean_texte) {
                                        int j = 0;
                                        for (size_t k = 0; k < lu; k++) {
                                            if (texte[k] >= 32 && texte[k] <= 126) {
                                                clean_texte[j++] = texte[k];
                                            }
                                        }
                                        clean_texte[j] = '\0';

                                        for (int idx = 0; idx < 9; idx++) {
                                            if (comparateur4(champ.type, codes[idx])) {
                                                g_entries_text[idx] = my_strdup(clean_texte);
                                                break;
                                            }
                                        }
                                        free(clean_texte);
                                    }
                                    free(texte);
                                }
                            }
                        }
                        fseek(fichier, champ.position + champ.taille, SEEK_SET);
                    }
                    break;
                }
                fseek(fichier, ilst_box.position + ilst_box.taille, SEEK_SET);
            }
            break;
        }
        fseek(fichier, b.position + b.taille, SEEK_SET);
    }
}

// --- UTILITAIRES ---
void effacer_texte() {
    if (!g_text_view) return;
    GtkTextBuffer *buf = gtk_text_view_get_buffer(g_text_view);
    gtk_text_buffer_set_text(buf, "", -1);
}

void afficher_texte_format(const char *format, ...) {
    if (!g_text_view) {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
        return;
    }
    char buffer[4096];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    gchar *texte_utf8 = g_utf8_make_valid(buffer, -1);
    GtkTextBuffer *buf = gtk_text_view_get_buffer(g_text_view);
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buf, &end);
    gtk_text_buffer_insert(buf, &end, texte_utf8, -1);
    g_free(texte_utf8);
    GtkTextMark *mark = gtk_text_buffer_create_mark(buf, NULL, &end, FALSE);
    gtk_text_view_scroll_to_mark(g_text_view, mark, 0.0, FALSE, 0.0, 0.0);
    gtk_text_buffer_delete_mark(buf, mark);
}

// --- PAGE CONFIRMATION NETTOYAGE ---
static void on_nettoyage_confirm(GtkButton *btn, gpointer data) {
    FILE *fichier = ouverturefichier(g_fichier_path);
    if (!fichier) {
        gtk_stack_set_visible_child_name(g_stack, "page_log");
        afficher_texte_format("❌ Erreur ouverture fichier !\n");
        return;
    }

    fseek(fichier, 0, SEEK_END);
    long taille = ftell(fichier);

    Metadata *metadata_vide = NULL;
    main_modification_nettoyage(fichier, taille, metadata_vide, 1);
    fclose(fichier);

    gtk_stack_set_visible_child_name(g_stack, "page_log");
    afficher_texte_format("🧹 Nettoyage TERMINÉ ! Métadonnées-Non essentiel supprimées.\n");
}

static void on_annuler_nettoyage(GtkButton *btn, gpointer data) {
    gtk_stack_set_visible_child_name(g_stack, "page_log");
    afficher_texte_format("Nettoyage annulé.\n");
}

static GtkWidget* creer_page_confirmation() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_margin_start(box, 50);
    gtk_widget_set_margin_end(box, 50);

    GtkWidget *titre = gtk_label_new("Confirmer le Nettoyage");
    gtk_widget_add_css_class(titre, "title-label");
    gtk_box_append(GTK_BOX(box), titre);

    GtkWidget *warning = gtk_label_new("⚠️  TOUTES les métadonnées utilisateurs seront SUPPRIMÉES définitivement.\n\nCette action est IRREVERSIBLE !");
    gtk_widget_set_halign(warning, GTK_ALIGN_CENTER);
    PangoAttrList *attrs = pango_attr_list_new();
    pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    gtk_label_set_attributes(GTK_LABEL(warning), attrs);
    gtk_box_append(GTK_BOX(box), warning);

    GtkWidget *btns_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    gtk_widget_set_halign(btns_box, GTK_ALIGN_CENTER);

    GtkWidget *btn_oui = gtk_button_new_with_label("✅ OUI, Nettoyer !");
    gtk_widget_add_css_class(btn_oui, "open-btn");
    g_signal_connect(btn_oui, "clicked", G_CALLBACK(on_nettoyage_confirm), NULL);

    GtkWidget *btn_non = gtk_button_new_with_label("❌ Annuler");
    gtk_widget_add_css_class(btn_non, "side-btn");
    g_signal_connect(btn_non, "clicked", G_CALLBACK(on_annuler_nettoyage), NULL);

    gtk_box_append(GTK_BOX(btns_box), btn_non);
    gtk_box_append(GTK_BOX(btns_box), btn_oui);
    gtk_box_append(GTK_BOX(box), btns_box);

    return box;
}

// --- PAGE MODIFICATION ---
static void on_valider_modif(GtkButton *btn, gpointer data) {
    if (g_fichier_path[0] == '\0') {
        gtk_stack_set_visible_child_name(g_stack, "page_log");
        afficher_texte_format("⚠️ ERREUR : Aucun fichier ouvert !\n");
        return;
    }

    int compteur = 0;
    const char *codes[] = {"\xA9" "nam", "\xA9" "ART", "\xA9" "alb", "\xA9" "cmt", "\xA9" "wrt", "desc", "ldes", "\xA9" "gen", "\xA9" "cpy"};
    const char *labels[] = {"Titre", "Auteur", "Album", "Commentaire", "Compositeur", "Description", "Synopsis", "Genre", "Copyright"};

    effacer_texte();
    afficher_texte_format("\n--- Résumé des modifications ---\n");

    FILE *fichier = ouverturefichier(g_fichier_path);
    if (!fichier) {
        afficher_texte_format("❌ Erreur ouverture fichier\n");
        return;
    }

    fseek(fichier, 0, SEEK_END);
    long taille = ftell(fichier);
    Metadata *metadata = NULL;

    for (int i = 0; i < 9; i++) {
        GtkEditable *editable = GTK_EDITABLE(g_entries[i]);
        const char *texte = gtk_editable_get_text(editable);

        if (texte && my_strlen(texte) > 0) {
            metadata = ajout_en_tete_metadata(metadata, texte, codes[i], my_strlen(texte));
            compteur++;
            afficher_texte_format(" + Ajout : %s -> %s\n", labels[i], texte);
        }
    }

    if (compteur == 0) {
        fclose(fichier);
        gtk_stack_set_visible_child_name(g_stack, "page_log");
        afficher_texte_format("⚠️ Aucun champ modifié. Rien à faire.\n");
        return;
    }

    main_modification_nettoyage(fichier, taille, metadata, 0);
    fclose(fichier);

    // Libérer mémoire
    for (int i = 0; i < 9; i++) {
        if (g_entries_text[i]) {
            free(g_entries_text[i]);
            g_entries_text[i] = NULL;
        }
    }

    gtk_stack_set_visible_child_name(g_stack, "page_log");
    afficher_texte_format("✅ %d modification(s) appliquée(s) avec succès !\n", compteur);

    for (int i = 0; i < 9; i++) {
        gtk_editable_set_text(GTK_EDITABLE(g_entries[i]), "");
    }
}

static GtkWidget* creer_page_formulaire() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_bottom(box, 20);
    gtk_widget_set_margin_start(box, 30);
    gtk_widget_set_margin_end(box, 30);

    GtkWidget *titre = gtk_label_new("📝 Modifier les Métadonnées");
    GtkStyleContext *context = gtk_widget_get_style_context(titre);
    gtk_style_context_add_class(context, "title-label");
    gtk_box_append(GTK_BOX(box), titre);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 20);
    gtk_widget_set_halign(grid, GTK_ALIGN_CENTER);

    const char *labels[] = {"Titre :", "Auteur :", "Album :", "Commentaire :", "Compositeur :", "Description :", "Synopsis :", "Genre :", "Copyright :"};
    const char *placeholders[] = {"...", "...", "...", "...", "...", "...", "...", "...", "..."};

    for (int i = 0; i < 9; i++) {
        GtkWidget *lbl = gtk_label_new(labels[i]);
        gtk_widget_set_halign(lbl, GTK_ALIGN_END);
        gtk_grid_attach(GTK_GRID(grid), lbl, 0, i, 1, 1);

        GtkWidget *entry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(entry), placeholders[i]);
        gtk_widget_set_size_request(entry, 350, -1);
        gtk_grid_attach(GTK_GRID(grid), entry, 1, i, 1, 1);

        g_entries[i] = entry;
    }

    gtk_box_append(GTK_BOX(box), grid);

    GtkWidget *btn_valider = gtk_button_new_with_label("💾 Appliquer les modifications");
    gtk_widget_add_css_class(btn_valider, "open-btn");
    gtk_widget_set_halign(btn_valider, GTK_ALIGN_CENTER);
    g_signal_connect(btn_valider, "clicked", G_CALLBACK(on_valider_modif), NULL);

    gtk_box_append(GTK_BOX(box), btn_valider);

    return box;
}

// --- BOUTONS SIDEBAR ---
static void on_side_btn_clicked(GtkButton *button, gpointer data) {
    int action = GPOINTER_TO_INT(data);

    if (g_fichier_path[0] == '\0') {
        gtk_stack_set_visible_child_name(g_stack, "page_log");
        effacer_texte();
        afficher_texte_format("⚠️ ERREUR : Veuillez d'abord ouvrir un fichier MP4 !");
        return;
    }

    FILE *fichier = NULL;

    switch (action) {
        case 0: // MetaData Affichage
            gtk_stack_set_visible_child_name(g_stack, "page_log");
            effacer_texte();
            afficher_texte_format("====== ANALYSE MÉTADONNÉES ======\n\n");

            fichier = ouverturefichier(g_fichier_path);
            if (fichier) {
                fseek(fichier, 0, SEEK_END);
                long taille = ftell(fichier);
                affichage_ftyp_menu(fichier, taille);
                affichage_udata_menu(fichier, taille);
                affichage_xyz_menu(fichier, taille);
                fclose(fichier);
            }
            afficher_texte_format("\n====== FIN ANALYSE ======\n");
            break;

        case 1: // Modification
            fichier = ouverturefichier(g_fichier_path);
            if (fichier) {
                fseek(fichier, 0, SEEK_END);
                long taille = ftell(fichier);
                lire_metadata_pour_form(fichier, taille);
                fclose(fichier);
            }

            gtk_stack_set_visible_child_name(g_stack, "page_form");

            // Préremplir les champs
            for (int i = 0; i < 9; i++) {
                if (g_entries[i] && g_entries_text[i]) {
                    gtk_editable_set_text(GTK_EDITABLE(g_entries[i]), g_entries_text[i]);
                } else if (g_entries[i]) {
                    gtk_editable_set_text(GTK_EDITABLE(g_entries[i]), "");
                }
            }
            break;

        case 2: // Nettoyage
            gtk_stack_set_visible_child_name(g_stack, "page_confirm");
            effacer_texte();
            afficher_texte_format("====== NETTOYAGE ======\n");
            break;

        case 3: // Malware
            gtk_stack_set_visible_child_name(g_stack, "page_log");
            effacer_texte();
            afficher_texte_format("====== SCAN MALWARE ======\n");
            afficher_texte_format("Analyse en cours...\n");
            virus_analyse(g_fichier_path);
            break;

        case 4: // Tracks
            gtk_stack_set_visible_child_name(g_stack, "page_log");
            effacer_texte();
            afficher_texte_format("====== TRACKS ======\n");
            fichier = ouverturefichier(g_fichier_path);
            if (fichier) {
                fseek(fichier, 0, SEEK_END);
                long taille = ftell(fichier);
                track_affichage_modif_menu(fichier, taille, 2);
                fclose(fichier);
            }
            break;
    }
}

// --- OUVERTURE FICHIER ---
static void open_callback(GtkNativeDialog *native, int response) {
    if (response == GTK_RESPONSE_ACCEPT) {
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(native);
        GFile *file = gtk_file_chooser_get_file(chooser);
        if (file) {
            char *path = g_file_get_path(file);
            my_strcpy(g_fichier_path, path);

            gtk_stack_set_visible_child_name(g_stack, "page_log");
            effacer_texte();
            afficher_texte_format("✅ Fichier chargé : %s\n", path);
            afficher_texte_format("👉 Sélectionnez 'Affichage MetaData' ou 'Modification'.\n");

            g_free(path);
            g_object_unref(file);
        }
    }
    g_object_unref(native);
}

static void on_open_clicked(GtkButton *button, gpointer data) {
    GtkWindow *parent = GTK_WINDOW(data);
    GtkFileChooserNative *native = gtk_file_chooser_native_new("Ouvrir MP4", parent, GTK_FILE_CHOOSER_ACTION_OPEN, "Ouvrir", "Annuler");
    g_signal_connect(native, "response", G_CALLBACK(open_callback), NULL);
    gtk_native_dialog_show(GTK_NATIVE_DIALOG(native));
}

// --- INIT GTK ---
static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_default_size(GTK_WINDOW(window), 1400, 850);
    gtk_window_set_title(GTK_WINDOW(window), "G-D MP4 Analyseur");

    GtkCssProvider *provider = gtk_css_provider_new();
    const char *css =
        "window { background: #F5F7F4; color: #21412A; font-family: 'Segoe UI'; } "
        ".sidebar-box { background: linear-gradient(180deg, #E4F3E8, #D4EBDE); padding: 16px; } "
        ".side-btn { background: transparent; color: #1E4D32; border-radius: 10px; padding: 12px; margin: 6px 0; font-weight: 600; } "
        ".side-btn:hover { background: #CBE4D2; } "
        ".open-btn { background: #2E7D32; color: #fff; padding: 12px 28px; border-radius: 20px; font-weight: bold; } "
        ".center-zone { background: #fff; border-radius: 14px; padding: 20px; font-family: 'Consolas', monospace; } "
        "textview text { font-family: 'Consolas', monospace; }";

    gtk_css_provider_load_from_data(provider, css, -1);
    gtk_style_context_add_provider_for_display(gdk_display_get_default(), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    GtkWidget *main_layout = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    // Sidebar
    GtkWidget *sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_add_css_class(sidebar, "sidebar-box");
    gtk_widget_set_size_request(sidebar, 260, -1);

    const char *btn_labels[] = {"Affichage MetaData", "Modification", "Nettoyage", "Analyse Malware", "Affichage Tracks"};
    for (int i = 0; i < 5; i++) {
        GtkWidget *btn = gtk_button_new_with_label(btn_labels[i]);
        gtk_widget_add_css_class(btn, "side-btn");
        g_signal_connect(btn, "clicked", G_CALLBACK(on_side_btn_clicked), GINT_TO_POINTER(i));
        gtk_box_append(GTK_BOX(sidebar), btn);
    }

    // Contenu Central
    GtkWidget *content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_widget_set_hexpand(content_box, TRUE);
    gtk_widget_set_margin_start(content_box, 40);
    gtk_widget_set_margin_end(content_box, 40);
    gtk_widget_set_margin_top(content_box, 40);
    gtk_widget_set_margin_bottom(content_box, 40);

    // GESTION DES ECRANS
    g_stack = GTK_STACK(gtk_stack_new());
    gtk_stack_set_transition_type(g_stack, GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);

    // Page 1 - Log
    GtkWidget *scrolled_log = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scrolled_log, TRUE);
    gtk_widget_add_css_class(scrolled_log, "center-zone");

    GtkWidget *text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD_CHAR);
    g_text_view = GTK_TEXT_VIEW(text_view);

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_log), text_view);
    gtk_stack_add_named(g_stack, scrolled_log, "page_log");

    // Page 2 - Formulaire
    GtkWidget *form_modif = creer_page_formulaire();
    GtkWidget *scrolled_form = gtk_scrolled_window_new();
    gtk_widget_add_css_class(scrolled_form, "center-zone");
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_form), form_modif);
    gtk_stack_add_named(g_stack, scrolled_form, "page_form");

    // Page 3 - Confirmation
    GtkWidget *confirm_nettoyage = creer_page_confirmation();
    GtkWidget *scrolled_confirm = gtk_scrolled_window_new();
    gtk_widget_add_css_class(scrolled_confirm, "center-zone");
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_confirm), confirm_nettoyage);
    gtk_stack_add_named(g_stack, scrolled_confirm, "page_confirm");

    gtk_box_append(GTK_BOX(content_box), GTK_WIDGET(g_stack));

    // Bouton Ouvrir
    GtkWidget *open_btn = gtk_button_new_with_label("📂 Ouvrir un fichier");
    gtk_widget_add_css_class(open_btn, "open-btn");
    gtk_widget_set_halign(open_btn, GTK_ALIGN_END);
    g_signal_connect(open_btn, "clicked", G_CALLBACK(on_open_clicked), window);
    gtk_box_append(GTK_BOX(content_box), open_btn);

    // Assemblage
    gtk_box_append(GTK_BOX(main_layout), sidebar);
    gtk_box_append(GTK_BOX(main_layout), content_box);
    gtk_window_set_child(GTK_WINDOW(window), main_layout);

    gtk_stack_set_visible_child_name(g_stack, "page_log");
    effacer_texte();
    afficher_texte_format("Bienvenue dans G-D MP4 !\nCommencez par ouvrir un fichier.");

    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("fr.gd.mp4", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
