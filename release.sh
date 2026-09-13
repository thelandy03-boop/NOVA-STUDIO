#!/bin/bash
set -e

REPO="thelandy03-boop/NOVA-STUDIO"
CORES=$(nproc)
ROOT="$(cd "$(dirname "$0")" && pwd)"
OUT_DIR="$HOME/nova-releases"
mkdir -p "$OUT_DIR"

echo "══════════════════════════════════════════════════════════════"
echo "  🚀 NOVA-STUDIO Release (rápido, solo Ardour)"
echo "══════════════════════════════════════════════════════════════"

sudo -v
while true; do sudo -n true; sleep 60; kill -0 "$$" || exit; done 2>/dev/null &

if [ -n "$1" ]; then
  TAG_VERSION="v${1#v}"
  CLEAN_VERSION="${1#v}"
else
  LATEST_TAG=$(gh release view --repo "$REPO" --json tagName -q .tagName 2>/dev/null || echo "v9.8.31")
  TAG_NO_V="${LATEST_TAG#v}"
  IFS='.' read -r MAJOR MINOR PATCH <<< "$TAG_NO_V"
  PATCH=$((PATCH + 1))
  CLEAN_VERSION="${MAJOR}.${MINOR}.${PATCH}"
  TAG_VERSION="v${CLEAN_VERSION}"
fi

echo "🎯 Versión: $TAG_VERSION"
read -p "¿Continuar? [S/n]: " CONFIRM
CONFIRM=${CONFIRM:-s}
[[ "$CONFIRM" =~ ^[sSyY]$ ]] || exit 0

echo ""
echo "🔨 [1/3] Compilar..."
./waf build -j"$CORES"

echo "📦 [2/3] Instalar..."
sudo ./waf install -j"$CORES"

echo "🗜️  [3/3] Empaquetar SOLO Ardour..."
STAGE=$(mktemp -d "$HOME/nova_stage_XXXX")
LINUX_TARBALL="$OUT_DIR/nova-studio-${TAG_VERSION}.tar.gz"
rm -f "$LINUX_TARBALL"

mkdir -p "$STAGE/bin" "$STAGE/lib" "$STAGE/share" "$STAGE/etc"

# Copiar solo Ardour (con sudo lectura si hace falta, escritura en $HOME)
sudo cp -a /usr/local/bin/ardour9* "$STAGE/bin/" 2>/dev/null || true
sudo cp -a /usr/local/lib/ardour9   "$STAGE/lib/"
sudo cp -a /usr/local/share/ardour9 "$STAGE/share/"
sudo cp -a /usr/local/etc/ardour9   "$STAGE/etc/" 2>/dev/null || true
sudo chown -R "$(whoami):$(whoami)" "$STAGE"

tar -C "$STAGE" -cf - bin lib share etc | gzip -1 > "$LINUX_TARBALL"
rm -rf "$STAGE"

SIZE=$(du -h "$LINUX_TARBALL" | awk '{print $1}')
echo "  ✓ $LINUX_TARBALL ($SIZE)"

echo "🏷️  Publicando en GitHub..."
git add -A
git diff-index --quiet HEAD || git commit -m "chore(release): release $TAG_VERSION"
git push origin main 2>/dev/null || true
git tag -a "$TAG_VERSION" -m "NOVA-STUDIO $TAG_VERSION" 2>/dev/null || true
git push origin "$TAG_VERSION" 2>/dev/null || true

if gh release view "$TAG_VERSION" --repo "$REPO" &>/dev/null; then
  gh release upload "$TAG_VERSION" "$LINUX_TARBALL" --repo "$REPO" --clobber
else
  gh release create "$TAG_VERSION" "$LINUX_TARBALL" \
    --repo "$REPO" \
    --title "NOVA-STUDIO $TAG_VERSION" \
    --notes "### NOVA-STUDIO $TAG_VERSION (Linux / WSL2)

\`\`\`bash
sudo tar -xzf nova-studio-${TAG_VERSION}.tar.gz -C /usr/local
pw-jack ardour9
\`\`\`
"
fi

echo ""
echo "✅ Listo: https://github.com/$REPO/releases/tag/$TAG_VERSION"
echo "📦 Paquete local: $LINUX_TARBALL"
