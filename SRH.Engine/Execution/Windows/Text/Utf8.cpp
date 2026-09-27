#include "pch.h"

#include "Execution/Windows/Text/Utf8.h"

#include <Windows.h>

namespace srh::engine::windows
{
    std::string WideToUtf8(
        const std::wstring_view value
    )
    {
        if (value.empty())
        {
            return {};
        }

        const int requiredSize =
            WideCharToMultiByte(
                CP_UTF8,
                WC_ERR_INVALID_CHARS,
                value.data(),
                static_cast<int>(
                    value.size()
                    ),
                nullptr,
                0,
                nullptr,
                nullptr
            );

        if (requiredSize <= 0)
        {
            return {};
        }

        std::string result(
            static_cast<std::size_t>(
                requiredSize
                ),
            '\0'
        );

        const int convertedSize =
            WideCharToMultiByte(
                CP_UTF8,
                WC_ERR_INVALID_CHARS,
                value.data(),
                static_cast<int>(
                    value.size()
                    ),
                result.data(),
                requiredSize,
                nullptr,
                nullptr
            );

        if (
            convertedSize !=
            requiredSize
            )
        {
            return {};
        }

        return result;
    }

    std::wstring Utf8ToWide(
        const std::string_view value
    )
    {
        if (value.empty())
        {
            return {};
        }

        const int requiredSize =
            MultiByteToWideChar(
                CP_UTF8,
                MB_ERR_INVALID_CHARS,
                value.data(),
                static_cast<int>(
                    value.size()
                    ),
                nullptr,
                0
            );

        if (requiredSize <= 0)
        {
            return {};
        }

        std::wstring result(
            static_cast<std::size_t>(
                requiredSize
                ),
            L'\0'
        );

        const int convertedSize =
            MultiByteToWideChar(
                CP_UTF8,
                MB_ERR_INVALID_CHARS,
                value.data(),
                static_cast<int>(
                    value.size()
                    ),
                result.data(),
                requiredSize
            );

        if (
            convertedSize !=
            requiredSize
            )
        {
            return {};
        }

        return result;
    }
}