#ifndef ISOTP_COMMON_H
#define ISOTP_COMMON_H

#define ISOTP_USE_DEBUG_TIMEOUTS false

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#include <cstdint>

constexpr uint32_t ISOTP_MaxTimeToWaitForSync_MS = 100;

constexpr uint8_t MAX_STMIN_MS_VALUE = 0x7F;
constexpr uint8_t MIN_STMIN_US_VALUE = 0xF1;
constexpr uint8_t MAX_STMIN_US_VALUE = 0xF9;

constexpr uint8_t MAX_STMIN_STR_SIZE = 9; // 3 (value) + 5 (unit) + 1 (null terminator)

using STminUnit = enum SeparationTimeMinUnit { ms, usX100 };

using STmin = struct SeparationTimeMin
{
    uint8_t   value;
    STminUnit unit;
};

using Mtype = enum Mtype { Mtype_Diagnostics, Mtype_Unknown };

using N_Result = enum N_Result {
    NOT_STARTED = 0,
    IN_PROGRESS_FF, // Only used by N_USData_Indication_Runner to indicate that the FF was received in this step.
    IN_PROGRESS,
    N_OK,
    N_TIMEOUT_A,
    N_TIMEOUT_Bs,
    N_TIMEOUT_Cr,
    N_WRONG_SN,
    N_INVALID_FS,
    N_UNEXP_PDU,
    N_WFT_OVRN,
    N_BUFFER_OVFLW,
    N_ERROR
};

const char* N_ResultToString(N_Result result);

const char* STminToString(const STmin& stMin);

uint32_t getStMinInMs(STmin stMin);

#endif // ISOTP_COMMON_H
