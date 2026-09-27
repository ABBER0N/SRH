#include "ConsoleUtf8.h"

#include <Windows.h>

namespace srh::smoketest
{
    void ConfigureUtf8Console()
    {
        SetConsoleCP(
            CP_UTF8
        );

        SetConsoleOutputCP(
            CP_UTF8
        );
    }
}