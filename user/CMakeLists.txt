# user 模块静态库：登录 UI（QCef）、HTTP（libhv）、会话与 token 持久化

add_library(user STATIC
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
)

target_include_directories(user PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

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
