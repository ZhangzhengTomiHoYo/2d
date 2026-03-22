#ifndef HEALTHCOMPONENT_H
#define HEALTHCOMPONENT_H

// 生命值组件：用于给实体（玩家/敌人）添加生命值属性，基于ECS架构
struct HealthComponent {
    // 生命值百分比（0-100，0表示死亡，100表示满血）
    int healthPercentage;

    // 构造函数：默认初始生命值为0，可传入自定义初始值
    HealthComponent(int healthPercentage = 0) {
        this->healthPercentage = healthPercentage;
    }
};

#endif