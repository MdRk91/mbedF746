#include <mbed.h>
#include <threadLvgl.h>
#include <cstdio>

// Déclaration des objets globaux
ThreadLvgl threadLvgl(30);  
AnalogIn lumSensor(A1);     // Capteur analogique connecté à A1 pour mesurer la luminosité ambiante
PwmOut led(D9);             // Sortie PWM connectée à D9 pour contrôler l'intensité de la LED

// Déclaration des éléments de l'interface LVGL
static lv_obj_t* labelAccueil;     // Label pour le message d'accueil
static lv_obj_t* labelLuminosite;  // Label pour afficher la luminosité ambiante
static lv_obj_t* labelIntensite;   // Label pour afficher l'intensité de la LED
static lv_obj_t* sliderIntensite;  // Slider pour régler l'intensité de la LED en mode manuel
static bool modeAuto = false;      // Booléen pour indiquer le mode automatique
static bool modeSelected = false;  // Booléen pour indiquer si un mode a été sélectionné

// Variables pour suivre les dernières valeurs affichées
static float lastLuminosite = -1;  // Dernière valeur de la luminosité affichée
static float lastIntensite = -1;   // Dernière valeur de l'intensité affichée

// Styles LVGL
static lv_style_t style_bg;      // Style pour le fond d'écran
static lv_style_t style_slider;  // Style pour le slider
static lv_style_t style_btn;     // Style pour les boutons

// Déclarations des fonctions
void update_auto_page(float valeurCapteur);
void slider_event_cb(lv_event_t * e);
void return_event_cb(lv_event_t * e);
void create_auto_page(lv_obj_t* parent);
void create_manual_page(lv_obj_t* parent);
void switch_page(lv_event_t * e);

// Fonction pour initialiser les styles
void init_styles() {
    // Initialisation du style pour le fond
    lv_style_init(&style_bg);
    lv_style_set_bg_color(&style_bg, lv_color_hex(0xCCFFCC)); // Vert pâle

    // Initialisation du style pour le slider
    lv_style_init(&style_slider);
    lv_style_set_bg_color(&style_slider, lv_color_hex(0x000000)); 
    lv_style_set_border_color(&style_slider, lv_color_hex(0x000000)); 

    // Initialisation du style pour les boutons
    lv_style_init(&style_btn);
    lv_style_set_bg_color(&style_btn, lv_color_hex(0xf44336)); // Orange
    lv_style_set_border_color(&style_btn, lv_color_hex(0xf44336)); // Orange
}

// Fonction pour mettre à jour l'interface en mode automatique
void update_auto_page(float valeurCapteur) {
    if (modeAuto) {
        // Calculer les pourcentages de luminosité et d'intensité de la LED
        float luminositePct = valeurCapteur * 100;
        float intensitePct = (1.0f - valeurCapteur) * 100;

        // Vérifier si la différence dépasse 2% pour mettre a jour la les valeurs afin de rendre moins sensible
        if (abs(luminositePct - lastLuminosite) >= 2 || abs(intensitePct - lastIntensite) >= 2) {
            threadLvgl.lock(); // Verrouiller le thread LVGL

            // Mettre à jour les labels avec les nouvelles valeurs
            char buffer[50];
            snprintf(buffer, 50, "Luminosite Ambiante: %.2f %%", luminositePct);
            lv_label_set_text(labelLuminosite, buffer);

            snprintf(buffer, 50, "Intensite LED: %.2f %%", intensitePct);
            lv_label_set_text(labelIntensite, buffer);

            threadLvgl.unlock(); // Déverrouiller le thread LVGL

            // Sauvegarder anciennes valeurs
            lastLuminosite = luminositePct;
            lastIntensite = intensitePct;
        }
    }
}

// fct de rappel du slider du mode manuel
void slider_event_cb(lv_event_t * e) {
    if (!modeAuto) {
        lv_obj_t * slider = lv_event_get_target(e);  // Récupérer le slider déclencheur de l'événement
        int16_t value = lv_slider_get_value(slider); // Recuperer la valeur du slider
        char buffer[50];
        snprintf(buffer, 50, "Intensite LED: %d %%", value); // Formater le texte de l'intensité
        threadLvgl.lock(); // Verrouiller le thread LVGL
        lv_label_set_text(labelIntensite, buffer); // Mettre à jour le label avec la nouvelle intensité
        threadLvgl.unlock(); // Déverrouiller le thread LVGL
        led.write((1.0f / 100) * value); // Ajuster  le rapport cyclique en fonction de la valeur du slide
    }
}

// fct de rappel pour l'événement de retour au menu principal
void return_event_cb(lv_event_t * e) {
    lv_obj_t * parent = lv_obj_get_parent(lv_event_get_target(e)); 
    lv_obj_clean(parent); 
    modeSelected = false;  // Réinitialiser la sélection de mode

    // Création du menu principal avec les boutons de sélection de mode
    lv_obj_t * btnm = lv_btnmatrix_create(lv_scr_act());
    static const char * btnm_map[] = { "Automatique", "Manuel", "" };
    lv_btnmatrix_set_map(btnm, btnm_map);
    lv_obj_align(btnm, LV_ALIGN_TOP_MID, 0, 60); // Ajuster l'alignement
    lv_obj_add_event_cb(btnm, switch_page, LV_EVENT_VALUE_CHANGED, NULL);

    // Appliquer le style de fond
    lv_obj_add_style(lv_scr_act(), &style_bg, 0);

    // Ajouter le label d'accueil
    labelAccueil = lv_label_create(lv_scr_act());
    lv_label_set_text(labelAccueil, "Bonjour, veuillez choisir un mode");
    lv_obj_align(labelAccueil, LV_ALIGN_TOP_MID, 0, 20);
}

// Fonction pour créer l'interface de la page automatique
void create_auto_page(lv_obj_t* parent) {
    //  label de luminosité
    labelLuminosite = lv_label_create(parent);
    lv_label_set_text(labelLuminosite, "Luminosite Ambiante: ");
    lv_obj_align(labelLuminosite, LV_ALIGN_TOP_MID, 0, 20);

    //label d'intensité
    labelIntensite = lv_label_create(parent);
    lv_label_set_text(labelIntensite, "Intensite LED: ");
    lv_obj_align(labelIntensite, LV_ALIGN_TOP_MID, 0, 50);

    // Ajouter un bouton pour retourner au menu principal
    lv_obj_t * btnRetour = lv_btn_create(parent);
    lv_obj_add_style(btnRetour, &style_btn, 0); // Appliquer le style du bouton
    lv_obj_set_size(btnRetour, 100, 50);
    lv_obj_align(btnRetour, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_t * labelRetour = lv_label_create(btnRetour);
    lv_label_set_text(labelRetour, "Retour");
    lv_obj_center(labelRetour); // Centrer le label sur le bouton
    lv_obj_add_event_cb(btnRetour, return_event_cb, LV_EVENT_CLICKED, NULL); 
}

// Fonction pour créer l'interface ddu mode manuel
void create_manual_page(lv_obj_t* parent) {
    // Créer et initialiser le slider à 0%
    sliderIntensite = lv_slider_create(parent);
    lv_obj_set_width(sliderIntensite, 200);
    lv_obj_align(sliderIntensite, LV_ALIGN_CENTER, 0, 0);
    lv_slider_set_range(sliderIntensite, 0, 100); // Définir la plage du slider
    lv_slider_set_value(sliderIntensite, 0, LV_ANIM_OFF); // Initialiser la valeur du slider
    lv_obj_add_style(sliderIntensite, &style_slider, LV_PART_MAIN); // Appliquer le style du slider
    lv_obj_add_event_cb(sliderIntensite, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL); // Associer le callback

    // Créer et aligner le label d'intensité
    labelIntensite = lv_label_create(parent);
    lv_label_set_text(labelIntensite, "Intensite LED: 0 %");
    lv_obj_align(labelIntensite, LV_ALIGN_CENTER, 0, 50);

    // Ajouter un bouton pour retourner au menu principal
    lv_obj_t * btnRetour = lv_btn_create(parent);
    lv_obj_add_style(btnRetour, &style_btn, 0); // Appliquer le style du bouton
    lv_obj_set_size(btnRetour, 100, 50);
    lv_obj_align(btnRetour, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_t * labelRetour = lv_label_create(btnRetour);
    lv_label_set_text(labelRetour, "Retour");
    lv_obj_center(labelRetour); // Centrer le label sur le bouton
    lv_obj_add_event_cb(btnRetour, return_event_cb, LV_EVENT_CLICKED, NULL); 
}

// changer de page en fonction du mode sélectionné
void switch_page(lv_event_t * e) {
    lv_obj_t * btn = lv_event_get_target(e); // Récupérer l'objet déclencheur
    uint32_t id = lv_btnmatrix_get_selected_btn(btn); // Obtenir l'ID du bouton sélectionné
    lv_obj_t * parent = lv_obj_get_parent(btn); 
    lv_obj_clean(parent); 

    modeSelected = true;  // Indiquer qu'un mode a été sélectionné

    if (id == 0) {
        modeAuto = true;  // Activer le mode automatique
        create_auto_page(parent); // Créer la page automatique
    } else {
        modeAuto = false; // Activer le mode manuel
        create_manual_page(parent); // Créer la page manuelle
    }
}

// Fonction principale
int main() {
    threadLvgl.lock(); // Verrouiller le thread LVGL

    // Initialiser les styles
    init_styles();

    // Appliquer le style de fond
    lv_obj_add_style(lv_scr_act(), &style_bg, 0);

    // Ajouter le label d'accueil
    labelAccueil = lv_label_create(lv_scr_act());
    lv_label_set_text(labelAccueil, "Bonjour, veuillez choisir un mode");
    lv_obj_align(labelAccueil, LV_ALIGN_TOP_MID, 0, 20);

    // Création du menu principal avec les boutons de sélection de mode
    lv_obj_t * btnm = lv_btnmatrix_create(lv_scr_act());
    static const char * btnm_map[] = { "Automatique", "Manuel", "" };
    lv_btnmatrix_set_map(btnm, btnm_map);
    lv_obj_align(btnm, LV_ALIGN_TOP_MID, 0, 60); // Ajuster l'alignement
    lv_obj_add_event_cb(btnm, switch_page, LV_EVENT_VALUE_CHANGED, NULL);

    threadLvgl.unlock(); // Déverrouiller le thread LVGL

    led.period(0.001f); // Définir une période de 1 ms pour la sortie PWM
    while (1) {
        if (modeSelected) { // Vérifier si un mode est sélectionné
            float valeurCapteur = lumSensor.read(); // Lire la valeur du capteur de luminosité
            if (modeAuto) {
                float luminositeLed = 1.0f - valeurCapteur; // Calculer l'intensité de la LED en mode automatique
                led.write(luminositeLed); // Ajuster l'intensité de la LED avec le PWM
                update_auto_page(valeurCapteur); // Mettre à jour la page automatique
            }
            printf("lumi %f\n\r", led.read()); // Afficher la valeur de luminosité dans le terminal
        }
        ThisThread::sleep_for(10ms); // Attendre 10 ms 
    }
}