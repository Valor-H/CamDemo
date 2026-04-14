# 获取本文件所在目录（假设放在 lib/cmake/qcefview/）
get_filename_component(QCEFVIEW_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY)
# 计算根目录：向上退两级（lib/cmake/ → lib/ → 根/）
get_filename_component(QCEFVIEW_DIR "${QCEFVIEW_CMAKE_DIR}/../../.." ABSOLUTE)

# Debug 版本标识符（统一修改此处即可改变所有路径）
set(QCEFVIEW_DEBUG_POSTFIX "d")

# 目录路径（使用变量拼接，自动扩展为 bind/libd）
set(QCEFVIEW_INCLUDE_DIR "${QCEFVIEW_DIR}/include")
set(QCEFVIEW_BIN_DEBUG   "${QCEFVIEW_DIR}/bin${QCEFVIEW_DEBUG_POSTFIX}")  # bind
set(QCEFVIEW_BIN_RELEASE "${QCEFVIEW_DIR}/bin")                           # bin
set(QCEFVIEW_LIB_DEBUG   "${QCEFVIEW_DIR}/lib${QCEFVIEW_DEBUG_POSTFIX}")  # libd
set(QCEFVIEW_LIB_RELEASE "${QCEFVIEW_DIR}/lib")                           # lib

# 创建导入目标（SHARED 表示动态库，IMPORTED 表示预编译）
add_library(QCefView SHARED IMPORTED GLOBAL)

# 设置头文件搜索路径（INTERFACE 表示使用此目标时会自动添加）
set_target_properties(QCefView PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${QCEFVIEW_INCLUDE_DIR}"
)

# Debug 配置：IMPLIB 指向 .lib 导入库，LOCATION 指向 .dll 运行时
set_target_properties(QCefView PROPERTIES
    IMPORTED_IMPLIB_DEBUG   "${QCEFVIEW_LIB_DEBUG}/QCefView${QCEFVIEW_DEBUG_POSTFIX}.lib"
    IMPORTED_LOCATION_DEBUG "${QCEFVIEW_BIN_DEBUG}/QCefView${QCEFVIEW_DEBUG_POSTFIX}.dll"
)

# Release 配置（标准命名，无后缀）
set_target_properties(QCefView PROPERTIES
    IMPORTED_IMPLIB_RELEASE   "${QCEFVIEW_LIB_RELEASE}/QCefView.lib"
    IMPORTED_LOCATION_RELEASE "${QCEFVIEW_BIN_RELEASE}/QCefView.dll"
)

# 默认回退（当 CMake 无法确定 Debug/Release 时使用 Debug 版本）
set_target_properties(QCefView PROPERTIES
    IMPORTED_IMPLIB   "${QCEFVIEW_LIB_DEBUG}/QCefView${QCEFVIEW_DEBUG_POSTFIX}.lib"
    IMPORTED_LOCATION "${QCEFVIEW_BIN_DEBUG}/QCefView${QCEFVIEW_DEBUG_POSTFIX}.dll"
)

# 标记查找成功
set(QCEFVIEW_FOUND TRUE)

# 输出提示（便于调试时确认加载的是哪个版本）
message(STATUS "QCefView: ${QCEFVIEW_DIR}")
message(STATUS "  Debug lib:   ${QCEFVIEW_LIB_DEBUG}")
message(STATUS "  Release lib: ${QCEFVIEW_LIB_RELEASE}")




# 3rdparty_macro.cmake 内容

function(QJ_3RDPARTY_ADD_LIBRARY _target _libname _incluir _libdir _linklibs _compileopts _type)
    QJ_3RDPARTY_ADD_LIBRARY_BASE(
        ${_target}
        ${_libname}
        _d
        ${_incluir}
        ${_libdir}
        ${_linklibs}
        ${_compileopts}
        ${_type})
endfunction()

fucntion(QJ_3RDPARTY_ADD_LIBRARY_BASE _target _libname _debug_Suffix _incluir _libdir _linklibs _compileopts _type)
    if (TARGET ${_target})
        return()
    endif()

    add_library(${_target} ${_type} IMPORTED)

    if (WIN32)
        set(_qj_3rd_suffix .lib)
    elseif (${_type} STREQAL "STATIC")
        set(_qj_3rd_suffix .a)
    else()
        set(_qj_3rd_suffix .so)
    endif()

    if (MSVC)
        set(_qj_3rd_prefix )
    else()
        set(_qj_3rd_prefix lib)
    endif()

    set(_qj_3rd_debug_suffix ${_debugSuffix})

    set(_qj_3rd_lib_name ${_qj_3rd_prefix}${_libname}${_qj_3rd_suffix})
    set(_qj_3rd_lib_name_debug ${_qj_3rd_prefix}${_libname}${_qj_3rd_debug_suffix}${_qj_3rd_suffix})

    set(_qj_3rd_library_path ${_libdir}/${_qj_3rd_lib_name})
    set(_qj_3rd_libray_path_debug ${_libdir}/${_qj_3rd_lib_name_debug})

    if (NOT EXISTS ${_qj_3rd_library_path})
        message(FATAL_ERROR "Not found library: ${_qj_3rd_library_path}")
    endif()

    if (NOT EXISTS ${_qj_3rd_libray_path_debug})
        message(FATAL_ERROR "Not found library: ${_qj_3rd_libray_path_debug}")
    endif()

    if (EXISTS ${_qj_3rd_library_path_debug})
        set_target_properties(${_target} PROPERTIES
            IMPORTED_CONFIGURATIONS "Release;Debug"
            IMPORTED_LOCATION_RELEASE ${_qj_3rd_library_path}
            IMPORTED_LOCATION_DEBUG   ${_qj_3rd_libray_path_debug}
            IMPORTED_IMPLIB_RELEASE ${_qj_3rd_library_path}
            IMPORTED_IMPLIB_DEBUG   ${_qj_3rd_libray_path_debug}
            INTERFACE_INCLUDE_DIRECTORIES ${_incluir}
        )
    else()
        set_target_properties(${_target} PROPERTIES
            IMPORTED_LOCATION ${_qj_3rd_library_path}
            IMPORTED_IMPLIB ${_qj_3rd_library_path}
            INTERFACE_INCLUDE_DIRECTORIES ${_incluir}
        )
    endif()

    if (NOT ${_linklibs} STREQUAL "")
        set_target_properties(${_target} PROPERTIES
            INTERFACE_LINK_LIBRARIES ${_linklibs}
        )
    endif()

    if (NOT ${_compileopts} STREQUAL "")
        set_target_properties(${_target} PROPERTIES
            INTERFACE_COMPILE_OPTIONS ${_compileopts}
        )
    endif()
endfunction()


# libhv 具体使用

例如 lib_hv 库导入，文件结构为 
3rdparty/libhv/bin/(hv.dll + hv_d.dll)
3rdparty/libhv/lib/(hv.lib + hv_d.lib)
3rdparty/libhv/include/hv

则使用如下：

get_filename_component(libhv_DIR "${CMAKE_CURRENT_LIST_DIR}/../libhv" ABSOLUTE)

set(libhv_FOUND TRUE)
set(libhv_INCLUDE_DIRS "${libhv_DIR}/include")
set(libhv_LIBRARY_DIR "${libhv_DIR}/lib")
include(${CMAKE_CURRENT_LIST_DIR}/3rdparty_macro.cmake)

if(NOT TARGET libhv)
    QJ_3RDPARTY_ADD_LIBRARY(
        libhv
        hv
        ${libhv_INCLUDE_DIRS}
        ${libhv_LIBRARY_DIR}
        ""
        ""
        SHARED)
endif()



# QCefView 导入配置

例如 QCefView 库的导入，文件结构为
QCefView/Debug/bin/QCefView.dll
QCefView/Debug/lib/QCefView.lib
QCefView/Release/bin/QCefView.dll
QCefView/Release/lib/QCefView.lib

则使用如下：

get_filename_component(QCEFVIEW_DIR "${CMAKE_CURRENT_LIST_DIR}/../QCefView" ABSOLUTE)

set(QCEFVIEW_FOUND TRUE)

if (NOT TARGET QCefView)
    add_library(QCefView SHARED IMPORTED)
    set_target_properties(QCefView PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${QCEFVIEW_DIR}/include"
        IMPORTED_IMPLIB_DEBUG   "${QCEFVIEW_DIR}/Debug/lib/QCefView.lib"
        IMPORTED_LOCATION_DEBUG "${QCEFVIEW_DIR}/Debug/bin/QCefView.dll"
        IMPORTED_IMPLIB_RELEASE "${QCEFVIEW_DIR}/Release/lib/QCefView.lib"
        IMPORTED_LOCATION_RELEASE "${QCEFVIEW_DIR}/Release/bin/QCefView.dll"
        IMPORTED_IMPLIB "${QCEFVIEW_DIR}/Debug/lib/QCefView.lib"
        IMPORTED_LOCATION "${QCEFVIEW_DIR}/Debug/bin/QCefView.dll"
    )
endif()