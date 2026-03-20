#include<iostream>
#include<fstream>
#include<SDL2/SDL.h>
#include<SDL2/SDL_image.h>
#include<glm/glm.hpp>
#include "Game.h"
#include "../Logger/Logger.h"
#include "../ECS/ECS.h"
#include "../Components/TransformComponent.h"
#include "../Components/RigidBodyComponent.h"
#include "../Components/AnimationComponent.h"
#include "../Components/SpriteComponent.h"
#include "../Systems/MovementSystem.h"
#include "../Systems/RenderSystem.h"
#include "../Systems/AnimationSystem.h"


Game::Game() {
    isRunning = false;
    registry = std::make_unique<Registry>();
    assetStore = std::make_unique<AssetStore>();
    Logger::Log("Game constructor called!");
}

Game::~Game() {
    Logger::Log("Game destructor called!");
}

void Game::Initialize() {
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        Logger::Err("Error initializing SDL.");
        return;
    }

    SDL_DisplayMode displayMode;
    SDL_GetCurrentDisplayMode(0, &displayMode);
    windowWidth = 800;//displayMode.w;
    windowHeight = 600;//displayMode.h;

    window = SDL_CreateWindow(
        "Zheng Zhang Game Engine", 
        SDL_WINDOWPOS_CENTERED, 
        SDL_WINDOWPOS_CENTERED, 
        windowWidth,
        windowHeight,
        SDL_WINDOW_BORDERLESS
    );
    if (!window) {
        Logger::Err("Error creating SDL window.");
        return;
    }
    renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer) {
        Logger::Err("Error creating SDL renderer.");
        return;
    }
    SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
    isRunning = true;
}

void Game::Run() {
    Setup();
    while (isRunning) {
        ProcessInput();
        Update();
        Render();
    }
}

void Game::ProcessInput() {
    SDL_Event sdlEvent;
    while (SDL_PollEvent(&sdlEvent)) {
        switch (sdlEvent.type) {
            case SDL_QUIT:
                isRunning = false;
                break;
            case SDL_KEYDOWN:
                if (sdlEvent.key.keysym.sym == SDLK_ESCAPE) {
                    isRunning = false;
                }
                break;
        }
    }
}

void Game::LoadLevel(int level) {
    // 把需要处理的系统加载到游戏中
    registry->AddSystem<MovementSystem>();
    registry->AddSystem<RenderSystem>();
    registry->AddSystem<AnimationSystem>();

    // 增加资产到资产库
    assetStore->AddTexture(renderer, "tank-image", "./assets/images/tank-panther-right.png");
    assetStore->AddTexture(renderer, "truck-image", "./assets/images/truck-ford-right.png");
    assetStore->AddTexture(renderer, "chopper-image", "./assets/images/chopper.png");
    assetStore->AddTexture(renderer, "radar-image", "./assets/images/radar.png");
    assetStore->AddTexture(renderer, "tilemap-image", "./assets/tilemaps/jungle.png");

    // 加载瓦片地图
    // ./assets/tilemaps/jungle.png 从资源中加载TileMap纹理
    // ./assets/jungle/jungle.map 根据逗号分割 拆分所有数据 读取数字 根据这些数据定位到png文件中正确的小区域
    // Tip: 你可以使用源矩形这个概念
    // Tip: 考虑为每个瓦片简历一个单独的实体
    int tileSize = 32;
    double tileScale = 4.0;
    int mapNumCols = 25;
    int mapNumRows = 20;

    std::fstream mapFile;
    mapFile.open("./assets/tilemaps/jungle.map");

    for (int y = 0; y < mapNumRows; y++) {
        for (int x = 0; x < mapNumCols; x++) {
            char ch;
            mapFile.get(ch);
            int srcRectY = std::atoi(&ch) * tileSize;
            mapFile.get(ch);
            int srcRectX = std::atoi(&ch) * tileSize;
            mapFile.ignore(); // 跳过逗号

            Entity tile = registry->CreateEntity();
            tile.AddComponent<TransformComponent>(glm::vec2(x * (tileScale * tileSize), y * (tileScale * tileSize)),  glm::vec2(tileScale, tileScale), 0.0);
            tile.AddComponent<SpriteComponent>("tilemap-image", tileSize, tileSize, 0, srcRectX, srcRectY);

        }
    }

    mapFile.close();

    // 创建一个实体
    Entity chopper = registry->CreateEntity();    
    // 增加一些组件到实体
    chopper.AddComponent<TransformComponent>(glm::vec2(10.0, 10.0), glm::vec2(4.0, 4.0), 0.0);
    chopper.AddComponent<RigidBodyComponent>(glm::vec2(0.0, 0.0));
    chopper.AddComponent<SpriteComponent>("chopper-image", 32, 32, 1);
    chopper.AddComponent<AnimationComponent>(2, 15, true);

    Entity radder = registry->CreateEntity();    
    radder.AddComponent<TransformComponent>(glm::vec2(windowWidth-74, 10.0), glm::vec2(1.0, 1.0), 0.0);
    radder.AddComponent<RigidBodyComponent>(glm::vec2(0.0, 0.0));
    radder.AddComponent<SpriteComponent>("radar-image", 64, 64, 2);
    radder.AddComponent<AnimationComponent>(8, 5, true);

    // // 创建一个实体
    // Entity tank = registry->CreateEntity();    
    // // 增加一些组件到实体
    // tank.AddComponent<TransformComponent>(glm::vec2(10.0, 30.0), glm::vec2(4.0, 4.0), 0.0);
    // tank.AddComponent<RigidBodyComponent>(glm::vec2(40.0, 0.0));
    // tank.AddComponent<SpriteComponent>("tank-image", 32, 32, 2);

    // Entity truck = registry->CreateEntity();
    // truck.AddComponent<TransformComponent>(glm::vec2(50.0, 100.0), glm::vec2(1.0, 1.0), 0.0);
    // truck.AddComponent<RigidBodyComponent>(glm::vec2(0.0, 50.0));
    // truck.AddComponent<SpriteComponent>("truck-image", 32, 32, 1);

}

void Game::Setup() {
    LoadLevel(1);
    
}

void Game::Update() {
    // TODO: 如果我们运行的太快，我们会浪费几个时钟周期
    // while (!SDL_TICKS_PASSED(SDL_GetTicks(), millsecsPreviousFrame + MILLISECS_PER_FRAME));
    int timeToWait = MILLISECS_PER_FRAME - (SDL_GetTicks() - millsecsPreviousFrame);
    if (timeToWait > 0 && timeToWait <= MILLISECS_PER_FRAME) {
        SDL_Delay(timeToWait);
    }
    // 自上一帧以来的时间戳差异，转换成秒
    double deltaTime = (SDL_GetTicks() - millsecsPreviousFrame) / 1000.0;

    // 记录这一帧的开始时间 
    millsecsPreviousFrame = SDL_GetTicks();

    // 让所有系统更新
    registry->GetSystem<MovementSystem>().Update(deltaTime);
    registry->GetSystem<AnimationSystem>().Update();

    // 更新注册表 以 创建或销毁等待的实体
    registry->Update();
    
}

void Game::Render() {
    SDL_SetRenderDrawColor(renderer, 21, 21, 12, 255);
    SDL_RenderClear(renderer);

    // 调用所有需要渲染的系统
    registry->GetSystem<RenderSystem>().Update(renderer, assetStore);
    

    SDL_RenderPresent(renderer);
}

void Game::Destroy() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}