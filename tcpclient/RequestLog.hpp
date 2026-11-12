#pragma once
#include <optional>

class RequestLog
{
public:
    std::string message;
    int requestID;
    std::optional<double> ping;
    bool isPacketLost;
};

