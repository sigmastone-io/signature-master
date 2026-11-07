# Renders the application icon from its SVG into the multi-size .ico the executable and the
# installer carry. The .ico is committed; run this when the SVG changes:
#
#   cmake --build --preset dev --target icon
qt_add_executable(render-icon render-icon.cpp)
target_link_libraries(render-icon PRIVATE Qt6::Gui Qt6::Svg)
target_compile_definitions(render-icon PRIVATE QT_NO_CAST_FROM_ASCII QT_NO_CAST_TO_ASCII)
if(MSVC)
    target_compile_options(render-icon PRIVATE /W4 /permissive- /utf-8)
endif()

add_custom_target(icon
    COMMAND render-icon
        "${CMAKE_SOURCE_DIR}/src/qml/icons/app.svg"
        "${CMAKE_SOURCE_DIR}/packaging/windows/signature-master.ico"
    DEPENDS render-icon
    COMMENT "Rendering packaging/windows/signature-master.ico from src/qml/icons/app.svg"
    VERBATIM)
