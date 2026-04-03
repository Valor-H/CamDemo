cmake_minimum_required(VERSION 3.12)

# 强制 v142 工具集（与 QCefView 一致）
if(CMAKE_GENERATOR MATCHES "Visual Studio")
    set(CMAKE_GENERATOR_TOOLSET "v142" CACHE STRING "Platform Toolset" FORCE)
endif()

project(CamDemo VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 17)

# ========== 1. 查找 Qt ==========
set(CMAKE_PREFIX_PATH "D:/Tools/QT5/5.15.2/msvc2019_64")
find_package(Qt5 COMPONENTS Core Gui Widgets Network LinguistTools REQUIRED)

# ========== 2. 集成预编译的 QCefView ==========
set(QCEF_BUILD_ROOT "D:/Codes/QCefView/build")
set(QCEF_INCLUDE_DIR "D:/Codes/QCefView/include")

set(QCEF_BIN_DEBUG "${QCEF_BUILD_ROOT}/output/Debug/bin")
set(QCEF_LIB_DEBUG "${QCEF_BUILD_ROOT}/output/Debug/lib")
set(QCEF_BIN_RELEASE "${QCEF_BUILD_ROOT}/output/Release/bin")
set(QCEF_LIB_RELEASE "${QCEF_BUILD_ROOT}/output/Release/lib")

if(NOT EXISTS "${QCEF_LIB_DEBUG}/QCefView.lib")
    message(WARNING "QCefView Debug lib not found: ${QCEF_LIB_DEBUG}/QCefView.lib")
endif()
if(NOT EXISTS "${QCEF_LIB_RELEASE}/QCefView.lib")
    message(WARNING "QCefView Release lib not found: ${QCEF_LIB_RELEASE}/QCefView.lib")
endif()

add_library(QCefView SHARED IMPORTED GLOBAL)
set_target_properties(QCefView PROPERTIES
    IMPORTED_LOCATION_DEBUG "${QCEF_BIN_DEBUG}/QCefView.dll"
    IMPORTED_IMPLIB_DEBUG "${QCEF_LIB_DEBUG}/QCefView.lib"
    IMPORTED_LOCATION_RELEASE "${QCEF_BIN_RELEASE}/QCefView.dll"
    IMPORTED_IMPLIB_RELEASE "${QCEF_LIB_RELEASE}/QCefView.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${QCEF_INCLUDE_DIR}"
)

message(STATUS ">>> QCefView Debug: ${QCEF_LIB_DEBUG}/QCefView.lib")
message(STATUS ">>> QCefView Release: ${QCEF_LIB_RELEASE}/QCefView.lib")

# ========== 3. 集成 SARibbonBar ==========
set(SARibbonBar_DIR "D:/Codes/SARibbon/bin_qt5.15.2_MSVC_x64/lib/cmake/SARibbonBar")
find_package(SARibbonBar REQUIRED)

# ========== 4. 集成 libhv（动态库）==========
set(LIBHV_ROOT "D:/Codes/libhv/installed_debug")
set(LIBHV_INCLUDE_DIR "${LIBHV_ROOT}/include")
set(LIBHV_LIB_DIR "${LIBHV_ROOT}/lib")
set(LIBHV_BIN_DIR "${LIBHV_ROOT}/bin")

if(NOT EXISTS "${LIBHV_LIB_DIR}/hv.lib")
    message(FATAL_ERROR "libhv not found at: ${LIBHV_LIB_DIR}/hv.lib")
endif()

add_library(libhv SHARED IMPORTED GLOBAL)
set_target_properties(libhv PROPERTIES
    IMPORTED_LOCATION_DEBUG "${LIBHV_BIN_DIR}/hv.dll"
    IMPORTED_IMPLIB_DEBUG "${LIBHV_LIB_DIR}/hv.lib"
    IMPORTED_LOCATION_RELEASE "${LIBHV_BIN_DIR}/hv.dll"
    IMPORTED_IMPLIB_RELEASE "${LIBHV_LIB_DIR}/hv.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${LIBHV_INCLUDE_DIR}"
    INTERFACE_LINK_LIBRARIES "secur32;crypt32;winmm;iphlpapi;ws2_32"
)

message(STATUS ">>> libhv: ${LIBHV_LIB_DIR}/hv.lib")

# ========== 5. user 模块 ==========
add_subdirectory(user)

# ========== 6. 主程序（仅壳与 Ribbon/标题栏 UI）==========
add_executable(CamDemo WIN32
    main.cpp
    CamDemo.cpp
    CamDemo.h
    TitleBarUserChip.cpp
    TitleBarUserChip.h
    CamDemo.ui
    CamDemo.qrc
)

# ========== 7. 生成 Qt 翻译文件 ==========
# 更新 .ts 时请将 user 目录一并纳入 lupdate 扫描（LoginDialog 等上下文在 CamDemo_zh_CN.ts 中）
set(CAMDEMO_TS_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/CamDemo_zh_CN.ts
)
qt5_add_translation(CAMDEMO_QM_FILES ${CAMDEMO_TS_FILES})
add_custom_target(CamDemoTranslations ALL DEPENDS ${CAMDEMO_QM_FILES})
add_dependencies(CamDemo CamDemoTranslations)

# ========== 8. 链接库（HTTP/CEF 由 user 目标传播）==========
target_link_libraries(CamDemo PRIVATE
    user
    SARibbonBar::SARibbonBar
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Network
)

# ========== 9. 自动复制运行时文件 ==========
if(CMAKE_CONFIGURATION_TYPES)
    add_custom_command(TARGET CamDemo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Copying runtime files for $(Configuration)..."
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${QCEF_BUILD_ROOT}/output/$(Configuration)/bin"
            "$<TARGET_FILE_DIR:CamDemo>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${LIBHV_BIN_DIR}/hv.dll"
            "$<TARGET_FILE_DIR:CamDemo>"
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:CamDemo>/translations"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CAMDEMO_QM_FILES}
            "$<TARGET_FILE_DIR:CamDemo>/translations"
        COMMENT "Auto-copy runtime files to output directory"
    )
else()
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(QCEF_COPY_FROM "${QCEF_BIN_DEBUG}")
    else()
        set(QCEF_COPY_FROM "${QCEF_BIN_RELEASE}")
    endif()

    add_custom_command(TARGET CamDemo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${QCEF_COPY_FROM}"
            "$<TARGET_FILE_DIR:CamDemo>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${LIBHV_BIN_DIR}/hv.dll"
            "$<TARGET_FILE_DIR:CamDemo>"
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:CamDemo>/translations"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CAMDEMO_QM_FILES}
            "$<TARGET_FILE_DIR:CamDemo>/translations"
        COMMENT "Auto-copy runtime files to output directory"
    )
endif()

# ========== 10. 目标属性 ==========
set_target_properties(CamDemo PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:CamDemo>"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

if(MSVC)
    target_compile_options(CamDemo PRIVATE /utf-8)
endif()
