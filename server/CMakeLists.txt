add_executable(gobang_server
    src/main.cpp
    src/gobang_server.cpp
    src/gobang_server.h
    src/account_store.cpp
    src/account_store.h
)

target_link_libraries(gobang_server
    PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Sql gobang_shared
)

target_include_directories(gobang_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
