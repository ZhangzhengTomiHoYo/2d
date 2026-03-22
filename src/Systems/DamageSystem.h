#ifndef DAMAGESYSTEM_H
#define DAMAGESYSTEM_H

#include "../ECS/ECS.h"
#include "../Components/BoxColliderComponent.h"
#include "../Components/ProjectileComponent.h"
#include "../Components/HealthComponent.h"
#include "../EventBus/EventBus.h"
#include "../Events/CollisionEvent.h"

class DamageSystem: public System {
    public:
        DamageSystem() {
            RequireComponent<BoxColliderComponent>();
        }

        void SubscribeToEvents(std::unique_ptr<EventBus>& eventBus) {
            eventBus->SubscribeToEvent<CollisionEvent>(this, &DamageSystem::onCollision);
        }

        void onCollision(CollisionEvent& event) {
            Entity a = event.a;
            Entity b = event.b;

            Logger::Log("伤害系统检测到了实体之间的碰撞 " + std::to_string(event.a.GetId()) + " and " + std::to_string(event.b.GetId()));
            
            if (a.BelongsToGroup("projectiles") && b.HasTag("player")) {
                OnProjectileHitsPlayer(a, b); // "a" 是子弹，"b" 是玩家
            }

            if (b.BelongsToGroup("projectiles") && a.HasTag("player")) {
                OnProjectileHitsPlayer(b, a); // "b" 是子弹，"a" 是玩家
            }

            if (a.BelongsToGroup("projectiles") && b.BelongsToGroup("enemies")) {
                OnProjectileHitsEnemy(a, b); // "a" 是子弹，"b" 是敌人
            }

            if (b.BelongsToGroup("projectiles") && a.BelongsToGroup("enemies")) {
                OnProjectileHitsEnemy(b, a); // "b" 是子弹，"a" 是敌人
            }
        }

        void OnProjectileHitsPlayer(Entity projectile, Entity player) {
            auto projectileComponent = projectile.GetComponent<ProjectileComponent>();

            // 仅处理非友方子弹（敌方子弹）
            if (!projectileComponent.isFriendly) {
                // 获取玩家的生命值组件（引用方式，直接修改原数据）
                auto& health = player.GetComponent<HealthComponent>();

                // 按子弹伤害百分比扣除玩家生命值
                health.healthPercentage -= projectileComponent.hitPercentDamage;

                // 生命值归零则击杀玩家
                if (health.healthPercentage <= 0) {
                    player.Kill();
                }

                // 销毁命中的子弹
                projectile.Kill();
            }
        }

        void OnProjectileHitsEnemy(Entity projectile, Entity enemy) {
            const auto projectileComponent = projectile.GetComponent<ProjectileComponent>();

            // 仅当子弹为友方（玩家发射）时，才对敌人造成伤害
            if (projectileComponent.isFriendly) {
                // 获取敌人的生命值组件（引用方式，直接修改原数据）
                auto& health = enemy.GetComponent<HealthComponent>();

                // 按子弹伤害百分比扣除敌人生命值
                health.healthPercentage -= projectileComponent.hitPercentDamage;

                // 生命值归零则击杀敌人
                if (health.healthPercentage <= 0) {
                    enemy.Kill();
                }

                // 无论是否造成伤害，命中后统一销毁子弹
                projectile.Kill();
            }
        }

        void Update() {

        }
};

#endif