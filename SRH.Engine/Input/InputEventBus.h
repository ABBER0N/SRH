#pragma once

#include "Domain/InputEvent.h"
#include <atomic>
#include <cstdint>
#include <functional>
#include <shared_mutex>
#include <unordered_map>

namespace srh::engine
{
    class InputEventBus
    {
    public:
        using SubscriptionId =
            std::uint64_t;

        using Callback =
            std::function<
            void(const InputEvent&)
            >;

        InputEventBus() = default;

        InputEventBus(
            const InputEventBus&
        ) = delete;

        InputEventBus& operator=(
            const InputEventBus&
            ) = delete;

        [[nodiscard]]
        SubscriptionId Subscribe(
            Callback callback
        );

        void Unsubscribe(
            SubscriptionId subscriptionId
        );

        void Publish(
            const InputEvent& event
        ) const;

    private:
        mutable std::shared_mutex
            m_mutex;

        std::unordered_map<
            SubscriptionId,
            Callback
        >
            m_subscribers;

        std::atomic<SubscriptionId>
            m_nextSubscriptionId{ 1 };
    };
}