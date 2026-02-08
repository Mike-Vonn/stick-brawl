cmake_minimum_required(VERSION 3.20)
project(StickBrawl VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dependencies - SFML 3.0 uses capitalized component names
find_package(SFML 3 COMPONENTS Graphics Window System Audio CONFIG REQUIRED)
find_package(box2d CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

set(SOURCES
    src/main.cpp
    src/Game.cpp
    src/Physics.cpp
    src/StickFigure.cpp
    src/Weapon.cpp
    src/WeaponFactory.cpp
    src/Arena.cpp
    src/Input.cpp
    src/Renderer.cpp
    src/RulesEngine.cpp
    src/HUD.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE src)

target_link_libraries(${PROJECT_NAME} PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
    SFML::Audio
    box2d::box2d
    nlohmann_json::nlohmann_json
)

# Copy assets to build directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
)
