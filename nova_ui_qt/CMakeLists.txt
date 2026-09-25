cmake_minimum_required(VERSION 3.20)
project(nova_ui_qt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Qml Core QuickControls2)

qt_standard_project_setup()
qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

include(cmake/NovaQml.cmake)

qt_add_executable(nova_ui_qt main.cpp)

# Declarar Singleton de Theme.qml
nova_mark_singletons()

# Auto-descubrimiento inteligente de todos los .qml bajo qml/
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

target_include_directories(nova_ui_qt PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/qml)
target_link_libraries(nova_ui_qt PRIVATE
    Qt6::Quick
    Qt6::Qml
    Qt6::Core
    Qt6::QuickControls2
)
