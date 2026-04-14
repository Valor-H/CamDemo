# user 模块动态库：登录 UI（QCef）、HTTP（libhv）、会话与 token 持久化
# 注意：QCefView 在根 CMakeLists.txt 中定义

add_library(user SHARED
    UserApi.h
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
    User.h
    CefInitializer.cpp
    CefInitializer.h
)

target_include_directories(user PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${QCEF_ROOT}/include"
)

target_compile_definitions(user PRIVATE USER_LIBRARY_BUILD)
target_compile_definitions(user INTERFACE USER_LIBRARY_SHARED)

target_link_libraries(user PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Network
    QCefView
    libhv
)

if(MSVC)
    target_compile_options(user PRIVATE /utf-8)
endif()

# 关键：开发阶段直接自动导出符号，避免逐个写导出宏
# 这样可快速生成可链接的 user_d.dll / user_d.lib
# 与主程序同一 bin/<Config>，避免运行 CamDemo 时找不到 user_d.dll
set_target_properties(user PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
    DEBUG_POSTFIX "_d"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)