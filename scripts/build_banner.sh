#!/bin/sh
# Cyberpunk banner for build system

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
V=$(sed -n 's/^#define IFTOP_VERSION "\(.*\)"/\1/p' "$SCRIPT_DIR/../src/include/conf.h")

C='\033[1;36m'  # bold cyan
M='\033[1;35m'  # bold magenta
G='\033[1;32m'  # bold green
Y='\033[1;33m'  # bold yellow
R='\033[0m'     # reset
B='\033[1m'     # bold

case "$1" in
    build-start)
        printf '\n'
        printf "  ${M}╔══════════════════════════════════════════════════════════════════════╗${R}\n"
        printf "  ${M}║${R}  ${C}▓▓ COMPILING NEURAL NETWORK INTERFACE // iftop ${B}v${V}${R}             ${M}║${R}\n"
        printf "  ${M}║${R}  ${Y}>> ENGAGING COMPILER UPLINK ...${R}                                    ${M}║${R}\n"
        printf "  ${M}╚══════════════════════════════════════════════════════════════════════╝${R}\n"
        printf '\n'
        ;;
    build-end)
        printf '\n'
        printf "  ${M}╔══════════════════════════════════════════════════════════════════════╗${R}\n"
        printf "  ${M}║${R}  ${G}▓▓ BUILD SEQUENCE COMPLETE // BINARY PAYLOAD ARMED${R}                 ${M}║${R}\n"
        printf "  ${M}║${R}  ${C}>> TARGET: iftop // STATUS: OPERATIONAL${R}                             ${M}║${R}\n"
        printf "  ${M}╚══════════════════════════════════════════════════════════════════════╝${R}\n"
        printf '\n'
        ;;
    clean-start)
        printf '\n'
        printf "  ${M}╔══════════════════════════════════════════════════════════════════════╗${R}\n"
        printf "  ${M}║${R}  ${Y}▓▓ INITIATING MEMORY WIPE // PURGING BUILD ARTIFACTS${R}               ${M}║${R}\n"
        printf "  ${M}║${R}  ${C}>> SCRUBBING OBJECT FILES ...${R}                                      ${M}║${R}\n"
        printf "  ${M}╚══════════════════════════════════════════════════════════════════════╝${R}\n"
        printf '\n'
        ;;
    install-start)
        printf '\n'
        printf "  ${M}╔══════════════════════════════════════════════════════════════════════╗${R}\n"
        printf "  ${M}║${R}  ${C}▓▓ DEPLOYING BINARY TO TARGET SYSTEM // iftop ${B}v${V}${R}               ${M}║${R}\n"
        printf "  ${M}║${R}  ${Y}>> INITIATING NEURAL IMPLANT SEQUENCE ...${R}                             ${M}║${R}\n"
        printf "  ${M}╚══════════════════════════════════════════════════════════════════════╝${R}\n"
        printf '\n'
        ;;
    install-end)
        printf '\n'
        printf "  ${M}╔══════════════════════════════════════════════════════════════════════╗${R}\n"
        printf "  ${M}║${R}  ${G}▓▓ IMPLANT SUCCESSFUL // BINARY DEPLOYED TO GRID${R}                    ${M}║${R}\n"
        printf "  ${M}║${R}  ${C}>> iftop IS NOW JACKED INTO THE SYSTEM${R}                                ${M}║${R}\n"
        printf "  ${M}╚══════════════════════════════════════════════════════════════════════╝${R}\n"
        printf '\n'
        ;;
    clean-end)
        printf '\n'
        printf "  ${M}╔══════════════════════════════════════════════════════════════════════╗${R}\n"
        printf "  ${M}║${R}  ${G}▓▓ PURGE COMPLETE // ALL TRACES ELIMINATED${R}                         ${M}║${R}\n"
        printf "  ${M}║${R}  ${C}>> SYSTEM RESET TO ZERO STATE${R}                                      ${M}║${R}\n"
        printf "  ${M}╚══════════════════════════════════════════════════════════════════════╝${R}\n"
        printf '\n'
        ;;
    distclean-start)
        printf '\n'
        printf "  ${M}╔══════════════════════════════════════════════════════════════════════╗${R}\n"
        printf "  ${M}║${R}  ${Y}▓▓ DEEP PURGE INITIATED // SCORCHED EARTH PROTOCOL${R}                ${M}║${R}\n"
        printf "  ${M}║${R}  ${C}>> WIPING BUILD ARTIFACTS, CONFIG CACHES, TAGS ...${R}                 ${M}║${R}\n"
        printf "  ${M}╚══════════════════════════════════════════════════════════════════════╝${R}\n"
        printf '\n'
        ;;
    distclean-end)
        printf '\n'
        printf "  ${M}╔══════════════════════════════════════════════════════════════════════╗${R}\n"
        printf "  ${M}║${R}  ${G}▓▓ TOTAL WIPE COMPLETE // FILESYSTEM STERILIZED${R}                   ${M}║${R}\n"
        printf "  ${M}║${R}  ${C}>> SOURCE TREE RESTORED TO FACTORY STATE${R}                           ${M}║${R}\n"
        printf "  ${M}╚══════════════════════════════════════════════════════════════════════╝${R}\n"
        printf '\n'
        ;;
esac
