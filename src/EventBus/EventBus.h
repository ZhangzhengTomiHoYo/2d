#ifndef EVENTBUS_H
#define EVENTBUS_H

#include "../Logger/Logger.h"
#include "Event.h"
#include <map>
#include <typeindex>
#include <list>
#include <memory>
#include <functional>

class IEventCallback {
    private:
        // 纯虚函数
        virtual void Call(Event& e) = 0;

    public:
        virtual ~IEventCallback() = default;

        void Execute(Event& e) {
            Call(e);
        }
};

template <typename TOwner, typename TEvent>
class EventCallback: public IEventCallback {
    private:
        typedef void (TOwner::*CallbackFunction)(TEvent&);

        TOwner* ownerInstance;
        CallbackFunction callbackFunction;

        virtual void Call(Event& e) override {
            std::invoke(callbackFunction, ownerInstance, static_cast<TEvent&>(e));
        }
    
    public:
        EventCallback(TOwner* ownerInstance, CallbackFunction callbackFunction) {
            this->ownerInstance = ownerInstance;
            this->callbackFunction = callbackFunction;
        }

        virtual ~EventCallback() override = default;
};

typedef std::list<std::unique_ptr<IEventCallback>> HandlerList;

class EventBus {
    private:
        std::map<std::type_index, std::unique_ptr<HandlerList>> subscribers;
    public:
        EventBus() {
            Logger::Log("EventBus constructor called!");
        }

        ~EventBus() {
            Logger::Log("EventBus destructor called!");
        }

        // 清理订阅者列表
        void Reset() {
            subscribers.clear();
        }

        ////////////////////////////////////////////////////////////////////////////
        // 订阅事件类型 <T>
        // 在我们的实现中 一个监听订阅一个事件
        // 例子: eventBus->SubscribeToEvent<CollisionEvent>(this, &Game::Collision)
        ////////////////////////////////////////////////////////////////////////////
        template <typename TEvent, typename TOwner>
        void SubscribeToEvent(TOwner* ownerInstance, void (TOwner::*callbackFunction)(TEvent&)) {
            if (!subscribers[typeid(TEvent)].get()) {
                subscribers[typeid(TEvent)] = std::make_unique<HandlerList>();
            }
            auto subscriber = std::make_unique<EventCallback<TOwner, TEvent>>(ownerInstance, callbackFunction);
            subscribers[typeid(TEvent)]->push_back(std::move(subscriber));
        }
        
        ////////////////////////////////////////////////////////////////////////////
        // 发出事件类型 <T>
        // 在我们的实现中 一旦有东西发出事件 我们就立即执行所有监听器回调
        // 例子: eventBus->EmitEvent<CollisionEvent>(player, enemy); 
        //                                         传递两个相互碰撞的实体
        ////////////////////////////////////////////////////////////////////////////
        template <typename TEvent, typename ...TArgs>
        void EmitEvent(TArgs&& ...args) {
            auto handlers = subscribers[typeid(TEvent)].get();
            if (handlers) {
                for (auto it = handlers->begin(); it != handlers->end(); it++){
                    auto handler = it->get();
                    TEvent event(std::forward<TArgs>(args)...);
                    handler->Execute(event);
                }
            }
        }
};

#endif