#!/bin/bash
ROOT_DIR="$(cd .. && pwd)"

# Buscar automáticamente todas las rutas con librerías .so dentro de build/
ALL_LIBS=$(find "${ROOT_DIR}/build" -type f -name "*.so*" -exec dirname {} \; | sort -u | tr '\n' ':')

export ARDOUR_DLL_PATH="${ROOT_DIR}/build/libs/backends:${ROOT_DIR}/build/libs/ardour:${ROOT_DIR}/build/libs/pbd"
export ARDOUR_BACKEND_PATH="${ROOT_DIR}/build/libs/backends/jack:${ROOT_DIR}/build/libs/backends/alsa:${ROOT_DIR}/build/libs/backends/dummy"
export ARDOUR_PANNER_PATH="${ROOT_DIR}/build/libs/panners"
export LD_LIBRARY_PATH="${ALL_LIBS}${LD_LIBRARY_PATH}"

# 🎯 Calibrar entrada de micrófono de Linux al 45% (nivel limpio sin distorsión)
amixer sset Capture 45% >/dev/null 2>&1 || wpctl set-volume @DEFAULT_AUDIO_SOURCE@ 0.45 >/dev/null 2>&1

echo "🔊 Ejecutando NOVA STUDIO con PipeWire-JACK y Entrada Calibrada..."
exec pw-jack ./build/nova_ui_qt
