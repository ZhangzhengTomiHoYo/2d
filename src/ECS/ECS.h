#ifndef ECS_H
#define ECS_H

#include "../Logger/Logger.h"
#include <bitset>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <set>
#include <memory>

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
        virtual ~IPool() {}
};

//////////////////////////////////////////////////
// Pool
//////////////////////////////////////////////////
// 池子本质上是一个向量，它是类型T的对象的连续序列
//////////////////////////////////////////////////
template <typename T>
class Pool : public IPool{
    private:
        std::vector<T> data;

    public:
        Pool(int size = 100) {
            data.resize(size);
        }

        virtual ~Pool() = default;

        bool isEmpty() const {
            return data.empty();
        }

        int GetSize() const {
            return data.size();
        }

        void Resize(int n) {
            data.resize(n);
        }

        void Clear() {
            data.clear();
        }

        void Add(T object) {
            data.push_back(object);
        }

        void Set(int index, T object) {
            data[index] = object;
        }

        T& Get(int index) {
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

        // 检查这个实体的组件签名，并将其添加到感兴趣的系统中
        void AddEntityToSystems(Entity entity);
        
        // TODO:
        //
        // 创建实体
        // 销毁实体
        //
        // 给实体添加组件
        // 从实体移除组件
        // 实体是否包含某个组件
        // 从实体获取一个组件
        //
        // 添加系统
        // 移除系统
        // 是否存在某个系统
        // 获取系统
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

    std::shared_ptr<Pool<TComponent>> componentPool = std::static_pointer_cast<Pool<TComponent>>(componentPools[componentId]);

    if (entityId >= componentPools.size()) {
        componentPool->Resize(numEntities);
    }

    TComponent newComponent(std::forward<TArgs>(args)...);

    componentPool->Set(entityId, newComponent);
    entityComponentSignatures[entityId].set(componentId);

    Logger::Log("实体 id = " + std::to_string(entityId) + " 添加组件 id = " + std::to_string(componentId));
}

template<typename TComponent>
void Registry::RemoveComponent(Entity entitiy) {
    const auto componentId = Component<TComponent>::GetId();
    const auto entityId = entitiy.GetId();
    
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