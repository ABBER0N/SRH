#include "pch.h"

#include "Mapping/MappingService.h"

#include <algorithm>
#include <cmath>
#include <mutex>
#include <utility>

namespace srh::engine
{
    void MappingService::SetRules(
        std::vector<MappingRule> rules
    )
    {
        std::unique_lock lock(
            m_mutex
        );

        m_rules =
            std::move(rules);
    }

    void MappingService::AddOrUpdateRule(
        MappingRule rule
    )
    {
        std::unique_lock lock(
            m_mutex
        );

        const auto iterator =
            std::find_if(
                m_rules.begin(),
                m_rules.end(),
                [&rule](
                    const MappingRule& existingRule
                    )
                {
                    return
                        existingRule.id ==
                        rule.id;
                }
            );

        if (
            iterator !=
            m_rules.end()
            )
        {
            *iterator =
                std::move(rule);

            return;
        }

        m_rules.push_back(
            std::move(rule)
        );
    }

    void MappingService::RemoveRule(
        const MappingRuleId ruleId
    )
    {
        std::unique_lock lock(
            m_mutex
        );

        std::erase_if(
            m_rules,
            [ruleId](
                const MappingRule& rule
                )
            {
                return
                    rule.id ==
                    ruleId;
            }
        );
    }

    void MappingService::Clear()
    {
        std::unique_lock lock(
            m_mutex
        );

        m_rules.clear();
    }

    std::optional<MappingRule>
        MappingService::FindRule(
            const MappingRuleId ruleId
        ) const
    {
        std::shared_lock lock(
            m_mutex
        );

        const auto iterator =
            std::find_if(
                m_rules.begin(),
                m_rules.end(),
                [ruleId](
                    const MappingRule& rule
                    )
                {
                    return
                        rule.id ==
                        ruleId;
                }
            );

        if (
            iterator ==
            m_rules.end()
            )
        {
            return std::nullopt;
        }

        return *iterator;
    }

    std::vector<MappingRule>
        MappingService::GetRules() const
    {
        std::shared_lock lock(
            m_mutex
        );

        return m_rules;
    }

    std::vector<Action>
        MappingService::Resolve(
            const InputEvent& event
        ) const
    {
        std::vector<MappingRule>
            matchingRules;

        {
            std::shared_lock lock(
                m_mutex
            );

            for (
                const auto& rule :
                m_rules
                )
            {
                if (
                    Matches(
                        rule,
                        event
                    )
                    )
                {
                    matchingRules.push_back(
                        rule
                    );
                }
            }
        }

        std::vector<Action>
            actions;

        actions.reserve(
            matchingRules.size()
        );

        for (
            const auto& rule :
            matchingRules
            )
        {
            actions.push_back(
                BuildAction(
                    rule,
                    event
                )
            );
        }

        return actions;
    }

    bool MappingService::Matches(
        const MappingRule& rule,
        const InputEvent& event
    )
    {
        if (!rule.enabled)
        {
            return false;
        }

        if (
            rule.input.nodeId !=
            event.nodeId
            )
        {
            return false;
        }

        if (
            rule.input.controlId !=
            event.controlId
            )
        {
            return false;
        }

        if (
            rule.input.eventType !=
            event.type
            )
        {
            return false;
        }

        const auto inputValue =
            GetInputValue(
                event
            );

        switch (
            rule.input.valueCondition
            )
        {
        case InputValueCondition::Any:
            return true;

        case InputValueCondition::Equal:
            return
                inputValue ==
                rule.input.compareValue;

        case InputValueCondition::Positive:
            return
                inputValue > 0;

        case InputValueCondition::Negative:
            return
                inputValue < 0;

        default:
            return false;
        }
    }

    Action MappingService::BuildAction(
        const MappingRule& rule,
        const InputEvent& event
    )
    {
        Action action =
            rule.action;

        if (
            rule.valueMode ==
            ActionValueMode::Fixed
            )
        {
            return action;
        }

        const auto inputValue =
            GetInputValue(
                event
            );

        const float transformedValue =
            static_cast<float>(
                inputValue
                ) *
            rule.valueScale +
            rule.valueOffset;

        std::visit(
            [transformedValue](
                auto& concreteAction
                )
            {
                using ActionType =
                    std::decay_t<
                    decltype(
                        concreteAction
                        )
                    >;

                if constexpr (
                    std::is_same_v<
                    ActionType,
                    VirtualControllerAction
                    >
                    )
                {
                    concreteAction.value =
                        static_cast<std::int32_t>(
                            std::lround(
                                transformedValue
                            )
                            );
                }
                else if constexpr (
                    std::is_same_v<
                    ActionType,
                    SystemAction
                    >
                    )
                {
                    concreteAction.value =
                        transformedValue;
                }
                else if constexpr (
                    std::is_same_v<
                    ActionType,
                    HubAction
                    >
                    )
                {
                    concreteAction.value =
                        static_cast<std::int32_t>(
                            std::lround(
                                transformedValue
                            )
                            );
                }
            },
            action
        );

        return action;
    }

    std::int32_t MappingService::GetInputValue(
        const InputEvent& event
    )
    {
        switch (event.type)
        {
        case InputEventType::ButtonDown:
            return 1;

        case InputEventType::ButtonUp:
            return 0;

        case InputEventType::EncoderDelta:
        case InputEventType::AxisValue:
            return event.value;

        case InputEventType::Unknown:
        default:
            return 0;
        }
    }
}