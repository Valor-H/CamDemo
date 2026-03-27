cmake_minimum_required(VERSION 3.5)

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
find_package(Qt5 COMPONENTS Core Gui Widgets LinguistTools REQUIRED)

# ========== 2. 集成预编译的 QCefView（关键修正）==========
set(QCEF_BUILD_ROOT "D:/Codes/QCefView/build")
set(QCEF_INCLUDE_DIR "D:/Codes/QCefView/include")

# 分别设置 Debug 和 Release 的绝对路径
set(QCEF_BIN_DEBUG "${QCEF_BUILD_ROOT}/output/Debug/bin")
set(QCEF_LIB_DEBUG "${QCEF_BUILD_ROOT}/output/Debug/lib")
set(QCEF_BIN_RELEASE "${QCEF_BUILD_ROOT}/output/Release/bin")
set(QCEF_LIB_RELEASE "${QCEF_BUILD_ROOT}/output/Release/lib")

# 验证路径存在
if(NOT EXISTS "${QCEF_LIB_DEBUG}/QCefView.lib")
    message(WARNING "QCefView Debug lib not found at: ${QCEF_LIB_DEBUG}/QCefView.lib")
endif()
if(NOT EXISTS "${QCEF_LIB_RELEASE}/QCefView.lib")
    message(WARNING "QCefView Release lib not found at: ${QCEF_LIB_RELEASE}/QCefView.lib")
endif()

# 创建 IMPORTED 目标（使用 _DEBUG/_RELEASE 属性）
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

# ========== 3. 其他依赖（SARibbon 等）==========
set(SARibbonBar_DIR "D:/Codes/SARibbon/bin_qt5.15.2_MSVC_x64/lib/cmake/SARibbonBar")
find_package(SARibbonBar REQUIRED)

# ========== 4. 添加源文件 ==========
add_executable(CamDemo WIN32
    main.cpp
    CamDemo.h
    CamDemo.cpp
    CamDemo.ui
    CamDemo.qrc
    LoginDialog.h
    LoginDialog.cpp
)

# ========== 4.1 生成 Qt 翻译文件（.qm）==========
# 运行时 main.cpp 从 <exe>/translations 加载 CamDemo_zh_CN.qm。
set(CAMDEMO_TS_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/CamDemo_zh_CN.ts
)
qt5_add_translation(CAMDEMO_QM_FILES ${CAMDEMO_TS_FILES})
add_custom_target(CamDemoTranslations ALL DEPENDS ${CAMDEMO_QM_FILES})
add_dependencies(CamDemo CamDemoTranslations)

# ========== 5. 链接库 ==========
target_link_libraries(CamDemo PUBLIC
    SARibbonBar::SARibbonBar
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    QCefView
)

# ========== 6. 自动复制 QCefView 运行时（修正版）==========
# Visual Studio 多配置：使用 $(Configuration) 变量
# Ninja/单配置：使用 CMAKE_BUILD_TYPE
if(CMAKE_CONFIGURATION_TYPES)
    # Visual Studio 多配置
    add_custom_command(TARGET CamDemo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Copying QCefView files for $(Configuration)..."
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${QCEF_BUILD_ROOT}/output/$(Configuration)/bin"
            "$<TARGET_FILE_DIR:CamDemo>"
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:CamDemo>/translations"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CAMDEMO_QM_FILES}
            "$<TARGET_FILE_DIR:CamDemo>/translations"
        COMMENT "Auto-copy QCefView runtime to output directory"
    )
else()
    # 单配置生成器（Ninja/Make）
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(QCEF_COPY_FROM "${QCEF_BIN_DEBUG}")
    else()
        set(QCEF_COPY_FROM "${QCEF_BIN_RELEASE}")
    endif()
    
    add_custom_command(TARGET CamDemo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${QCEF_COPY_FROM}"
            "$<TARGET_FILE_DIR:CamDemo>"
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:CamDemo>/translations"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CAMDEMO_QM_FILES}
            "$<TARGET_FILE_DIR:CamDemo>/translations"
        COMMENT "Auto-copy QCefView runtime to output directory"
    )
endif()

# VS 调试工作目录
set_target_properties(CamDemo PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:CamDemo>"
)

# ========== 7. 输出目录 ==========
set_target_properties(CamDemo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# UTF-8 编译
if(MSVC)
    target_compile_options(CamDemo PRIVATE /utf-8)
endif()