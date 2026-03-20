#ifndef MOVEMENTSYSTEM_H
#define MOVEMENTSYSTEM_H

#include "../ECS/ECS.h"
#include "../Components/TransformComponent.h"
#include "../Components/RigidBodyComponent.h"

class MovementSystem: public System {
    public:
        MovementSystem() {
            RequireComponent<TransformComponent>();
            RequireComponent<RigidBodyComponent>();
        }

        void Update(double deltaTime) {
            // TODO:
            // 遍历系统关注的实体
            for (auto entity: GetSystemEntities()) {
                // 基于实体 速度 更新他的 位置
                auto& transform = entity.GetComponent<TransformComponent>();
                const auto rigidbody = entity.GetComponent<RigidBodyComponent>();

                transform.position.x += rigidbody.velocity.x * deltaTime;
                transform.position.y += rigidbody.velocity.y * deltaTime;

                Logger::Log(
                    "实体 id = " + 
                    std::to_string(entity.GetId()) + 
                    " 位置现在位于 (" + 
                    std::to_string(transform.position.x) + 
                    ", " + 
                    std::to_string(transform.position.y) + ")"
                );
            }
                
        }   
};

#endif