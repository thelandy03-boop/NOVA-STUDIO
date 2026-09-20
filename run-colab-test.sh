#!/bin/bash

export NOVA_ALLOW_MULTIPLE=1

echo "🚀 Iniciando NOVA-STUDIO Instancia 1 (Host)..."
./run-dev-win.sh &

sleep 4

echo "🚀 Iniciando NOVA-STUDIO Instancia 2 (Colaborador con Dummy Audio)..."
./run-dev-win.sh -d &
