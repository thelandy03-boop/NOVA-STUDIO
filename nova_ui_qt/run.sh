#!/bin/bash
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
NOVA_ROOT="$SCRIPT_DIR/.."
ARDOUR_BUILD="$NOVA_ROOT/build"
ARDOUR_LIBS="$ARDOUR_BUILD/libs"

# 1. Generar enlaces simbólicos SONAME si no existen
python3 -c "
import os, glob, subprocess
libs = glob.glob('$ARDOUR_LIBS/**/*.so*', recursive=True)
for so in libs:
    try:
        out = subprocess.check_output(['readelf', '-d', so], text=True, stderr=subprocess.DEVNULL)
        for line in out.splitlines():
            if 'SONAME' in line:
                soname = line.split('[')[1].split(']')[0]
                dir_path = os.path.dirname(so)
                link_path = os.path.join(dir_path, soname)
                if not os.path.exists(link_path):
                    os.symlink(os.path.basename(so), link_path)
    except Exception:
        pass
"

# 2. Configurar rutas de enlace de librerías
ALL_PATHS=$(find "$ARDOUR_LIBS" -type f -name "*.so*" -exec dirname {} + | sort -u | tr '\n' ':')
export LD_LIBRARY_PATH="$ALL_PATHS:$LD_LIBRARY_PATH"

# 3. Configurar variables de entorno requeridas por Ardour
export ARDOUR_CONFIG_PATH="$NOVA_ROOT/system_config:$NOVA_ROOT/share"
export ARDOUR_DATA_PATH="$NOVA_ROOT/share"

# Rutas para el descubrimiento de plugins de panorama y backends
export ARDOUR_PANNER_PATH="$ARDOUR_LIBS/panners/1in2out:$ARDOUR_LIBS/panners/2in2out:$ARDOUR_LIBS/panners/stereobalance:$ARDOUR_LIBS/panners/vbap"
export ARDOUR_DLL_PATH="$ARDOUR_LIBS/backends/alsa:$ARDOUR_LIBS/backends/dummy:$ARDOUR_LIBS/panners/1in2out:$ARDOUR_LIBS/panners/2in2out:$ARDOUR_LIBS/panners/stereobalance:$ARDOUR_LIBS/panners/vbap"
export ARDOUR_BACKEND_PATH="$ARDOUR_LIBS/backends/alsa:$ARDOUR_LIBS/backends/dummy"

# 4. Lanzar la aplicación
exec "$SCRIPT_DIR/build/nova_ui_qt" "$@"