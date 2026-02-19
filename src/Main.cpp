#include "BattleState.h"
#include "Blocker.h"
#include "Encounter.h"
#include "Evolution.h"
#include "FlyLocation.h"
#include "GameConfig.h"
#include "Giver.h"
#include "Item.h"
#include "LevelUpMove.h"
#include "Main.h"
#include "MartItem.h"
#include "Monster.h"
#include "Move.h"
#include "Npc.h"
#include "OverworldGui.h"
#include "PokeMap.h"
#include "Player.h"
#include "SpriteManager.h"
#include "TextRenderer.h"
#include "TmLearnsets.h"
#include "Trader.h"
#include "Trainer.h"
#include "Types.h"
#include "Warp.h"
#include "WorldObject.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <algorithm>
#include <filesystem>
#include <functional>
#include <fstream>
#include <iostream>
#include <vector>

static bool initSdl(SDL_Window *&window, SDL_Renderer *&renderer) {
    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);

    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "windows");
    SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "wasapi");

    // Log available video drivers
    int numDrivers = SDL_GetNumVideoDrivers();
    std::cerr << "SDL video drivers (" << numDrivers << "):";
    for (int i = 0; i < numDrivers; ++i) {
        const char *d = SDL_GetVideoDriver(i);
        if (d) std::cerr << " " << d;
    }
    std::cerr << std::endl;

    bool initOk = SDL_Init(SDL_INIT_VIDEO);
    if (!initOk) {
        const char *err = SDL_GetError();
        std::cerr << "SDL_Init(SDL_INIT_VIDEO) failed: " << (err ? err : "") << std::endl;
        std::cerr << "Retrying with offscreen video driver..." << std::endl;
        SDL_Quit();
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
        initOk = SDL_Init(SDL_INIT_VIDEO);
        if (!initOk) {
            err = SDL_GetError();
            std::cerr << "SDL_Init(SDL_INIT_VIDEO) with offscreen failed: " << (err ? err : "") << std::endl;
            std::cerr << "Retrying with dummy video driver..." << std::endl;
            SDL_Quit();
            SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
            initOk = SDL_Init(SDL_INIT_VIDEO);
            if (!initOk) {
                err = SDL_GetError();
                std::cerr << "SDL_Init(SDL_INIT_VIDEO) with dummy failed: " << (err ? err : "") << std::endl;
                return false;
            }
        }
    }
    const char *chosenDriver = SDL_GetCurrentVideoDriver();
    std::cerr << "Using video driver: " << (chosenDriver ? chosenDriver : "<none>") << std::endl;
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        const char *err = SDL_GetError();
        std::cerr << "SDL audio init warning: " << (err ? err : "") << std::endl;
    }
    if (!TTF_Init()) {
        const char *terr = SDL_GetError();
        std::cerr << "TTF_Init failed: " << (terr ? terr : "") << std::endl;
        return false;
    }
    window = SDL_CreateWindow("Pokemon Purple", GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT, 0);
    if (!window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        return false;
    }
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
        return false;
    }
    SDL_SetRenderLogicalPresentation(renderer, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT,
                                     SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
    SDL_SetRenderVSync(renderer, 1);
    SpriteManager::setRenderer(renderer);
    return true;
}

static std::string findFontPath() {
    std::vector<std::string> candidates;
    candidates.push_back("assets/PKMN-RBYGSC.ttf");

    const char *base = SDL_GetBasePath();
    if (base) {
        std::string basePath(base);
        SDL_free(const_cast<char *>(base));
        candidates.push_back(basePath + "assets/PKMN-RBYGSC.ttf");
        candidates.push_back(basePath + "..\\assets\\PKMN-RBYGSC.ttf");
        candidates.push_back(basePath + "..\\..\\assets\\PKMN-RBYGSC.ttf");
    }

    for (const auto &path : candidates) {
        std::ifstream file(path);
        if (file.good()) {
            return path;
        }
    }
    return candidates.front();
}

static bool fileExists(const std::string &path) {
    return std::filesystem::exists(std::filesystem::u8path(path));
}

static void ensureWorkingDir() {
    namespace fs = std::filesystem;
    fs::path cwd = fs::current_path();
    if (fs::exists(cwd / "assets")) {
        return;
    }
    const char *base = SDL_GetBasePath();
    if (!base) {
        return;
    }
    fs::path basePath(base);
    SDL_free(const_cast<char *>(base));
    fs::path candidate = fs::weakly_canonical(basePath / ".." / "..");
    if (fs::exists(candidate / "assets")) {
        fs::current_path(candidate);
    }
}

static void logStartupInfo(const std::string &fontPath) {
    std::vector<std::string> assetChecks = {
        "assets/tiles/0.png",
        "assets/sprites/RED/0.png",
        "assets/sprites/SEEL/0.png",
        "assets/battlers/0.png",
        "assets/battlers_back/0.png",
        "assets/dance/0.png"
    };
    std::vector<std::string> dataChecks = {
        "data/maps/RedsHouse2F.txt",
        "data/maps/WorldMap.txt",
        "data/maps/Warps.txt"
    };
    (void)assetChecks;
    (void)dataChecks;
}

static void shutdownSdl(SDL_Window *window, SDL_Renderer *renderer) {
    SpriteManager::clear();
    TextRenderer::shutdown();
    if (renderer) {
        SDL_DestroyRenderer(renderer);
    }
    if (window) {
        SDL_DestroyWindow(window);
    }
    TTF_Quit();
    SDL_Quit();
}

static void buildGameData() {
    auto runStep = [](const char *name, const std::function<void()> &fn) {
        (void)name;
        fn();
    };
    runStep("Types", [] { Types::buildTypes(); });
    runStep("Monsters", [] { Monster::buildMonsters(); });
    runStep("Moves", [] { Move::buildMoves(); });
    runStep("Learnsets", [] { LevelUpMove::buildLevelUpMoves(); });
    runStep("Evolutions", [] { Evolution::buildEvolutions(); });
    runStep("PokeMaps", [] { PokeMap::buildPokeMaps(); });
    runStep("WorldMap", [] { FlyLocation::buildWorldMap(); });
    runStep("Warps", [] { Warp::buildWarps(); });
    runStep("TmLearnsets", [] { TmLearnsets::buildTmLearnsets(); });
    runStep("Encounters", [] { Encounter::buildEncounterRates(); });
    runStep("Items", [] { Item::buildItems(); });
    runStep("MartItems", [] { MartItem::buildMartItems(); });
    runStep("Givers", [] { Giver::buildGivers(); });
    runStep("Blockers", [] { Blocker::buildBlockers(); });
    runStep("Traders", [] { Trader::buildTraders(); });
    runStep("Npcs", [] { Npc::buildNpcs(); });
}

int main(int argc, char **argv) {
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    if (!initSdl(window, renderer)) {
        shutdownSdl(window, renderer);
        return 1;
    }
    ensureWorkingDir();
    std::string fontPath = findFontPath();
    logStartupInfo(fontPath);
    if (!TextRenderer::init(fontPath, 24)) {
        std::cerr << "Failed to load font " << fontPath << std::endl;
    }

    try {
        buildGameData();
    } catch (const std::exception &e) {
        std::cerr << "buildGameData failed: " << e.what() << std::endl;
        shutdownSdl(window, renderer);
        return 1;
    } catch (...) {
        std::cerr << "buildGameData failed: unknown error" << std::endl;
        shutdownSdl(window, renderer);
        return 1;
    }

    try {
        Player *player = nullptr;
        std::ifstream saveFile("save.txt");
        if (saveFile.good()) {
            std::string pInfo;
            std::string pcS;
            std::string partyS;
            std::string trainS;
            std::string leadS;
            std::string gioRS;
            std::string wobS;
            std::string dexS;
            std::string itemS;
            std::string tmS;
            std::string fLoc;
            std::getline(saveFile, pInfo);
            std::getline(saveFile, pcS);
            std::getline(saveFile, partyS);
            std::getline(saveFile, trainS);
            std::getline(saveFile, leadS);
            std::getline(saveFile, gioRS);
            std::getline(saveFile, wobS);
            std::getline(saveFile, dexS);
            std::getline(saveFile, itemS);
            std::getline(saveFile, tmS);
            std::getline(saveFile, fLoc);
            player = new Player(pcS, partyS, dexS, itemS, tmS);
            for (size_t i = 0; i < fLoc.size() && i + 1 < FlyLocation::INDEX_MEANINGS.size(); i++) {
                if (fLoc[i] == '1') {
                    FlyLocation::FLY_LOCATIONS[FlyLocation::INDEX_MEANINGS[i + 1]]->visited = true;
                }
            }
            Trainer::buildTrainers(player, trainS, leadS, gioRS);
            WorldObject::buildWorldObjects(player, wobS);
            if (player->hasItem(Item::ITEM_MAP["Shiny Charm"])) {
                Main::SHINY_CHANCE = 256;
            }
            OverworldGui gui(player, renderer, pInfo);
            bool running = true;
            while (running) {
                SDL_Event event;
                while (SDL_PollEvent(&event)) {
                    if (event.type == SDL_EVENT_QUIT) {
                        running = false;
                    }
                    gui.handleEvent(event);
                }
                gui.update();
                gui.render();
                SDL_Delay(16);
            }
        } else {
            Trainer::buildTrainers();
            WorldObject::buildWorldObjects();
            player = new Player("Purple");
            OverworldGui gui(player, renderer);
            std::string name = OverworldGui::promptText(
                "Welcome to Pokemon Purple! Controls are in the README.\nWhat is your name?");
            if (!name.empty()) {
                name.erase(std::remove(name.begin(), name.end(), ','), name.end());
                if (name.empty()) {
                    player->name = "Purple";
                } else {
                    player->name = name.size() < 11 ? name : name.substr(0, 10);
                }
            }
            OverworldGui::pickingStarter = true;
            bool running = true;
            while (running) {
                SDL_Event event;
                while (SDL_PollEvent(&event)) {
                    if (event.type == SDL_EVENT_QUIT) {
                        running = false;
                    }
                    gui.handleEvent(event);
                }
                gui.update();
                gui.render();
                SDL_Delay(16);
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "Runtime error: " << e.what() << std::endl;
        shutdownSdl(window, renderer);
        return 1;
    } catch (...) {
        std::cerr << "Runtime error: unknown error" << std::endl;
        shutdownSdl(window, renderer);
        return 1;
    }
    shutdownSdl(window, renderer);
    return 0;
}
#include "Main.h"

int Main::SHINY_CHANCE = 8192;
