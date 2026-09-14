#!/bin/bash
STAGE_DIR="/c/msys64/tmp/nova_stage"

# Sincronizar el ejecutable recién compilado por Waf a NOVA-STUDIO.exe
if [ -f "$STAGE_DIR/lib/ardourv9/ardour-9.8.32.exe" ]; then
    cp "$STAGE_DIR/lib/ardourv9/ardour-9.8.32.exe" "$STAGE_DIR/NOVA-STUDIO.exe"
elif [ -f "$STAGE_DIR/bin/ardour-9.8.32.exe" ]; then
    cp "$STAGE_DIR/bin/ardour-9.8.32.exe" "$STAGE_DIR/NOVA-STUDIO.exe"
fi

# Copiar ejecutable directo desde build si existe
find build/gtk2_ardour/ -name "ardour-*.exe" -exec cp {} "$STAGE_DIR/NOVA-STUDIO.exe" \; 2>/dev/null

echo "🚀 Lanzando NOVA-STUDIO actualizado..."
"$STAGE_DIR/NOVA-STUDIO.exe"