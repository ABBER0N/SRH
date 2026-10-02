#include "pch.h"

#include "Public/SrhEngine.h"

#include "Devices/DeviceRegistry.h"
#include "Devices/VirtualController/VirtualControllerRegistry.h"
#include "Execution/ActionExecutor.h"
#include "Execution/VirtualController/VirtualControllerService.h"
#include "Execution/Windows/Media/MediaService.h"
#include "Execution/Windows/WindowsActionExecutor.h"
#include "Input/InputEventBus.h"
#include "Input/InputStateStore.h"
#include "Mapping/MappingService.h"

#include <mutex>
#include <utility>

namespace srh::engine
{
    class SrhEngine::Impl
    {
    public:
        Impl()
        {
            //
            // Default controller keeps the current
            // SRH behaviour compatible with the
            // existing DeviceId = 0 mappings.
            //

            VirtualControllerState
                defaultController;

            defaultController.deviceId =
                0;

            defaultController.name =
                "Virtual Controller 1";

            defaultController.enabled =
                true;

            defaultController.active =
                false;

            virtualControllerRegistry
                .AddOrUpdate(
                    std::move(
                        defaultController
                    )
                );

            actionExecutor.SetVirtualControllerHandler(
                [this](
                    const VirtualControllerAction& action
                    )
                {
                    std::scoped_lock lock(
                        virtualControllerMutex
                    );

                    const auto controller =
                        virtualControllerRegistry
                        .Find(
                            action.deviceId
                        );

                    if (
                        !controller.has_value() ||
                        !controller->enabled
                        )
                    {
                        return
                            ActionExecutionStatus::
                            Failed;
                    }

                    const auto status =
                        virtualControllerService
                        .Execute(
                            action
                        );

                    if (
                        status ==
                        ActionExecutionStatus::
                        Executed
                        )
                    {
                        (void)
                            virtualControllerRegistry
                            .SetActive(
                                action.deviceId,
                                true
                            );
                    }

                    return status;
                }
            );

            actionExecutor.SetSystemHandler(
                [this](
                    const SystemAction& action
                    )
                {
                    return
                        windowsActionExecutor.Execute(
                            action
                        );
                }
            );
        }

        DeviceRegistry
            deviceRegistry;

        InputStateStore
            inputStateStore;

        InputEventBus
            inputEventBus;

        MappingService
            mappingService;

        mutable std::mutex
            virtualControllerMutex;

        VirtualControllerRegistry
            virtualControllerRegistry;

        virtual_controller::
            VirtualControllerService
            virtualControllerService;

        WindowsActionExecutor
            windowsActionExecutor;

        ActionExecutor
            actionExecutor;

        MediaService
            mediaService;
    };

    SrhEngine::SrhEngine()
        : m_impl(
            std::make_unique<Impl>()
        )
    {
    }

    SrhEngine::~SrhEngine() =
        default;

    SrhEngine::SrhEngine(
        SrhEngine&&
    ) noexcept = default;

    SrhEngine& SrhEngine::operator=(
        SrhEngine&&
        ) noexcept = default;

    //
    // Devices
    //

    void SrhEngine::SetHubState(
        HubState state
    )
    {
        m_impl->deviceRegistry.SetHubState(
            std::move(state)
        );
    }

    void SrhEngine::UpsertNodeState(
        NodeState state
    )
    {
        m_impl->deviceRegistry.UpsertNode(
            std::move(state)
        );
    }

    void SrhEngine::RemoveNode(
        const NodeId nodeId
    )
    {
        m_impl->deviceRegistry.RemoveNode(
            nodeId
        );

        m_impl->inputStateStore.ClearNode(
            nodeId
        );
    }

    void SrhEngine::ClearNodes()
    {
        m_impl->deviceRegistry.ClearNodes();

        m_impl->inputStateStore.Clear();
    }

    HubState SrhEngine::GetHubState() const
    {
        return
            m_impl->deviceRegistry
            .GetHubState();
    }

    std::optional<NodeState>
        SrhEngine::FindNode(
            const NodeId nodeId
        ) const
    {
        return
            m_impl->deviceRegistry
            .FindNode(
                nodeId
            );
    }

    DeviceSnapshot
        SrhEngine::GetDeviceSnapshot() const
    {
        return
            m_impl->deviceRegistry
            .GetSnapshot();
    }

    //
    // Input
    //

    InputProcessingResult
        SrhEngine::SubmitInputEvent(
            const InputEvent& event
        )
    {
        InputProcessingResult result;

        const bool applied =
            m_impl->inputStateStore.Apply(
                event
            );

        if (!applied)
        {
            return result;
        }

        result.accepted =
            true;

        const auto actions =
            m_impl->mappingService.Resolve(
                event
            );

        result.executions.reserve(
            actions.size()
        );

        for (
            const auto& action :
            actions
            )
        {
            ActionExecutionResult
                execution;

            execution.action =
                action;

            execution.status =
                m_impl->actionExecutor.Execute(
                    action
                );

            result.executions.push_back(
                std::move(
                    execution
                )
            );
        }

        m_impl->inputEventBus.Publish(
            event
        );

        return result;
    }

    std::optional<InputControlState>
        SrhEngine::FindInputState(
            const NodeId nodeId,
            const ControlId controlId
        ) const
    {
        return
            m_impl->inputStateStore.Find(
                nodeId,
                controlId
            );
    }

    InputSnapshot
        SrhEngine::GetInputSnapshot() const
    {
        return
            m_impl->inputStateStore
            .GetSnapshot();
    }

    void SrhEngine::ClearInputState()
    {
        m_impl->inputStateStore.Clear();
    }

    //
    // Input events
    //

    SrhEngine::InputSubscriptionId
        SrhEngine::SubscribeInput(
            InputCallback callback
        )
    {
        return
            m_impl->inputEventBus.Subscribe(
                std::move(callback)
            );
    }

    void SrhEngine::UnsubscribeInput(
        const InputSubscriptionId subscriptionId
    )
    {
        m_impl->inputEventBus.Unsubscribe(
            subscriptionId
        );
    }

    //
    // Mapping
    //

    void SrhEngine::SetMappingRules(
        std::vector<MappingRule> rules
    )
    {
        m_impl->mappingService.SetRules(
            std::move(rules)
        );
    }

    void SrhEngine::AddOrUpdateMappingRule(
        MappingRule rule
    )
    {
        m_impl->mappingService.AddOrUpdateRule(
            std::move(rule)
        );
    }

    void SrhEngine::RemoveMappingRule(
        const MappingRuleId ruleId
    )
    {
        m_impl->mappingService.RemoveRule(
            ruleId
        );
    }

    void SrhEngine::ClearMappingRules()
    {
        m_impl->mappingService.Clear();
    }

    std::optional<MappingRule>
        SrhEngine::FindMappingRule(
            const MappingRuleId ruleId
        ) const
    {
        return
            m_impl->mappingService.FindRule(
                ruleId
            );
    }

    std::vector<MappingRule>
        SrhEngine::GetMappingRules() const
    {
        return
            m_impl->mappingService.GetRules();
    }

    std::vector<Action>
        SrhEngine::ResolveActions(
            const InputEvent& event
        ) const
    {
        return
            m_impl->mappingService.Resolve(
                event
            );
    }

    //
    // Virtual controller configuration
    //

    void SrhEngine::SetVirtualControllers(
        std::vector<VirtualControllerState>
        controllers
    )
    {
        std::scoped_lock lock(
            m_impl->virtualControllerMutex
        );

        (void)
            m_impl->
            virtualControllerService
            .ResetAll();

        for (
            auto& controller :
            controllers
            )
        {
            controller.active =
                false;
        }

        m_impl->
            virtualControllerRegistry
            .SetControllers(
                std::move(
                    controllers
                )
            );
    }

    void SrhEngine::
        AddOrUpdateVirtualController(
            VirtualControllerState controller
        )
    {
        std::scoped_lock lock(
            m_impl->virtualControllerMutex
        );

        const auto existing =
            m_impl->
            virtualControllerRegistry
            .Find(
                controller.deviceId
            );

        if (existing.has_value())
        {
            if (!controller.enabled)
            {
                //
                // Publish the disabled configuration
                // first. No concurrent input action
                // can revive it while it is reset.
                //

                controller.active =
                    false;

                m_impl->
                    virtualControllerRegistry
                    .AddOrUpdate(
                        controller
                    );

                (void)
                    m_impl->
                    virtualControllerService
                    .ResetDevice(
                        controller.deviceId
                    );

                return;
            }

            //
            // Name/configuration changes do not make
            // an already active controller inactive.
            //

            controller.active =
                existing->active;
        }
        else
        {
            controller.active =
                false;
        }

        m_impl->
            virtualControllerRegistry
            .AddOrUpdate(
                std::move(
                    controller
                )
            );
    }

    bool SrhEngine::
        RemoveVirtualController(
            const VirtualDeviceId deviceId
        )
    {
        std::scoped_lock lock(
            m_impl->virtualControllerMutex
        );

        const bool removed =
            m_impl->
            virtualControllerRegistry
            .Remove(
                deviceId
            );

        if (!removed)
        {
            return false;
        }

        (void)
            m_impl->
            virtualControllerService
            .ResetDevice(
                deviceId
            );

        return true;
    }

    void SrhEngine::
        ClearVirtualControllers()
    {
        std::scoped_lock lock(
            m_impl->virtualControllerMutex
        );

        (void)
            m_impl->
            virtualControllerService
            .ResetAll();

        m_impl->
            virtualControllerRegistry
            .Clear();
    }

    std::optional<VirtualControllerState>
        SrhEngine::FindVirtualController(
            const VirtualDeviceId deviceId
        ) const
    {
        std::scoped_lock lock(
            m_impl->virtualControllerMutex
        );

        return
            m_impl->
            virtualControllerRegistry
            .Find(
                deviceId
            );
    }

    std::vector<VirtualControllerState>
        SrhEngine::GetVirtualControllers()
        const
    {
        std::scoped_lock lock(
            m_impl->virtualControllerMutex
        );

        return
            m_impl->
            virtualControllerRegistry
            .GetControllers();
    }

    VirtualControllerSnapshot
        SrhEngine::
        GetVirtualControllerSnapshot()
        const
    {
        std::scoped_lock lock(
            m_impl->virtualControllerMutex
        );

        return
            m_impl->
            virtualControllerRegistry
            .GetSnapshot();
    }

    bool SrhEngine::
        SetVirtualControllerEnabled(
            const VirtualDeviceId deviceId,
            const bool enabled
        )
    {
        std::scoped_lock lock(
            m_impl->virtualControllerMutex
        );

        const auto controller =
            m_impl->
            virtualControllerRegistry
            .Find(
                deviceId
            );

        if (!controller.has_value())
        {
            return false;
        }

        if (
            controller->enabled ==
            enabled
            )
        {
            return true;
        }

        if (!enabled)
        {
            //
            // Disable first so no new action can
            // reach the driver during neutralization.
            //

            (void)
                m_impl->
                virtualControllerRegistry
                .SetEnabled(
                    deviceId,
                    false
                );

            (void)
                m_impl->
                virtualControllerRegistry
                .SetActive(
                    deviceId,
                    false
                );

            (void)
                m_impl->
                virtualControllerService
                .ResetDevice(
                    deviceId
                );

            return true;
        }

        (void)
            m_impl->
            virtualControllerRegistry
            .SetEnabled(
                deviceId,
                true
            );

        (void)
            m_impl->
            virtualControllerRegistry
            .SetActive(
                deviceId,
                false
            );

        return true;
    }

    //
    // Virtual controller driver
    //

    bool SrhEngine::
        ConnectVirtualControllerDriver()
        noexcept
    {
        std::scoped_lock lock(
            m_impl->virtualControllerMutex
        );

        return
            m_impl->
            virtualControllerService
            .ConnectFirstAvailable();
    }

    void SrhEngine::
        DisconnectVirtualControllerDriver()
        noexcept
    {
        std::scoped_lock lock(
            m_impl->virtualControllerMutex
        );

        m_impl->
            virtualControllerService
            .Disconnect();

        m_impl->
            virtualControllerRegistry
            .SetAllActive(
                false
            );
    }

    bool SrhEngine::
        IsVirtualControllerDriverConnected()
        const noexcept
    {
        std::scoped_lock lock(
            m_impl->virtualControllerMutex
        );

        return
            m_impl->
            virtualControllerService
            .IsConnected();
    }

    std::uint32_t
        SrhEngine::
        GetVirtualControllerDriverError()
        const noexcept
    {
        std::scoped_lock lock(
            m_impl->virtualControllerMutex
        );

        return
            m_impl->
            virtualControllerService
            .LastError();
    }

    bool SrhEngine::
        ResetVirtualControllers()
        noexcept
    {
        std::scoped_lock lock(
            m_impl->virtualControllerMutex
        );

        const bool success =
            m_impl->
            virtualControllerService
            .ResetAll();

        m_impl->
            virtualControllerRegistry
            .SetAllActive(
                false
            );

        return success;
    }

    //
    // Media state
    //

    std::optional<MediaSessionInfo>
        SrhEngine::GetCurrentMediaSession()
        const
    {
        return
            m_impl->mediaService
            .GetCurrentSessionInfo();
    }
}