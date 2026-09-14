#!/bin/bash

STAGE_DIR="/tmp/nova_clean/msys64/tmp"

if [ ! -f "$STAGE_DIR/NOVA-STUDIO.exe" ]; then
    echo "📦 No se encontró el binario empaquetado. Preparando..."
    ./build-installer.sh
fi

echo "🚀 Lanzando NOVA-STUDIO (Modo Desarrollador / Windows Native)..."
(cd "$STAGE_DIR" && ./NOVA-STUDIO.exe "$@")
