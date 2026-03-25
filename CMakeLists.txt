cmake_minimum_required(VERSION 3.5)

project(CamDemo VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ========== 1. 查找 Qt ==========
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} 5.8 COMPONENTS Core Gui Widgets WebEngineWidgets REQUIRED)

# lrelease：生成默认中文 .qm（代码里仍用 tr("English")）
set(CAMDEMO_TS "${CMAKE_CURRENT_SOURCE_DIR}/translations/CamDemo_zh_CN.ts")
set(CAMDEMO_QM "${CMAKE_CURRENT_BINARY_DIR}/translations/CamDemo_zh_CN.qm")
if(QT_VERSION_MAJOR EQUAL 6)
    get_filename_component(_QT_BIN_PREFIX "${Qt6_DIR}/../../../bin" ABSOLUTE)
else()
    get_filename_component(_QT_BIN_PREFIX "${Qt5_DIR}/../../../bin" ABSOLUTE)
endif()
find_program(LRELEASE_EXECUTABLE NAMES lrelease lrelease.exe HINTS "${_QT_BIN_PREFIX}" REQUIRED)
add_custom_command(
    OUTPUT "${CAMDEMO_QM}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/translations"
    COMMAND "${LRELEASE_EXECUTABLE}" "${CAMDEMO_TS}" -qm "${CAMDEMO_QM}"
    DEPENDS "${CAMDEMO_TS}"
    COMMENT "lrelease CamDemo_zh_CN"
    VERBATIM
)
add_custom_target(camdemo_zh_qm DEPENDS "${CAMDEMO_QM}")

# ========== 2. 指定 SARibbon 安装目录（必须在 find_package 之前）==========
# 注意：路径用正斜杠，指向包含 SARibbonBarConfig.cmake 的目录
set(SARibbonBar_DIR "D:/code/SARibbon/bin_qt5.15.2_MSVC_x64/lib/cmake/SARibbonBar")
# ========== 3. 加载 SARibbonBar ==========
find_package(SARibbonBar REQUIRED)

# ========== 4. 添加你的源文件 ==========
add_executable(CamDemo
    main.cpp
    CamDemo.h
    CamDemo.cpp
    CamDemo.ui
    CamDemo.qrc
    LoginDialog.h
    LoginDialog.cpp
)
add_dependencies(CamDemo camdemo_zh_qm)

# ========== 5. Windows 下隐藏控制台窗口（可选）==========
if(WIN32)
    set_target_properties(CamDemo PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# MSVC：源文件按 UTF-8 解析，便于 tr / 翻译与中文 .ts
if(MSVC)
    target_compile_options(CamDemo PRIVATE /utf-8)
endif()

# ========== 6. 链接库 ==========
target_link_libraries(CamDemo PUBLIC
    SARibbonBar::SARibbonBar
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::WebEngineWidgets
)

# ========== 7. 输出目录（可选）==========
set_target_properties(CamDemo PROPERTIES
    CXX_EXTENSIONS OFF
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# ========== 8. 安装规则（可选）==========
install(TARGETS CamDemo
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
    ARCHIVE DESTINATION lib
)
install(FILES "${CAMDEMO_QM}" DESTINATION bin/translations)

add_custom_command(TARGET CamDemo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:CamDemo>/translations"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CAMDEMO_QM}" "$<TARGET_FILE_DIR:CamDemo>/translations/CamDemo_zh_CN.qm"
    COMMENT "Copy CamDemo_zh_CN.qm next to executable"
)