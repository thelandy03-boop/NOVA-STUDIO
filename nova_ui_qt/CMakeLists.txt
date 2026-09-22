cmake_minimum_required(VERSION 3.20)
project(nova_ui_qt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Qml Core QuickControls2)

qt_standard_project_setup()
qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

qt_add_executable(nova_ui_qt main.cpp)

qt_add_qml_module(nova_ui_qt
    URI NovaStudio
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/theme/Theme.qml
        qml/theme/qmldir
        qml/screens/HomeScreen.qml
        qml/screens/WorkspaceScreen.qml
        qml/screens/qmldir
        qml/components/home/HomeSidebar.qml
        qml/components/home/QuickActionCard.qml
        qml/components/home/qmldir
        qml/components/workspace/DeviceRack.qml
        qml/components/workspace/MasterStrip.qml
        qml/components/workspace/TrackStrip.qml
        qml/components/workspace/TransportBar.qml
        qml/components/workspace/qmldir
        qml/components/workspace/controls/ReaperFader.qml
        qml/components/workspace/controls/ReaperKnob.qml
        qml/components/workspace/controls/ReaperMetalThumb.qml
        qml/components/workspace/controls/ReaperTrackRecButton.qml
        qml/components/workspace/controls/ReaperTransportRecButton.qml
        qml/components/workspace/controls/ReaperTinyBtn.qml
        qml/components/workspace/controls/ReaperRouteBtn.qml
        qml/components/workspace/controls/ReaperFxBtn.qml
        qml/components/workspace/controls/ReaperNodesBtn.qml
        qml/components/workspace/controls/ReaperSpeakerIcon.qml
        qml/components/workspace/controls/qmldir
)

target_include_directories(nova_ui_qt PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/qml)
target_link_libraries(nova_ui_qt PRIVATE Qt6::Quick Qt6::Qml Qt6::Core Qt6::QuickControls2)