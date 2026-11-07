# The API test drives a real ApiServer over HTTP on a loopback port, with the request store on a
# temporary work directory and a fake certificate probe.
qt_add_executable(tst_api
    tst_api.cpp
    ${SM_CORE_SOURCES})

target_include_directories(tst_api PRIVATE
    "${SM_SOURCE_DIR}"
    "${SM_SOURCE_DIR}/app")

target_compile_definitions(tst_api PRIVATE
    QT_NO_CAST_FROM_ASCII
    QT_NO_CAST_TO_ASCII
    QT_NO_KEYWORDS)

target_link_libraries(tst_api PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::HttpServer
    Qt6::Qml
    Qt6::Test)

if(WIN32)
    target_link_libraries(tst_api PRIVATE crypt32 cryptui version)
    target_compile_definitions(tst_api PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE)
endif()

if(MSVC)
    target_compile_options(tst_api PRIVATE /W4 /wd4702 /permissive- /utf-8 /Zc:__cplusplus)
endif()

add_test(NAME api COMMAND tst_api)
