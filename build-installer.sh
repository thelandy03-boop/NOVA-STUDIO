#!/bin/bash
set -e

echo "══════════════════════════════════════════════════════════════"
echo "  🚀 NOVA-STUDIO - Construcción Final Limpia"
echo "══════════════════════════════════════════════════════════════"

# 1. Compilar e Instalar localmente
python waf build -j12

STAGE="/tmp/nova_stage"
rm -rf /tmp/waf_dest "$STAGE"
mkdir -p "$STAGE"

python waf install --destdir=/tmp/waf_dest
cp -r /tmp/waf_dest/msys64/tmp/* "$STAGE/"

# 2. Renombrar ejecutable principal
MAIN_EXE=$(find "$STAGE/lib/ardourv9" "$STAGE/lib/ardour9" "$STAGE/bin" -name "ardour-v*.exe" 2>/dev/null | head -n 1)
if [ -n "$MAIN_EXE" ]; then
    cp "$MAIN_EXE" "$STAGE/NOVA-STUDIO.exe"
fi

# 3. SOLUCIÓN CRÍTICA: Duplicar archivos de interfaz en todas las rutas de búsqueda
echo "📁 Duplicando temas, atajos y configs en etc/ y share/..."
mkdir -p "$STAGE/share/ardourv9" "$STAGE/etc/ardourv9" "$STAGE/share/ardour9" "$STAGE/etc/ardour9"

# Copiar estilos, atajos y configs a la raíz de share y etc
cp -r "$STAGE"/etc/ardourv9/* "$STAGE/etc/" 2>/dev/null || true
cp -r "$STAGE"/etc/ardourv9/* "$STAGE/share/ardourv9/" 2>/dev/null || true
cp -r "$STAGE"/etc/ardourv9/* "$STAGE/share/" 2>/dev/null || true
cp -r "$STAGE"/share/ardourv9/* "$STAGE/share/" 2>/dev/null || true

# 4. Consolidar DLLs de Windows en la raíz junto a NOVA-STUDIO.exe
echo "🔗 Consolidando DLLs de Windows..."
cp -u /mingw64/bin/*.dll "$STAGE/" 2>/dev/null || true
cp -u "$STAGE"/lib/ardourv9/*.dll "$STAGE/" 2>/dev/null || true
cp -u "$STAGE"/bin/*.dll "$STAGE/" 2>/dev/null || true
(cd "$STAGE" && for f in lib*.dll; do cp -u "$f" "${f#lib}" 2>/dev/null || true; done) || true

# 5. Optimizar peso con strip
strip --strip-unneeded "$STAGE"/*.exe "$STAGE"/*.dll 2>/dev/null || true

# 6. Generar instalador NSIS
echo "🗜️  Empaquetando NOVA-STUDIO-Setup.exe con NSIS..."
makensis -V3 tools/windows_packaging/nova_studio.nsi

echo ""
echo "══════════════════════════════════════════════════════════════"
EXE_FILE="tools/windows_packaging/NOVA-STUDIO-Setup.exe"
if [ -f "$EXE_FILE" ]; then
    echo "  ✅ ¡INSTALADOR COMPLETO Y LISTO PARA TU AMIGO!"
    echo "  📁 Ubicación: $EXE_FILE"
    echo "  📊 Tamaño final: $(du -h "$EXE_FILE" | awk '{print $1}')"
else
    echo "  ❌ Error al empaquetar."
fi
echo "══════════════════════════════════════════════════════════════"
