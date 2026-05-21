cmake_minimum_required(VERSION 3.28)
project(gobang_qt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(DEFINED Qt6_DIR AND EXISTS "${Qt6_DIR}/Qt6Config.cmake")
    get_filename_component(_qt6_prefix "${Qt6_DIR}/../../.." ABSOLUTE)
    list(APPEND CMAKE_PREFIX_PATH "${_qt6_prefix}")
    message(STATUS "Qt6 found at ${_qt6_prefix}")
elseif(DEFINED Qt5_DIR AND EXISTS "${Qt5_DIR}/Qt5Config.cmake")
    get_filename_component(_qt5_prefix "${Qt5_DIR}/../../.." ABSOLUTE)
    list(APPEND CMAKE_PREFIX_PATH "${_qt5_prefix}")
    message(STATUS "Qt5 found at ${_qt5_prefix}")
endif()

set(_gobang_qt_hints
    "$ENV{Qt6_DIR}"
    "$ENV{QT6_DIR}"
    "$ENV{QT_DIR}"
    "$ENV{QTDIR}"
    "$ENV{CMAKE_PREFIX_PATH}"
)

if(WIN32)
    file(GLOB _gobang_qt_windows_roots LIST_DIRECTORIES true
        "C:/Qt/6.*/mingw_*"
        "C:/Qt/6.*/msvc*"
        "C:/Qt/5.*/mingw*_64"
        "C:/Qt/5.*/mingw*"
        "C:/Qt/5.*/msvc*"
        "D:/Qt/6.*/mingw_*"
        "D:/Qt/6.*/msvc*"
        "D:/Qt/5.*/mingw*_64"
        "D:/Qt/5.*/mingw*"
        "D:/Qt/5.*/msvc*"
        "$ENV{USERPROFILE}/Qt/6.*/mingw_*"
        "$ENV{USERPROFILE}/Qt/6.*/msvc*"
        "$ENV{USERPROFILE}/Qt/5.*/mingw*_64"
        "$ENV{USERPROFILE}/Qt/5.*/mingw*"
        "$ENV{USERPROFILE}/Qt/5.*/msvc*"
    )
    list(APPEND _gobang_qt_hints ${_gobang_qt_windows_roots})
else()
    list(APPEND _gobang_qt_hints
        "/opt/homebrew/opt/qtbase"
        "/opt/homebrew"
        "/usr/local/opt/qtbase"
        "/opt/homebrew/opt/qt"
        "/usr/local/opt/qt"
    )
endif()

foreach(_hint IN LISTS _gobang_qt_hints)
    if(DEFINED Qt6_DIR OR DEFINED Qt5_DIR)
        break()
    endif()

    if(NOT _hint)
        continue()
    endif()

    if(EXISTS "${_hint}/Qt6Config.cmake")
        get_filename_component(_qt6_dir "${_hint}" ABSOLUTE)
        get_filename_component(_qt_prefix "${_qt6_dir}/../../.." ABSOLUTE)
        set(Qt6_DIR "${_qt6_dir}" CACHE PATH "Qt6 CMake package directory" FORCE)
        list(APPEND CMAKE_PREFIX_PATH "${_qt_prefix}")
        message(STATUS "Qt6 found at ${_qt_prefix}")
        break()
    endif()

    if(EXISTS "${_hint}/lib/cmake/Qt6/Qt6Config.cmake")
        get_filename_component(_qt_prefix "${_hint}" ABSOLUTE)
        set(Qt6_DIR "${_qt_prefix}/lib/cmake/Qt6" CACHE PATH "Qt6 CMake package directory" FORCE)
        list(APPEND CMAKE_PREFIX_PATH "${_qt_prefix}")
        message(STATUS "Qt6 found at ${_hint}")
        break()
    endif()

    if(EXISTS "${_hint}/lib/cmake/Qt5/Qt5Config.cmake")
        get_filename_component(_qt_prefix "${_hint}" ABSOLUTE)
        set(Qt5_DIR "${_qt_prefix}/lib/cmake/Qt5" CACHE PATH "Qt5 CMake package directory" FORCE)
        list(APPEND CMAKE_PREFIX_PATH "${_qt_prefix}")
        message(STATUS "Qt5 found at ${_hint}")
        break()
    endif()
endforeach()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network Sql)

add_subdirectory(shared)
add_subdirectory(server)
add_subdirectory(client)
