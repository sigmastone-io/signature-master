# Each test is an executable over the core sources, with the request store on a temporary work
# directory and a fake certificate probe. tst_api drives a real ApiServer over HTTP on a loopback
# port; tst_signing drives the store and the signing service with a fake signer.
function(sm_add_test name)
    qt_add_executable(${name}
        ${name}.cpp
        ${SM_CORE_SOURCES})
    target_include_directories(${name} PRIVATE
        "${SM_SOURCE_DIR}"
        "${SM_SOURCE_DIR}/app")
    target_compile_definitions(${name} PRIVATE
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
        QT_NO_KEYWORDS)
    target_link_libraries(${name} PRIVATE
        Qt6::Core
        Qt6::Network
        Qt6::HttpServer
        Qt6::Qml
        Qt6::Test
        OpenSSL::Crypto)
    if(WIN32)
        target_link_libraries(${name} PRIVATE crypt32 cryptui version)
        target_compile_definitions(${name} PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE)
    endif()
    if(MSVC)
        target_compile_options(${name} PRIVATE /W4 /wd4702 /permissive- /utf-8 /Zc:__cplusplus)
    endif()
    add_test(NAME ${name} COMMAND ${name})
endfunction()

sm_add_test(tst_api)
sm_add_test(tst_signing)
sm_add_test(tst_oidc)
