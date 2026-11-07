cmake_minimum_required(VERSION 3.28)

project(SignatureMaster
    VERSION 0.1.0
    DESCRIPTION "Signing desk for Sigmastone releases"
    HOMEPAGE_URL "https://github.com/sigmastone-io/signature-master"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SM_DEPLOY_QT "Run windeployqt after each build so the executable runs from the build tree" ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Qml
    Quick
    QuickControls2
    Svg
    Network
    HttpServer)

qt_standard_project_setup(REQUIRES 6.8)

add_subdirectory(src)
