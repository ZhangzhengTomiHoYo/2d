#ifndef ECS_H
#define ECS_H

#include "../Logger/Logger.h"
#include <bitset>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <set>
#include <memory>
#include <deque>

const unsigned int MAX_ENTITIES = 32;

//////////////////////////////////////////////////////////
// 签名 / Signature
//////////////////////////////////////////////////////////
// 我们利用 位集合，使用1和0来追踪一个实体所拥有的各个组件
// 而且，这同样有助于追踪特定系统所关注的实体
//////////////////////////////////////////////////////////
typedef std::bitset<MAX_ENTITIES> Signature;

struct IComponent {
    protected:
        static int nextId;
};

// 用来为组件分配唯一的id
template <typename T>
class Component: public IComponent {
    public:
        // 返回 Component<T> 唯一的id
        static int GetId() {
            static auto id = nextId++;
            return id;
        }
};

class Entity {
    private:
        int id;
    public:
        Entity(int id): id(id) {};
        Entity(const Entity& entity) = default;
        void Kill();
        int GetId() const;

        Entity& operator =(const Entity& other) = default;
        bool operator ==(const Entity& other) const {return id == other.id;}
        bool operator !=(const Entity& other) const {return id != other.id;}
        bool operator >(const Entity& other) const {return id > other.id;}
        bool operator <(const Entity& other) const {return id < other.id;}

        template <typename TComponent, typename ...TArgs> void AddComponent(TArgs&& ...args);
        template <typename TComponent> void RemoveComponent();
        template <typename TComponent> bool HasComponent() const;
        template <typename TComponent> TComponent& GetComponent() const;

        // 持有一个 注册表 指针
        class Registry* registry;

        // 管理实体 tags 和 groups
        void Tag(const std::string& tag);
        bool HasTag(const std::string& tag) const;
        void Group(const std::string& group);
        bool BelongsToGroup(const std::string& group) const;
};

//////////////////////////////////////////////////
// System
//////////////////////////////////////////////////
// 系统处理包含特定 签名/Signature  的实体
//////////////////////////////////////////////////
class System {
    private:
        // 1. 组件签名
        Signature componentSignature;
        // 2. 实体列表
        std::vector<Entity> entities;

    public:
        System() = default;
        ~System() = default;

        void AddEntityToSystem(Entity entity);
        void RemoveEntityFromSystem(Entity entity);
        std::vector<Entity> GetSystemEntities() const;
        const Signature& GetComponentSignature() const;

        // 定义了实体必须拥有的组件类型，以便于系统能够考虑他们
        template <typename TComponent> void RequireComponent();
};

class IPool {
    public:
        virtual ~IPool() = default;
        virtual void RemoveEntityFromPool(int entityId) = 0; // 关键
};

//////////////////////////////////////////////////
// Pool
//////////////////////////////////////////////////
// 池子本质上是一个向量，它是类型T的对象的连续序列
//////////////////////////////////////////////////
template <typename T>
class Pool : public IPool{
    private:
        // 实际内容与实际大小
        std::vector<T> data;
        int size;

        // 两个辅助map跟踪每个索引的EntityIds
        //
        // 实体ID 与 数组索引的对应关系
        std::unordered_map<int,int> entityIdToIndex;
        // 数组索引 和 实体ID 的对应关系
        std::unordered_map<int,int> indexToEntityId;

    public:
        Pool(int capacity = 100) {
            size = 0;
            data.resize(capacity);
        }

        virtual ~Pool() = default;

        bool IsEmpty() const {
            return size == 0;
        }

        int GetSize() const {
            return size;
        }

        void Resize(int n) {
            data.resize(n);
        }

        void Clear() {
            data.clear();
            size = 0;
        }

        void Add(T object) {
            data.push_back(object);
        }

        void Set(int entityId, T object) {
            if (entityIdToIndex.find(entityId) != entityIdToIndex.end()) {
                // 如果 实体ID 存在 只需要简单的替换组件对象
                int index = entityIdToIndex[entityId];
                data[index] = object;
            } else {
                // 在添加新对象的过程中
                // 我需要记录实体ID及其向量索引
                int index = size;
                entityIdToIndex.emplace(entityId, index);
                indexToEntityId.emplace(index, entityId);
                if (index >= data.capacity()) {
                    data.resize(size * 2);
                }
                data[index] = object;
                size++;
            }
        }

        void Remove(int entityId) {
            // 最后一个元素 放到 删除的位置上 以保持数组紧凑
            int indexOfRemoved = entityIdToIndex[entityId];
            int indexOfLast = size - 1;
            data[indexOfRemoved] = data[indexOfLast];

            // 索引是Pool的
            int entityIdOfLastElement = indexToEntityId[indexOfLast];
            entityIdToIndex[entityIdOfLastElement] = indexOfRemoved;
            indexToEntityId[indexOfRemoved] = entityIdOfLastElement;

            entityIdToIndex.erase(entityId);
            indexToEntityId.erase(indexOfLast);

            size--;
        }

        void RemoveEntityFromPool(int entityId) override {
            if (entityIdToIndex.find(entityId) != entityIdToIndex.end()) {
                Remove(entityId);
            }
        }

        T& Get(int entityId) {
            int index = entityIdToIndex[entityId];
            return static_cast<T&>(data[index]);
        }

        T& operator [](unsigned int index) {
            return data[index];
        }        
};


//////////////////////////////////////////////////
// Registry
//////////////////////////////////////////////////
// 注册系统负责管理实体的创建与销毁，以及添加系统组件
//////////////////////////////////////////////////
class Registry {
    private:
        int numEntities = 0;

        // 组件池向量 每个池子都包含了一种特定组件类型的数据
        // 向量索引对应的是组件类型的ID [vector index = component type id]
        // 对于每个池子位置，池子索引将作为我的实体ID [Pool index = entity id]
        std::vector<std::shared_ptr<IPool>> componentPools;
        
        // 实体组件签名 记录每个实体的组件签名
        // 对于实体而言，这是说明那个组件是为它开启的
        // 在此情况下，向量索引将标识实体ID [vector index = entity id]
        std::vector<Signature> entityComponentSignatures;

        // 活跃系统的映射
        // [Map key = system type id]
        std::unordered_map<std::type_index, std::shared_ptr<System>> systems;

        // 一组将要被添加或删除的实体，在下一个注册更新
        std::set<Entity> entitiesToBeAdded;
        std::set<Entity> entitiesToBeKilled;

        // 包含了之前移除的可用实体ID
        std::deque<int> freeIds;

        // 增加或移除 实体 从他们所在的系统中
        void AddEntityToSystems(Entity entity); // 检查这个实体的组件签名，并将其添加到感兴趣的系统中
        void RemoveEntityFromSystems(Entity entity);
        
        // Entity tags 每一个实体一个tag
        std::unordered_map<std::string, Entity> entityPerTag;
        std::unordered_map<int, std::string> tagPerEntity;

        // Entity groups 每一个组有多个实体
        std::unordered_map<std::string, std::set<Entity>> entitiesPerGroup;
        std::unordered_map<int, std::string> groupPerEntity;

    public:
        Registry() {
            Logger::Log("Registry constructor called!");
        }
        ~Registry() {
            Logger::Log("Registry destructor called!");
        }

        // 注册表的 Update() 最后处理等待被 add/kill 的实体
        void Update();

        // 实体管理
        Entity CreateEntity();
        void KillEntity(Entity entity);

        // 组件管理
        template <typename TComponent, typename ...TArgs> void AddComponent(Entity entity, TArgs&& ...args);
        template <typename TComponent> void RemoveComponent(Entity entity);
        template <typename TComponent> bool HasComponent(Entity entity) const;
        template <typename TComponent> TComponent& GetComponent(Entity entity) const;

        // 系统管理
        template <typename TSystem, typename ...TArgs> void AddSystem(TArgs&& ...args);
        template <typename TSystem> void RemoveSystem();
        template <typename TSystem> bool HasSystem() const;
        template <typename TSystem> TSystem& GetSystem() const;

        // Tag 管理
        void TagEntity(Entity entity, const std::string& tag);
        bool EntityHasTag(Entity entity, const std::string& tag) const;
        Entity GetEntityByTag(const std::string& tag) const;
        void RemoveEntityTag(Entity entity);

        // Group 管理
        void GroupEntity(Entity entity, const std::string& group);
        bool EntityBelongsToGroup(Entity entity, const std::string& group) const;
        std::vector<Entity> GetEntitiesByGroup(const std::string& group) const;
        void RemoveEntityGroup(Entity entity);
};

template <typename TComponent> 
void System::RequireComponent() {
    const auto componentId = Component<TComponent>::GetId();
    componentSignature.set(componentId);
}

template <typename TSystem, typename ...TArgs>
void Registry::AddSystem(TArgs&& ...args) {
    std::shared_ptr<TSystem> newSystem = std::make_shared<TSystem>(std::forward<TArgs>(args)...);
    systems.insert(std::make_pair(std::type_index(typeid(TSystem)), newSystem));
}

template <typename TSystem>
void Registry::RemoveSystem() {
    auto system = systems.find(std::type_index(typeid(TSystem)));
    systems.erase(system);
}

template <typename TSystem>
bool Registry::HasSystem() const {
    return systems.find(std::type_index(typeid(TSystem))) != systems.end();
}

template <typename TSystem>
TSystem& Registry::GetSystem() const {
    // 用键找到一个迭代器指针
    auto system = systems.find(std::type_index(typeid(TSystem)));
    // 用迭代器指针获取值，即获取系统
    // 将所有内容都转换为类型T系统的静态指针强制转换
    // * 解引用 智能指针已重载该运算符 返回引用
    return *(std::static_pointer_cast<TSystem>(system->second));
}

template <typename TComponent, typename ...TArgs>
void Registry::AddComponent(Entity entity, TArgs&& ...args) {
    const auto componentId = Component<TComponent>::GetId();
    const auto entityId = entity.GetId();

    if (componentId >= componentPools.size()) {
        componentPools.resize(componentId + 1, nullptr);
    }

    if (!componentPools[componentId]) {
        // 1. 创建具体组件池的智能指针
        std::shared_ptr<Pool<TComponent>> newComponentPool = std::make_shared<Pool<TComponent>>();
        
        // 注意！！！核心知识点：
        // componentPools 里面存的是 vector<shared_ptr<IPool>>，而 newComponentPool 是 shared_ptr<Pool<TComponent>>
        // 这样赋值会发生：向上转型（Upcasting） + 类型擦除（Type Erasure）
        // 结果：变成了一个【基类智能指针】指向了【派生类对象】
        
        // -> 为什么这里安全？
        // 因为使用了 shared_ptr，其底层的【控制块（Control Block）】在 make_shared 时就已经记录了针对 Pool<TComponent> 的专属删除器。
        // 即便表面类型变成了 IPool，最后引用计数归零时，依然会调用派生类的析构函数。
        
        // -> 如果换成 unique_ptr 会怎样？
        // unique_ptr 是零开销的，没有控制块来记录专属删除器。
        // 如果基类 IPool 没有写 virtual 析构函数，使用 unique_ptr 就会导致未定义行为（通常是派生类资源泄漏）。
        // （这就是为什么作为多态基类的 IPool，一定要老老实实写上 virtual ~IPool() = default; 的原因）
        
        componentPools[componentId] = newComponentPool;
    }

    // 找到组件池
    std::shared_ptr<Pool<TComponent>> componentPool = std::static_pointer_cast<Pool<TComponent>>(componentPools[componentId]);

    TComponent newComponent(std::forward<TArgs>(args)...);

    componentPool->Set(entityId, newComponent);

    entityComponentSignatures[entityId].set(componentId);

    Logger::Log("实体 id = " + std::to_string(entityId) + " 添加组件 id = " + std::to_string(componentId));
    Logger::Log("组件 id = " + std::to_string(componentId) + " --> 对应组件池向量 Size: " + std::to_string(componentPool->GetSize()));
}

template<typename TComponent>
void Registry::RemoveComponent(Entity entitiy) {
    const auto componentId = Component<TComponent>::GetId();
    const auto entityId = entitiy.GetId();
    
    // 找到组件池
    // 从组件池向量中移除该实体对应的组件
    std::shared_ptr<Pool<TComponent>> componentPool = std::static_pointer_cast<Pool<TComponent>>(componentPools[componentId]);
    componentPool->Remove(entityId);

    // 将该实体的组件签名设置为不活动状态
    entityComponentSignatures[entityId].set(componentId, false);

    Logger::Log("实体 id = " + std::to_string(entityId) + " 移除组件 id = " + std::to_string(componentId));
}

template<typename TComponent>
bool Registry::HasComponent(Entity entity) const {
    const auto componentId = Component<TComponent>::GetId();
    const auto entityId = entity.GetId();

    return entityComponentSignatures[entityId].test(componentId);
}

template<typename TComponent>
TComponent& Registry::GetComponent(Entity entity) const {
    const auto componentId = Component<TComponent>::GetId();
    const auto entityId = entity.GetId();

    // 错误的: return componentPools[componentId][entityId]; 这里面是个Pool<TComponent>模板类 你又没有重载[]
    // 正确的:
    auto componentPool = std::static_pointer_cast<Pool<TComponent>>(componentPools[componentId]);
    return componentPool->Get(entityId);
}

/* 下面的实现是为了
使用tank.AddComponent<TransformComponent>(tank, glm::vec2(10.0, 30.0), glm::vec2(1.0, 1.0), 0.0);
代替registry->AddComponent<TransformComponent>(tank, glm::vec2(10.0, 30.0), glm::vec2(1.0, 1.0), 0.0);
*/
template <typename TComponent, typename ...TArgs>
void Entity::AddComponent(TArgs&& ...args) {
    registry->AddComponent<TComponent>(*this, std::forward<TArgs>(args)...);
}

template<typename TComponent>
void Entity::RemoveComponent() {
    registry->RemoveComponent<TComponent>(*this);
}

template<typename TComponent>
bool Entity::HasComponent() const {
    return registry->HasComponent<TComponent>(*this);
}

/* 复习一下
    *this 解开后，拿到的确实是对象的真身（左值）。
    它可以作为引用被传递，也可以作为值被拷贝。
    在这行代码里，因为 Registry::GetComponent 明确要求按值接收（传参没有加 &），所以编译器把 *this 拷贝了一份传了进去。
    这样设计是因为 Entity 只有骨架没有肉（只有 ID），拷贝它的性能比传引用更高、更安全。
*/
template<typename TComponent>
TComponent& Entity::GetComponent() const {
    return registry->GetComponent<TComponent>(*this);
}

#endif