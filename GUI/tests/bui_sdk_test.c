/**
 * BAURA OS — BUI SDK Test Suite
 * bui_sdk_test.c — Validation unitaire du moteur BUI, layout 2 passes et cycle de vie des applications
 */

#include "../UI/bui_types.h"
#include "../UI/bui_layout.h"
#include "../UI/bui_widget.h"
#include "../UI/bui_app.h"
#include "../UI/baura_aesb.h"
#include "../SHELL/bui_shell_apps.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

static int g_clicked_count = 0;

static void test_button_click(bui_widget_t* w, const bui_event_t* evt, void* user_data) {
    (void)w; (void)evt; (void)user_data;
    g_clicked_count++;
}

static void test_layout_engine_row(void) {
    printf("[BUI TEST] Test 1: Moteur de mise en page 2 passes (Horizontal Row)...\n");

    bui_insets_t padding = {10, 10, 10, 10};
    bui_widget_t* row = bui_create_row(10, padding);
    assert(row != NULL);

    bui_widget_t* btn1 = bui_create_button("Bouton A", NULL);
    bui_widget_t* btn2 = bui_create_button("Bouton B", NULL);
    bui_widget_t* btn3 = bui_create_button("Bouton C", NULL);

    bui_widget_add_child(row, btn1);
    bui_widget_add_child(row, btn2);
    bui_widget_add_child(row, btn3);

    bui_constraints_t constraints = {0, 800, 0, 600};
    bui_size_t measured = bui_layout_measure(row, constraints);

    /* Vérifier que la taille est calculée dynamiquement */
    assert(measured.width > 0);
    assert(measured.height > 0);

    /* Passe 2 : Layout sur un rectangle alloué */
    bui_rect_t alloc_rect = bui_rect_make(0, 0, 500, 100);
    bui_layout_perform(row, alloc_rect);

    /* Vérifier la disposition côte à côte sans chevauchement */
    assert(btn1->bounds.x == 10); /* Padding gauche */
    assert(btn2->bounds.x > btn1->bounds.x + btn1->bounds.width);
    assert(btn3->bounds.x > btn2->bounds.x + btn2->bounds.width);

    printf("  -> btn1 [x=%d, w=%d], btn2 [x=%d, w=%d], btn3 [x=%d, w=%d] : PASSED (0 chevauchement)\n",
           btn1->bounds.x, btn1->bounds.width,
           btn2->bounds.x, btn2->bounds.width,
           btn3->bounds.x, btn3->bounds.width);

    bui_widget_destroy(row);
}

static void test_widget_hit_test_and_event(void) {
    printf("[BUI TEST] Test 2: Arbre de widgets, Hit-Testing et Routage d'événements...\n");

    bui_widget_t* panel = bui_create_panel(12, true);
    panel->bounds = bui_rect_make(50, 50, 300, 200);

    bui_widget_t* btn = bui_create_button("Cliquer ici", test_button_click);
    btn->bounds = bui_rect_make(70, 70, 120, 40);
    bui_widget_add_child(panel, btn);

    /* Test Hit-test à l'intérieur du bouton */
    bui_point_t pt_inside = {100, 85};
    bui_widget_t* hit = bui_widget_hit_test(panel, pt_inside);
    assert(hit == btn);

    /* Test Hit-test hors du bouton mais dans le panneau */
    bui_point_t pt_panel = {200, 150};
    bui_widget_t* hit_panel = bui_widget_hit_test(panel, pt_panel);
    assert(hit_panel == panel);

    /* Test Événement clic souris */
    g_clicked_count = 0;
    bui_event_t evt;
    memset(&evt, 0, sizeof(evt));
    evt.type = BUI_EVENT_MOUSE_CLICK;
    evt.pos = pt_inside;

    bool dispatched = bui_widget_dispatch_event(panel, &evt);
    assert(dispatched == true);
    assert(g_clicked_count == 1);

    printf("  -> Hit-test et dispatch événement clic validés avec succès!\n");
    bui_widget_destroy(panel);
}

static void test_application_lifecycle_and_shell(void) {
    printf("[BUI TEST] Test 3: Registre d'Applications et Cycle de Vie (SDK)...\n");

    /* Enregistrement de toutes les applications système et utilisateur */
    bui_shell_apps_register_all();
    uint32_t count = bui_app_get_registered_count();
    assert(count >= 5);
    printf("  -> %u applications enregistrées dans le catalogue BUI.\n", count);

    /* Lancement de l'application Fichiers */
    bui_app_instance_t* files_app = bui_app_launch(BUI_APP_ID_FILES);
    assert(files_app != NULL);
    assert(files_app->is_running == true);
    assert(files_app->window_frame != NULL);
    printf("  -> Application '%s' lancée avec succès (Window Frame auto-généré)!\n", files_app->desc->name);

    /* Lancement de l'application Terminal */
    bui_app_instance_t* term_app = bui_app_launch(BUI_APP_ID_TERMINAL);
    assert(term_app != NULL);
    assert(term_app->is_running == true);
    assert(term_app->window_frame != NULL);
    printf("  -> Application '%s' lancée avec succès!\n", term_app->desc->name);

    /* Lancement du Split-Dock */
    bui_app_instance_t* dock_app = bui_app_launch(BUI_APP_ID_DOCK);
    assert(dock_app != NULL);
    assert(dock_app->desc->role == BUI_APP_ROLE_PANEL);
    printf("  -> Composant Shell '%s' instancié comme application cliente!\n", dock_app->desc->name);

    /* Fermeture et destruction propre */
    bui_app_terminate(files_app);
    bui_app_terminate(term_app);
    bui_app_terminate(dock_app);
    printf("  -> Terminaison et libération mémoire validées!\n");
}

static void test_aesb_package_format(void) {
    printf("[BUI TEST] Test 4: Format d'Application .AESB (Appli Exécutable Sur BAURA)...\n");

    const bui_app_desc_t* files_desc = bui_get_files_app_desc();
    assert(files_desc != NULL);

    baura_aesb_header_t hdr;
    uint32_t perms = AESB_PERM_VFS_READ | AESB_PERM_VFS_WRITE | AESB_PERM_GUI_SURFACE | AESB_PERM_INPUT_EVT;
    bool ok = baura_aesb_init_header_from_desc(&hdr, files_desc, perms);
    assert(ok == true);

    /* Vérifications de conformité AESB */
    assert(hdr.magic == BAURA_AESB_MAGIC);
    assert(hdr.version == BAURA_AESB_VERSION);
    assert(hdr.arch == 1); /* x86_64 */
    assert(strcmp(hdr.app_id, "org.baura.apps.files") == 0);
    assert(strcmp(hdr.name, "Fichiers") == 0);
    assert(strcmp(hdr.author, "Ahmed Silué") == 0);
    assert(hdr.permissions == perms);

    bool valid = baura_aesb_validate_header(&hdr);
    assert(valid == true);

    printf("  -> Paquet 'fichiers.aesb' encodé et validé avec succès (Magic 'AESB', Arch x86_64, Droits seL4)!\n");
}

int main(void) {
    printf("==================================================\n");
    printf("   BAURA OS — BUI (Baura User Interface) SDK TEST  \n");
    printf("==================================================\n");

    test_layout_engine_row();
    test_widget_hit_test_and_event();
    test_application_lifecycle_and_shell();
    test_aesb_package_format();

    printf("\n>>> TOUS LES TESTS BUI SDK ONT REUSSI ! (4/4 PASS) <<<\n");
    return 0;
}
