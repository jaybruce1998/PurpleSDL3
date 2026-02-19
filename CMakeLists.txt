cmake_minimum_required(VERSION 3.16)
project(PokemonPurple LANGUAGES CXX)
# Hint: on Windows with vcpkg, configure with
# cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL3 REQUIRED CONFIG)
find_package(SDL3_image REQUIRED CONFIG)
find_package(SDL3_ttf REQUIRED CONFIG)

include_directories(
    ${CMAKE_SOURCE_DIR}/include
)

add_executable(PokemonPurple
    src/main.cpp
    src/OverworldGui.cpp
    src/BattleInfoPanel.cpp
    src/BattlerPanel.cpp
    src/BattleState.cpp
    src/PokeMap.cpp
    src/Player.cpp
    src/Battler.cpp
    src/Monster.cpp
    src/Move.cpp
        src/SpriteManager.cpp
    src/Types.cpp
    src/Effect.cpp
    src/Encounter.cpp
    src/Evolution.cpp
    src/LevelUpMove.cpp
    src/Item.cpp
    src/MartItem.cpp
    src/Trainer.cpp
    src/Npc.cpp
    src/NpcStrings.cpp
    src/WorldObject.cpp
    src/Warp.cpp
    src/Giver.cpp
    src/Trader.cpp
    src/Blocker.cpp
    src/FlyLocation.cpp
    src/TmLearnsets.cpp
    src/BlackjackDialog.cpp
    src/TextRenderer.cpp
)

target_link_libraries(PokemonPurple
    SDL3::SDL3
    SDL3_image::SDL3_image
    SDL3_ttf::SDL3_ttf
)

target_compile_definitions(PokemonPurple PRIVATE SDL_ENABLE_OLD_NAMES=1)
