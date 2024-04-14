#pragma once

namespace TS45ToMQTT
{

typedef enum
{
    IDDLE,
    RECEIVING_DATA,
    MESSAGE_RECEIVED,
    DATA_OVERFLOW,
    TIMEOUT,
}  Status;

} // namespace TS45ToMQTT