#ifndef CAMERAMOVEMENTSYSTEM_H
#define CAMERAMOVEMENTSYSTEM_H

#include "../ECS/ECS.h"
#include "../Components/CameraFollowComponent.h"
#include "../Components/TransformComponent.h"
#include <SDL2/SDL.h>

// 相机跟随移动系统：实现【玩家居中】的相机跟随，并限制相机在地图/屏幕范围内
class CameraMovementSystem : public System {
public:
    CameraMovementSystem() {
        // 要求实体必须同时拥有【相机跟随组件】和【变换组件】，才能被该系统处理
        RequireComponent<CameraFollowComponent>();
        RequireComponent<TransformComponent>();
    }

    // 更新相机位置的核心方法，传入当前SDL相机矩形
    void Update(SDL_Rect& camera) {
        // 遍历系统管理的所有符合条件的实体（通常是玩家）
        for (auto entity : GetSystemEntities()) {
            // 获取实体的变换组件（包含位置信息）
            auto transform = entity.GetComponent<TransformComponent>();

            // ========== 玩家居中跟随逻辑 ==========
            // 条件：玩家位置 + 半窗口宽 < 地图总宽 → 说明玩家未到地图右边界，允许相机跟随
            if (transform.position.x + (camera.w / 2) < Game::mapWidth) {
                // 相机X = 玩家X - 半窗口宽 → 让玩家始终在屏幕水平居中
                camera.x = transform.position.x - (Game::windowWidth / 2);
            }

            // 条件：玩家位置 + 半窗口高 < 地图总高 → 说明玩家未到地图下边界，允许相机跟随
            if (transform.position.y + (camera.h / 2) < Game::mapHeight) {
                // 相机Y = 玩家Y - 半窗口高 → 让玩家始终在屏幕垂直居中
                camera.y = transform.position.y - (Game::windowHeight / 2);
            }

            // ========== 相机边界限制（修正原代码错误） ==========
            // 1. 限制左边界：相机X不能小于0（防止视野超出地图左上角）
            camera.x = camera.x < 0 ? 0 : camera.x;
            // 2. 限制上边界：相机Y不能小于0（防止视野超出地图左上角）
            camera.y = camera.y < 0 ? 0 : camera.y;
            // 3. 限制右边界：相机X不能超过（地图宽 - 窗口宽），防止视野超出地图右侧
            camera.x = camera.x > (Game::mapWidth - Game::windowWidth) ? (Game::mapWidth - Game::windowWidth) : camera.x;
            // 4. 限制下边界：相机Y不能超过（地图高 - 窗口高），防止视野超出地图底部
            camera.y = camera.y > (Game::mapHeight - Game::windowHeight) ? (Game::mapHeight - Game::windowHeight) : camera.y;
        }
    }
};

#endif