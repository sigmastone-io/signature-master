# Everything below the window, shared with the tests.
set(sm_core_sources
    app/ApiServer.cpp
    app/ApiServer.h
    app/Authenticator.cpp
    app/Authenticator.h
    app/Authenticode.cpp
    app/Authenticode.h
    app/CertificateStore.cpp
    app/CertificateStore.h
    app/FileVersion.cpp
    app/FileVersion.h
    app/Jwt.cpp
    app/Jwt.h
    app/JwksCache.cpp
    app/JwksCache.h
    app/RequestStore.cpp
    app/RequestStore.h
    app/Settings.cpp
    app/Settings.h
    app/SignatureLog.cpp
    app/SignatureLog.h
    app/Signer.cpp
    app/Signer.h
    app/SigningRequest.cpp
    app/SigningRequest.h
    app/SigningService.cpp
    app/SigningService.h)
set(SM_CORE_SOURCES "")
foreach(file IN LISTS sm_core_sources)
    list(APPEND SM_CORE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${file}")
endforeach()
set(SM_CORE_SOURCES "${SM_CORE_SOURCES}" PARENT_SCOPE)
set(SM_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" PARENT_SCOPE)

qt_add_executable(signature-master
    main.cpp
    ${sm_core_sources}
    app/TrayController.cpp
    app/TrayController.h)

# The QML module. Source files live in qml/, qml/pages and qml/components for tidiness but are
# published at the module root, so every type is reachable by its file name and relative URLs
# from any QML file resolve against the module root ("icons/shield.svg").
set(sm_qml_files
    qml/Main.qml
    qml/Theme.qml
    qml/components/Toast.qml
    qml/components/CheckRow.qml
    qml/components/Fact.qml
    qml/components/RailItem.qml
    qml/components/SettingsCard.qml
    qml/components/StatePill.qml
    qml/components/StatusFooter.qml
    qml/components/TextRow.qml
    qml/components/ToggleRow.qml
    qml/pages/LogPage.qml
    qml/pages/RequestsPage.qml
    qml/pages/SettingsPage.qml)

set(sm_resource_files
    qml/icons/app.svg
    qml/icons/bolt.svg
    qml/icons/certificate.svg
    qml/icons/key.svg
    qml/icons/network.svg
    qml/icons/pen.svg
    qml/icons/log.svg
    qml/icons/requests.svg
    qml/icons/settings.svg
    qml/icons/shield.svg
    qml/icons/warning.svg)

foreach(file IN LISTS sm_qml_files)
    get_filename_component(alias "${file}" NAME)
    set_source_files_properties("${file}" PROPERTIES QT_RESOURCE_ALIAS "${alias}")
endforeach()
foreach(file IN LISTS sm_resource_files)
    string(REGEX REPLACE "^qml/" "" alias "${file}")
    set_source_files_properties("${file}" PROPERTIES QT_RESOURCE_ALIAS "${alias}")
endforeach()
set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(signature-master
    URI SignatureMaster
    VERSION 1.0
    QML_FILES ${sm_qml_files}
    RESOURCES ${sm_resource_files}
    SOURCES
        app/DeskStatus.cpp
        app/DeskStatus.h
        app/LogModel.cpp
        app/LogModel.h
        app/RequestModel.cpp
        app/RequestModel.h)

# The generated QML type registration includes the SOURCES headers by file name.
target_include_directories(signature-master PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app)

target_compile_definitions(signature-master PRIVATE
    SM_VERSION="${PROJECT_VERSION}"
    QT_NO_CAST_FROM_ASCII
    QT_NO_CAST_TO_ASCII
    QT_NO_KEYWORDS)

target_link_libraries(signature-master PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Svg
    Qt6::Network
    Qt6::HttpServer
    OpenSSL::Crypto)

if(WIN32)
    # The icon and the version resource, from app.rc.in and the project version.
    file(TO_NATIVE_PATH "${CMAKE_SOURCE_DIR}/packaging/windows/signature-master.ico" SM_RC_ICON)
    string(REPLACE "\\" "\\\\" SM_RC_ICON "${SM_RC_ICON}")
    configure_file(app.rc.in "${CMAKE_CURRENT_BINARY_DIR}/app.rc" @ONLY)
    target_sources(signature-master PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/app.rc")
    target_link_libraries(signature-master PRIVATE crypt32 cryptui version)
    target_compile_definitions(signature-master PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE)
    # A console in development builds keeps qDebug visible; the release build is a plain GUI
    # application.
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set_target_properties(signature-master PROPERTIES WIN32_EXECUTABLE TRUE)
    endif()
endif()

if(MSVC)
    # C4702 (unreachable code) fires inside Qt's own headers at /W4.
    target_compile_options(signature-master PRIVATE /W4 /wd4702 /permissive- /utf-8 /Zc:__cplusplus)
endif()

# `cmake --install` lays the executable, the libraries vcpkg built (X_VCPKG_APPLOCAL_DEPS_INSTALL
# in the presets), the Qt plugins and QML modules, and the Visual C++ runtime flat into the
# prefix, the same shape as the build tree; the MSI packages that tree. windeployqt is run here
# directly rather than through Qt's deploy script, which insists on bin/ and Qt6/ subdirectories.
install(TARGETS signature-master RUNTIME DESTINATION .)
if(WIN32)
    install(CODE "
        execute_process(
            COMMAND \"$<TARGET_FILE:Qt6::windeployqt>\"
                --no-compiler-runtime
                --no-translations
                --qmldir \"${CMAKE_CURRENT_SOURCE_DIR}/qml\"
                --qml-deploy-dir \"\${CMAKE_INSTALL_PREFIX}\"
                \"\${CMAKE_INSTALL_PREFIX}/signature-master.exe\"
            RESULT_VARIABLE sm_deploy_result)
        if(NOT sm_deploy_result EQUAL 0)
            message(FATAL_ERROR \"windeployqt failed: \${sm_deploy_result}\")
        endif()")
    # msvcp140.dll, vcruntime140.dll and vcruntime140_1.dll beside the executable, so the MSI
    # needs no redistributable installer.
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ".")
    include(InstallRequiredSystemLibraries)
endif()

if(WIN32 AND SM_DEPLOY_QT)
    # QML modules go flat beside the executable, which is on the engine's default import path;
    # windeployqt's own default, a qml/ subdirectory, is not.
    add_custom_command(TARGET signature-master POST_BUILD
        COMMAND Qt6::windeployqt
            --no-compiler-runtime
            --no-translations
            --qmldir "${CMAKE_CURRENT_SOURCE_DIR}/qml"
            --qml-deploy-dir "$<TARGET_FILE_DIR:signature-master>"
            "$<TARGET_FILE:signature-master>"
        COMMENT "Deploying the Qt runtime beside signature-master.exe"
        VERBATIM)
endif()
