cmake_minimum_required(VERSION 3.20)
project(nova_ui_qt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

get_filename_component(ARDOUR_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../build" ABSOLUTE)
get_filename_component(ARDOUR_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

# CONFIGURACIÓN COMPLETA DE RPATH PARA TODAS LAS LIBRERÍAS DE ARDOUR
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH
    "${ARDOUR_BUILD_DIR}/libs/ardour"
    "${ARDOUR_BUILD_DIR}/libs/pbd"
    "${ARDOUR_BUILD_DIR}/libs/temporal"
    "${ARDOUR_BUILD_DIR}/libs/evoral"
    "${ARDOUR_BUILD_DIR}/libs/midi++2"
    "${ARDOUR_BUILD_DIR}/libs/fst"
    "${ARDOUR_BUILD_DIR}/libs/tk/suil"
    "${ARDOUR_BUILD_DIR}/libs/audiographer"
    "${ARDOUR_BUILD_DIR}/libs/backends/alsa"
    "${ARDOUR_BUILD_DIR}/libs/backends/dummy"
)

find_package(Qt6 REQUIRED COMPONENTS Quick Qml Core QuickControls2)
find_package(PkgConfig REQUIRED)

pkg_check_modules(SIGCPP REQUIRED IMPORTED_TARGET sigc++-2.0)
pkg_check_modules(GLIBMM REQUIRED IMPORTED_TARGET glibmm-2.4)
pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET glib-2.0)
pkg_check_modules(GIOMM REQUIRED IMPORTED_TARGET giomm-2.4)
pkg_check_modules(LIBXML REQUIRED IMPORTED_TARGET libxml-2.0)
pkg_check_modules(SNDFILE REQUIRED IMPORTED_TARGET sndfile)

qt_standard_project_setup()
qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

include(cmake/NovaQml.cmake)

qt_add_executable(nova_ui_qt 
    main.cpp
    src/NovaAudioEngine.h
    src/NovaAudioEngine.cpp
)

nova_mark_singletons()
nova_collect_qml(NOVA_QML_FILES)

qt_add_qml_module(nova_ui_qt
    URI NovaStudio
    VERSION 1.0
    QML_FILES
        ${NOVA_QML_FILES}
    RESOURCES
        qml/skins/bandlab/topbar/icons/svg/IconLoop.svg
        qml/skins/bandlab/topbar/icons/svg/IconLoopActive.svg
        qml/skins/logic/topbar/icons/svg/LogicIconChevron.svg
        qml/skins/logic/topbar/icons/svg/LogicIconPencil.svg
        qml/skins/logic/topbar/icons/svg/LogicIconPointer.svg
        qml/skins/logic/topbar/icons/svg/LogicIconScissors.svg
)

target_include_directories(nova_ui_qt PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/qml
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${ARDOUR_SRC_DIR}
    ${ARDOUR_SRC_DIR}/libs
    ${ARDOUR_SRC_DIR}/libs/ardour
    ${ARDOUR_SRC_DIR}/libs/pbd
    ${ARDOUR_SRC_DIR}/libs/temporal
    ${ARDOUR_SRC_DIR}/libs/evoral
    ${ARDOUR_SRC_DIR}/libs/midi++2
    ${ARDOUR_SRC_DIR}/libs/libltc
    ${ARDOUR_SRC_DIR}/libs/libltc/ltc
    ${ARDOUR_SRC_DIR}/libs/lua
    ${ARDOUR_SRC_DIR}/libs/zita-resampler
    ${ARDOUR_SRC_DIR}/libs/zita-convolver
    ${ARDOUR_SRC_DIR}/libs/qm-dsp
    ${ARDOUR_SRC_DIR}/libs/audiographer
    ${ARDOUR_SRC_DIR}/libs/ptformat
    ${ARDOUR_BUILD_DIR}
    ${ARDOUR_BUILD_DIR}/libs
    ${ARDOUR_BUILD_DIR}/libs/ardour
    ${ARDOUR_BUILD_DIR}/libs/pbd
)

target_compile_definitions(nova_ui_qt PRIVATE
    COMPILER_INT128_SUPPORT
    WAF_BUILD
    PROGRAM_NAME="Ardour"
    PROGRAM_VERSION="v9"
)

# Buscar libfst.so de forma opcional/inteligente
find_library(FST_LIB fst HINTS "${ARDOUR_BUILD_DIR}/libs/fst")

set(EXTRA_ARDOUR_LIBS "")
if(EXISTS "${ARDOUR_BUILD_DIR}/libs/tk/suil/libsuil.so")
    list(APPEND EXTRA_ARDOUR_LIBS "${ARDOUR_BUILD_DIR}/libs/tk/suil/libsuil.so")
endif()
if(FST_LIB)
    list(APPEND EXTRA_ARDOUR_LIBS "${FST_LIB}")
elseif(EXISTS "${ARDOUR_BUILD_DIR}/libs/fst/libfst.so")
    list(APPEND EXTRA_ARDOUR_LIBS "${ARDOUR_BUILD_DIR}/libs/fst/libfst.so")
endif()

target_link_libraries(nova_ui_qt PRIVATE
    Qt6::Quick
    Qt6::Qml
    Qt6::Core
    Qt6::QuickControls2
    PkgConfig::SIGCPP
    PkgConfig::GLIBMM
    PkgConfig::GLIB
    PkgConfig::GIOMM
    PkgConfig::LIBXML
    PkgConfig::SNDFILE
    ${ARDOUR_BUILD_DIR}/libs/ardour/libardour.so
    ${ARDOUR_BUILD_DIR}/libs/pbd/libpbd.so
    ${ARDOUR_BUILD_DIR}/libs/temporal/libtemporal.so
    ${ARDOUR_BUILD_DIR}/libs/evoral/libevoral.so
    ${ARDOUR_BUILD_DIR}/libs/midi++2/libmidipp.so
    ${EXTRA_ARDOUR_LIBS}
)
