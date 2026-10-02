#include "pch.h"

#include "Game/Telemetry/TelemetryService.h"

#include <limits>
#include <mutex>
#include <shared_mutex>
#include <utility>

namespace srh::engine::game
{
    std::uint64_t
        TelemetryService::BeginEpoch()
    {
        std::unique_lock lock(
            m_mutex
        );

        const std::uint64_t newEpoch =
            NextCounter(
                m_state.epoch
            );

        m_state =
            TelemetryState{};

        m_state.epoch =
            newEpoch;

        m_epochActive =
            true;

        return
            newEpoch;
    }

    bool TelemetryService::EndEpoch(
        const std::uint64_t epoch
    )
    {
        std::unique_lock lock(
            m_mutex
        );

        if (
            !m_epochActive ||
            epoch == 0 ||
            epoch != m_state.epoch
            )
        {
            return false;
        }

        const std::uint64_t invalidationEpoch =
            NextCounter(
                m_state.epoch
            );

        m_state =
            TelemetryState{};

        m_state.epoch =
            invalidationEpoch;

        m_epochActive =
            false;

        return true;
    }

    bool TelemetryService::
        IsEpochActive() const
    {
        std::shared_lock lock(
            m_mutex
        );

        return
            m_epochActive;
    }

    std::uint64_t
        TelemetryService::
        CurrentEpoch() const
    {
        std::shared_lock lock(
            m_mutex
        );

        return
            m_state.epoch;
    }

    bool TelemetryService::PublishStatic(
        const std::uint64_t epoch,
        TelemetryStaticState state
    )
    {
        std::unique_lock lock(
            m_mutex
        );

        if (
            !m_epochActive ||
            epoch == 0 ||
            epoch != m_state.epoch
            )
        {
            return false;
        }

        state.generation =
            NextCounter(
                m_state
                .staticState
                .generation
            );

        m_state.staticState =
            std::move(
                state
            );

        return true;
    }

    bool TelemetryService::PublishSession(
        const std::uint64_t epoch,
        TelemetrySessionState state
    )
    {
        std::unique_lock lock(
            m_mutex
        );

        if (
            !m_epochActive ||
            epoch == 0 ||
            epoch != m_state.epoch
            )
        {
            return false;
        }

        state.generation =
            NextCounter(
                m_state
                .sessionState
                .generation
            );

        m_state.sessionState =
            std::move(
                state
            );

        return true;
    }

    bool TelemetryService::PublishRealtime(
        const std::uint64_t epoch,
        TelemetryRealtimeState state
    )
    {
        std::unique_lock lock(
            m_mutex
        );

        if (
            !m_epochActive ||
            epoch == 0 ||
            epoch != m_state.epoch
            )
        {
            return false;
        }

        state.generation =
            NextCounter(
                m_state
                .realtimeState
                .generation
            );

        m_state.realtimeState =
            std::move(
                state
            );

        return true;
    }

    bool TelemetryService::PublishNative(
        const std::uint64_t epoch,
        TelemetryNativeState state
    )
    {
        std::unique_lock lock(
            m_mutex
        );

        if (
            !m_epochActive ||
            epoch == 0 ||
            epoch != m_state.epoch
            )
        {
            return false;
        }

        state.generation =
            NextCounter(
                m_state
                .nativeState
                .generation
            );

        m_state.nativeState =
            std::move(
                state
            );

        return true;
    }

    TelemetryStaticState
        TelemetryService::
        GetStaticState() const
    {
        std::shared_lock lock(
            m_mutex
        );

        return
            m_state.staticState;
    }

    TelemetrySessionState
        TelemetryService::
        GetSessionState() const
    {
        std::shared_lock lock(
            m_mutex
        );

        return
            m_state.sessionState;
    }

    TelemetryRealtimeState
        TelemetryService::
        GetRealtimeState() const
    {
        std::shared_lock lock(
            m_mutex
        );

        return
            m_state.realtimeState;
    }

    TelemetryNativeState
        TelemetryService::
        GetNativeState() const
    {
        std::shared_lock lock(
            m_mutex
        );

        return
            m_state.nativeState;
    }

    TelemetryState
        TelemetryService::
        GetState() const
    {
        std::shared_lock lock(
            m_mutex
        );

        return
            m_state;
    }

    TelemetrySnapshot
        TelemetryService::
        GetSnapshot() const
    {
        std::shared_lock lock(
            m_mutex
        );

        TelemetrySnapshot
            snapshot;

        //
        // Coherency metadata.
        //

        snapshot.meta.active =
            m_epochActive;

        snapshot.meta.epoch =
            m_state.epoch;

        snapshot.meta.staticGeneration =
            m_state
            .staticState
            .generation;

        snapshot.meta.sessionGeneration =
            m_state
            .sessionState
            .generation;

        snapshot.meta.realtimeGeneration =
            m_state
            .realtimeState
            .generation;

        snapshot.meta.nativeGeneration =
            m_state
            .nativeState
            .generation;

        snapshot.meta.staticFrame =
            m_state
            .staticState
            .frame;

        snapshot.meta.sessionFrame =
            m_state
            .sessionState
            .frame;

        snapshot.meta.realtimeFrame =
            m_state
            .realtimeState
            .frame;

        snapshot.meta.nativeFrame =
            m_state
            .nativeState
            .frame;

        //
        // Static partition.
        //

        snapshot.game =
            m_state
            .staticState
            .game;

        snapshot.track =
            m_state
            .staticState
            .track;

        snapshot.player.driver =
            m_state
            .staticState
            .playerDriver;

        snapshot.player.identity =
            m_state
            .staticState
            .playerVehicle;

        snapshot.capabilities =
            m_state
            .staticState
            .capabilities;

        //
        // Session partition.
        //

        snapshot.session =
            m_state
            .sessionState
            .session;

        snapshot.environment =
            m_state
            .sessionState
            .environment;

        snapshot.weatherForecast =
            m_state
            .sessionState
            .weatherForecast;

        snapshot.participants =
            m_state
            .sessionState
            .participants;

        snapshot.raceControl =
            m_state
            .sessionState
            .raceControl;

        snapshot.player.timing =
            m_state
            .sessionState
            .playerTiming;

        snapshot.player.pit =
            m_state
            .sessionState
            .playerPit;

        snapshot.player.strategy =
            m_state
            .sessionState
            .playerStrategy;

        //
        // Realtime partition.
        //

        snapshot.player.controls =
            m_state
            .realtimeState
            .controls;

        snapshot.player.motion =
            m_state
            .realtimeState
            .motion;

        snapshot.player.forceFeedback =
            m_state
            .realtimeState
            .forceFeedback;

        snapshot.player.powertrain =
            m_state
            .realtimeState
            .powertrain;

        snapshot.player.transmission =
            m_state
            .realtimeState
            .transmission;

        snapshot.player.differential =
            m_state
            .realtimeState
            .differential;

        snapshot.player.wheels =
            m_state
            .realtimeState
            .wheels;

        snapshot.player.brakes =
            m_state
            .realtimeState
            .brakes;

        snapshot.player.aero =
            m_state
            .realtimeState
            .aero;

        snapshot.player.electronics =
            m_state
            .realtimeState
            .electronics;

        snapshot.player.driverControls =
            m_state
            .realtimeState
            .driverControls;

        snapshot.player.instrumentation =
            m_state
            .realtimeState
            .instrumentation;

        snapshot.player.lightingWipers =
            m_state
            .realtimeState
            .lightingWipers;

        snapshot.player.energy =
            m_state
            .realtimeState
            .energy;

        snapshot.player.damage =
            m_state
            .realtimeState
            .damage;

        snapshot.proximity =
            m_state
            .realtimeState
            .proximity;

        snapshot.performance =
            m_state
            .realtimeState
            .performance;

        //
        // Native partition.
        //

        snapshot.nativeChannels =
            m_state
            .nativeState
            .channels;

        return
            snapshot;
    }

    std::uint64_t
        TelemetryService::
        NextCounter(
            const std::uint64_t value
        ) noexcept
    {
        if (
            value ==
            std::numeric_limits<
            std::uint64_t
            >::max()
            )
        {
            //
            // Zero is reserved for
            // "never published / no epoch".
            //

            return 1;
        }

        return
            value + 1;
    }
}