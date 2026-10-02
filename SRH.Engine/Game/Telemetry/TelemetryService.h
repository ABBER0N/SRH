#pragma once

#include "Game/Domain/TelemetryAggregateState.h"
#include "Game/Domain/TelemetrySnapshot.h"

#include <cstdint>
#include <shared_mutex>

namespace srh::engine::game
{
    //
    // Thread-safe owner of normalized telemetry.
    //
    // Providers never receive direct mutable
    // access to the internal state.
    //
    // Every provider connection works inside an
    // epoch. Publications carrying a stale epoch
    // are rejected.
    //

    class TelemetryService
    {
    public:
        TelemetryService() = default;

        TelemetryService(
            const TelemetryService&
        ) = delete;

        TelemetryService& operator=(
            const TelemetryService&
            ) = delete;

        TelemetryService(
            TelemetryService&&
        ) = delete;

        TelemetryService& operator=(
            TelemetryService&&
            ) = delete;

        //
        // Starts a completely new telemetry
        // lifetime.
        //
        // All previous partitions are discarded.
        // Returns the token that the new provider
        // must use for every publication.
        //

        [[nodiscard]]
        std::uint64_t BeginEpoch();

        //
        // Ends an active epoch only if the caller
        // still owns the current token.
        //
        // On success all telemetry is cleared and
        // stale publications using the old token
        // are rejected.
        //

        [[nodiscard]]
        bool EndEpoch(
            std::uint64_t epoch
        );

        [[nodiscard]]
        bool IsEpochActive() const;

        [[nodiscard]]
        std::uint64_t CurrentEpoch() const;

        //
        // Publication methods.
        //
        // Generation counters are controlled by
        // TelemetryService.
        //
        // A generation supplied inside the input
        // state is ignored and replaced.
        //

        [[nodiscard]]
        bool PublishStatic(
            std::uint64_t epoch,
            TelemetryStaticState state
        );

        [[nodiscard]]
        bool PublishSession(
            std::uint64_t epoch,
            TelemetrySessionState state
        );

        [[nodiscard]]
        bool PublishRealtime(
            std::uint64_t epoch,
            TelemetryRealtimeState state
        );

        [[nodiscard]]
        bool PublishNative(
            std::uint64_t epoch,
            TelemetryNativeState state
        );

        //
        // Partition reads avoid copying unrelated
        // telemetry when a consumer needs only one
        // update-rate domain.
        //

        [[nodiscard]]
        TelemetryStaticState
            GetStaticState() const;

        [[nodiscard]]
        TelemetrySessionState
            GetSessionState() const;

        [[nodiscard]]
        TelemetryRealtimeState
            GetRealtimeState() const;

        [[nodiscard]]
        TelemetryNativeState
            GetNativeState() const;

        //
        // Complete internal state copy.
        //
        // Primarily useful for diagnostics/tests.
        //

        [[nodiscard]]
        TelemetryState
            GetState() const;

        //
        // Produces one coherent normalized view.
        //
        // All four partitions are observed under
        // the same shared lock so the generation
        // metadata exactly describes the data in
        // the returned snapshot.
        //

        [[nodiscard]]
        TelemetrySnapshot
            GetSnapshot() const;

    private:
        [[nodiscard]]
        static std::uint64_t
            NextCounter(
                std::uint64_t value
            ) noexcept;

        mutable std::shared_mutex
            m_mutex;

        TelemetryState
            m_state;

        bool m_epochActive{
            false
        };
    };
}