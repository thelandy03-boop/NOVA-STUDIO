#!/bin/bash
set -e

echo "🚀 [NOVA RELEASE AUTO] Detectando archivos e incrementando versión..."

# 1. Rutas de los instaladores locales
APK_FILE="nova_ui_qt/apks/NovaStudio-arm64-v8a.apk"
EXE_FILE=$(find . -name "*.exe" -type f | head -n 1)

if [ ! -f "$APK_FILE" ]; then
    echo "⚠️ APK no encontrado en $APK_FILE. Generando APK..."
    cd nova_ui_qt && ./build_android.sh && cd ..
fi

# 2. Detectar última versión de GitHub o Git Tags
LATEST_TAG=$(git describe --tags --abbrev=0 2>/dev/null || echo "v9.8.34")

# Quitar la 'v' inicial para calcular números
VERSION_NUM=${LATEST_TAG#v}

# Separar Mayor.Menor.Parche
IFS='.' read -r MAJOR MINOR PATCH <<< "$VERSION_NUM"
NEW_PATCH=$((PATCH + 1))
NEW_TAG="v${MAJOR}.${MINOR}.${NEW_PATCH}"

echo "  - Versión anterior: ${LATEST_TAG}"
echo "  - 🌟 NUEVA VERSIÓN AUTOMÁTICA: ${NEW_TAG}"

# 3. Preparar lista de archivos a adjuntar
FILES_TO_UPLOAD=("$APK_FILE")

if [ -n "$EXE_FILE" ] && [ -f "$EXE_FILE" ]; then
    echo "  - 🪟 Instalador Windows detectado: ${EXE_FILE}"
    FILES_TO_UPLOAD+=("$EXE_FILE")
else
    echo "  - ℹ️ Sin archivo .exe en el directorio (Se publicará solo el APK)."
fi

# 4. Crear Tag local y hacer push
git tag -a "${NEW_TAG}" -m "Release oficial ${NEW_TAG}" 2>/dev/null || true
git push origin "${NEW_TAG}" 2>/dev/null || true

# 5. Crear el Release en GitHub con 'gh'
echo "📦 Publicando ${NEW_TAG} en GitHub Releases..."

if command -v gh &> /dev/null; then
    gh release create "${NEW_TAG}" "${FILES_TO_UPLOAD[@]}" \
        --title "NOVA-STUDIO ${NEW_TAG}" \
        --notes "🎉 Release automático ${NEW_TAG} con instaladores listos." \
        --latest
    echo ""
    echo "🎉 ========================================================"
    echo "✅ ¡RELEASE ${NEW_TAG} PUBLICADO EXITOSAMENTE EN GITHUB!"
    echo "==========================================================="
else
    echo "❌ La herramienta 'gh' no está instalada. Para instalarla de inmediato:"
    echo "   sudo apt install gh -y && gh auth login"
fi