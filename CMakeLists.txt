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

# ========== 5. 添加源文件 ==========
file(GLOB CAMDEMO_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB CAMDEMO_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_executable(CamDemo WIN32
    ${CAMDEMO_SOURCES}
    ${CAMDEMO_HEADERS}
    CamDemo.ui
    CamDemo.qrc
)

# ========== 6. 生成 Qt 翻译文件 ==========
set(CAMDEMO_TS_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/CamDemo_zh_CN.ts
)
qt5_add_translation(CAMDEMO_QM_FILES ${CAMDEMO_TS_FILES})
add_custom_target(CamDemoTranslations ALL DEPENDS ${CAMDEMO_QM_FILES})
add_dependencies(CamDemo CamDemoTranslations)

# ========== 7. 链接库 ==========
target_link_libraries(CamDemo PUBLIC
    SARibbonBar::SARibbonBar
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Network
    QCefView
    libhv
)

# ========== 8. 自动复制运行时文件 ==========
if(CMAKE_CONFIGURATION_TYPES)
    # Visual Studio 多配置
    add_custom_command(TARGET CamDemo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Copying runtime files for $(Configuration)..."
        # 复制 QCefView
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${QCEF_BUILD_ROOT}/output/$(Configuration)/bin"
            "$<TARGET_FILE_DIR:CamDemo>"
        # 复制 libhv
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${LIBHV_BIN_DIR}/hv.dll"
            "$<TARGET_FILE_DIR:CamDemo>"
        # 复制翻译文件
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:CamDemo>/translations"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CAMDEMO_QM_FILES}
            "$<TARGET_FILE_DIR:CamDemo>/translations"
        COMMENT "Auto-copy runtime files to output directory"
    )
else()
    # 单配置生成器
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

# ========== 9. 目标属性 ==========
set_target_properties(CamDemo PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:CamDemo>"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# UTF-8 编译
if(MSVC)
    target_compile_options(CamDemo PRIVATE /utf-8)
endif()