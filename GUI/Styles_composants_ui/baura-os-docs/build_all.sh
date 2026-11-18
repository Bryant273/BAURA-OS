#!/usr/bin/env bash
# Compile, teste et régénère TOUS les rendus (00 → 14 ; 12 et 13 demandent cargo) contre le dépôt BAURA-OS, SANS le modifier ni rien pousser.
# Usage : BAURA_OS_REPO=/chemin/vers/BAURA-OS ./build_all.sh [doc...]      ex. : ./build_all.sh 06 08
# Prérequis : g++ (C++17), python3 + Pillow (avec Raqm pour re-cuire les polices), polices Inter / DejaVu.
set -euo pipefail
REPO="${BAURA_OS_REPO:-$HOME/baura-os}"; HERE="$(cd "$(dirname "$0")" && pwd)"; BGE="$REPO/GUI/BGE"; FW="$REPO/GUI/UI_FRAMEWORK"
FLAGS="-std=c++17 -O2 -Wall -Wextra ${STRICT:+-Werror}"   # STRICT=1 : mêmes exigences que `make test-gui` du dépôt (-Werror) ; les docs 04–09 y satisfont
SRC="$BGE/renderer.cpp $BGE/renderer_core.cpp $BGE/effects_engine.cpp"
WANT="${*:-00 01 02 03 04 05 06 07 08 09 11 12 13 14}"; want() { [[ " $WANT " == *" $1 "* ]]; }
TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT
INTER=/usr/share/fonts/opentype/inter; DJV=/usr/share/fonts/truetype/dejavu
bake() { python3 "$HERE/02-TEXTE/tools/bake_font.py" "$@"; }
if [ -z "${SKIP_FONTS:-}" ]; then   # polices cuites (une seule fois)
  bake "$HERE/common/inter.bgf" $INTER/Inter-Regular.otf $INTER/Inter-SemiBold.otf
  bake "$HERE/common/mono.bgf" $DJV/DejaVuSansMono.ttf $DJV/DejaVuSansMono-Bold.ttf --sizes=12,13,14
  bake "$HERE/common/arabic.bgf" $DJV/DejaVuSans.ttf $DJV/DejaVuSans-Bold.ttf --ranges=060C-060C,061B-061B,061F-061F,0621-064A,0660-0669,FE70-FEFC --sizes=12,14,16,20,28
fi
run() {  # run <dossier> <source sans .cpp> [sources/inclusions en plus...] ; compile puis exécute dans <dossier>/code
  local d="$1" t="$2"; shift 2; ( cd "$HERE/$d/code" && g++ $FLAGS -I"$HERE/common" -I. -I"$BGE" -I"$FW" "$t.cpp" $SRC "$@" -o "$TMP/$t" && "$TMP/$t" ${RUNARGS:-} ) ; }
png() { ( cd "$HERE/$1/code" && python3 "$HERE/common/tools/ppm2png.py" ../render "${@:2}" ); }

want 00 && { ( cd "$HERE/00-AUDIT-GUI/code" && g++ -std=c++17 -O2 -I"$HERE/common" -I"$REPO/GUI" -I"$REPO/BOOT/INSTALLER" -I"$REPO" capture_current.cpp \
    $REPO/GUI/DESKTOP/baura_desktop.cpp $REPO/GUI/LOGIN_SCREEN/login_screen.cpp $REPO/BOOT/INSTALLER/installer_gui.cpp $FW/baura_ui_toolkit.cpp $FW/baura_hybrid_taskbar.cpp \
    $FW/papirus_icon_catalog.cpp $FW/system_bubble.cpp $FW/ui_extensions.cpp $BGE/ui_framework.cpp $BGE/window_manager.cpp $BGE/gpu_accel_3d.cpp $SRC -o "$TMP/cap" && "$TMP/cap" && python3 "$HERE/common/tools/ppm2png.py" ../render $(ls *.ppm | sed "s/\.ppm//" | sed "s/^ui_\(.*\)/&=actuel_\1/") ); echo "OK 00"; }
for d in 01-IDENTITE-ET-VECTEUR:test_identity:identite-avant-apres 02-TEXTE:test_text:texte-avant-apres 03-VERRE-ET-OMBRES:test_glass:verre-avant-apres; do
  IFS=: read dir bin out <<<"$d"; want "${dir:0:2}" && { RUNARGS=../../common/inter.bgf run "$dir" "$bin" >/dev/null; png "$dir" "$(basename "$(ls "$HERE/$dir/code"/*.ppm | head -1)" .ppm)=$out"; echo "OK $dir"; }
done
want 04 && { run 04-COMPOSANTS test_components; png 04-COMPOSANTS comp_light=composants_light comp_dark=composants_dark; run 04-COMPOSANTS test_textedit; echo "OK 04"; }
want 05 && { cp -r "$REPO/GUI" "$TMP/GUI"; ( cd "$TMP" && patch -s -p1 < "$HERE/05-MOUVEMENT/patches/0001-window-manager-expose-state.patch" )   # copie PATCHÉE : le dépôt n'est pas touché
  ( cd "$HERE/05-MOUVEMENT/code" && g++ $FLAGS -I"$HERE/common" -I"$TMP/GUI/BGE" -I"$TMP/GUI/UI_FRAMEWORK" test_motion.cpp $TMP/GUI/BGE/renderer.cpp $TMP/GUI/BGE/renderer_core.cpp $TMP/GUI/BGE/effects_engine.cpp $TMP/GUI/BGE/window_manager.cpp -o "$TMP/tm" && "$TMP/tm" ); png 05-MOUVEMENT; echo "OK 05"; }
want 06 && { run 06-DEMARRAGE test_boot; png 06-DEMARRAGE splash_strip=splash-sequence splash_final=splash-final; echo "OK 06"; }
want 07 && { run 07-BUREAU-CIBLE test_screens -DBAURA_ICON_DEBUG; png 07-BUREAU-CIBLE; python3 "$HERE/07-BUREAU-CIBLE/tools/make_sheets.py" "$HERE/07-BUREAU-CIBLE/render"; echo "OK 07"; }
want 08 && { python3 "$HERE/08-LANGUES-ET-ACCESSIBILITE/tools/gen_arabic_table.py" "$HERE/08-LANGUES-ET-ACCESSIBILITE/code/arabic_forms.h" >/dev/null; run 08-LANGUES-ET-ACCESSIBILITE test_i18n_a11y
  ( cd "$HERE/08-LANGUES-ET-ACCESSIBILITE/code" && mv -f contraste.md arbre_a11y.txt ../render/ ); png 08-LANGUES-ET-ACCESSIBILITE rtl_comparaison=rtl-comparaison arabe_jonction=arabe-jonction a11y_themes=a11y-themes a11y_echelle=a11y-echelle a11y_ordre-focus=a11y-ordre-focus; echo "OK 08"; }
want 09 && { run 09-ICONES icon_sheets; png 09-ICONES; echo "OK 09"; }
want 11 && { python3 "$HERE/11-ICONES-APPLICATIONS/tools/gen_appicons.py"; run 11-ICONES-APPLICATIONS test_appicons; ( cd "$HERE/11-ICONES-APPLICATIONS/render" && python3 "$HERE/common/tools/ppm2png.py" . ); echo "OK 11"; }
BK="$HERE/12-BACKEND/baura-ui-services"; BKLIB="$BK/target/release/libbaura_ui_services.a"; BKLD="-lpthread -ldl -lm"
want 12 && { ( cd "$BK" && cargo test --quiet && cargo build --release --quiet ); g++ -std=c++17 -O2 -Wall -Wextra -Werror -I"$BK/include" "$BK/tests/test_backend.cpp" "$BKLIB" $BKLD -o "$TMP/test_backend" && "$TMP/test_backend"; echo "OK 12"; }
want 13 && { [ -f "$BKLIB" ] || ( cd "$BK" && cargo build --release --quiet ); run 13-MONITEUR test_monitor -I"$BK/include" "$BKLIB" $BKLD; ( cd "$HERE/13-MONITEUR/render" && python3 "$HERE/common/tools/ppm2png.py" . ); echo "OK 13"; }
want 14 && { run 14-NATIF test_native -I"$HERE/07-BUREAU-CIBLE/code"; png 14-NATIF; echo "OK 14"; }
echo "Terminé. Rien n'a été modifié dans $REPO."
