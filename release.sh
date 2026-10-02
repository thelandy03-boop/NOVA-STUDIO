#!/bin/bash
set -e

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT_DIR"

mkdir -p nova_ui_qt/exes
mkdir -p nova_ui_qt/apks

echo "🚀 [NOVA RELEASE AUTO] Verificando entorno de publicación..."

# 1. Verificar herramienta gh CLI
if ! command -v gh &> /dev/null; then
    echo "❌ Error: La herramienta 'gh' (GitHub CLI) no está instalada."
    exit 1
fi

# 2. Verificar o compilar APK de Android
APK_PATH="nova_ui_qt/apks/NovaStudio-arm64-v8a.apk"
if [ ! -f "$APK_PATH" ]; then
    echo "📱 APK no encontrado. Compilando automáticamente..."
    (cd nova_ui_qt && ./build_android.sh)
fi

# 3. Detectar última versión desde GitHub API
echo "🔍 Obteniendo última versión registrada en GitHub..."
LATEST_RELEASE=$(gh release list --repo thelandy03-boop/NOVA-STUDIO --json tagName --jq '.[0].tagName' 2>/dev/null || echo "")

if [ -z "$LATEST_RELEASE" ] || [[ ! "$LATEST_RELEASE" =~ ^v[0-9] ]]; then
    LATEST_RELEASE="v9.8.37"
fi

echo "  - Última versión registrada: ${LATEST_RELEASE}"

CLEAN_VER=${LATEST_RELEASE#v}
IFS='.' read -r MAJOR MINOR PATCH <<< "$CLEAN_VER"

if [ -z "$PATCH" ]; then PATCH=37; fi
NEW_PATCH=$((PATCH + 1))
NEW_TAG="v${MAJOR}.${MINOR}.${NEW_PATCH}"

echo "  - 🌟 NUEVA VERSIÓN A PUBLICAR: ${NEW_TAG}"

# 4. Recopilar ÚNICAMENTE los instaladores oficiales
FILES=()

if [ -f "$APK_PATH" ]; then
    echo "  - 📱 Adjuntando APK Android Oficial: $APK_PATH"
    FILES+=("$APK_PATH")
fi

OFFICIAL_EXE="nova_ui_qt/exes/NovaStudio-Win64.exe"
if [ -f "$OFFICIAL_EXE" ]; then
    echo "  - 🪟 Adjuntando Windows EXE Oficial: $OFFICIAL_EXE"
    FILES+=("$OFFICIAL_EXE")
fi

if [ ${#FILES[@]} -eq 0 ]; then
    echo "❌ Error: No se encontró ningún archivo oficial para subir."
    exit 1
fi

# 5. Crear Tag local y subir a GitHub
echo "🏷️ Sincronizando Tag Git ${NEW_TAG}..."
git tag -a "${NEW_TAG}" -m "Release oficial ${NEW_TAG}" 2>/dev/null || true
git push origin "${NEW_TAG}" 2>/dev/null || true

# 6. Publicar Release oficial
echo "📦 Publicando ${NEW_TAG} en GitHub Releases..."
gh release create "${NEW_TAG}" "${FILES[@]}" \
    --repo thelandy03-boop/NOVA-STUDIO \
    --title "NOVA-STUDIO ${NEW_TAG}" \
    --notes "🎉 **NOVA-STUDIO ${NEW_TAG} Official Release**

### 📥 Instaladores Oficiales Disponibles:
- 📱 **Android (ARM64):** \`NovaStudio-arm64-v8a.apk\`
- 🪟 **Windows (x64):** \`NovaStudio-Win64.exe\`" \
    --latest

echo ""
echo "🎉 ========================================================"
echo "✅ ¡RELEASE ${NEW_TAG} PUBLICADO EXITOSAMENTE EN GITHUB!"
echo "🌐 Ver en: https://github.com/thelandy03-boop/NOVA-STUDIO/releases/tag/${NEW_TAG}"
echo "==========================================================="
