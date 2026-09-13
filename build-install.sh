#!/bin/bash
set -e

echo "🔨 Compilando cambios..."
./waf build

echo "📦 Instalando binarios y librerías en /usr/local..."
sudo ./waf install

echo "✅ ¡Listo! Puedes ejecutar 'pw-jack ardour9'"
