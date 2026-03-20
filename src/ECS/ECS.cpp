#include "ECS.h"
#include "../Logger/Logger.h"
#include <unordered_map>
#include <typeindex>
#include <set>
#include <algorithm> 

int IComponent::nextId = 0;

int Entity::GetId() const {
    return id;
}

void System::AddEntityToSystem(Entity entity) {
    entities.push_back(entity);
}

void System::RemoveEntityFromSystem(Entity entity) {
    entities.erase(std::remove_if(entities.begin(), entities.end(), [&entity](Entity other) {
        return entity == other;
    }), entities.end());
}

std::vector<Entity> System::GetSystemEntities() const {
    return entities;
}

const Signature& System::GetComponentSignature() const {
    return componentSignature;
}

Entity Registry::CreateEntity() {
    int entityId;

    entityId = numEntities++;

    Entity entity(entityId);
    // 持有一个 注册表 指针
    entity.registry = this;
    entitiesToBeAdded.insert(entity);

    // 确保 实体向量签名 能够容纳新的实体
    if (entityId >= entityComponentSignatures.size()) {
        entityComponentSignatures.resize(entityId + 1);
    }

    Logger::Log("实体创建成功, id = " + std::to_string(entityId));

    return entity;
}

void Registry::AddEntityToSystems(Entity entity) {
    const auto entityId = entity.GetId();

    const auto& entityComponentSignature = entityComponentSignatures[entityId];

    // 遍历所有系统
    for (auto& system: systems) {
        const auto& systemComponentSignature = system.second->GetComponentSignature();

        // 进行位运算
        bool isInterested = (systemComponentSignature & entityComponentSignature) == systemComponentSignature;

        if (isInterested) {
            // TODO: 添加实体到系统中
            system.second->AddEntityToSystem(entity);
        }
    }
}

void Registry::Update() {
    // 我们需要把那些等待创建的实体加入到游戏的活动系统中
    for (auto entity: entitiesToBeAdded) {
        AddEntityToSystems(entity);
    }
    entitiesToBeAdded.clear();
    // TODO: 我们需要从活动系统中移除那些等待被销毁的实体
}