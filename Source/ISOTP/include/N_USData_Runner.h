#ifndef N_USDATA_RUNNER_H
#define N_USDATA_RUNNER_H

#include "CANInterface.h"
#include "ISOTP_Common.h"

#define NewCANFrameISOTP()                                                                                             \
    {.extd             = 1,                                                                                            \
     .rtr              = 0,                                                                                            \
     .ss               = 0,                                                                                            \
     .self             = 0,                                                                                            \
     .dlc_non_comp     = 0,                                                                                            \
     .reserved         = 0,                                                                                            \
     .identifier       = {.N_NFA_Header  = N_NFA_Header_Value,                                                         \
                          .N_NFA_Padding = N_NFA_Padding_Value,                                                        \
                          .N_TAtype      = CAN_UNKNOWN,                                                                \
                          .N_TA          = 0,                                                                          \
                          .N_SA          = 0},                                                                                  \
     .data_length_code = 0,                                                                                            \
     .data             = {0}}

#define updateInternalStatus(newStatus)                                                                                \
    do                                                                                                                 \
    {                                                                                                                  \
        auto oldStatus = internalStatus;                                                                               \
        internalStatus = newStatus;                                                                                    \
        OSInterfaceLogDebug(tag, "internalStatus changed from %s (%d) to %s (%d)", internalStatusToString(oldStatus),  \
                            oldStatus, internalStatusToString(internalStatus), internalStatus);                        \
    }                                                                                                                  \
    while (0)

#define returnError(errorCode)                                                                                         \
    do                                                                                                                 \
    {                                                                                                                  \
        updateInternalStatus(ERROR);                                                                                   \
        result = errorCode;                                                                                            \
        OSInterfaceLogError(tag, "Returning error %s.", N_ResultToString(errorCode));                                  \
        return result;                                                                                                 \
    }                                                                                                                  \
    while (false)

#define returnErrorWithLog(errorCode, fmt, ...)                                                                        \
    do                                                                                                                 \
    {                                                                                                                  \
        updateInternalStatus(ERROR);                                                                                   \
        result = errorCode;                                                                                            \
        OSInterfaceLogError(tag, "Returning error %s. " fmt, N_ResultToString(errorCode), ##__VA_ARGS__);              \
        return result;                                                                                                 \
    }                                                                                                                  \
    while (false)

class N_USData_Runner
{
public:
    using RunnerType = enum { RunnerUnknownType, RunnerRequestType, RunnerIndicationType };
    using FrameCode  = enum { SF_CODE = 0b0000, FF_CODE = 0b0001, CF_CODE = 0b0010, FC_CODE = 0b0011 };
    using FlowStatus = enum { CONTINUE_TO_SEND = 0, WAIT = 1, OVERFLOW = 2, INVALID_FS };

    constexpr static uint8_t  MAX_SF_MESSAGE_LENGTH          = 7;
    constexpr static uint8_t  MAX_CF_MESSAGE_LENGTH          = 7;
    constexpr static uint8_t  FC_MESSAGE_LENGTH              = 3;
    constexpr static uint32_t MIN_FF_DL_WITH_ESCAPE_SEQUENCE = 4096;

#if ISOTP_USE_DEBUG_TIMEOUTS
    constexpr static int32_t N_As_TIMEOUT_MS = 100000000;
    constexpr static int32_t N_Ar_TIMEOUT_MS = 100000000;
    constexpr static int32_t N_Bs_TIMEOUT_MS = 100000000;
    constexpr static int32_t N_Br_TIMEOUT_MS = 0.9 * N_Bs_TIMEOUT_MS; // Those are performance requirements.
    constexpr static int32_t N_Cr_TIMEOUT_MS = 100000000;
    constexpr static int32_t N_Cs_TIMEOUT_MS = 0.9 * N_Cr_TIMEOUT_MS; // Those are performance requirements.
    constexpr static int32_t MAX_TIMEOUT_MS  = 300000000;
#else
    constexpr static int32_t N_As_TIMEOUT_MS = 1000;
    constexpr static int32_t N_Ar_TIMEOUT_MS = 1000;
    constexpr static int32_t N_Bs_TIMEOUT_MS = 1000;
    constexpr static int32_t N_Br_TIMEOUT_MS = 0.9 * N_Bs_TIMEOUT_MS; // Those are performance requirements.
    constexpr static int32_t N_Cr_TIMEOUT_MS = 1000;
    constexpr static int32_t N_Cs_TIMEOUT_MS = 0.9 * N_Cr_TIMEOUT_MS; // Those are performance requirements.
    constexpr static int32_t MAX_TIMEOUT_MS  = 30000;
#endif

    constexpr static STmin DEFAULT_STMIN = {20, ms};

    N_USData_Runner()          = default;
    virtual ~N_USData_Runner() = default;

    static const char* runnerTypeToString(RunnerType type);
    static const char* frameCodeToString(FrameCode code);
    static const char* flowStatusToString(FlowStatus status);

    /**
     * @brief Runs the runner.
     *
     * If no frame is received, the runner will only execute if it is not awaiting a message, otherwise it will return
     * an error.
     *
     * @param receivedFrame Pointer to the received frame. If nullptr, no frame is received.
     * @return The result of the run.
     */
    virtual N_Result runStep(CANFrame* receivedFrame) = 0;

    /**
     * @brief Returns the next timestamp the runner will run. The timestamp is derived from OsInterface::millis().
     * @return The next timestamp the runner will run.
     */
    [[nodiscard]] virtual uint32_t getNextRunTime() = 0;

    /**
     * @brief Returns the N_AI of the runner.
     * @return The N_AI of the runner.
     */
    [[nodiscard]] virtual N_AI getN_AI() const = 0;

    /**
     * @brief Returns the message data of the runner.
     * @return The message data of the runner.
     */
    [[nodiscard]] virtual uint8_t* getMessageData() const = 0;

    /**
     * @brief Returns the message length of the runner.
     * @return The message length of the runner.
     */
    [[nodiscard]] virtual uint32_t getMessageLength() const = 0;

    /**
     * @brief Returns the result of the last runStep().
     * @return The result of the runner.
     */
    [[nodiscard]] virtual N_Result getResult() const = 0;

    /**
     * @brief Returns the Mtype of the runner.
     * @return The Mtype of the runner.
     */
    [[nodiscard]] virtual Mtype getMtype() const = 0;

    /**
     * @brief Returns the type of the runner.
     * @return The type of the runner.
     */
    [[nodiscard]] virtual RunnerType getRunnerType() const = 0;

    /**
     * @brief Callback for when a message is received.
     * @param success True if the message was received successfully, false otherwise.
     */
    virtual void messageACKReceivedCallback(CANInterface::ACKResult success) = 0;

    /**
     * @brief Returns the logging tag of the runner.
     * @return The logging tag of the runner.
     */
    [[nodiscard]] virtual const char*
    getTAG() const = 0; // TODO: in the future, allow ISOTP to set logging level of the runner.

    /**
     * @brief Returns if the frame is for this runner.
     * @param frame The frame to check.
     * @return True if the frame is for this runner, false otherwise.
     */
    [[nodiscard]] virtual bool isThisFrameForMe(const CANFrame& frame) const = 0;
};

#endif // N_USDATA_RUNNER_H
