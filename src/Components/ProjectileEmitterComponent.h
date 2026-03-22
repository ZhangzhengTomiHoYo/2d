#ifndef PROJECTILEEMITTERCOMPONENT_H
#define PROJECTILEEMITTERCOMPONENT_H

#include <glm/glm.hpp>
#include <SDL2/SDL.h>

// 抛射体发射器组件：用于给实体（如玩家/敌人）添加发射子弹/技能的能力，基于ECS架构
struct ProjectileEmitterComponent {
    glm::vec2 projectileVelocity;    // 抛射体的发射速度（x/y方向）
    int repeatFrequency;             // 重复发射的频率（单位：毫秒，0表示不重复）
    int projectileDuration;          // 抛射体的存活时间（单位：毫秒，默认10000=10秒）
    int hitPercentDamage;             // 命中时造成的生命值百分比伤害（默认10%）
    bool isFriendly;                  // 是否为友方抛射体（true=不会伤害友方，false=伤害敌方，默认false）
    int lastEmissionTime;             // 上一次发射抛射体的时间（用于控制发射间隔）

    // 构造函数：带默认参数，方便创建不同类型的发射器
    ProjectileEmitterComponent(
        glm::vec2 projectileVelocity = glm::vec2(0),
        int repeatFrequency = 0,
        int projectileDuration = 10000,
        int hitPercentDamage = 10,
        bool isFriendly = false
    ) {
        this->projectileVelocity = projectileVelocity;
        this->repeatFrequency = repeatFrequency;
        this->projectileDuration = projectileDuration;
        this->hitPercentDamage = hitPercentDamage;
        this->isFriendly = isFriendly;
        // 初始化时记录当前SDL时间，作为首次发射的时间基准
        this->lastEmissionTime = SDL_GetTicks();
    }
};

#endif