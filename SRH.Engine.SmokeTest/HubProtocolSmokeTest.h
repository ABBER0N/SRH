#pragma once

#include "HubConnectionSmokeTest.h"
#include "HubIngressSmokeTest.h"
#include "HubMessageSmokeTest.h"
#include "HubSerialTransportSmokeTest.h"

namespace srh::smoketest
{
    [[nodiscard]]
    bool RunHubProtocolSmokeTest();
}