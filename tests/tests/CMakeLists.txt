add_executable(web-eid-tests WIN32
    changecertificatevaliduntil.hpp
    getcommandhandler-mock.cpp
    getcommandhandler-mock.hpp
    main.cpp
)
target_compile_definitions(web-eid-tests PRIVATE _CRT_SECURE_NO_WARNINGS)
target_include_directories(web-eid-tests PRIVATE
    ${CMAKE_SOURCE_DIR}/lib/libelectronic-id/tests/mock
)
target_link_libraries(web-eid-tests controller mock-ui pcsc-mock Qt5::Test)

add_test(web-eid-tests web-eid-tests)
