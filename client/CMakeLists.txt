add_executable(gobang_client
    src/main.cpp
    src/login_dialog.cpp
    src/login_dialog.h
    src/network_client.cpp
    src/network_client.h
    src/game_board_widget.cpp
    src/game_board_widget.h
    src/ai_engine.cpp
    src/ai_engine.h
    src/game_controller.cpp
    src/game_controller.h
    src/main_window.cpp
    src/main_window.h
)

target_link_libraries(gobang_client
    PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network gobang_shared
)

target_include_directories(gobang_client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
