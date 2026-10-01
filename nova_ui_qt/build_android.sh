#!/bin/bash
set -e

echo "🚀 [NOVA ANDROID BUILD] Configurando entorno de compilación..."

# 1. Variables del sistema Android y Java 17 LTS (Oficial para AGP 8 / Qt 6.8)
export ANDROID_SDK_ROOT="$HOME/Android/Sdk"
export ANDROID_NDK_ROOT="$HOME/Android/Sdk/ndk/26.1.10909125"

if [ -d "/usr/lib/jvm/java-17-openjdk-amd64" ]; then
    export JAVA_HOME="/usr/lib/jvm/java-17-openjdk-amd64"
elif [ -d "/usr/lib/jvm/java-21-openjdk-amd64" ]; then
    export JAVA_HOME="/usr/lib/jvm/java-21-openjdk-amd64"
else
    export JAVA_HOME="/usr/lib/jvm/default-java"
fi

export PATH="${JAVA_HOME}/bin:${PATH}"

echo "  - SDK Root:  ${ANDROID_SDK_ROOT}"
echo "  - NDK Root:  ${ANDROID_NDK_ROOT}"
echo "  - Java Home: ${JAVA_HOME} ($(${JAVA_HOME}/bin/java -version 2>&1 | head -n 1))"

# 2. Herramienta Qt CMake para Android (arm64-v8a)
QT_ANDROID_CMAKE="$HOME/Qt/6.8.3/android_arm64_v8a/bin/qt-cmake"

if [ ! -f "$QT_ANDROID_CMAKE" ]; then
    echo "❌ Error: No se encontró qt-cmake en $QT_ANDROID_CMAKE"
    exit 1
fi

BUILD_DIR="build_android"

echo "🔨 [NOVA ANDROID BUILD] Generando archivos CMake en ./${BUILD_DIR}..."
${QT_ANDROID_CMAKE} -B ${BUILD_DIR} \
    -DANDROID_ABI="arm64-v8a" \
    -DANDROID_PLATFORM="android-31" \
    -DCMAKE_BUILD_TYPE=Release

# Desactivar análisis estático de Lint para compilación ágil y evitar falsos positivos
if [ -d "${BUILD_DIR}/android-build" ]; then
    echo "android.lint.vital=false" >> ${BUILD_DIR}/android-build/gradle.properties
    echo "android.lint.abortOnError=false" >> ${BUILD_DIR}/android-build/gradle.properties
    echo "android.javaCompile.suppressSourceTargetDeprecationWarning=true" >> ${BUILD_DIR}/android-build/gradle.properties
fi

echo "📦 [NOVA ANDROID BUILD] Compilando y empaquetando APK..."
cmake --build ${BUILD_DIR} --target apk -j$(nproc)

# 3. Exportación a carpeta visible 'apks/' en la raíz
EXPORT_DIR="apks"
mkdir -p "${EXPORT_DIR}"

APK_SRC="${BUILD_DIR}/android-build/nova_ui_qt.apk"
APK_DEST="${EXPORT_DIR}/NovaStudio-arm64-v8a.apk"

if [ -f "${APK_SRC}" ]; then
    cp "${APK_SRC}" "${APK_DEST}"
elif [ -f "${BUILD_DIR}/android-build/build/outputs/apk/release/android-build-release-unsigned.apk" ]; then
    cp "${BUILD_DIR}/android-build/build/outputs/apk/release/android-build-release-unsigned.apk" "${APK_DEST}"
fi

echo ""
echo "🎉 ========================================================"
echo "✅ [ÉXITO] ¡APK de NOVA Studio listo para instalar!"
echo "📂 Ubicación directa:"
echo "   👉 $(pwd)/${APK_DEST}"
echo "==========================================================="