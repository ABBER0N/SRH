#pragma once

#include "Domain/Action.h"
#include "Domain/InputEvent.h"
#include "Mapping/MappingRule.h"

#include <optional>
#include <shared_mutex>
#include <vector>

namespace srh::engine
{
    class MappingService
    {
    public:
        MappingService() = default;

        MappingService(
            const MappingService&
        ) = delete;

        MappingService& operator=(
            const MappingService&
            ) = delete;

        void SetRules(
            std::vector<MappingRule> rules
        );

        void AddOrUpdateRule(
            MappingRule rule
        );

        void RemoveRule(
            MappingRuleId ruleId
        );

        void Clear();

        [[nodiscard]]
        std::optional<MappingRule>
            FindRule(
                MappingRuleId ruleId
            ) const;

        [[nodiscard]]
        std::vector<MappingRule>
            GetRules() const;

        [[nodiscard]]
        std::vector<Action>
            Resolve(
                const InputEvent& event
            ) const;

    private:
        [[nodiscard]]
        static bool Matches(
            const MappingRule& rule,
            const InputEvent& event
        );

        [[nodiscard]]
        static Action BuildAction(
            const MappingRule& rule,
            const InputEvent& event
        );

        [[nodiscard]]
        static std::int32_t GetInputValue(
            const InputEvent& event
        );

        mutable std::shared_mutex
            m_mutex;

        std::vector<MappingRule>
            m_rules;
    };
}