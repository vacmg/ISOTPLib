#include "N_USData_Request_Runner.h"

#include <CANMessageACKQueue.h>
#include "ASSERT_MACROS.h"
#include "ISOTP.h"
#include "LinuxOSInterface.h"
#include "LocalCANNetwork.h"
#include "gtest/gtest.h"

static LinuxOSInterface linuxOSInterface;

constexpr int64_t DEFAULT_AVAILABLE_MEMORY_CONST = 200;

TEST(N_USData_Request_Runner, constructor_arguments_set)
{
    LocalCANNetwork    can_network;
    Atomic_int64_t     availableMemoryMock(DEFAULT_AVAILABLE_MEMORY_CONST, linuxOSInterface);
    CANInterface*      canInterface = can_network.newCANInterfaceConnection();
    CANMessageACKQueue canMessageACKQueue(*canInterface, linuxOSInterface);
    N_AI               NAi               = ISOTP_N_AI_CONFIG(N_TATYPE_6_CAN_CLASSIC_29bit_Functional, 1, 2);
    const char*        testMessageString = "Message";
    size_t             messageLen        = strlen(testMessageString);
    const uint8_t*     testMessage       = reinterpret_cast<const uint8_t*>(testMessageString);
    bool               result;

    N_USData_Request_Runner runner(result, NAi, availableMemoryMock, Mtype_Diagnostics, testMessage, messageLen,
                                   linuxOSInterface, canMessageACKQueue);

    ASSERT_TRUE(result);
    ASSERT_EQ(N_USData_Runner::RunnerRequestType, runner.getRunnerType());
    ASSERT_EQ(NOT_STARTED, runner.getResult());
    ASSERT_EQ(Mtype_Diagnostics, runner.getMtype());
    ASSERT_EQ_N_AI(NAi, runner.getN_AI());
    ASSERT_EQ(messageLen, runner.getMessageLength());
    ASSERT_EQ_ARRAY(testMessage, runner.getMessageData(), messageLen);

    delete canInterface;
}

TEST(N_USData_Request_Runner, constructor_destructor_argument_availableMemoryTest)
{
    LocalCANNetwork    can_network;
    Atomic_int64_t     availableMemoryMock(DEFAULT_AVAILABLE_MEMORY_CONST, linuxOSInterface);
    CANInterface*      canInterface = can_network.newCANInterfaceConnection();
    CANMessageACKQueue canMessageACKQueue(*canInterface, linuxOSInterface);
    N_AI               NAi               = ISOTP_N_AI_CONFIG(N_TATYPE_6_CAN_CLASSIC_29bit_Functional, 1, 2);
    const char*        testMessageString = "Message";
    size_t             messageLen        = strlen(testMessageString);
    const uint8_t*     testMessage       = reinterpret_cast<const uint8_t*>(testMessageString);

    {
        bool                    result;
        N_USData_Request_Runner runner(result, NAi, availableMemoryMock, Mtype_Diagnostics, testMessage, messageLen,
                                       linuxOSInterface, canMessageACKQueue);

        int64_t actualMemory;
        ASSERT_TRUE(availableMemoryMock.get(&actualMemory));
        ASSERT_EQ(DEFAULT_AVAILABLE_MEMORY_CONST, actualMemory + messageLen + N_USDATA_REQUEST_RUNNER_TAG_SIZE);
        ASSERT_TRUE(result);
    }
    int64_t actualMemory;
    ASSERT_TRUE(availableMemoryMock.get(&actualMemory));
    ASSERT_EQ(DEFAULT_AVAILABLE_MEMORY_CONST, actualMemory);

    delete canInterface;
}

TEST(N_USData_Request_Runner, constructor_destructor_argument_notAvailableMemoryTest)
{
    LocalCANNetwork    can_network;
    int64_t            availableMemoryConst = 2;
    Atomic_int64_t     availableMemoryMock(availableMemoryConst, linuxOSInterface);
    CANInterface*      canInterface = can_network.newCANInterfaceConnection();
    CANMessageACKQueue canMessageACKQueue(*canInterface, linuxOSInterface);
    N_AI               NAi               = ISOTP_N_AI_CONFIG(N_TATYPE_6_CAN_CLASSIC_29bit_Functional, 1, 2);
    const char*        testMessageString = "Message";
    size_t             messageLen        = strlen(testMessageString);
    const uint8_t*     testMessage       = reinterpret_cast<const uint8_t*>(testMessageString);

    {
        bool                    result;
        N_USData_Request_Runner runner(result, NAi, availableMemoryMock, Mtype_Diagnostics, testMessage, messageLen,
                                       linuxOSInterface, canMessageACKQueue);

        int64_t actualMemory;
        ASSERT_TRUE(availableMemoryMock.get(&actualMemory));
        ASSERT_EQ(availableMemoryConst, actualMemory);
        ASSERT_FALSE(result);
    }

    int64_t actualMemory;
    ASSERT_TRUE(availableMemoryMock.get(&actualMemory));
    ASSERT_EQ(availableMemoryConst, actualMemory);

    delete canInterface;
}

TEST(N_USData_Request_Runner, runStep_SF_valid)
{
    LocalCANNetwork    can_network;
    Atomic_int64_t     availableMemoryMock(DEFAULT_AVAILABLE_MEMORY_CONST, linuxOSInterface);
    CANInterface*      canInterfaceRunner = can_network.newCANInterfaceConnection();
    CANMessageACKQueue canMessageACKQueue(*canInterfaceRunner, linuxOSInterface);
    N_AI               NAi               = ISOTP_N_AI_CONFIG(N_TATYPE_5_CAN_CLASSIC_29bit_Physical, 1, 2);
    const char*        testMessageString = "1234567"; // strlen = 7
    size_t             messageLen        = strlen(testMessageString);
    const uint8_t*     testMessage       = reinterpret_cast<const uint8_t*>(testMessageString);
    bool               result;
    CANInterface*      canInterface = can_network.newCANInterfaceConnection();

    N_USData_Request_Runner runner(result, NAi, availableMemoryMock, Mtype_Diagnostics, testMessage, messageLen,
                                   linuxOSInterface, canMessageACKQueue);

    ASSERT_EQ(IN_PROGRESS, runner.runStep(nullptr));

    CANFrame receivedFrame;
    ASSERT_TRUE(canInterface->readFrame(&receivedFrame));

    canMessageACKQueue.runStep(); // Get ACK
    canMessageACKQueue.runAvailableAckCallbacks();
    ASSERT_EQ(N_OK, runner.runStep(nullptr));
    ASSERT_EQ(N_OK, runner.getResult());

    ASSERT_EQ(1, receivedFrame.extd);
    ASSERT_EQ(0, receivedFrame.dlc_non_comp);
    ASSERT_EQ_N_AI(NAi, receivedFrame.identifier);
    ASSERT_EQ(messageLen + 1, receivedFrame.data_length_code); // SF has 1 byte + data for N_PCI_SF
    ASSERT_EQ(N_USData_Runner::SF_CODE, receivedFrame.data[0] >> 4);
    ASSERT_EQ(messageLen, receivedFrame.data[0] & 0x0F);
    ASSERT_EQ(0, memcmp(testMessage, &receivedFrame.data[1], messageLen));

    delete canInterface;
    delete canInterfaceRunner;
}

TEST(N_USData_Request_Runner, runStep_SF_valid_empty)
{
    LocalCANNetwork    can_network;
    Atomic_int64_t     availableMemoryMock(DEFAULT_AVAILABLE_MEMORY_CONST, linuxOSInterface);
    CANInterface*      canInterfaceRunner = can_network.newCANInterfaceConnection();
    CANMessageACKQueue canMessageACKQueue(*canInterfaceRunner, linuxOSInterface);
    N_AI               NAi               = ISOTP_N_AI_CONFIG(N_TATYPE_6_CAN_CLASSIC_29bit_Functional, 1, 2);
    const char*        testMessageString = ""; // strlen = 0
    size_t             messageLen        = strlen(testMessageString);
    const uint8_t*     testMessage       = reinterpret_cast<const uint8_t*>(testMessageString);
    bool               result;
    CANInterface*      canInterface = can_network.newCANInterfaceConnection();

    N_USData_Request_Runner runner(result, NAi, availableMemoryMock, Mtype_Diagnostics, testMessage, messageLen,
                                   linuxOSInterface, canMessageACKQueue);

    ASSERT_EQ(IN_PROGRESS, runner.runStep(nullptr));

    CANFrame receivedFrame;
    ASSERT_TRUE(canInterface->readFrame(&receivedFrame));

    canMessageACKQueue.runStep(); // Get ACK
    canMessageACKQueue.runAvailableAckCallbacks();
    ASSERT_EQ(N_OK, runner.runStep(nullptr));
    ASSERT_EQ(N_OK, runner.getResult());

    ASSERT_EQ(1, receivedFrame.extd);
    ASSERT_EQ(0, receivedFrame.dlc_non_comp);
    ASSERT_EQ_N_AI(NAi, receivedFrame.identifier);
    ASSERT_EQ(messageLen + 1, receivedFrame.data_length_code); // SF has 1 byte + data for N_PCI_SF
    ASSERT_EQ(N_USData_Runner::SF_CODE, receivedFrame.data[0] >> 4);
    ASSERT_EQ(messageLen, receivedFrame.data[0] & 0x0F);
    ASSERT_EQ(0, memcmp(testMessage, &receivedFrame.data[1], messageLen));

    delete canInterface;
    delete canInterfaceRunner;
}

TEST(N_USData_Request_Runner, runStep_SF_timeoutAs)
{
    LocalCANNetwork    can_network;
    Atomic_int64_t     availableMemoryMock(DEFAULT_AVAILABLE_MEMORY_CONST, linuxOSInterface);
    CANInterface*      canInterfaceRunner = can_network.newCANInterfaceConnection();
    CANMessageACKQueue canMessageACKQueue(*canInterfaceRunner, linuxOSInterface);
    N_AI               NAi               = ISOTP_N_AI_CONFIG(N_TATYPE_6_CAN_CLASSIC_29bit_Functional, 1, 2);
    const char*        testMessageString = ""; // strlen = 0
    size_t             messageLen        = strlen(testMessageString);
    const uint8_t*     testMessage       = reinterpret_cast<const uint8_t*>(testMessageString);
    bool               result;

    N_USData_Request_Runner runner(result, NAi, availableMemoryMock, Mtype_Diagnostics, testMessage, messageLen,
                                   linuxOSInterface, canMessageACKQueue);
    CANInterface*           canInterface = can_network.newCANInterfaceConnection();

    ASSERT_EQ(IN_PROGRESS, runner.runStep(nullptr));
    ASSERT_GT(runner.getNextRunTime(), linuxOSInterface.osMillis());
    linuxOSInterface.osSleep(N_USData_Runner::N_As_TIMEOUT_MS + 1);
    ASSERT_LE(runner.getNextRunTime(), linuxOSInterface.osMillis());
    ASSERT_EQ(N_TIMEOUT_A, runner.runStep(nullptr));

    delete canInterfaceRunner;
    delete canInterface;
}

TEST(N_USData_Request_Runner, runStep_SF_unexpectedFrame)
{
    LocalCANNetwork    can_network;
    Atomic_int64_t     availableMemoryMock(DEFAULT_AVAILABLE_MEMORY_CONST, linuxOSInterface);
    CANInterface*      canInterfaceRunner = can_network.newCANInterfaceConnection();
    CANMessageACKQueue canMessageACKQueue(*canInterfaceRunner, linuxOSInterface);
    N_AI               NAi               = ISOTP_N_AI_CONFIG(N_TATYPE_6_CAN_CLASSIC_29bit_Functional, 1, 2);
    const char*        testMessageString = ""; // strlen = 0
    size_t             messageLen        = strlen(testMessageString);
    const uint8_t*     testMessage       = reinterpret_cast<const uint8_t*>(testMessageString);
    bool               result;

    N_USData_Request_Runner runner(result, NAi, availableMemoryMock, Mtype_Diagnostics, testMessage, messageLen,
                                   linuxOSInterface, canMessageACKQueue);

    CANFrame receivedFrame;

    ASSERT_EQ(N_ERROR, runner.runStep(&receivedFrame));

    delete canInterfaceRunner;
}

TEST(N_USData_Request_Runner, runStep_FF_valid)
{
    LocalCANNetwork    can_network;
    Atomic_int64_t     availableMemoryMock(DEFAULT_AVAILABLE_MEMORY_CONST, linuxOSInterface);
    CANInterface*      canInterfaceRunner = can_network.newCANInterfaceConnection();
    CANMessageACKQueue canMessageACKQueue(*canInterfaceRunner, linuxOSInterface);
    N_AI               NAi               = ISOTP_N_AI_CONFIG(N_TATYPE_5_CAN_CLASSIC_29bit_Physical, 1, 2);
    const char*        testMessageString = "0123456789"; // strlen = 10
    size_t             messageLen        = strlen(testMessageString);
    const uint8_t*     testMessage       = reinterpret_cast<const uint8_t*>(testMessageString);
    bool               result;

    N_USData_Request_Runner runner(result, NAi, availableMemoryMock, Mtype_Diagnostics, testMessage, messageLen,
                                   linuxOSInterface, canMessageACKQueue);
    CANInterface*           receiverCanInterface = can_network.newCANInterfaceConnection();

    ASSERT_EQ(IN_PROGRESS, runner.runStep(nullptr));
    ASSERT_EQ(IN_PROGRESS, runner.getResult());
    CANFrame receivedFrame;

    receiverCanInterface->readFrame(&receivedFrame);

    ASSERT_EQ(1, receivedFrame.extd);
    ASSERT_EQ(0, receivedFrame.dlc_non_comp);
    ASSERT_EQ_N_AI(NAi, receivedFrame.identifier);
    ASSERT_EQ(8, receivedFrame.data_length_code);
    ASSERT_EQ(N_USData_Runner::FF_CODE, receivedFrame.data[0] >> 4);
    uint32_t length = (receivedFrame.data[0] & 0x0F) << 8;
    length          = length | receivedFrame.data[1];
    ASSERT_EQ(10, length);
    ASSERT_EQ(0, memcmp(testMessage, &receivedFrame.data[2], 6));

    delete canInterfaceRunner;
    delete receiverCanInterface;
}

TEST(N_USData_Request_Runner, runStep_FF_big_valid)
{
    LocalCANNetwork    can_network;
    int64_t            availableMemoryConst = 10000;
    Atomic_int64_t     availableMemoryMock(availableMemoryConst, linuxOSInterface);
    CANInterface*      canInterfaceRunner = can_network.newCANInterfaceConnection();
    CANMessageACKQueue canMessageACKQueue(*canInterfaceRunner, linuxOSInterface);
    N_AI               NAi                      = ISOTP_N_AI_CONFIG(N_TATYPE_5_CAN_CLASSIC_29bit_Physical, 1, 2);
    const char*        testInitialMessageString = "0123456789"; // strlen = 10

    char* testMessageString = static_cast<char*>(malloc(5001));
    for (int i = 0; i < 500; i++)
    {
        memcpy(&testMessageString[i * 10], testInitialMessageString, 10);
    }
    testMessageString[5000] = '\0';

    size_t         messageLen  = strlen(testMessageString);
    const uint8_t* testMessage = reinterpret_cast<const uint8_t*>(testMessageString);
    bool           result;

    N_USData_Request_Runner runner(result, NAi, availableMemoryMock, Mtype_Diagnostics, testMessage, messageLen,
                                   linuxOSInterface, canMessageACKQueue);
    CANInterface*           receiverCanInterface = can_network.newCANInterfaceConnection();

    ASSERT_EQ(IN_PROGRESS, runner.runStep(nullptr));
    ASSERT_EQ(IN_PROGRESS, runner.getResult());
    CANFrame receivedFrame;

    receiverCanInterface->readFrame(&receivedFrame);

    ASSERT_EQ(1, receivedFrame.extd);
    ASSERT_EQ(0, receivedFrame.dlc_non_comp);
    ASSERT_EQ_N_AI(NAi, receivedFrame.identifier);
    ASSERT_EQ(8, receivedFrame.data_length_code);
    ASSERT_EQ(N_USData_Runner::FF_CODE, receivedFrame.data[0] >> 4);
    uint32_t length = (receivedFrame.data[0] & 0x0F) << 8;
    length          = length | receivedFrame.data[1];
    ASSERT_EQ(0, length);
    length =
        receivedFrame.data[2] << 24 | receivedFrame.data[3] << 16 | receivedFrame.data[4] << 8 |
        receivedFrame
            .data[5]; // unpack the message length (32 bits) 8 in data[2], 8 in data[3], 8 in data[4] and 8 in data[5]
    ASSERT_EQ(5000, length);
    ASSERT_EQ(0, memcmp(testMessage, &receivedFrame.data[6], 2));

    free(testMessageString);
    delete canInterfaceRunner;
    delete receiverCanInterface;
}

TEST(N_USData_Request_Runner, runStep_FF_unexpectedFrame)
{
    LocalCANNetwork    can_network;
    Atomic_int64_t     availableMemoryMock(DEFAULT_AVAILABLE_MEMORY_CONST, linuxOSInterface);
    CANInterface*      canInterfaceRunner = can_network.newCANInterfaceConnection();
    CANMessageACKQueue canMessageACKQueue(*canInterfaceRunner, linuxOSInterface);
    N_AI               NAi               = ISOTP_N_AI_CONFIG(N_TATYPE_5_CAN_CLASSIC_29bit_Physical, 1, 2);
    const char*        testMessageString = "0123456789"; // strlen = 10
    size_t             messageLen        = strlen(testMessageString);
    const uint8_t*     testMessage       = reinterpret_cast<const uint8_t*>(testMessageString);
    bool               result;

    N_USData_Request_Runner runner(result, NAi, availableMemoryMock, Mtype_Diagnostics, testMessage, messageLen,
                                   linuxOSInterface, canMessageACKQueue);

    CANFrame receivedFrame = NewCANFrameISOTP();
    ASSERT_EQ(N_ERROR, runner.runStep(&receivedFrame));
    ASSERT_EQ(N_ERROR, runner.getResult());

    delete canInterfaceRunner;
}

TEST(N_USData_Request_Runner, runStep_FF_wrong_frame_type)
{
    LocalCANNetwork    can_network;
    Atomic_int64_t     availableMemoryMock(DEFAULT_AVAILABLE_MEMORY_CONST, linuxOSInterface);
    CANInterface*      canInterfaceRunner = can_network.newCANInterfaceConnection();
    CANMessageACKQueue canMessageACKQueue(*canInterfaceRunner, linuxOSInterface);
    N_AI               NAi               = ISOTP_N_AI_CONFIG(N_TATYPE_6_CAN_CLASSIC_29bit_Functional, 1, 2);
    const char*        testMessageString = "0123456789"; // strlen = 10
    size_t             messageLen        = strlen(testMessageString);
    const uint8_t*     testMessage       = reinterpret_cast<const uint8_t*>(testMessageString);
    bool               result;

    N_USData_Request_Runner runner(result, NAi, availableMemoryMock, Mtype_Diagnostics, testMessage, messageLen,
                                   linuxOSInterface, canMessageACKQueue);
    ASSERT_FALSE(result);

    delete canInterfaceRunner;
}

TEST(N_USData_Request_Runner, runStep_First_CF_valid)
{
    LocalCANNetwork    can_network;
    Atomic_int64_t     availableMemoryMock(DEFAULT_AVAILABLE_MEMORY_CONST, linuxOSInterface);
    CANInterface*      canInterfaceRunner = can_network.newCANInterfaceConnection();
    CANMessageACKQueue canMessageACKQueue(*canInterfaceRunner, linuxOSInterface);
    N_AI               NAi               = ISOTP_N_AI_CONFIG(N_TATYPE_5_CAN_CLASSIC_29bit_Physical, 1, 2);
    const char*        testMessageString = "01234567890123456789"; // strlen = 20
    size_t             messageLen        = strlen(testMessageString);
    const uint8_t*     testMessage       = reinterpret_cast<const uint8_t*>(testMessageString);
    bool               result;

    N_USData_Request_Runner runner(result, NAi, availableMemoryMock, Mtype_Diagnostics, testMessage, messageLen,
                                   linuxOSInterface, canMessageACKQueue);
    CANInterface*           receiverCanInterface = can_network.newCANInterfaceConnection();

    runner.runStep(nullptr);
    CANFrame receivedFrame;

    // Indication Runner

    receiverCanInterface->readFrame(&receivedFrame);

    canMessageACKQueue.runStep(); // Get ACK
    canMessageACKQueue.runAvailableAckCallbacks();

    uint8_t blockSize = 4;
    STmin   stMin     = {10, ms};

    CANFrame fcFrame            = NewCANFrameISOTP();
    fcFrame.identifier.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical;
    fcFrame.identifier.N_TA     = NAi.N_SA;
    fcFrame.identifier.N_SA     = NAi.N_TA;

    fcFrame.data[0] = N_USData_Runner::FC_CODE << 4 | N_USData_Runner::FlowStatus::CONTINUE_TO_SEND;
    fcFrame.data[1] = blockSize;
    fcFrame.data[2] = stMin.value;

    fcFrame.data_length_code = 3;

    // Indication Runner

    ASSERT_EQ(IN_PROGRESS, runner.runStep(&fcFrame));
    ASSERT_EQ(IN_PROGRESS, runner.runStep(nullptr));

    receiverCanInterface->readFrame(&receivedFrame);

    ASSERT_EQ(1, receivedFrame.extd);
    ASSERT_EQ(0, receivedFrame.dlc_non_comp);
    ASSERT_EQ_N_AI(NAi, receivedFrame.identifier);
    ASSERT_EQ(8, receivedFrame.data_length_code);
    ASSERT_EQ(N_USData_Runner::CF_CODE, receivedFrame.data[0] >> 4);
    uint8_t sequenceNumber = receivedFrame.data[0] & 0x0F;
    ASSERT_EQ(1, sequenceNumber);
    ASSERT_EQ(0, memcmp(&testMessage[6], &receivedFrame.data[1], 7));

    delete canInterfaceRunner;
    delete receiverCanInterface;
}

TEST(N_USData_Request_Runner, runStep_First_Last_CF_valid)
{
    LocalCANNetwork    can_network;
    Atomic_int64_t     availableMemoryMock(DEFAULT_AVAILABLE_MEMORY_CONST, linuxOSInterface);
    CANInterface*      canInterfaceRunner = can_network.newCANInterfaceConnection();
    CANMessageACKQueue canMessageACKQueue(*canInterfaceRunner, linuxOSInterface);
    N_AI               NAi               = ISOTP_N_AI_CONFIG(N_TATYPE_5_CAN_CLASSIC_29bit_Physical, 1, 2);
    const char*        testMessageString = "0123456789"; // strlen = 10
    size_t             messageLen        = strlen(testMessageString);
    const uint8_t*     testMessage       = reinterpret_cast<const uint8_t*>(testMessageString);
    bool               result;

    N_USData_Request_Runner runner(result, NAi, availableMemoryMock, Mtype_Diagnostics, testMessage, messageLen,
                                   linuxOSInterface, canMessageACKQueue);
    CANInterface*           receiverCanInterface = can_network.newCANInterfaceConnection();

    runner.runStep(nullptr);
    CANFrame receivedFrame;

    // Indication Runner

    receiverCanInterface->readFrame(&receivedFrame);

    canMessageACKQueue.runStep(); // Get ACK
    canMessageACKQueue.runAvailableAckCallbacks();

    uint8_t blockSize = 4;
    STmin   stMin     = {10, ms};

    CANFrame fcFrame            = NewCANFrameISOTP();
    fcFrame.identifier.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical;
    fcFrame.identifier.N_TA     = NAi.N_SA;
    fcFrame.identifier.N_SA     = NAi.N_TA;

    fcFrame.data[0] = N_USData_Runner::FC_CODE << 4 | N_USData_Runner::FlowStatus::CONTINUE_TO_SEND;
    fcFrame.data[1] = blockSize;
    fcFrame.data[2] = stMin.value;

    fcFrame.data_length_code = 3;

    // Indication Runner

    ASSERT_EQ(IN_PROGRESS, runner.runStep(&fcFrame));
    ASSERT_EQ(IN_PROGRESS, runner.runStep(nullptr));

    receiverCanInterface->readFrame(&receivedFrame);

    canMessageACKQueue.runStep(); // Get ACK
    canMessageACKQueue.runAvailableAckCallbacks();

    ASSERT_EQ(N_OK, runner.runStep(nullptr));

    ASSERT_EQ(1, receivedFrame.extd);
    ASSERT_EQ(0, receivedFrame.dlc_non_comp);
    ASSERT_EQ_N_AI(NAi, receivedFrame.identifier);
    ASSERT_EQ(5, receivedFrame.data_length_code);
    ASSERT_EQ(N_USData_Runner::CF_CODE, receivedFrame.data[0] >> 4);
    uint8_t sequenceNumber = receivedFrame.data[0] & 0x0F;
    ASSERT_EQ(1, sequenceNumber);
    ASSERT_EQ(0, memcmp(&testMessage[6], &receivedFrame.data[1], 4));

    delete canInterfaceRunner;
    delete receiverCanInterface;
}

TEST(N_USData_Request_Runner, runStep_Intermediate_CF_valid)
{
    LocalCANNetwork    can_network;
    Atomic_int64_t     availableMemoryMock(DEFAULT_AVAILABLE_MEMORY_CONST, linuxOSInterface);
    CANInterface*      canInterfaceRunner = can_network.newCANInterfaceConnection();
    CANMessageACKQueue canMessageACKQueue(*canInterfaceRunner, linuxOSInterface);
    N_AI               NAi               = ISOTP_N_AI_CONFIG(N_TATYPE_5_CAN_CLASSIC_29bit_Physical, 1, 2);
    const char*        testMessageString = "012345678901234567890123456789"; // strlen = 30
    size_t             messageLen        = strlen(testMessageString);
    const uint8_t*     testMessage       = reinterpret_cast<const uint8_t*>(testMessageString);
    bool               result;

    N_USData_Request_Runner runner(result, NAi, availableMemoryMock, Mtype_Diagnostics, testMessage, messageLen,
                                   linuxOSInterface, canMessageACKQueue);
    CANInterface*           receiverCanInterface = can_network.newCANInterfaceConnection();

    runner.runStep(nullptr);
    CANFrame receivedFrame;

    // Indication Runner

    receiverCanInterface->readFrame(&receivedFrame);

    canMessageACKQueue.runStep(); // Get ACK
    canMessageACKQueue.runAvailableAckCallbacks();

    uint8_t blockSize = 4;
    STmin   stMin     = {10, ms};

    CANFrame fcFrame            = NewCANFrameISOTP();
    fcFrame.identifier.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical;
    fcFrame.identifier.N_TA     = NAi.N_SA;
    fcFrame.identifier.N_SA     = NAi.N_TA;

    fcFrame.data[0] = N_USData_Runner::FC_CODE << 4 | N_USData_Runner::FlowStatus::CONTINUE_TO_SEND;
    fcFrame.data[1] = blockSize;
    fcFrame.data[2] = stMin.value;

    fcFrame.data_length_code = 3;

    // Indication Runner

    ASSERT_EQ(IN_PROGRESS, runner.runStep(&fcFrame));
    ASSERT_EQ(IN_PROGRESS, runner.runStep(nullptr));

    receiverCanInterface->readFrame(&receivedFrame);

    canMessageACKQueue.runStep(); // Get ACK
    canMessageACKQueue.runAvailableAckCallbacks();

    ASSERT_EQ(IN_PROGRESS, runner.runStep(nullptr));

    receiverCanInterface->readFrame(&receivedFrame);

    canMessageACKQueue.runStep(); // Get ACK
    canMessageACKQueue.runAvailableAckCallbacks();

    ASSERT_EQ(1, receivedFrame.extd);
    ASSERT_EQ(0, receivedFrame.dlc_non_comp);
    ASSERT_EQ_N_AI(NAi, receivedFrame.identifier);
    ASSERT_EQ(8, receivedFrame.data_length_code);
    ASSERT_EQ(N_USData_Runner::CF_CODE, receivedFrame.data[0] >> 4);
    uint8_t sequenceNumber = receivedFrame.data[0] & 0x0F;
    ASSERT_EQ(2, sequenceNumber);
    ASSERT_EQ(0, memcmp(&testMessage[13], &receivedFrame.data[1], 7));

    ASSERT_EQ(IN_PROGRESS, runner.runStep(nullptr));

    delete canInterfaceRunner;
    delete receiverCanInterface;
}

TEST(N_USData_Request_Runner, runStep_Last_CF_valid)
{
    LocalCANNetwork    can_network;
    Atomic_int64_t     availableMemoryMock(DEFAULT_AVAILABLE_MEMORY_CONST, linuxOSInterface);
    CANInterface*      canInterfaceRunner = can_network.newCANInterfaceConnection();
    CANMessageACKQueue canMessageACKQueue(*canInterfaceRunner, linuxOSInterface);
    N_AI               NAi               = ISOTP_N_AI_CONFIG(N_TATYPE_5_CAN_CLASSIC_29bit_Physical, 1, 2);
    const char*        testMessageString = "0123456789012345678901234"; // strlen = 25
    size_t             messageLen        = strlen(testMessageString);
    const uint8_t*     testMessage       = reinterpret_cast<const uint8_t*>(testMessageString);
    bool               result;

    N_USData_Request_Runner runner(result, NAi, availableMemoryMock, Mtype_Diagnostics, testMessage, messageLen,
                                   linuxOSInterface, canMessageACKQueue);
    CANInterface*           receiverCanInterface = can_network.newCANInterfaceConnection();

    runner.runStep(nullptr);
    CANFrame receivedFrame;

    // Indication Runner

    receiverCanInterface->readFrame(&receivedFrame);

    canMessageACKQueue.runStep(); // Get ACK
    canMessageACKQueue.runAvailableAckCallbacks();

    uint8_t blockSize = 4;
    STmin   stMin     = {10, ms};

    CANFrame fcFrame            = NewCANFrameISOTP();
    fcFrame.identifier.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical;
    fcFrame.identifier.N_TA     = NAi.N_SA;
    fcFrame.identifier.N_SA     = NAi.N_TA;

    fcFrame.data[0] = N_USData_Runner::FC_CODE << 4 | N_USData_Runner::FlowStatus::CONTINUE_TO_SEND;
    fcFrame.data[1] = blockSize;
    fcFrame.data[2] = stMin.value;

    fcFrame.data_length_code = 3;

    // Indication Runner

    ASSERT_EQ(IN_PROGRESS, runner.runStep(&fcFrame));
    ASSERT_EQ(IN_PROGRESS, runner.runStep(nullptr));

    receiverCanInterface->readFrame(&receivedFrame);

    canMessageACKQueue.runStep(); // Get ACK
    canMessageACKQueue.runAvailableAckCallbacks();

    ASSERT_EQ(IN_PROGRESS, runner.runStep(nullptr));

    receiverCanInterface->readFrame(&receivedFrame);

    canMessageACKQueue.runStep(); // Get ACK
    canMessageACKQueue.runAvailableAckCallbacks();

    ASSERT_EQ(IN_PROGRESS, runner.runStep(nullptr));

    receiverCanInterface->readFrame(&receivedFrame);

    canMessageACKQueue.runStep(); // Get ACK
    canMessageACKQueue.runAvailableAckCallbacks();

    ASSERT_EQ(1, receivedFrame.extd);
    ASSERT_EQ(0, receivedFrame.dlc_non_comp);
    ASSERT_EQ_N_AI(NAi, receivedFrame.identifier);
    ASSERT_EQ(6, receivedFrame.data_length_code);
    ASSERT_EQ(N_USData_Runner::CF_CODE, receivedFrame.data[0] >> 4);
    uint8_t sequenceNumber = receivedFrame.data[0] & 0x0F;
    ASSERT_EQ(3, sequenceNumber);
    ASSERT_EQ(0, memcmp(&testMessage[20], &receivedFrame.data[1], 5));

    ASSERT_EQ(N_OK, runner.runStep(nullptr));

    delete canInterfaceRunner;
    delete receiverCanInterface;
}

TEST(N_USData_Request_Runner, runStep_AnotherFC_valid)
{
    LocalCANNetwork    can_network;
    Atomic_int64_t     availableMemoryMock(DEFAULT_AVAILABLE_MEMORY_CONST, linuxOSInterface);
    CANInterface*      canInterfaceRunner = can_network.newCANInterfaceConnection();
    CANMessageACKQueue canMessageACKQueue(*canInterfaceRunner, linuxOSInterface);
    N_AI               NAi               = ISOTP_N_AI_CONFIG(N_TATYPE_5_CAN_CLASSIC_29bit_Physical, 1, 2);
    const char*        testMessageString = "012345678901234567890123456789"; // strlen = 30
    size_t             messageLen        = strlen(testMessageString);
    const uint8_t*     testMessage       = reinterpret_cast<const uint8_t*>(testMessageString);
    bool               result;

    N_USData_Request_Runner runner(result, NAi, availableMemoryMock, Mtype_Diagnostics, testMessage, messageLen,
                                   linuxOSInterface, canMessageACKQueue);
    CANInterface*           receiverCanInterface = can_network.newCANInterfaceConnection();

    runner.runStep(nullptr);
    CANFrame receivedFrame;

    // Indication Runner

    receiverCanInterface->readFrame(&receivedFrame);

    canMessageACKQueue.runStep(); // Get ACK
    canMessageACKQueue.runAvailableAckCallbacks();

    uint8_t blockSize = 3;
    STmin   stMin     = {10, ms};

    CANFrame fcFrame            = NewCANFrameISOTP();
    fcFrame.identifier.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical;
    fcFrame.identifier.N_TA     = NAi.N_SA;
    fcFrame.identifier.N_SA     = NAi.N_TA;

    fcFrame.data[0] = N_USData_Runner::FC_CODE << 4 | N_USData_Runner::FlowStatus::CONTINUE_TO_SEND;
    fcFrame.data[1] = blockSize;
    fcFrame.data[2] = stMin.value;

    fcFrame.data_length_code = 3;

    // Indication Runner

    ASSERT_EQ(IN_PROGRESS, runner.runStep(&fcFrame));

    for (int32_t i = 0; i < 3; i++)
    {
        ASSERT_EQ(IN_PROGRESS, runner.runStep(nullptr));
        receiverCanInterface->readFrame(&receivedFrame);
        canMessageACKQueue.runStep(); // Get ACK
        canMessageACKQueue.runAvailableAckCallbacks();
    }

    ASSERT_EQ(IN_PROGRESS, runner.runStep(&fcFrame));

    ASSERT_EQ(IN_PROGRESS, runner.runStep(nullptr));
    receiverCanInterface->readFrame(&receivedFrame);
    canMessageACKQueue.runStep(); // Get ACK
    canMessageACKQueue.runAvailableAckCallbacks();

    delete canInterfaceRunner;
    delete receiverCanInterface;
}

TEST(N_USData_Request_Runner, runStep_AnotherFC_NotSent)
{
    LocalCANNetwork    can_network;
    Atomic_int64_t     availableMemoryMock(DEFAULT_AVAILABLE_MEMORY_CONST, linuxOSInterface);
    CANInterface*      canInterfaceRunner = can_network.newCANInterfaceConnection();
    CANMessageACKQueue canMessageACKQueue(*canInterfaceRunner, linuxOSInterface);
    N_AI               NAi               = ISOTP_N_AI_CONFIG(N_TATYPE_5_CAN_CLASSIC_29bit_Physical, 1, 2);
    const char*        testMessageString = "012345678901234567890123456789"; // strlen = 30
    size_t             messageLen        = strlen(testMessageString);
    const uint8_t*     testMessage       = reinterpret_cast<const uint8_t*>(testMessageString);
    bool               result;

    N_USData_Request_Runner runner(result, NAi, availableMemoryMock, Mtype_Diagnostics, testMessage, messageLen,
                                   linuxOSInterface, canMessageACKQueue);
    CANInterface*           receiverCanInterface = can_network.newCANInterfaceConnection();

    runner.runStep(nullptr);
    CANFrame receivedFrame;

    // Indication Runner

    receiverCanInterface->readFrame(&receivedFrame);

    canMessageACKQueue.runStep(); // Get ACK
    canMessageACKQueue.runAvailableAckCallbacks();

    uint8_t blockSize = 3;
    STmin   stMin     = {10, ms};

    CANFrame fcFrame            = NewCANFrameISOTP();
    fcFrame.identifier.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical;
    fcFrame.identifier.N_TA     = NAi.N_SA;
    fcFrame.identifier.N_SA     = NAi.N_TA;

    fcFrame.data[0] = N_USData_Runner::FC_CODE << 4 | N_USData_Runner::FlowStatus::CONTINUE_TO_SEND;
    fcFrame.data[1] = blockSize;
    fcFrame.data[2] = stMin.value;

    fcFrame.data_length_code = 3;

    // Indication Runner

    ASSERT_EQ(IN_PROGRESS, runner.runStep(&fcFrame));

    for (int32_t i = 0; i < 3; i++)
    {
        ASSERT_EQ(IN_PROGRESS, runner.runStep(nullptr));
        receiverCanInterface->readFrame(&receivedFrame);
        canMessageACKQueue.runStep(); // Get ACK
        canMessageACKQueue.runAvailableAckCallbacks();
    }

    ASSERT_EQ(N_ERROR, runner.runStep(nullptr));

    delete canInterfaceRunner;
    delete receiverCanInterface;
}

TEST(N_USData_Request_Runner, timeout_N_As_FF_noACK)
{
    LocalCANNetwork    can_network;
    Atomic_int64_t     availableMemoryMock(DEFAULT_AVAILABLE_MEMORY_CONST, linuxOSInterface);
    CANInterface*      canInterfaceRunner = can_network.newCANInterfaceConnection();
    CANMessageACKQueue canMessageACKQueue(*canInterfaceRunner, linuxOSInterface);
    N_AI               NAi               = ISOTP_N_AI_CONFIG(N_TATYPE_5_CAN_CLASSIC_29bit_Physical, 1, 2);
    const char*        testMessageString = "0123456789012345678901234"; // strlen = 25
    size_t             messageLen        = strlen(testMessageString);
    const uint8_t*     testMessage       = reinterpret_cast<const uint8_t*>(testMessageString);
    bool               result;

    N_USData_Request_Runner runner(result, NAi, availableMemoryMock, Mtype_Diagnostics, testMessage, messageLen,
                                   linuxOSInterface, canMessageACKQueue);
    CANInterface*           receiverCanInterface = can_network.newCANInterfaceConnection();

    runner.runStep(nullptr);
    CANFrame receivedFrame;

    // Indication Runner

    receiverCanInterface->readFrame(&receivedFrame);

    ASSERT_GT(runner.getNextRunTime(), linuxOSInterface.osMillis());
    linuxOSInterface.osSleep(N_USData_Runner::N_As_TIMEOUT_MS + 1);
    ASSERT_LE(runner.getNextRunTime(), linuxOSInterface.osMillis());

    ASSERT_EQ(N_TIMEOUT_A, runner.runStep(nullptr));

    delete canInterfaceRunner;
    delete receiverCanInterface;
}

TEST(N_USData_Request_Runner, timeout_N_As_FF_lateACK)
{
    LocalCANNetwork    can_network;
    Atomic_int64_t     availableMemoryMock(DEFAULT_AVAILABLE_MEMORY_CONST, linuxOSInterface);
    CANInterface*      canInterfaceRunner = can_network.newCANInterfaceConnection();
    CANMessageACKQueue canMessageACKQueue(*canInterfaceRunner, linuxOSInterface);
    N_AI               NAi               = ISOTP_N_AI_CONFIG(N_TATYPE_5_CAN_CLASSIC_29bit_Physical, 1, 2);
    const char*        testMessageString = "0123456789012345678901234"; // strlen = 25
    size_t             messageLen        = strlen(testMessageString);
    const uint8_t*     testMessage       = reinterpret_cast<const uint8_t*>(testMessageString);
    bool               result;

    N_USData_Request_Runner runner(result, NAi, availableMemoryMock, Mtype_Diagnostics, testMessage, messageLen,
                                   linuxOSInterface, canMessageACKQueue);
    CANInterface*           receiverCanInterface = can_network.newCANInterfaceConnection();

    runner.runStep(nullptr);
    CANFrame receivedFrame;

    // Indication Runner

    receiverCanInterface->readFrame(&receivedFrame);

    ASSERT_GT(runner.getNextRunTime(), linuxOSInterface.osMillis());
    linuxOSInterface.osSleep(N_USData_Runner::N_As_TIMEOUT_MS + 1);
    ASSERT_LE(runner.getNextRunTime(), linuxOSInterface.osMillis());

    canMessageACKQueue.runStep(); // Get ACK
    canMessageACKQueue.runAvailableAckCallbacks();

    ASSERT_EQ(N_TIMEOUT_A, runner.runStep(nullptr));

    delete canInterfaceRunner;
    delete receiverCanInterface;
}

TEST(N_USData_Request_Runner, timeout_N_As_CF_lateACK)
{
    LocalCANNetwork    can_network;
    Atomic_int64_t     availableMemoryMock(DEFAULT_AVAILABLE_MEMORY_CONST, linuxOSInterface);
    CANInterface*      canInterfaceRunner = can_network.newCANInterfaceConnection();
    CANMessageACKQueue canMessageACKQueue(*canInterfaceRunner, linuxOSInterface);
    N_AI               NAi               = ISOTP_N_AI_CONFIG(N_TATYPE_5_CAN_CLASSIC_29bit_Physical, 1, 2);
    const char*        testMessageString = "0123456789012345678901234"; // strlen = 25
    size_t             messageLen        = strlen(testMessageString);
    const uint8_t*     testMessage       = reinterpret_cast<const uint8_t*>(testMessageString);
    bool               result;

    N_USData_Request_Runner runner(result, NAi, availableMemoryMock, Mtype_Diagnostics, testMessage, messageLen,
                                   linuxOSInterface, canMessageACKQueue);
    CANInterface*           receiverCanInterface = can_network.newCANInterfaceConnection();

    runner.runStep(nullptr);
    CANFrame receivedFrame;

    // Indication Runner

    receiverCanInterface->readFrame(&receivedFrame);

    canMessageACKQueue.runStep(); // Get ACK
    canMessageACKQueue.runAvailableAckCallbacks();

    uint8_t blockSize = 4;
    STmin   stMin     = {10, ms};

    CANFrame fcFrame            = NewCANFrameISOTP();
    fcFrame.identifier.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical;
    fcFrame.identifier.N_TA     = NAi.N_SA;
    fcFrame.identifier.N_SA     = NAi.N_TA;

    fcFrame.data[0] = N_USData_Runner::FC_CODE << 4 | N_USData_Runner::FlowStatus::CONTINUE_TO_SEND;
    fcFrame.data[1] = blockSize;
    fcFrame.data[2] = stMin.value;

    fcFrame.data_length_code = 3;

    // Indication Runner

    ASSERT_EQ(IN_PROGRESS, runner.runStep(&fcFrame));
    ASSERT_EQ(IN_PROGRESS, runner.runStep(nullptr));

    ASSERT_GT(runner.getNextRunTime(), linuxOSInterface.osMillis());
    linuxOSInterface.osSleep(N_USData_Runner::N_As_TIMEOUT_MS + 1);
    ASSERT_LE(runner.getNextRunTime(), linuxOSInterface.osMillis());

    receiverCanInterface->readFrame(&receivedFrame);

    canMessageACKQueue.runStep(); // Get ACK
    canMessageACKQueue.runAvailableAckCallbacks();

    ASSERT_EQ(N_TIMEOUT_A, runner.runStep(nullptr));

    delete canInterfaceRunner;
    delete receiverCanInterface;
}

TEST(N_USData_Request_Runner, timeout_N_As_CF_noACK)
{
    LocalCANNetwork    can_network;
    Atomic_int64_t     availableMemoryMock(DEFAULT_AVAILABLE_MEMORY_CONST, linuxOSInterface);
    CANInterface*      canInterfaceRunner = can_network.newCANInterfaceConnection();
    CANMessageACKQueue canMessageACKQueue(*canInterfaceRunner, linuxOSInterface);
    N_AI               NAi               = ISOTP_N_AI_CONFIG(N_TATYPE_5_CAN_CLASSIC_29bit_Physical, 1, 2);
    const char*        testMessageString = "0123456789012345678901234"; // strlen = 25
    size_t             messageLen        = strlen(testMessageString);
    const uint8_t*     testMessage       = reinterpret_cast<const uint8_t*>(testMessageString);
    bool               result;

    N_USData_Request_Runner runner(result, NAi, availableMemoryMock, Mtype_Diagnostics, testMessage, messageLen,
                                   linuxOSInterface, canMessageACKQueue);
    CANInterface*           receiverCanInterface = can_network.newCANInterfaceConnection();

    runner.runStep(nullptr);
    CANFrame receivedFrame;

    // Indication Runner

    receiverCanInterface->readFrame(&receivedFrame);

    canMessageACKQueue.runStep(); // Get ACK
    canMessageACKQueue.runAvailableAckCallbacks();

    uint8_t blockSize = 4;
    STmin   stMin     = {10, ms};

    CANFrame fcFrame            = NewCANFrameISOTP();
    fcFrame.identifier.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical;
    fcFrame.identifier.N_TA     = NAi.N_SA;
    fcFrame.identifier.N_SA     = NAi.N_TA;

    fcFrame.data[0] = N_USData_Runner::FC_CODE << 4 | N_USData_Runner::FlowStatus::CONTINUE_TO_SEND;
    fcFrame.data[1] = blockSize;
    fcFrame.data[2] = stMin.value;

    fcFrame.data_length_code = 3;

    // Indication Runner

    ASSERT_EQ(IN_PROGRESS, runner.runStep(&fcFrame));
    ASSERT_EQ(IN_PROGRESS, runner.runStep(nullptr));

    ASSERT_GT(runner.getNextRunTime(), linuxOSInterface.osMillis());
    linuxOSInterface.osSleep(N_USData_Runner::N_As_TIMEOUT_MS + 1);
    ASSERT_LE(runner.getNextRunTime(), linuxOSInterface.osMillis());

    receiverCanInterface->readFrame(&receivedFrame);

    ASSERT_EQ(N_TIMEOUT_A, runner.runStep(nullptr));

    delete canInterfaceRunner;
    delete receiverCanInterface;
}

TEST(N_USData_Request_Runner, timeout_N_Bs_FF_lateFC)
{
    LocalCANNetwork    can_network;
    Atomic_int64_t     availableMemoryMock(DEFAULT_AVAILABLE_MEMORY_CONST, linuxOSInterface);
    CANInterface*      canInterfaceRunner = can_network.newCANInterfaceConnection();
    CANMessageACKQueue canMessageACKQueue(*canInterfaceRunner, linuxOSInterface);
    N_AI               NAi               = ISOTP_N_AI_CONFIG(N_TATYPE_5_CAN_CLASSIC_29bit_Physical, 1, 2);
    const char*        testMessageString = "0123456789012345678901234"; // strlen = 25
    size_t             messageLen        = strlen(testMessageString);
    const uint8_t*     testMessage       = reinterpret_cast<const uint8_t*>(testMessageString);
    bool               result;

    N_USData_Request_Runner runner(result, NAi, availableMemoryMock, Mtype_Diagnostics, testMessage, messageLen,
                                   linuxOSInterface, canMessageACKQueue);
    CANInterface*           receiverCanInterface = can_network.newCANInterfaceConnection();

    runner.runStep(nullptr);
    CANFrame receivedFrame;

    // Indication Runner

    receiverCanInterface->readFrame(&receivedFrame);

    canMessageACKQueue.runStep(); // Get ACK
    canMessageACKQueue.runAvailableAckCallbacks();

    ASSERT_GT(runner.getNextRunTime(), linuxOSInterface.osMillis());
    linuxOSInterface.osSleep(N_USData_Runner::N_Bs_TIMEOUT_MS + 1);
    ASSERT_LE(runner.getNextRunTime(), linuxOSInterface.osMillis());

    uint8_t blockSize = 4;
    STmin   stMin     = {10, ms};

    CANFrame fcFrame            = NewCANFrameISOTP();
    fcFrame.identifier.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical;
    fcFrame.identifier.N_TA     = NAi.N_SA;
    fcFrame.identifier.N_SA     = NAi.N_TA;

    fcFrame.data[0] = N_USData_Runner::FC_CODE << 4 | N_USData_Runner::FlowStatus::CONTINUE_TO_SEND;
    fcFrame.data[1] = blockSize;
    fcFrame.data[2] = stMin.value;

    fcFrame.data_length_code = 3;

    // Indication Runner

    ASSERT_EQ(N_TIMEOUT_Bs, runner.runStep(&fcFrame));

    delete canInterfaceRunner;
    delete receiverCanInterface;
}

TEST(N_USData_Request_Runner, timeout_N_Bs_FF_noFC)
{
    LocalCANNetwork    can_network;
    Atomic_int64_t     availableMemoryMock(DEFAULT_AVAILABLE_MEMORY_CONST, linuxOSInterface);
    CANInterface*      canInterfaceRunner = can_network.newCANInterfaceConnection();
    CANMessageACKQueue canMessageACKQueue(*canInterfaceRunner, linuxOSInterface);
    N_AI               NAi               = ISOTP_N_AI_CONFIG(N_TATYPE_5_CAN_CLASSIC_29bit_Physical, 1, 2);
    const char*        testMessageString = "0123456789012345678901234"; // strlen = 25
    size_t             messageLen        = strlen(testMessageString);
    const uint8_t*     testMessage       = reinterpret_cast<const uint8_t*>(testMessageString);
    bool               result;

    N_USData_Request_Runner runner(result, NAi, availableMemoryMock, Mtype_Diagnostics, testMessage, messageLen,
                                   linuxOSInterface, canMessageACKQueue);
    CANInterface*           receiverCanInterface = can_network.newCANInterfaceConnection();

    runner.runStep(nullptr);
    CANFrame receivedFrame;

    // Indication Runner

    receiverCanInterface->readFrame(&receivedFrame);

    canMessageACKQueue.runStep(); // Get ACK
    canMessageACKQueue.runAvailableAckCallbacks();

    ASSERT_GT(runner.getNextRunTime(), linuxOSInterface.osMillis());
    linuxOSInterface.osSleep(N_USData_Runner::N_Bs_TIMEOUT_MS + 1);
    ASSERT_LE(runner.getNextRunTime(), linuxOSInterface.osMillis());

    ASSERT_EQ(N_TIMEOUT_Bs, runner.runStep(nullptr));

    delete canInterfaceRunner;
    delete receiverCanInterface;
}

TEST(N_USData_Request_Runner, timeout_N_Bs_CF_lateFC)
{

    LocalCANNetwork    can_network;
    Atomic_int64_t     availableMemoryMock(DEFAULT_AVAILABLE_MEMORY_CONST, linuxOSInterface);
    CANInterface*      canInterfaceRunner = can_network.newCANInterfaceConnection();
    CANMessageACKQueue canMessageACKQueue(*canInterfaceRunner, linuxOSInterface);
    N_AI               NAi               = ISOTP_N_AI_CONFIG(N_TATYPE_5_CAN_CLASSIC_29bit_Physical, 1, 2);
    const char*        testMessageString = "0123456789012345678901234567"; // strlen = 28
    size_t             messageLen        = strlen(testMessageString);
    const uint8_t*     testMessage       = reinterpret_cast<const uint8_t*>(testMessageString);
    bool               result;

    N_USData_Request_Runner runner(result, NAi, availableMemoryMock, Mtype_Diagnostics, testMessage, messageLen,
                                   linuxOSInterface, canMessageACKQueue);
    CANInterface*           receiverCanInterface = can_network.newCANInterfaceConnection();

    runner.runStep(nullptr);
    CANFrame receivedFrame;

    // Indication Runner

    receiverCanInterface->readFrame(&receivedFrame);

    canMessageACKQueue.runStep(); // Get ACK
    canMessageACKQueue.runAvailableAckCallbacks();

    uint8_t blockSize = 3;
    STmin   stMin     = {10, ms};

    CANFrame fcFrame            = NewCANFrameISOTP();
    fcFrame.identifier.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical;
    fcFrame.identifier.N_TA     = NAi.N_SA;
    fcFrame.identifier.N_SA     = NAi.N_TA;

    fcFrame.data[0] = N_USData_Runner::FC_CODE << 4 | N_USData_Runner::FlowStatus::CONTINUE_TO_SEND;
    fcFrame.data[1] = blockSize;
    fcFrame.data[2] = stMin.value;

    fcFrame.data_length_code = 3;

    // Indication Runner

    ASSERT_EQ(IN_PROGRESS, runner.runStep(&fcFrame));

    ASSERT_EQ(IN_PROGRESS, runner.runStep(nullptr));

    receiverCanInterface->readFrame(&receivedFrame);

    canMessageACKQueue.runStep(); // Get ACK
    canMessageACKQueue.runAvailableAckCallbacks();

    ASSERT_EQ(IN_PROGRESS, runner.runStep(nullptr));

    receiverCanInterface->readFrame(&receivedFrame);

    canMessageACKQueue.runStep(); // Get ACK
    canMessageACKQueue.runAvailableAckCallbacks();

    ASSERT_EQ(IN_PROGRESS, runner.runStep(nullptr));

    receiverCanInterface->readFrame(&receivedFrame);

    canMessageACKQueue.runStep(); // Get ACK
    canMessageACKQueue.runAvailableAckCallbacks();

    ASSERT_GT(runner.getNextRunTime(), linuxOSInterface.osMillis());
    linuxOSInterface.osSleep(N_USData_Runner::N_Bs_TIMEOUT_MS + 1);
    ASSERT_LE(runner.getNextRunTime(), linuxOSInterface.osMillis());

    ASSERT_EQ(N_TIMEOUT_Bs, runner.runStep(&fcFrame));

    delete canInterfaceRunner;
    delete receiverCanInterface;
}

TEST(N_USData_Request_Runner, timeout_N_Bs_CF_noFC)
{

    LocalCANNetwork    can_network;
    Atomic_int64_t     availableMemoryMock(DEFAULT_AVAILABLE_MEMORY_CONST, linuxOSInterface);
    CANInterface*      canInterfaceRunner = can_network.newCANInterfaceConnection();
    CANMessageACKQueue canMessageACKQueue(*canInterfaceRunner, linuxOSInterface);
    N_AI               NAi               = ISOTP_N_AI_CONFIG(N_TATYPE_5_CAN_CLASSIC_29bit_Physical, 1, 2);
    const char*        testMessageString = "0123456789012345678901234567"; // strlen = 28
    size_t             messageLen        = strlen(testMessageString);
    const uint8_t*     testMessage       = reinterpret_cast<const uint8_t*>(testMessageString);
    bool               result;

    N_USData_Request_Runner runner(result, NAi, availableMemoryMock, Mtype_Diagnostics, testMessage, messageLen,
                                   linuxOSInterface, canMessageACKQueue);
    CANInterface*           receiverCanInterface = can_network.newCANInterfaceConnection();

    runner.runStep(nullptr);
    CANFrame receivedFrame;

    // Indication Runner

    receiverCanInterface->readFrame(&receivedFrame);

    canMessageACKQueue.runStep(); // Get ACK
    canMessageACKQueue.runAvailableAckCallbacks();

    uint8_t blockSize = 3;
    STmin   stMin     = {10, ms};

    CANFrame fcFrame            = NewCANFrameISOTP();
    fcFrame.identifier.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical;
    fcFrame.identifier.N_TA     = NAi.N_SA;
    fcFrame.identifier.N_SA     = NAi.N_TA;

    fcFrame.data[0] = N_USData_Runner::FC_CODE << 4 | N_USData_Runner::FlowStatus::CONTINUE_TO_SEND;
    fcFrame.data[1] = blockSize;
    fcFrame.data[2] = stMin.value;

    fcFrame.data_length_code = 3;

    // Indication Runner

    ASSERT_EQ(IN_PROGRESS, runner.runStep(&fcFrame));

    ASSERT_EQ(IN_PROGRESS, runner.runStep(nullptr));

    receiverCanInterface->readFrame(&receivedFrame);

    canMessageACKQueue.runStep(); // Get ACK
    canMessageACKQueue.runAvailableAckCallbacks();

    ASSERT_EQ(IN_PROGRESS, runner.runStep(nullptr));

    receiverCanInterface->readFrame(&receivedFrame);

    canMessageACKQueue.runStep(); // Get ACK
    canMessageACKQueue.runAvailableAckCallbacks();

    ASSERT_EQ(IN_PROGRESS, runner.runStep(nullptr));

    receiverCanInterface->readFrame(&receivedFrame);

    canMessageACKQueue.runStep(); // Get ACK
    canMessageACKQueue.runAvailableAckCallbacks();

    ASSERT_GT(runner.getNextRunTime(), linuxOSInterface.osMillis());
    linuxOSInterface.osSleep(N_USData_Runner::N_Bs_TIMEOUT_MS + 1);
    ASSERT_LE(runner.getNextRunTime(), linuxOSInterface.osMillis());

    ASSERT_EQ(N_TIMEOUT_Bs, runner.runStep(nullptr));

    delete canInterfaceRunner;
    delete receiverCanInterface;
}

TEST(N_USData_Request_Runner, timeout_N_Cs_FC_CF)
{
    LocalCANNetwork    can_network;
    Atomic_int64_t     availableMemoryMock(DEFAULT_AVAILABLE_MEMORY_CONST, linuxOSInterface);
    CANInterface*      canInterfaceRunner = can_network.newCANInterfaceConnection();
    CANMessageACKQueue canMessageACKQueue(*canInterfaceRunner, linuxOSInterface);
    N_AI               NAi               = ISOTP_N_AI_CONFIG(N_TATYPE_5_CAN_CLASSIC_29bit_Physical, 1, 2);
    const char*        testMessageString = "0123456789012345678901234"; // strlen = 25
    size_t             messageLen        = strlen(testMessageString);
    const uint8_t*     testMessage       = reinterpret_cast<const uint8_t*>(testMessageString);
    bool               result;

    N_USData_Request_Runner runner(result, NAi, availableMemoryMock, Mtype_Diagnostics, testMessage, messageLen,
                                   linuxOSInterface, canMessageACKQueue);
    CANInterface*           receiverCanInterface = can_network.newCANInterfaceConnection();

    runner.runStep(nullptr);
    CANFrame receivedFrame;

    // Indication Runner

    receiverCanInterface->readFrame(&receivedFrame);

    canMessageACKQueue.runStep(); // Get ACK
    canMessageACKQueue.runAvailableAckCallbacks();

    uint8_t blockSize = 4;
    STmin   stMin     = {10, ms};

    CANFrame fcFrame            = NewCANFrameISOTP();
    fcFrame.identifier.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical;
    fcFrame.identifier.N_TA     = NAi.N_SA;
    fcFrame.identifier.N_SA     = NAi.N_TA;

    fcFrame.data[0] = N_USData_Runner::FC_CODE << 4 | N_USData_Runner::FlowStatus::CONTINUE_TO_SEND;
    fcFrame.data[1] = blockSize;
    fcFrame.data[2] = stMin.value;

    fcFrame.data_length_code = 3;

    // Indication Runner

    ASSERT_EQ(IN_PROGRESS, runner.runStep(&fcFrame));

    ASSERT_GT(runner.getNextRunTime(), linuxOSInterface.osMillis());
    linuxOSInterface.osSleep(getStMinInMs(stMin) + 1);
    ASSERT_LE(runner.getNextRunTime(), linuxOSInterface.osMillis());

    ASSERT_EQ(IN_PROGRESS, runner.runStep(nullptr));

    delete canInterfaceRunner;
    delete receiverCanInterface;
}

TEST(N_USData_Request_Runner, timeout_N_Cs_Performance)
{
    LocalCANNetwork    can_network;
    Atomic_int64_t     availableMemoryMock(DEFAULT_AVAILABLE_MEMORY_CONST, linuxOSInterface);
    CANInterface*      canInterfaceRunner = can_network.newCANInterfaceConnection();
    CANMessageACKQueue canMessageACKQueue(*canInterfaceRunner, linuxOSInterface);
    N_AI               NAi               = ISOTP_N_AI_CONFIG(N_TATYPE_5_CAN_CLASSIC_29bit_Physical, 1, 2);
    const char*        testMessageString = "0123456789012345678901234"; // strlen = 25
    size_t             messageLen        = strlen(testMessageString);
    const uint8_t*     testMessage       = reinterpret_cast<const uint8_t*>(testMessageString);
    bool               result;

    N_USData_Request_Runner runner(result, NAi, availableMemoryMock, Mtype_Diagnostics, testMessage, messageLen,
                                   linuxOSInterface, canMessageACKQueue);
    CANInterface*           receiverCanInterface = can_network.newCANInterfaceConnection();

    runner.runStep(nullptr);
    CANFrame receivedFrame;

    // Indication Runner

    receiverCanInterface->readFrame(&receivedFrame);

    canMessageACKQueue.runStep(); // Get ACK
    canMessageACKQueue.runAvailableAckCallbacks();

    uint8_t blockSize = 4;
    STmin   stMin     = {10, ms};

    CANFrame fcFrame            = NewCANFrameISOTP();
    fcFrame.identifier.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical;
    fcFrame.identifier.N_TA     = NAi.N_SA;
    fcFrame.identifier.N_SA     = NAi.N_TA;

    fcFrame.data[0] = N_USData_Runner::FC_CODE << 4 | N_USData_Runner::FlowStatus::CONTINUE_TO_SEND;
    fcFrame.data[1] = blockSize;
    fcFrame.data[2] = stMin.value;

    fcFrame.data_length_code = 3;

    // Indication Runner

    ASSERT_EQ(IN_PROGRESS, runner.runStep(&fcFrame));

    ASSERT_GT(runner.getNextRunTime(), linuxOSInterface.osMillis());
    linuxOSInterface.osSleep(N_USData_Runner::N_Cs_TIMEOUT_MS + 1); // This should trigger a warning.
    ASSERT_LE(runner.getNextRunTime(), linuxOSInterface.osMillis());

    ASSERT_EQ(IN_PROGRESS, runner.runStep(nullptr));

    delete canInterfaceRunner;
    delete receiverCanInterface;
}

TEST(N_USData_Request_Runner, timeout_N_Cs_FC_LastCFInBlock)
{

    LocalCANNetwork    can_network;
    Atomic_int64_t     availableMemoryMock(DEFAULT_AVAILABLE_MEMORY_CONST, linuxOSInterface);
    CANInterface*      canInterfaceRunner = can_network.newCANInterfaceConnection();
    CANMessageACKQueue canMessageACKQueue(*canInterfaceRunner, linuxOSInterface);
    N_AI               NAi               = ISOTP_N_AI_CONFIG(N_TATYPE_5_CAN_CLASSIC_29bit_Physical, 1, 2);
    const char*        testMessageString = "0123456789012345678901234567"; // strlen = 28
    size_t             messageLen        = strlen(testMessageString);
    const uint8_t*     testMessage       = reinterpret_cast<const uint8_t*>(testMessageString);
    bool               result;

    N_USData_Request_Runner runner(result, NAi, availableMemoryMock, Mtype_Diagnostics, testMessage, messageLen,
                                   linuxOSInterface, canMessageACKQueue);
    CANInterface*           receiverCanInterface = can_network.newCANInterfaceConnection();

    runner.runStep(nullptr);
    CANFrame receivedFrame;

    // Indication Runner

    receiverCanInterface->readFrame(&receivedFrame);

    canMessageACKQueue.runStep(); // Get ACK
    canMessageACKQueue.runAvailableAckCallbacks();

    uint8_t blockSize = 3;
    STmin   stMin     = {10, ms};

    CANFrame fcFrame            = NewCANFrameISOTP();
    fcFrame.identifier.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical;
    fcFrame.identifier.N_TA     = NAi.N_SA;
    fcFrame.identifier.N_SA     = NAi.N_TA;

    fcFrame.data[0] = N_USData_Runner::FC_CODE << 4 | N_USData_Runner::FlowStatus::CONTINUE_TO_SEND;
    fcFrame.data[1] = blockSize;
    fcFrame.data[2] = stMin.value;

    fcFrame.data_length_code = 3;

    // Indication Runner

    ASSERT_EQ(IN_PROGRESS, runner.runStep(&fcFrame));

    ASSERT_GT(runner.getNextRunTime(), linuxOSInterface.osMillis());
    linuxOSInterface.osSleep(getStMinInMs(stMin) + 1);
    ASSERT_LE(runner.getNextRunTime(), linuxOSInterface.osMillis());

    ASSERT_EQ(IN_PROGRESS, runner.runStep(nullptr));

    receiverCanInterface->readFrame(&receivedFrame);

    canMessageACKQueue.runStep(); // Get ACK
    canMessageACKQueue.runAvailableAckCallbacks();

    ASSERT_GT(runner.getNextRunTime(), linuxOSInterface.osMillis());
    linuxOSInterface.osSleep(getStMinInMs(stMin) + 1);
    ASSERT_LE(runner.getNextRunTime(), linuxOSInterface.osMillis());

    ASSERT_EQ(IN_PROGRESS, runner.runStep(nullptr));

    receiverCanInterface->readFrame(&receivedFrame);

    canMessageACKQueue.runStep(); // Get ACK
    canMessageACKQueue.runAvailableAckCallbacks();

    ASSERT_GT(runner.getNextRunTime(), linuxOSInterface.osMillis());
    linuxOSInterface.osSleep(getStMinInMs(stMin) + 1);
    ASSERT_LE(runner.getNextRunTime(), linuxOSInterface.osMillis());

    ASSERT_EQ(IN_PROGRESS, runner.runStep(nullptr));

    receiverCanInterface->readFrame(&receivedFrame);

    canMessageACKQueue.runStep(); // Get ACK
    canMessageACKQueue.runAvailableAckCallbacks();

    ASSERT_GT(runner.getNextRunTime(), linuxOSInterface.osMillis());
    linuxOSInterface.osSleep(getStMinInMs(stMin) + 1);
    ASSERT_GT(runner.getNextRunTime(), linuxOSInterface.osMillis());

    ASSERT_EQ(IN_PROGRESS, runner.runStep(&fcFrame));

    delete canInterfaceRunner;
    delete receiverCanInterface;
}
