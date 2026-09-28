#!/bin/bash
export QT_QPA_PLATFORM=xcb

NOVA_BUILD_DIR="/home/landy/Descargas/NOVA-STUDIO/build"
NOVA_BUILD_LIBS="$NOVA_BUILD_DIR/libs"

# 🎯 Rutas de librerías y dependencias de Ardour
export ARDOUR_DLL_PATH="$NOVA_BUILD_LIBS/backends:$NOVA_BUILD_LIBS/ardour:$NOVA_BUILD_LIBS/pbd"
export ARDOUR_BACKEND_PATH="$NOVA_BUILD_LIBS/backends/jack:$NOVA_BUILD_LIBS/backends/alsa:$NOVA_BUILD_LIBS/backends/dummy"

# 🚀 NUEVA RUTA REQUERIDA: Localización de los paneadores internos de audio
export ARDOUR_PANNER_PATH="$NOVA_BUILD_LIBS/panners"

export LD_LIBRARY_PATH="$NOVA_BUILD_LIBS/ardour:$NOVA_BUILD_LIBS/pbd:$NOVA_BUILD_LIBS/temporal:$NOVA_BUILD_LIBS/evoral:$NOVA_BUILD_LIBS/midi++2:$NOVA_BUILD_LIBS/fst:$NOVA_BUILD_LIBS/tk/suil:$NOVA_BUILD_LIBS/audiographer:$NOVA_BUILD_LIBS/ardouralsautil:$NOVA_BUILD_LIBS/backends/jack:$NOVA_BUILD_LIBS/backends/alsa:$NOVA_BUILD_LIBS/backends/dummy:$LD_LIBRARY_PATH"

if command -v pw-jack >/dev/null 2>&1; then
    echo "🔊 Ejecutando con PipeWire-JACK Wrapper (pw-jack)..."
    exec pw-jack ./build/nova_ui_qt "$@"
else
    echo "🚀 Ejecutando de forma directa..."
    exec ./build/nova_ui_qt "$@"
fi