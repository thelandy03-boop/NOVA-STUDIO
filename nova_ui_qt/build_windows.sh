#!/bin/bash
set -e

echo "🚀 [NOVA WINDOWS BUILD] Compilando ejecutable .exe para Windows (x64)..."

# 1. Definir directorios
EXES_DIR="$(pwd)/exes"
mkdir -p "${EXES_DIR}"

QT_WIN_DIR="/home/landy/Qt/6.8.3/mingw_64"
QT_HOST_DIR="/home/landy/Qt/6.8.3/gcc_64"

if [ ! -d "${QT_WIN_DIR}" ]; then
    echo "❌ Error: No se encontraron las librerías Qt para Windows en ${QT_WIN_DIR}."
    exit 1
fi

BUILD_DIR="build_win64"
rm -rf ${BUILD_DIR}

TOOLCHAIN_FILE="cmake/mingw-w64-x86_64.cmake"
mkdir -p cmake

# 2. Generar Toolchain File para MinGW
cat << 'EOF_TC' > ${TOOLCHAIN_FILE}
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32 ${QT_WIN_DIR})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
EOF_TC

echo "🔨 Configurando CMake con Qt6 Windows + Host Tools Linux..."
cmake -B ${BUILD_DIR} \
    -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE} \
    -DCMAKE_PREFIX_PATH="${QT_WIN_DIR}" \
    -DQT_HOST_PATH="${QT_HOST_DIR}" \
    -DCMAKE_BUILD_TYPE=Release

echo "📦 Compilando archivo .exe..."
cmake --build ${BUILD_DIR} -j$(nproc)

EXE_SRC="${BUILD_DIR}/nova_ui_qt.exe"
EXE_DEST="${EXES_DIR}/NovaStudio-Win64.exe"

if [ -f "${EXE_SRC}" ]; then
    cp "${EXE_SRC}" "${EXE_DEST}"
    echo ""
    echo "🎉 ========================================================"
    echo "✅ [ÉXITO] ¡Ejecutable de Windows generado correctamente!"
    echo "📂 Ubicación física:"
    echo "   👉 ${EXE_DEST}"
    echo "==========================================================="
fi
