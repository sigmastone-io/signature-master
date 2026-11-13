# SPDX-FileCopyrightText: 2026 Sigmastone OÜ
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

project(SignatureMaster
    VERSION 0.1.0
    DESCRIPTION "Signing desk for Sigmastone releases"
    HOMEPAGE_URL "https://github.com/sigmastone-io/signature-master"
    LANGUAGES CXX)

# The version the application, its version resource and the MSI report. A release build passes the
# tag's version here (`-DSM_VERSION=0.2.0-rc.1`); it must start with the project version above, so
# the tag and the source cannot disagree. Left empty, it is the project version.
set(SM_VERSION "" CACHE STRING "Full version string, e.g. 0.2.0-rc.1; empty for the project version")
if(SM_VERSION STREQUAL "")
    set(SM_VERSION_FULL "${PROJECT_VERSION}")
else()
    string(REPLACE "." "\\." sm_version_pattern "${PROJECT_VERSION}")
    if(NOT SM_VERSION MATCHES "^${sm_version_pattern}([-+][0-9A-Za-z.+-]+)?$")
        message(FATAL_ERROR
            "SM_VERSION '${SM_VERSION}' does not match the project version ${PROJECT_VERSION}")
    endif()
    set(SM_VERSION_FULL "${SM_VERSION}")
endif()
message(STATUS "Signature Master ${SM_VERSION_FULL}")

if(WIN32)
    enable_language(RC) # the icon and version resource
endif()

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
    HttpServer
    Test)

find_package(OpenSSL REQUIRED) # RS256 verification of the GitHub Actions tokens

qt_standard_project_setup(REQUIRES 6.8)

add_subdirectory(src)
add_subdirectory(tools/render-icon)

enable_testing()
add_subdirectory(tests)
