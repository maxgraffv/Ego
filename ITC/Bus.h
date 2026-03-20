#ifndef BUS_H
#define BUS_H

#include <functional>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>
#include <stdexcept>
#include <algorithm>

namespace ITC
{
    class Bus
    {
    private:
        struct ICallbackList
        {
            virtual ~ICallbackList() = default;
            virtual std::type_index type() const = 0;
            virtual void removeSubscriber(std::size_t id) = 0;
        };

        template<typename T>
        struct CallbackList : ICallbackList
        {
            struct Entry
            {
                std::size_t id;
                std::function<void(const T&)> callback;
            };

            std::vector<Entry> subscribers;

            std::type_index type() const override
            {
                return std::type_index(typeid(T));
            }

            void removeSubscriber(std::size_t id) override
            {
                subscribers.erase(
                    std::remove_if(
                        subscribers.begin(),
                        subscribers.end(),
                        [id](const Entry& e) { return e.id == id; }),
                    subscribers.end());
            }
        };

    public:
        class Subscription
        {
        public:
            Subscription() = default;

            Subscription(Bus* bus, std::string topic, std::size_t id)
                : bus_(bus), topic_(std::move(topic)), id_(id), active_(true)
            {
            }

            Subscription(const Subscription&) = delete;
            Subscription& operator=(const Subscription&) = delete;

            Subscription(Subscription&& other) noexcept
                : bus_(other.bus_),
                topic_(std::move(other.topic_)),
                id_(other.id_),
                active_(other.active_)
            {
                other.bus_ = nullptr;
                other.active_ = false;
            }

            Subscription& operator=(Subscription&& other) noexcept
            {
                if (this != &other)
                {
                    unsubscribe();

                    bus_ = other.bus_;
                    topic_ = std::move(other.topic_);
                    id_ = other.id_;
                    active_ = other.active_;

                    other.bus_ = nullptr;
                    other.active_ = false;
                }
                return *this;
            }

            ~Subscription()
            {
                unsubscribe();
            }

            void unsubscribe()
            {
                if (active_ && bus_)
                {
                    bus_->unsubscribe(topic_, id_);
                    active_ = false;
                }
            }

            bool active() const
            {
                return active_;
            }

        private:
            Bus* bus_ = nullptr;
            std::string topic_;
            std::size_t id_ = 0;
            bool active_ = false;
        };

    public:
        Bus() = default;

        template<typename T>
        Subscription subscribe(const std::string& topic, std::function<void(const T&)> callback)
        {
            auto* list = getOrCreateList<T>(topic);

            const std::size_t id = next_id_++;
            list->subscribers.push_back({id, std::move(callback)});

            return Subscription(this, topic, id);
        }

        template<typename T, typename Callable>
        Subscription subscribe(const std::string& topic, Callable&& callback)
        {
            return subscribe<T>(
                topic,
                std::function<void(const T&)>(std::forward<Callable>(callback)));
        }

        template<typename T>
        void publish(const std::string& topic, const T& message)
        {
            auto it = topics_.find(topic);
            if (it == topics_.end())
            {
                return;
            }

            auto* base = it->second.get();

            if (base->type() != std::type_index(typeid(T)))
            {
                throw std::runtime_error("Bus::publish - topic exists with different message type");
            }

            auto* list = static_cast<CallbackList<T>*>(base);

            for (const auto& entry : list->subscribers)
            {
                entry.callback(message);
            }
        }

        bool hasTopic(const std::string& topic) const
        {
            return topics_.find(topic) != topics_.end();
        }

    private:
        void unsubscribe(const std::string& topic, std::size_t id)
        {
            auto it = topics_.find(topic);
            if (it == topics_.end())
            {
                return;
            }

            it->second->removeSubscriber(id);
        }

        template<typename T>
        CallbackList<T>* getOrCreateList(const std::string& topic)
        {
            auto it = topics_.find(topic);

            if (it == topics_.end())
            {
                auto list = std::make_unique<CallbackList<T>>();
                auto* ptr = list.get();
                topics_[topic] = std::move(list);
                return ptr;
            }

            if (it->second->type() != std::type_index(typeid(T)))
            {
                throw std::runtime_error("Bus::subscribe - topic exists with different message type");
            }

            return static_cast<CallbackList<T>*>(it->second.get());
        }

    private:
        std::unordered_map<std::string, std::unique_ptr<ICallbackList>> topics_;
        std::size_t next_id_ = 1;
    };
}

#endif