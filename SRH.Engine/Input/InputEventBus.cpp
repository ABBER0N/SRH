#include "pch.h"
#include "Input/InputEventBus.h"
#include <mutex>
#include <utility>
#include <vector>

namespace srh::engine
{
    InputEventBus::SubscriptionId
        InputEventBus::Subscribe(
            Callback callback
        )
    {
        if (!callback)
        {
            return 0;
        }

        const auto subscriptionId =
            m_nextSubscriptionId.fetch_add(
                1,
                std::memory_order_relaxed
            );

        std::unique_lock lock(
            m_mutex
        );

        m_subscribers.emplace(
            subscriptionId,
            std::move(callback)
        );

        return subscriptionId;
    }

    void InputEventBus::Unsubscribe(
        const SubscriptionId subscriptionId
    )
    {
        if (subscriptionId == 0)
        {
            return;
        }

        std::unique_lock lock(
            m_mutex
        );

        m_subscribers.erase(
            subscriptionId
        );
    }

    void InputEventBus::Publish(
        const InputEvent& event
    ) const
    {
        std::vector<Callback>
            callbacks;

        {
            std::shared_lock lock(
                m_mutex
            );

            callbacks.reserve(
                m_subscribers.size()
            );

            for (
                const auto& [
                    subscriptionId,
                    callback
                ] :
                m_subscribers
                )
            {
                (void)subscriptionId;

                callbacks.push_back(
                    callback
                );
            }
        }

        //
        // Never invoke external callbacks
        // while holding the event-bus mutex.
        //

        for (
            const auto& callback :
            callbacks
            )
        {
            callback(
                event
            );
        }
    }
}