#!/bin/bash
# ==============================================================================
# ASE Markdown Parser — Layer 1 Core
# Professional Build Script with Ninja
# ==============================================================================

# NOTE: Do NOT use "set -e" here. This script handles errors explicitly.

BUILD_START_TIME=$(date +%s)

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ASE_ROOT="$( cd "$SCRIPT_DIR/../.." && pwd )"

# Terminal output framework (SSOT: sha-client-web/sha-web-console)
#
# OHNE ERSATZWEICHE, UND DAS IST DER PUNKT — 2026-09-17.
#
# Hier stand ein `if [ -f ] … else <eigene section_*-Funktionen>`: ein Nachbau der SSOT mit
# eigenen ANSI-Folgen, eigener Rahmenbreite und eigener Symbolliste. Sein `section_spin` war
# die gefaehrlichste Zeile der Datei — es fing die Ausgabe ab und setzte `SPIN_EXIT`/`SPIN_OUT`,
# aber ohne Spinner, ohne `SPIN_MS` und ohne `section_gate`/`section_perf_*`. Damit LIEF eine
# Prebuild-Batterie unter ihm an und zerfiel beim ersten `section_gate` in „command not found",
# waehrend die Zahlen schon gezaehlt waren.
#
# **Ein Nachbau, der genug kann um zu starten und zu wenig um durchzulaufen, ist schlimmer als
# gar keiner.** Die sieben grossen Bauskripte sourcen diese Datei unbedingt (gemessen an
# servers/ase-server-dist/build.sh:27 und tools/ase-cli/build.sh:28) — faellt sie aus, faellt
# der Bau, und das ist die richtige Richtung.
source "$ASE_ROOT/clients/sha-client-web/public/console/bash/console.sh"

# CPU/IO priority
BUILD_JOBS=$(( $(nproc) - 2 ))
[ "$BUILD_JOBS" -lt 2 ] && BUILD_JOBS=2
BUILD_PREFIX="nice -n 10 ionice -c2 -n 7"

# SCRIPT_DIR steht oben, vor dem Sourcen der Konsole — hier stand es ein ZWEITES Mal, mit
# demselben Ausdruck. Zwei Ableitungen derselben Sache: die zweite kann still hinter der ersten
# zuruckbleiben, sobald eine von beiden angefasst wird.
VERSION_FILE="$SCRIPT_DIR/VERSION"

# ============================================
# VERSION MANAGEMENT
# ============================================

get_version() {
    local key="$1"
    if [ -f "$VERSION_FILE" ]; then
        local result
        result=$(grep "^${key}=" "$VERSION_FILE" 2>/dev/null | cut -d'=' -f2)
        echo "${result:-00.00.00.00000}"
    else
        echo "00.00.00.00000"
    fi
}

get_build() {
    if [ -f "$VERSION_FILE" ]; then
        local version
        version=$(grep "^ASE_MARKDOWN=" "$VERSION_FILE" 2>/dev/null | cut -d'=' -f2)
        echo "${version##*.}"
    else
        echo "00001"
    fi
}

get_status() {
    if [ -f "$VERSION_FILE" ]; then
        local result
        result=$(grep "^ASE_MARKDOWN_STATUS=" "$VERSION_FILE" 2>/dev/null | cut -d'=' -f2)
        echo "${result:-seed}"
    else
        echo "seed"
    fi
}

bump_build() {
    if [ -f "$VERSION_FILE" ]; then
        local current
        current=$(get_build)
        local new_build=$((10#$current + 1))
        local major=0
        local minor=$((new_build / 50))
        local patch=$((new_build % 50))
        local new_version
        new_version=$(printf "%02d.%02d.%02d.%05d" $major $minor $patch $new_build)
        sed -i "s/^ASE_MARKDOWN=.*/ASE_MARKDOWN=${new_version}/" "$VERSION_FILE"
        sed -i "s/^ASE_MARKDOWN_UPDATED=.*/ASE_MARKDOWN_UPDATED=$(date +%Y-%m-%d)/" "$VERSION_FILE"
        printf "%05d" "$new_build"
    fi
}

get_status_color() {
    local status_val="$1"
    case "$status_val" in
        seed)       echo "${GRAY}" ;;
        poc)        echo "${MAGENTA}" ;;
        init|core|feat) echo "${YELLOW}" ;;
        refine|alpha)   echo "${CYAN}" ;;
        beta)       echo "${BLUE}" ;;
        stable)     echo "${CYAN}" ;;
        *)          echo "${GRAY}" ;;
    esac
}

get_status_icon() {
    local status_val="$1"
    case "$status_val" in
        seed) echo "○" ;; poc) echo "◐" ;; init) echo "◑" ;; core) echo "◒" ;;
        feat) echo "◓" ;; refine) echo "●" ;; alpha) echo "α" ;; beta) echo "β" ;;
        stable) echo "★" ;; *) echo "?" ;;
    esac
}

VERSION=$(get_version "ASE_MARKDOWN")
BUILD_NUM=$(get_build)
STATUS=$(get_status)
STATUS_COLOR=$(get_status_color "$STATUS")
STATUS_ICON=$(get_status_icon "$STATUS")

# ============================================
# CLI FLAGS
# ============================================

DO_TEST=false
IS_CLEAN=false
BUILD_TYPE="${BUILD_TYPE:-Release}"
BUILD_DIR="${SCRIPT_DIR}/build"

for arg in "$@"; do
    case "$arg" in
        test)  DO_TEST=true ;;
        clean) IS_CLEAN=true ;;
        debug) BUILD_TYPE="Debug" ;;
        --help|-h)
            echo -e "${MAGENTA}ase-markdown — Markdown Parser (L1 Core)${NC}"
            echo -e "  ${CYAN}./build.sh${NC}         Release build"
            echo -e "  ${CYAN}./build.sh test${NC}    Build + run tests"
            echo -e "  ${CYAN}./build.sh debug${NC}   Debug build"
            echo -e "  ${CYAN}./build.sh clean${NC}   Clean build cache"
            exit 0 ;;
    esac
done

# ============================================
# HEADER
# ============================================

section_header "ase-markdown — Markdown Parser" 170
section_line "·" "CommonMark/GFM + ASE DSL parser producing typed AST"
section_pipe
section_line "*" "Version" "v${VERSION} [${STATUS}]"
section_line "$HASH" "Build" "${BUILD_TYPE} (${BUILD_JOBS} threads)"

# ============================================
# CLEAN
# ============================================

if [ "$IS_CLEAN" = true ]; then
    section_header "Clean (preserving _deps)" 71
    if [ -d "$BUILD_DIR" ]; then
        cd "$BUILD_DIR"
        find . -name "*.ninja*" -not -path "./_deps/*" -delete 2>/dev/null || true
        find . -name "CMakeCache.txt" -not -path "./_deps/*" -delete 2>/dev/null || true
        find . -name "CMakeFiles" -type d -not -path "./_deps/*" -exec rm -rf {} + 2>/dev/null || true
        rm -rf bin lib 2>/dev/null || true
        cd "$SCRIPT_DIR"
        section_line "$CHECK" "Build cache cleaned"
    else
        section_line "$SKIP" "Nothing to clean"
    fi
fi

# ============================================
# PREBUILD GATES
# ============================================
# Bis 2026-08-18 fuhr dieser Einstiegspunkt KEIN einziges Prebuild-Tor — wie fuenf weitere von
# elf. Ein Tor, das an einer Stelle nicht laeuft, meldet dort nichts, und das sieht aus wie Ruhe.
# Die Liste der Tore liegt genau einmal, in gates.conf; Drift zwischen Bau-Skripten ist damit
# ausgeschlossen.
#
# ── ZWEI DEFEKTE AN EINER STELLE, BEIDE AM 2026-09-17 ─────────────────────────────────────────
#
# (1) FAIL-OPEN. Hier stand `if [ -x "$ASE_PREBUILD_GATES" ]; then … fi` OHNE else-Zweig. War
#     der Laeufer nicht ausfuehrbar, lief KEIN einziges Tor, der Bau ging mit rc=0 weiter, und
#     in der Ausgabe stand nichts darueber. **Ein Messwerkzeug, das weniger prueft als der
#     Ernstfall, faellt nicht auf — sein Ergebnis sieht BESSER aus als die Wirklichkeit.** Das
#     stand seit dem 2026-08-18 als WARNUNG in tools/ase-codegen/build.sh, namentlich gegen
#     diese Datei gerichtet; gelesen hat es hier niemand.
#
# (2) DER LAEUFER IST DER MASCHINENKANAL, NICHT DIE ANZEIGE. `run_prebuild_gates.sh` druckt
#     `GATE-BEGIN:`, `GATE-SCOPE:`, `GATE-COUNT:`, `GATE-INFO:` und `GATE-SUMMARY:`, damit eine
#     SITZUNG ihren Stand auswerten kann, dazu den vollen Prosatext jedes Tores. In einem Bau,
#     den ein MENSCH liest, laufen genau diese Zeilen ohne `│`, ohne Symbol und ohne Spalte quer
#     durch die Rahmen.
#
#     > **Eine Zeile, die niemand lesen soll, darf nicht dort stehen, wo Menschen lesen.**
#
#     Der Satz steht in prebuild_battery.sh und hat dort am 2026-09-16 die drei Clients von
#     diesem Griff geholt. Diese Datei und tools/ase-codegen/build.sh blieben stehen — nicht aus
#     einem Grund, sondern weil niemand sie mitgezaehlt hat.
_ASE_BATTERY="$ASE_ROOT/core/ase-validator/scripts/prebuild/prebuild_battery.sh"
GATES_CONF="$ASE_ROOT/core/ase-validator/scripts/prebuild/gates.conf"

if [ ! -f "$GATES_CONF" ]; then
    section_line "$CROSS" "PREBUILD BLOCKED" "gates.conf MISSING"
    section_detail "$GATES_CONF"
    exit 1
fi
if [ ! -f "$_ASE_BATTERY" ]; then
    section_line "$CROSS" "PREBUILD BLOCKED" "battery MISSING"
    section_detail "$_ASE_BATTERY"
    exit 1
fi
source "$_ASE_BATTERY"

# DER ZUSCHNITT BLEIBT, WAS ER WAR: ALLE 27 TORE.
#
# Die Zeile ist ABGELESEN, nicht geschaetzt. `run_prebuild_gates.sh` ignoriert das Feld
# `zuschnitt` in gates.conf und faehrt jedes Tor. gates.conf fuehrt 20 Tore mit `all` und 7 mit
# `module`; `false` faehrt also 20. Eine Umstellung der DARSTELLUNG darf den Pruefumfang nicht
# verschieben, in keine Richtung — und diese Datei hat schon einmal zu wenig geprueft.
ASE_GATE_LOADS_MODULES=true

do_pre_validation

# ============================================
# BUILD
# ============================================

mkdir -p "$BUILD_DIR"

section_header "Build" 214

# WERKZEUGAUSGABE LAEUFT DURCH `section_spin`, NICHT DURCH `tail` — 2026-09-17.
#
# Hier stand `cmake … 2>&1 | tail -5` und `ninja … 2>&1 | tail -3`. Dieselbe Klasse wie die rohe
# Torausgabe, nur kleiner: fremde Zeilen ohne `│`, ohne Symbol, ohne Spalte, mitten in der Box —
# und ZUGLEICH ein Informationsverlust, denn `tail -3` wirft im Fehlerfall genau den Anfang weg,
# an dem die erste Fehlermeldung steht.
if [ ! -f "$BUILD_DIR/build.ninja" ] || [ -n "$(find "$SCRIPT_DIR" -name 'CMakeLists.txt' -newer "$BUILD_DIR/build.ninja" 2>/dev/null | head -1)" ]; then
    section_spin "CMake configure" $BUILD_PREFIX cmake -B "$BUILD_DIR" -G Ninja \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DASE_BUILD_TESTS=ON \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
        -S "$SCRIPT_DIR"
    if [ "$SPIN_EXIT" -ne 0 ]; then
        section_line "$CROSS" "CMake configure failed"
        section_detail "$SPIN_OUT"
        exit 1
    fi
    section_line "$CHECK" "CMake configured" "$(section_ms "$SPIN_MS")"
else
    section_line "$SKIP" "CMake up to date"
fi

section_spin "Ninja build" $BUILD_PREFIX ninja -C "$BUILD_DIR" -j "$BUILD_JOBS"
if [ "$SPIN_EXIT" -ne 0 ]; then
    section_line "$CROSS" "Build failed"
    section_detail "$SPIN_OUT"
    exit 1
fi

NEW_BUILD=$(bump_build)
section_line "$CHECK" "Build #${NEW_BUILD} complete" "$(section_ms "$SPIN_MS")"

# ============================================
# TESTS
# ============================================

if [ "$DO_TEST" = true ]; then
    section_header "Tests" 71
    # DIE TESTAUSGABE IST WERKZEUGAUSGABE WIE JEDE ANDERE. Bis 2026-09-17 lief sie mit `2>&1`
    # blank in die Konsole: doctest schreibt seinen eigenen Rahmen, und der stand mitten im
    # Rahmen dieser Section. Bestanden zeigt die Zeile jetzt nur die Zeit, erst ein Fehlschlag
    # zeigt den Bericht — dann aber vollstaendig, an der Boxkante gefaltet.
    #
    # DIE EXISTENZPRUEFUNG FEHLTE HIER GANZ: war das Binaer nicht gebaut, lief die Zeile in ein
    # „No such file or directory" der Shell und `$?` trug 127. Der Bau brach ab und sagte
    # „Tests failed" — eine Falschaussage ueber einen Test, der nie lief.
    if [ ! -x "$BUILD_DIR/bin/ase-markdown-test" ]; then
        section_line "$CROSS" "ase-markdown-test not built"
        section_detail "$BUILD_DIR/bin/ase-markdown-test"
        exit 1
    fi
    section_spin "ase-markdown-test" "$BUILD_DIR/bin/ase-markdown-test"
    if [ "$SPIN_EXIT" -eq 0 ]; then
        section_line "$CHECK" "All tests passed" "$(section_ms "$SPIN_MS")"
    else
        section_line "$CROSS" "Tests failed" "$(section_ms "$SPIN_MS")"
        section_detail "$SPIN_OUT"
        exit 1
    fi
fi

# ============================================
# SUMMARY
# ============================================

BUILD_END_TIME=$(date +%s)
BUILD_DURATION=$((BUILD_END_TIME - BUILD_START_TIME))
section_pipe
section_line "$CHECK" "Done in ${BUILD_DURATION}s"
echo ""
