#ifndef PROJECTILEEMITSYSTEM_H
#define PROJECTILEEMITSYSTEM_H

#include "../ECS/ECS.h"
#include "../Components/TransformComponent.h"
#include "../Components/RigidBodyComponent.h"
#include "../Components/SpriteComponent.h"
#include "../Components/BoxColliderComponent.h"
#include "../Components/ProjectileEmitterComponent.h"
#include "../Components/ProjectileComponent.h"
#include <SDL2/SDL.h>

// 抛射体发射系统：处理所有带发射器组件的实体，按间隔生成子弹/技能抛射体
class ProjectileEmitSystem: public System {
public:
    ProjectileEmitSystem() {
        // 要求实体必须同时拥有【抛射体发射器组件】和【变换组件】，才能被该系统处理
        RequireComponent<ProjectileEmitterComponent>();
        RequireComponent<TransformComponent>();
    }

    void SubscribeToEvents(std::unique_ptr<EventBus>& eventBus) {
        // 修复：使用正确的成员函数指针语法
        eventBus->SubscribeToEvent<KeyPressedEvent>(this, &ProjectileEmitSystem::OnKeyPressed);
    }

    // 修复：方法名改为 OnKeyPressed 以匹配截图和调用
    void OnKeyPressed(KeyPressedEvent& event) {
        if (event.symbol == SDLK_SPACE) {
            for (auto entity: GetSystemEntities()) {
                if (entity.HasTag("player")) {
                    const auto projectileEmitter = entity.GetComponent<ProjectileEmitterComponent>();
                    const auto transform = entity.GetComponent<TransformComponent>();
                    const auto rigidbody = entity.GetComponent<RigidBodyComponent>();

                    // 如果父实体有精灵图，则让投射物从中心位置发射
                    glm::vec2 projectilePosition = transform.position;
                    if (entity.HasComponent<SpriteComponent>()) {
                        auto sprite = entity.GetComponent<SpriteComponent>();
                        projectilePosition.x += (transform.scale.x * sprite.width / 2);
                        projectilePosition.y += (transform.scale.y * sprite.height / 2);
                    }

                    // 如果父实体的方向由键盘控制，根据朝向修改投射物速度
                    glm::vec2 projectileVelocity = projectileEmitter.projectileVelocity;
                    int directionX = 0;
                    int directionY = 0;
                    if (rigidbody.velocity.x > 0) directionX = +1;
                    if (rigidbody.velocity.x < 0) directionX = -1;
                    if (rigidbody.velocity.y > 0) directionY = +1;
                    if (rigidbody.velocity.y < 0) directionY = -1;
                    projectileVelocity.x = projectileEmitter.projectileVelocity.x * directionX;
                    projectileVelocity.y = projectileEmitter.projectileVelocity.y * directionY;

                    // 生成新的投射物实体并加入世界
                    Entity projectile = entity.registry->CreateEntity();
                    projectile.Group("projectiles");
                    
                    // 添加组件 (注：部分参数在截图边缘被截断，这里补全了常规的默认参数)
                    projectile.AddComponent<TransformComponent>(projectilePosition, glm::vec2(1.0, 1.0), 0.0);
                    projectile.AddComponent<RigidBodyComponent>(projectileVelocity);
                    projectile.AddComponent<SpriteComponent>("bullet-image", 4, 4, 4);
                    projectile.AddComponent<BoxColliderComponent>(4, 4);
                    projectile.AddComponent<ProjectileComponent>(
                        projectileEmitter.isFriendly,
                        projectileEmitter.hitPercentDamage,
                        projectileEmitter.projectileDuration
                    );
                }
            }
        }
    }

    void Update(std::unique_ptr<Registry>& registry) {
        // 遍历所有符合条件的实体（玩家/敌人/陷阱等发射器）
        for (auto entity: GetSystemEntities()) {
            // 获取发射器组件（引用方式，可直接修改）和变换组件
            auto& projectileEmitter = entity.GetComponent<ProjectileEmitterComponent>();
            const auto transform = entity.GetComponent<TransformComponent>();

            // If emission frequency is zero, bypass re-emission logic
            if (projectileEmitter.repeatFrequency == 0) {
                continue;
            }

            // ========== 发射间隔判断 ==========
            // 当前时间 - 上次发射时间 > 发射频率 → 满足发射条件
            if (SDL_GetTicks() - projectileEmitter.lastEmissionTime > projectileEmitter.repeatFrequency) {
                
                // ========== 计算抛射体生成位置（从发射器中心发射） ==========
                glm::vec2 projectilePosition = transform.position;
                // 如果发射器实体有精灵组件，将抛射体生成在精灵中心，而非实体原点
                if (entity.HasComponent<SpriteComponent>()) {
                    const auto sprite = entity.GetComponent<SpriteComponent>();
                    projectilePosition.x += (transform.scale.x * sprite.width / 2);
                    projectilePosition.y += (transform.scale.y * sprite.height / 2);
                }

                // ========== 创建抛射体实体并添加组件 ==========
                Entity projectile = registry->CreateEntity();
                projectile.Group("projectiles");
                // 1. 变换组件：位置（发射器中心）、缩放（1倍）、旋转（0度）
                projectile.AddComponent<TransformComponent>(projectilePosition, glm::vec2(1.0, 1.0), 0.0);
                // 2. 刚体组件：赋予抛射体速度，按发射器设定的方向/速度移动
                projectile.AddComponent<RigidBodyComponent>(projectileEmitter.projectileVelocity);
                // 3. 精灵组件：使用"bullet-image"资源，大小4x4，渲染层级4
                projectile.AddComponent<SpriteComponent>("bullet-image", 4, 4, 4);
                // 4. 碰撞盒组件：4x4大小，用于碰撞检测（命中敌人/玩家）
                projectile.AddComponent<BoxColliderComponent>(4, 4);
                // 5. 补充：添加抛射体生命周期组件，超时自动销毁（建议补充）
                projectile.AddComponent<ProjectileComponent>(
                    projectileEmitter.isFriendly,
                    projectileEmitter.hitPercentDamage,
                    projectileEmitter.projectileDuration
                );

                // ========== 更新上次发射时间，实现发射间隔控制 ==========
                projectileEmitter.lastEmissionTime = SDL_GetTicks();
            }
        }
    }
};

#endif
