# qj_user 模块动态库：登录 UI（QCef）、HTTP（libhv）、会话与 token 持久化
# 注意：QCefView 在根 CMakeLists.txt 中定义

add_library(qj_user SHARED
    qj_user_global.h
    UserSession.cpp
    UserSession.h
    AuthHttpClient.cpp
    AuthHttpClient.h
    LoginWebAuthHelpers.cpp
    LoginWebAuthHelpers.h
    DesktopAuthBridge.h
    AccountAuthDialog.cpp
    AccountAuthDialog.h
    UserAuthService.cpp
    UserAuthService.h
    UserModuleConfig.h
    DesktopWeb.h
    qj_user.h
    CefInitializer.cpp
    CefInitializer.h
)

target_include_directories(qj_user PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${QCEF_ROOT}/include"
)

target_compile_definitions(qj_user PRIVATE QJ_USER_LIBRARY)

target_link_libraries(qj_user PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Network
    QCefView
    libhv
)

if(MSVC)
    target_compile_options(qj_user PRIVATE /utf-8)
endif()

set_target_properties(qj_user PROPERTIES
    DEBUG_POSTFIX "_d"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)