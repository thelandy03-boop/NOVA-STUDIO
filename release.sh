#!/bin/bash
set -e

REPO="thelandy03-boop/NOVA-STUDIO"

echo "══════════════════════════════════════════════════════"
echo "  🚀 NOVA-STUDIO - Creador Automatizado de Releases"
echo "══════════════════════════════════════════════════════"

# 1. Pedir permisos de sudo al inicio y mantenerlos vivos
sudo -v
while true; do sudo -n true; sleep 60; kill -0 "$$" || exit; done 2>/dev/null &

# 2. Comprobar GitHub CLI
if ! gh auth status &> /dev/null; then
    echo "⚠️  Iniciando sesión en GitHub CLI..."
    gh auth login
fi

# 3. Determinar la versión
if [ -n "$1" ]; then
    NEW_VERSION="$1"
    TAG_VERSION="v${NEW_VERSION#v}"
    CLEAN_VERSION="${NEW_VERSION#v}"
else
    # Obtener último tag publicado en GitHub
    LATEST_TAG=$(gh release view --repo "$REPO" --json tagName -q .tagName 2>/dev/null || echo "v9.8.29")
    
    echo "📌 Última versión detectada: $LATEST_TAG"
    
    # Incrementar versión
    TAG_NO_V="${LATEST_TAG#v}"
    IFS='.' read -r MAJOR MINOR PATCH <<< "$TAG_NO_V"
    PATCH=$((PATCH + 1))
    CLEAN_VERSION="${MAJOR}.${MINOR}.${PATCH}"
    TAG_VERSION="v${CLEAN_VERSION}"
fi

echo "🎯 Versión a publicar: $TAG_VERSION ($CLEAN_VERSION)"
read -p "¿Continuar? [S/n]: " CONFIRM
CONFIRM=${CONFIRM:-s}
if [[ ! "$CONFIRM" =~ ^[sSyY]$ ]]; then
    echo "Cancelado."
    exit 0
fi

# 4. Compilar (rápido si ya está compilado)
echo ""
echo "🔨 [1/4] Verificando compilación..."
./waf build

# 5. Instalar
echo "📦 [2/4] Instalando en /usr/local..."
sudo ./waf install

# 6. Empaquetar
echo "🗜️  [3/4] Empaquetando binarios..."
TARBALL_NAME="nova-studio-${TAG_VERSION}.tar.gz"
TARBALL_PATH="/tmp/${TARBALL_NAME}"

sudo tar -czf "$TARBALL_PATH" -C /usr/local bin/ardour9 lib/ardour9 share/ardour9 etc/ardour9

echo "  ✓ Archivo creado: $TARBALL_PATH ($(du -h "$TARBALL_PATH" | awk '{print $1}'))"

# 7. Git commit, tag y push
echo "🏷️  [4/4] Publicando en GitHub..."

git add -A
git diff-index --quiet HEAD || git commit -m "chore(release): release $TAG_VERSION"
git push origin main 2>/dev/null || true

git tag -a "$TAG_VERSION" -m "NOVA-STUDIO $TAG_VERSION" 2>/dev/null || true
git push origin "$TAG_VERSION" 2>/dev/null || true

# Crear release en GitHub con el binario adjunto
gh release create "$TAG_VERSION" "$TARBALL_PATH" \
    --repo "$REPO" \
    --title "NOVA-STUDIO $TAG_VERSION" \
    --notes "### NOVA-STUDIO $TAG_VERSION

- Actualizador automático integrado en la interfaz.
- Compatibilidad nativa con WSL2 y PipeWire.
- Paquete binario precompilado listo para auto-instalación."

rm -f "$TARBALL_PATH"

echo ""
echo "══════════════════════════════════════════════════════"
echo "  ✅ ¡Release $TAG_VERSION publicado exitosamente!"
echo "  🔗 https://github.com/$REPO/releases/tag/$TAG_VERSION"
echo "══════════════════════════════════════════════════════"
