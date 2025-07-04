#include "ISOTP.h"

#include <LocalCANNetwork.h>
#include "ASSERT_MACROS.h"
#include "LinuxOSInterface.h"
#include "gtest/gtest.h"

static LinuxOSInterface   osInterface;
constexpr uint32_t DEFAULT_TIMEOUT = 10000;

volatile bool senderKeepRunning   = true;
volatile bool receiverKeepRunning = true;

// SimpleSendReceiveTestSF
constexpr char     SimpleSendReceiveTestSF_message[]     = "patata";
constexpr uint32_t SimpleSendReceiveTestSF_messageLength = 7;

static uint32_t SimpleSendReceiveTestSF_N_USData_confirm_cb_calls = 0;
void            SimpleSendReceiveTestSF_N_USData_confirm_cb(N_AI nAi, N_Result nResult, Mtype mtype)
{
    SimpleSendReceiveTestSF_N_USData_confirm_cb_calls++;

    N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 1};
    EXPECT_EQ_N_AI(expectedNAi, nAi);
    EXPECT_EQ(N_OK, nResult);
    EXPECT_EQ(Mtype_Diagnostics, mtype);

    OSInterfaceLogInfo("SimpleSendReceiveTestSF_N_USData_confirm_cb", "SenderKeepRunning set to false");
    senderKeepRunning = false;
}

static uint32_t SimpleSendReceiveTestSF_N_USData_indication_cb_calls = 0;
void SimpleSendReceiveTestSF_N_USData_indication_cb(N_AI nAi, const uint8_t* messageData, uint32_t messageLength,
                                                    N_Result nResult, Mtype mtype)
{
    SimpleSendReceiveTestSF_N_USData_indication_cb_calls++;
    N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 1};
    EXPECT_EQ(N_OK, nResult);
    EXPECT_EQ(Mtype_Diagnostics, mtype);
    EXPECT_EQ_N_AI(expectedNAi, nAi);
    ASSERT_EQ(SimpleSendReceiveTestSF_messageLength, messageLength);
    ASSERT_NE(nullptr, messageData);
    EXPECT_EQ_ARRAY(SimpleSendReceiveTestSF_message, messageData, SimpleSendReceiveTestSF_messageLength);

    OSInterfaceLogInfo("SimpleSendReceiveTestSF_N_USData_indication_cb", "ReceiverKeepRunning set to false");
    receiverKeepRunning = false;
}

static uint32_t SimpleSendReceiveTestSF_N_USData_FF_indication_cb_calls = 0;
void SimpleSendReceiveTestSF_N_USData_FF_indication_cb(const N_AI nAi, const uint32_t messageLength, const Mtype mtype)
{
    SimpleSendReceiveTestSF_N_USData_FF_indication_cb_calls++;
}

TEST(ISOTP_SystemTests, SimpleSendReceiveTestSF)
{
    constexpr uint32_t TIMEOUT = 10000; // 10 seconds
    senderKeepRunning          = true;
    receiverKeepRunning        = true;

    LocalCANNetwork network;
    CANInterface*   senderInterface   = network.newCANInterfaceConnection("senderInterface");
    CANInterface*   receiverInterface = network.newCANInterfaceConnection("receiverInterface");
    ISOTP*       senderISOTP =
        new ISOTP(1, 2000, SimpleSendReceiveTestSF_N_USData_confirm_cb,
                     SimpleSendReceiveTestSF_N_USData_indication_cb, SimpleSendReceiveTestSF_N_USData_FF_indication_cb,
                     osInterface, *senderInterface, 2, ISOTP_DefaultSTmin, "senderISOTP");
    ISOTP* receiverISOTP =
        new ISOTP(2, 2000, SimpleSendReceiveTestSF_N_USData_confirm_cb,
                     SimpleSendReceiveTestSF_N_USData_indication_cb, SimpleSendReceiveTestSF_N_USData_FF_indication_cb,
                     osInterface, *receiverInterface, 2, ISOTP_DefaultSTmin, "receiverISOTP");

    uint32_t initialTime = osInterface.osMillis();
    uint32_t step        = 0;
    while ((senderKeepRunning || receiverKeepRunning) && osInterface.osMillis() - initialTime < TIMEOUT)
    {
        senderISOTP->runStep();
        senderISOTP->canMessageACKQueueRunStep();
        receiverISOTP->runStep();
        receiverISOTP->canMessageACKQueueRunStep();

        if (step == 5)
        {
            EXPECT_TRUE(
                senderISOTP->N_USData_request(2, N_TATYPE_5_CAN_CLASSIC_29bit_Physical,
                                                 reinterpret_cast<const uint8_t*>(SimpleSendReceiveTestSF_message),
                                                 SimpleSendReceiveTestSF_messageLength, Mtype_Diagnostics));
        }

        step++;
    }
    uint32_t elapsedTime = osInterface.osMillis() - initialTime;

    EXPECT_EQ(0, SimpleSendReceiveTestSF_N_USData_FF_indication_cb_calls);
    EXPECT_EQ(1, SimpleSendReceiveTestSF_N_USData_confirm_cb_calls);
    EXPECT_EQ(1, SimpleSendReceiveTestSF_N_USData_indication_cb_calls);

    ASSERT_LT(elapsedTime, TIMEOUT) << "Test took too long: " << elapsedTime << " ms, Timeout was: " << TIMEOUT;

    delete senderISOTP;
    delete receiverISOTP;
    delete senderInterface;
    delete receiverInterface;
}
// END SimpleSendReceiveTestSF

// ManySendReceiveTestSF
constexpr char     ManySendReceiveTestSF_message1[]     = "patata";
constexpr uint32_t ManySendReceiveTestSF_messageLength1 = 7;
constexpr char     ManySendReceiveTestSF_message2[]     = "cocida";
constexpr uint32_t ManySendReceiveTestSF_messageLength2 = 7;

static uint32_t ManySendReceiveTestSF_N_USData_confirm_cb_calls = 0;
void            ManySendReceiveTestSF_N_USData_confirm_cb(N_AI nAi, N_Result nResult, Mtype mtype)
{
    ManySendReceiveTestSF_N_USData_confirm_cb_calls++;

    N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 1};
    EXPECT_EQ_N_AI(expectedNAi, nAi);
    EXPECT_EQ(N_OK, nResult);
    EXPECT_EQ(Mtype_Diagnostics, mtype);

    if (ManySendReceiveTestSF_N_USData_confirm_cb_calls == 1)
    {
        OSInterfaceLogInfo("ManySendReceiveTestSF_N_USData_confirm_cb", "First call");
    }

    if (ManySendReceiveTestSF_N_USData_confirm_cb_calls == 2)
    {
        OSInterfaceLogInfo("ManySendReceiveTestSF_N_USData_confirm_cb", "SenderKeepRunning set to false");
        senderKeepRunning = false;
    }
}

static uint32_t ManySendReceiveTestSF_N_USData_indication_cb_calls = 0;
void ManySendReceiveTestSF_N_USData_indication_cb(N_AI nAi, const uint8_t* messageData, uint32_t messageLength,
                                                  N_Result nResult, Mtype mtype)
{
    ManySendReceiveTestSF_N_USData_indication_cb_calls++;
    N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 1};
    EXPECT_EQ(N_OK, nResult);
    EXPECT_EQ(Mtype_Diagnostics, mtype);
    EXPECT_EQ_N_AI(expectedNAi, nAi);
    if (ManySendReceiveTestSF_N_USData_indication_cb_calls == 1)
    {
        ASSERT_EQ(ManySendReceiveTestSF_messageLength1, messageLength);
        ASSERT_NE(nullptr, messageData);
        EXPECT_EQ_ARRAY(ManySendReceiveTestSF_message1, messageData, ManySendReceiveTestSF_messageLength1);

        OSInterfaceLogInfo("ManySendReceiveTestSF_N_USData_indication_cb", "First call");
    }
    else if (ManySendReceiveTestSF_N_USData_indication_cb_calls == 2)
    {
        ASSERT_EQ(ManySendReceiveTestSF_messageLength2, messageLength);
        ASSERT_NE(nullptr, messageData);
        EXPECT_EQ_ARRAY(ManySendReceiveTestSF_message2, messageData, ManySendReceiveTestSF_messageLength2);

        OSInterfaceLogInfo("ManySendReceiveTestSF_N_USData_indication_cb", "ReceiverKeepRunning set to false");
        receiverKeepRunning = false;
    }
}

static uint32_t ManySendReceiveTestSF_N_USData_FF_indication_cb_calls = 0;
void ManySendReceiveTestSF_N_USData_FF_indication_cb(const N_AI nAi, const uint32_t messageLength, const Mtype mtype)
{
    ManySendReceiveTestSF_N_USData_FF_indication_cb_calls++;
}

TEST(ISOTP_SystemTests, ManySendReceiveTestSF)
{
    constexpr uint32_t TIMEOUT = 10000; // 10 seconds
    senderKeepRunning          = true;
    receiverKeepRunning        = true;

    LocalCANNetwork network;
    CANInterface*   senderInterface   = network.newCANInterfaceConnection("senderInterface");
    CANInterface*   receiverInterface = network.newCANInterfaceConnection("receiverInterface");
    ISOTP*       senderISOTP =
        new ISOTP(1, 2000, ManySendReceiveTestSF_N_USData_confirm_cb, ManySendReceiveTestSF_N_USData_indication_cb,
                     ManySendReceiveTestSF_N_USData_FF_indication_cb, osInterface, *senderInterface, 2,
                     ISOTP_DefaultSTmin, "senderISOTP");
    ISOTP* receiverISOTP =
        new ISOTP(2, 2000, ManySendReceiveTestSF_N_USData_confirm_cb, ManySendReceiveTestSF_N_USData_indication_cb,
                     ManySendReceiveTestSF_N_USData_FF_indication_cb, osInterface, *receiverInterface, 2,
                     ISOTP_DefaultSTmin, "receiverISOTP");

    uint32_t initialTime = osInterface.osMillis();
    uint32_t step        = 0;
    while ((senderKeepRunning || receiverKeepRunning) && osInterface.osMillis() - initialTime < TIMEOUT)
    {
        senderISOTP->runStep();
        senderISOTP->canMessageACKQueueRunStep();
        receiverISOTP->runStep();
        receiverISOTP->canMessageACKQueueRunStep();

        if (step == 5)
        {
            EXPECT_TRUE(
                senderISOTP->N_USData_request(2, N_TATYPE_5_CAN_CLASSIC_29bit_Physical,
                                                 reinterpret_cast<const uint8_t*>(ManySendReceiveTestSF_message1),
                                                 ManySendReceiveTestSF_messageLength1, Mtype_Diagnostics));
            EXPECT_TRUE(
                senderISOTP->N_USData_request(2, N_TATYPE_5_CAN_CLASSIC_29bit_Physical,
                                                 reinterpret_cast<const uint8_t*>(ManySendReceiveTestSF_message2),
                                                 ManySendReceiveTestSF_messageLength2, Mtype_Diagnostics));
        }

        step++;
    }
    uint32_t elapsedTime = osInterface.osMillis() - initialTime;

    EXPECT_EQ(0, ManySendReceiveTestSF_N_USData_FF_indication_cb_calls);
    EXPECT_EQ(2, ManySendReceiveTestSF_N_USData_confirm_cb_calls);
    EXPECT_EQ(2, ManySendReceiveTestSF_N_USData_indication_cb_calls);

    ASSERT_LT(elapsedTime, TIMEOUT) << "Test took too long: " << elapsedTime << " ms, Timeout was: " << TIMEOUT;

    delete senderISOTP;
    delete receiverISOTP;
    delete senderInterface;
    delete receiverInterface;
}
// END ManySendReceiveTestSF

// BigSFTestBroadcast
constexpr char     BigSFTestBroadcast_message[]     = "patatasFritas";
constexpr uint32_t BigSFTestBroadcast_messageLength = 14;

static uint32_t BigSFTestBroadcast_N_USData_confirm_cb_calls = 0;
void            BigSFTestBroadcast_N_USData_confirm_cb(N_AI nAi, N_Result nResult, Mtype mtype)
{
    BigSFTestBroadcast_N_USData_confirm_cb_calls++;

    N_AI expectedNAi = {.N_TAtype = N_TATYPE_6_CAN_CLASSIC_29bit_Functional, .N_TA = 2, .N_SA = 1};
    EXPECT_EQ_N_AI(expectedNAi, nAi);
    EXPECT_EQ(N_OK, nResult);
    EXPECT_EQ(Mtype_Diagnostics, mtype);

    OSInterfaceLogInfo("BigSFTestBroadcast_N_USData_confirm_cb", "SenderKeepRunning set to false");
    senderKeepRunning = false;
}

static uint32_t BigSFTestBroadcast_N_USData_indication_cb_calls = 0;
void            BigSFTestBroadcast_N_USData_indication_cb(N_AI nAi, const uint8_t* messageData, uint32_t messageLength,
                                                          N_Result nResult, Mtype mtype)
{
    BigSFTestBroadcast_N_USData_indication_cb_calls++;
    N_AI expectedNAi = {.N_TAtype = N_TATYPE_6_CAN_CLASSIC_29bit_Functional, .N_TA = 2, .N_SA = 1};
    EXPECT_EQ(N_OK, nResult);
    EXPECT_EQ(Mtype_Diagnostics, mtype);
    EXPECT_EQ_N_AI(expectedNAi, nAi);
    ASSERT_EQ(BigSFTestBroadcast_messageLength, messageLength);
    ASSERT_NE(nullptr, messageData);
    EXPECT_EQ_ARRAY(BigSFTestBroadcast_message, messageData, BigSFTestBroadcast_messageLength);

    OSInterfaceLogInfo("BigSFTestBroadcast_N_USData_indication_cb", "ReceiverKeepRunning set to false");
    receiverKeepRunning = false;
}

static uint32_t BigSFTestBroadcast_N_USData_FF_indication_cb_calls = 0;
void BigSFTestBroadcast_N_USData_FF_indication_cb(const N_AI nAi, const uint32_t messageLength, const Mtype mtype)
{
    BigSFTestBroadcast_N_USData_FF_indication_cb_calls++;
}

TEST(ISOTP_SystemTests, BigSFTestBroadcast)
{
    constexpr uint32_t TIMEOUT = 10000; // 10 seconds
    senderKeepRunning          = true;
    receiverKeepRunning        = true;

    LocalCANNetwork network;
    CANInterface*   senderInterface    = network.newCANInterfaceConnection("senderInterface");
    CANInterface*   receiverInterface1 = network.newCANInterfaceConnection("receiverInterface1");
    CANInterface*   receiverInterface2 = network.newCANInterfaceConnection("receiverInterface2");
    ISOTP*       senderISOTP =
        new ISOTP(1, 2000, BigSFTestBroadcast_N_USData_confirm_cb, BigSFTestBroadcast_N_USData_indication_cb,
                     BigSFTestBroadcast_N_USData_FF_indication_cb, osInterface, *senderInterface, 2,
                     ISOTP_DefaultSTmin, "senderISOTP");
    ISOTP* receiverISOTP1 =
        new ISOTP(2, 2000, BigSFTestBroadcast_N_USData_confirm_cb, BigSFTestBroadcast_N_USData_indication_cb,
                     BigSFTestBroadcast_N_USData_FF_indication_cb, osInterface, *receiverInterface1, 2,
                     ISOTP_DefaultSTmin, "receiverISOTP1");
    ISOTP* receiverISOTP2 =
        new ISOTP(3, 2000, BigSFTestBroadcast_N_USData_confirm_cb, BigSFTestBroadcast_N_USData_indication_cb,
                     BigSFTestBroadcast_N_USData_FF_indication_cb, osInterface, *receiverInterface2, 2,
                     ISOTP_DefaultSTmin, "receiverISOTP2");

    uint32_t initialTime = osInterface.osMillis();
    uint32_t step        = 0;

    receiverISOTP1->addAcceptedFunctionalN_TA(2);
    receiverISOTP2->addAcceptedFunctionalN_TA(2);

    while ((senderKeepRunning || receiverKeepRunning) && osInterface.osMillis() - initialTime < TIMEOUT)
    {
        senderISOTP->runStep();
        senderISOTP->canMessageACKQueueRunStep();
        receiverISOTP1->runStep();
        receiverISOTP1->canMessageACKQueueRunStep();
        receiverISOTP2->runStep();
        receiverISOTP2->canMessageACKQueueRunStep();

        if (step == 5)
        {
            EXPECT_FALSE(senderISOTP->N_USData_request(2, N_TATYPE_6_CAN_CLASSIC_29bit_Functional,
                                                          reinterpret_cast<const uint8_t*>(BigSFTestBroadcast_message),
                                                          BigSFTestBroadcast_messageLength, Mtype_Diagnostics));
        }

        if (step == 10)
        {
            senderKeepRunning   = false;
            receiverKeepRunning = false;
        }

        step++;
    }
    uint32_t elapsedTime = osInterface.osMillis() - initialTime;

    EXPECT_EQ(0, BigSFTestBroadcast_N_USData_FF_indication_cb_calls);
    EXPECT_EQ(0, BigSFTestBroadcast_N_USData_confirm_cb_calls);
    EXPECT_EQ(0, BigSFTestBroadcast_N_USData_indication_cb_calls);

    ASSERT_LT(elapsedTime, TIMEOUT) << "Test took too long: " << elapsedTime << " ms, Timeout was: " << TIMEOUT;

    delete senderISOTP;
    delete receiverISOTP1;
    delete receiverISOTP2;
    delete senderInterface;
    delete receiverInterface1;
    delete receiverInterface2;
}
// END BigSFTestBroadcast

// SimpleSendReceiveTestBroadcast
constexpr char     SimpleSendReceiveTestBroadcast_message[]     = "patata";
constexpr uint32_t SimpleSendReceiveTestBroadcast_messageLength = 7;

static uint32_t SimpleSendReceiveTestBroadcast_N_USData_confirm_cb_calls = 0;
void            SimpleSendReceiveTestBroadcast_N_USData_confirm_cb(N_AI nAi, N_Result nResult, Mtype mtype)
{
    SimpleSendReceiveTestBroadcast_N_USData_confirm_cb_calls++;

    N_AI expectedNAi = {.N_TAtype = N_TATYPE_6_CAN_CLASSIC_29bit_Functional, .N_TA = 2, .N_SA = 1};
    EXPECT_EQ_N_AI(expectedNAi, nAi);
    EXPECT_EQ(N_OK, nResult);
    EXPECT_EQ(Mtype_Diagnostics, mtype);

    OSInterfaceLogInfo("SimpleSendReceiveTestBroadcast_N_USData_confirm_cb", "SenderKeepRunning set to false");
    senderKeepRunning = false;
}

static uint32_t SimpleSendReceiveTestBroadcast_N_USData_indication_cb_calls = 0;
void SimpleSendReceiveTestBroadcast_N_USData_indication_cb(N_AI nAi, const uint8_t* messageData, uint32_t messageLength,
                                                           N_Result nResult, Mtype mtype)
{
    SimpleSendReceiveTestBroadcast_N_USData_indication_cb_calls++;
    N_AI expectedNAi = {.N_TAtype = N_TATYPE_6_CAN_CLASSIC_29bit_Functional, .N_TA = 2, .N_SA = 1};
    EXPECT_EQ(N_OK, nResult);
    EXPECT_EQ(Mtype_Diagnostics, mtype);
    EXPECT_EQ_N_AI(expectedNAi, nAi);
    ASSERT_EQ(SimpleSendReceiveTestBroadcast_messageLength, messageLength);
    ASSERT_NE(nullptr, messageData);
    EXPECT_EQ_ARRAY(SimpleSendReceiveTestBroadcast_message, messageData, SimpleSendReceiveTestBroadcast_messageLength);

    OSInterfaceLogInfo("SimpleSendReceiveTestBroadcast_N_USData_indication_cb", "ReceiverKeepRunning set to false");
    receiverKeepRunning = false;
}

static uint32_t SimpleSendReceiveTestBroadcast_N_USData_FF_indication_cb_calls = 0;
void            SimpleSendReceiveTestBroadcast_N_USData_FF_indication_cb(const N_AI nAi, const uint32_t messageLength,
                                                                         const Mtype mtype)
{
    SimpleSendReceiveTestBroadcast_N_USData_FF_indication_cb_calls++;
}

TEST(ISOTP_SystemTests, SimpleSendReceiveTestBroadcast)
{
    constexpr uint32_t TIMEOUT = 10000; // 10 seconds
    senderKeepRunning          = true;
    receiverKeepRunning        = true;

    LocalCANNetwork network;
    CANInterface*   senderInterface    = network.newCANInterfaceConnection("senderInterface");
    CANInterface*   receiverInterface1 = network.newCANInterfaceConnection("receiverInterface1");
    CANInterface*   receiverInterface2 = network.newCANInterfaceConnection("receiverInterface2");
    ISOTP*       senderISOTP     = new ISOTP(1, 2000, SimpleSendReceiveTestBroadcast_N_USData_confirm_cb,
                                                      SimpleSendReceiveTestBroadcast_N_USData_indication_cb,
                                                      SimpleSendReceiveTestBroadcast_N_USData_FF_indication_cb, osInterface,
                                                      *senderInterface, 2, ISOTP_DefaultSTmin, "senderISOTP");
    ISOTP*       receiverISOTP1  = new ISOTP(2, 2000, SimpleSendReceiveTestBroadcast_N_USData_confirm_cb,
                                                      SimpleSendReceiveTestBroadcast_N_USData_indication_cb,
                                                      SimpleSendReceiveTestBroadcast_N_USData_FF_indication_cb, osInterface,
                                                      *receiverInterface1, 2, ISOTP_DefaultSTmin, "receiverISOTP1");
    ISOTP*       receiverISOTP2  = new ISOTP(3, 2000, SimpleSendReceiveTestBroadcast_N_USData_confirm_cb,
                                                      SimpleSendReceiveTestBroadcast_N_USData_indication_cb,
                                                      SimpleSendReceiveTestBroadcast_N_USData_FF_indication_cb, osInterface,
                                                      *receiverInterface2, 2, ISOTP_DefaultSTmin, "receiverISOTP2");

    uint32_t initialTime = osInterface.osMillis();
    uint32_t step        = 0;

    receiverISOTP1->addAcceptedFunctionalN_TA(2);
    receiverISOTP2->addAcceptedFunctionalN_TA(2);

    while ((senderKeepRunning || receiverKeepRunning) && osInterface.osMillis() - initialTime < TIMEOUT)
    {
        senderISOTP->runStep();
        senderISOTP->canMessageACKQueueRunStep();
        receiverISOTP1->runStep();
        receiverISOTP1->canMessageACKQueueRunStep();
        receiverISOTP2->runStep();
        receiverISOTP2->canMessageACKQueueRunStep();

        if (step == 5)
        {
            EXPECT_TRUE(senderISOTP->N_USData_request(
                2, N_TATYPE_6_CAN_CLASSIC_29bit_Functional,
                reinterpret_cast<const uint8_t*>(SimpleSendReceiveTestBroadcast_message),
                SimpleSendReceiveTestBroadcast_messageLength, Mtype_Diagnostics));
        }

        step++;
    }
    uint32_t elapsedTime = osInterface.osMillis() - initialTime;

    EXPECT_EQ(0, SimpleSendReceiveTestBroadcast_N_USData_FF_indication_cb_calls);
    EXPECT_EQ(1, SimpleSendReceiveTestBroadcast_N_USData_confirm_cb_calls);
    EXPECT_EQ(2, SimpleSendReceiveTestBroadcast_N_USData_indication_cb_calls);

    ASSERT_LT(elapsedTime, TIMEOUT) << "Test took too long: " << elapsedTime << " ms, Timeout was: " << TIMEOUT;

    delete senderISOTP;
    delete receiverISOTP1;
    delete receiverISOTP2;
    delete senderInterface;
    delete receiverInterface1;
    delete receiverInterface2;
}
// END SimpleSendReceiveTestBroadcast

// ManySendReceiveTestBroadcast
constexpr char     ManySendReceiveTestBroadcast_message1[]     = "patata";
constexpr uint32_t ManySendReceiveTestBroadcast_messageLength1 = 7;
constexpr char     ManySendReceiveTestBroadcast_message2[]     = "cocida";
constexpr uint32_t ManySendReceiveTestBroadcast_messageLength2 = 7;

static uint32_t ManySendReceiveTestBroadcast_N_USData_confirm_cb_calls = 0;
void            ManySendReceiveTestBroadcast_N_USData_confirm_cb(N_AI nAi, N_Result nResult, Mtype mtype)
{
    ManySendReceiveTestBroadcast_N_USData_confirm_cb_calls++;

    if (ManySendReceiveTestBroadcast_N_USData_confirm_cb_calls == 2)
    {
        N_AI expectedNAi = {.N_TAtype = N_TATYPE_6_CAN_CLASSIC_29bit_Functional, .N_TA = 2, .N_SA = 1};
        EXPECT_EQ_N_AI(expectedNAi, nAi);
        EXPECT_EQ(N_OK, nResult);
        EXPECT_EQ(Mtype_Diagnostics, mtype);

        OSInterfaceLogInfo("ManySendReceiveTestBroadcast_N_USData_confirm_cb", "SenderKeepRunning set to false");
        senderKeepRunning = false;
    }

    if (ManySendReceiveTestBroadcast_N_USData_confirm_cb_calls == 1)
    {
        N_AI expectedNAi = {.N_TAtype = N_TATYPE_6_CAN_CLASSIC_29bit_Functional, .N_TA = 3, .N_SA = 1};
        EXPECT_EQ_N_AI(expectedNAi, nAi);
        EXPECT_EQ(N_OK, nResult);
        EXPECT_EQ(Mtype_Diagnostics, mtype);

        OSInterfaceLogInfo("ManySendReceiveTestBroadcast_N_USData_confirm_cb", "First call");
    }
}

static uint32_t ManySendReceiveTestBroadcast_N_USData_indication_cb_calls = 0;
void ManySendReceiveTestBroadcast_N_USData_indication_cb(N_AI nAi, const uint8_t* messageData, uint32_t messageLength,
                                                         N_Result nResult, Mtype mtype)
{
    ManySendReceiveTestBroadcast_N_USData_indication_cb_calls++;

    if (ManySendReceiveTestBroadcast_N_USData_indication_cb_calls == 1)
    {
        N_AI expectedNAi = {.N_TAtype = N_TATYPE_6_CAN_CLASSIC_29bit_Functional, .N_TA = 3, .N_SA = 1};
        EXPECT_EQ(N_OK, nResult);
        EXPECT_EQ(Mtype_Diagnostics, mtype);
        EXPECT_EQ_N_AI(expectedNAi, nAi);

        ASSERT_EQ(ManySendReceiveTestBroadcast_messageLength2, messageLength);
        ASSERT_NE(nullptr, messageData);
        EXPECT_EQ_ARRAY(ManySendReceiveTestBroadcast_message2, messageData,
                        ManySendReceiveTestBroadcast_messageLength2);

        OSInterfaceLogInfo("ManySendReceiveTestBroadcast_N_USData_indication_cb", "First call");
    }
    else if (ManySendReceiveTestBroadcast_N_USData_indication_cb_calls <= 3)
    {
        N_AI expectedNAi = {.N_TAtype = N_TATYPE_6_CAN_CLASSIC_29bit_Functional, .N_TA = 2, .N_SA = 1};
        EXPECT_EQ(N_OK, nResult);
        EXPECT_EQ(Mtype_Diagnostics, mtype);
        EXPECT_EQ_N_AI(expectedNAi, nAi);

        ASSERT_EQ(ManySendReceiveTestBroadcast_messageLength1, messageLength);
        ASSERT_NE(nullptr, messageData);
        EXPECT_EQ_ARRAY(ManySendReceiveTestBroadcast_message1, messageData,
                        ManySendReceiveTestBroadcast_messageLength1);

        if (ManySendReceiveTestBroadcast_N_USData_indication_cb_calls == 2)
        {
            OSInterfaceLogInfo("ManySendReceiveTestBroadcast_N_USData_indication_cb", "Second call");
        }
        else if (ManySendReceiveTestBroadcast_N_USData_indication_cb_calls == 3)
        {
            OSInterfaceLogInfo("ManySendReceiveTestBroadcast_N_USData_indication_cb",
                               "ReceiverKeepRunning set to false");
            receiverKeepRunning = false;
        }
    }
}

static uint32_t ManySendReceiveTestBroadcast_N_USData_FF_indication_cb_calls = 0;
void            ManySendReceiveTestBroadcast_N_USData_FF_indication_cb(const N_AI nAi, const uint32_t messageLength,
                                                                       const Mtype mtype)
{
    ManySendReceiveTestBroadcast_N_USData_FF_indication_cb_calls++;
}

TEST(ISOTP_SystemTests, ManySendReceiveTestBroadcast)
{
    constexpr uint32_t TIMEOUT = 10000; // 10 seconds
    senderKeepRunning          = true;
    receiverKeepRunning        = true;

    LocalCANNetwork network;
    CANInterface*   senderInterface    = network.newCANInterfaceConnection("senderInterface");
    CANInterface*   receiverInterface1 = network.newCANInterfaceConnection("receiverInterface1");
    CANInterface*   receiverInterface2 = network.newCANInterfaceConnection("receiverInterface2");
    ISOTP*       senderISOTP     = new ISOTP(1, 2000, ManySendReceiveTestBroadcast_N_USData_confirm_cb,
                                                      ManySendReceiveTestBroadcast_N_USData_indication_cb,
                                                      ManySendReceiveTestBroadcast_N_USData_FF_indication_cb, osInterface,
                                                      *senderInterface, 2, ISOTP_DefaultSTmin, "senderISOTP");
    ISOTP*       receiverISOTP1  = new ISOTP(2, 2000, ManySendReceiveTestBroadcast_N_USData_confirm_cb,
                                                      ManySendReceiveTestBroadcast_N_USData_indication_cb,
                                                      ManySendReceiveTestBroadcast_N_USData_FF_indication_cb, osInterface,
                                                      *receiverInterface1, 2, ISOTP_DefaultSTmin, "receiverISOTP1");
    ISOTP*       receiverISOTP2  = new ISOTP(3, 2000, ManySendReceiveTestBroadcast_N_USData_confirm_cb,
                                                      ManySendReceiveTestBroadcast_N_USData_indication_cb,
                                                      SimpleSendReceiveTestBroadcast_N_USData_FF_indication_cb, osInterface,
                                                      *receiverInterface2, 2, ISOTP_DefaultSTmin, "receiverISOTP2");

    uint32_t initialTime = osInterface.osMillis();
    uint32_t step        = 0;

    receiverISOTP1->addAcceptedFunctionalN_TA(2);
    receiverISOTP1->addAcceptedFunctionalN_TA(3);

    receiverISOTP2->addAcceptedFunctionalN_TA(2);

    while ((senderKeepRunning || receiverKeepRunning) && osInterface.osMillis() - initialTime < TIMEOUT)
    {
        senderISOTP->runStep();
        senderISOTP->canMessageACKQueueRunStep();
        receiverISOTP1->runStep();
        receiverISOTP1->canMessageACKQueueRunStep();
        receiverISOTP2->runStep();
        receiverISOTP2->canMessageACKQueueRunStep();

        if (step == 5)
        {
            EXPECT_TRUE(senderISOTP->N_USData_request(
                2, N_TATYPE_6_CAN_CLASSIC_29bit_Functional,
                reinterpret_cast<const uint8_t*>(ManySendReceiveTestBroadcast_message1),
                ManySendReceiveTestBroadcast_messageLength1, Mtype_Diagnostics));
            EXPECT_TRUE(senderISOTP->N_USData_request(
                3, N_TATYPE_6_CAN_CLASSIC_29bit_Functional,
                reinterpret_cast<const uint8_t*>(ManySendReceiveTestBroadcast_message2),
                ManySendReceiveTestBroadcast_messageLength2, Mtype_Diagnostics));
        }

        step++;
    }
    uint32_t elapsedTime = osInterface.osMillis() - initialTime;

    EXPECT_EQ(0, ManySendReceiveTestBroadcast_N_USData_FF_indication_cb_calls);
    EXPECT_EQ(2, ManySendReceiveTestBroadcast_N_USData_confirm_cb_calls);
    EXPECT_EQ(3, ManySendReceiveTestBroadcast_N_USData_indication_cb_calls);

    ASSERT_LT(elapsedTime, TIMEOUT) << "Test took too long: " << elapsedTime << " ms, Timeout was: " << TIMEOUT;

    delete senderISOTP;
    delete receiverISOTP1;
    delete receiverISOTP2;
    delete senderInterface;
    delete receiverInterface1;
    delete receiverInterface2;
}
// END ManySendReceiveTestBroadcast

// SimpleSendReceiveTestMF
constexpr char     SimpleSendReceiveTestMF_message[]     = "01234567890123456789";
constexpr uint32_t SimpleSendReceiveTestMF_messageLength = 21;

static uint32_t SimpleSendReceiveTestMF_N_USData_confirm_cb_calls = 0;
void            SimpleSendReceiveTestMF_N_USData_confirm_cb(N_AI nAi, N_Result nResult, Mtype mtype)
{
    SimpleSendReceiveTestMF_N_USData_confirm_cb_calls++;

    N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 1};
    EXPECT_EQ_N_AI(expectedNAi, nAi);
    EXPECT_EQ(N_OK, nResult);
    EXPECT_EQ(Mtype_Diagnostics, mtype);

    OSInterfaceLogInfo("SimpleSendReceiveTestMF_N_USData_confirm_cb", "SenderKeepRunning set to false");
    senderKeepRunning = false;
}

static uint32_t SimpleSendReceiveTestMF_N_USData_indication_cb_calls = 0;
void SimpleSendReceiveTestMF_N_USData_indication_cb(N_AI nAi, const uint8_t* messageData, uint32_t messageLength,
                                                    N_Result nResult, Mtype mtype)
{
    SimpleSendReceiveTestMF_N_USData_indication_cb_calls++;
    N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 1};
    EXPECT_EQ(N_OK, nResult);
    EXPECT_EQ(Mtype_Diagnostics, mtype);
    EXPECT_EQ_N_AI(expectedNAi, nAi);
    ASSERT_EQ(SimpleSendReceiveTestMF_messageLength, messageLength);
    ASSERT_NE(nullptr, messageData);
    ASSERT_EQ_ARRAY(SimpleSendReceiveTestMF_message, messageData, SimpleSendReceiveTestMF_messageLength);

    OSInterfaceLogInfo("SimpleSendReceiveTestMF_N_USData_indication_cb", "ReceiverKeepRunning set to false");
    receiverKeepRunning = false;
}

static uint32_t SimpleSendReceiveTestMF_N_USData_FF_indication_cb_calls = 0;
void SimpleSendReceiveTestMF_N_USData_FF_indication_cb(const N_AI nAi, const uint32_t messageLength, const Mtype mtype)
{
    SimpleSendReceiveTestMF_N_USData_FF_indication_cb_calls++;
    N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 1};
    EXPECT_EQ_N_AI(expectedNAi, nAi);
    ASSERT_EQ(SimpleSendReceiveTestMF_messageLength, messageLength);
    EXPECT_EQ(Mtype_Diagnostics, mtype);
}

TEST(ISOTP_SystemTests, SimpleSendReceiveTestMF)
{
    constexpr uint32_t TIMEOUT = 10000;
    senderKeepRunning          = true;
    receiverKeepRunning        = true;

    LocalCANNetwork network;
    CANInterface*   senderInterface   = network.newCANInterfaceConnection();
    CANInterface*   receiverInterface = network.newCANInterfaceConnection();
    ISOTP*       senderISOTP =
        new ISOTP(1, 2000, SimpleSendReceiveTestMF_N_USData_confirm_cb,
                     SimpleSendReceiveTestMF_N_USData_indication_cb, SimpleSendReceiveTestMF_N_USData_FF_indication_cb,
                     osInterface, *senderInterface, 2, ISOTP_DefaultSTmin, "senderISOTP");
    ISOTP* receiverISOTP =
        new ISOTP(2, 2000, SimpleSendReceiveTestMF_N_USData_confirm_cb,
                     SimpleSendReceiveTestMF_N_USData_indication_cb, SimpleSendReceiveTestMF_N_USData_FF_indication_cb,
                     osInterface, *receiverInterface, 2, ISOTP_DefaultSTmin, "receiverISOTP");

    uint32_t initialTime = osInterface.osMillis();
    uint32_t step        = 0;
    while ((senderKeepRunning || receiverKeepRunning) && osInterface.osMillis() - initialTime < TIMEOUT)
    {
        senderISOTP->runStep();
        senderISOTP->canMessageACKQueueRunStep();
        receiverISOTP->runStep();
        receiverISOTP->canMessageACKQueueRunStep();

        if (step == 5)
        {
            EXPECT_TRUE(
                senderISOTP->N_USData_request(2, N_TATYPE_5_CAN_CLASSIC_29bit_Physical,
                                                 reinterpret_cast<const uint8_t*>(SimpleSendReceiveTestMF_message),
                                                 SimpleSendReceiveTestMF_messageLength, Mtype_Diagnostics));
        }
        step++;
    }
    uint32_t elapsedTime = osInterface.osMillis() - initialTime;

    EXPECT_EQ(1, SimpleSendReceiveTestMF_N_USData_FF_indication_cb_calls);
    EXPECT_EQ(1, SimpleSendReceiveTestMF_N_USData_confirm_cb_calls);
    EXPECT_EQ(1, SimpleSendReceiveTestMF_N_USData_indication_cb_calls);

    ASSERT_LT(elapsedTime, TIMEOUT) << "Test took too long: " << elapsedTime << " ms, Timeout was: " << TIMEOUT;

    delete senderISOTP;
    delete receiverISOTP;
    delete senderInterface;
    delete receiverInterface;
}
// END SimpleSendReceiveTestMF

// ManySendReceiveTestMF
constexpr char     ManySendReceiveTestMF_message1[]     = "01234567890123456789";
constexpr uint32_t ManySendReceiveTestMF_messageLength1 = 21;
constexpr char     ManySendReceiveTestMF_message2[]     = "98765432109876543210";
constexpr uint32_t ManySendReceiveTestMF_messageLength2 = 21;

static uint32_t ManySendReceiveTestMF_N_USData_confirm_cb_calls = 0;
void            ManySendReceiveTestMF_N_USData_confirm_cb(N_AI nAi, N_Result nResult, Mtype mtype)
{
    ManySendReceiveTestMF_N_USData_confirm_cb_calls++;

    N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 1};
    EXPECT_EQ_N_AI(expectedNAi, nAi);
    EXPECT_EQ(N_OK, nResult);
    EXPECT_EQ(Mtype_Diagnostics, mtype);

    if (ManySendReceiveTestMF_N_USData_confirm_cb_calls == 1)
    {
        OSInterfaceLogInfo("ManySendReceiveTestMF_N_USData_confirm_cb", "First call");
    }
    else if (ManySendReceiveTestMF_N_USData_confirm_cb_calls == 2)
    {
        OSInterfaceLogInfo("ManySendReceiveTestMF_N_USData_confirm_cb", "SenderKeepRunning set to false");
        senderKeepRunning = false;
    }
}

static uint32_t ManySendReceiveTestMF_N_USData_indication_cb_calls = 0;
void ManySendReceiveTestMF_N_USData_indication_cb(N_AI nAi, const uint8_t* messageData, uint32_t messageLength,
                                                  N_Result nResult, Mtype mtype)
{
    ManySendReceiveTestMF_N_USData_indication_cb_calls++;
    N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 1};

    if (ManySendReceiveTestMF_N_USData_indication_cb_calls == 1)
    {
        EXPECT_EQ(N_OK, nResult);
        EXPECT_EQ(Mtype_Diagnostics, mtype);
        EXPECT_EQ_N_AI(expectedNAi, nAi);
        ASSERT_EQ(ManySendReceiveTestMF_messageLength1, messageLength);
        ASSERT_NE(nullptr, messageData);
        ASSERT_EQ_ARRAY(ManySendReceiveTestMF_message1, messageData, ManySendReceiveTestMF_messageLength1);

        OSInterfaceLogInfo("ManySendReceiveTestMF_N_USData_indication_cb", "First call");
    }
    else if (ManySendReceiveTestMF_N_USData_indication_cb_calls == 2)
    {
        EXPECT_EQ(N_OK, nResult);
        EXPECT_EQ(Mtype_Diagnostics, mtype);
        EXPECT_EQ_N_AI(expectedNAi, nAi);
        ASSERT_EQ(ManySendReceiveTestMF_messageLength2, messageLength);
        ASSERT_NE(nullptr, messageData);
        ASSERT_EQ_ARRAY(ManySendReceiveTestMF_message2, messageData, ManySendReceiveTestMF_messageLength2);

        OSInterfaceLogInfo("ManySendReceiveTestMF_N_USData_indication_cb", "ReceiverKeepRunning set to false");
        receiverKeepRunning = false;
    }
}

static uint32_t ManySendReceiveTestMF_N_USData_FF_indication_cb_calls = 0;
void ManySendReceiveTestMF_N_USData_FF_indication_cb(const N_AI nAi, const uint32_t messageLength, const Mtype mtype)
{
    ManySendReceiveTestMF_N_USData_FF_indication_cb_calls++;
    N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 1};
    if (ManySendReceiveTestMF_N_USData_FF_indication_cb_calls == 1)
    {
        EXPECT_EQ_N_AI(expectedNAi, nAi);
        ASSERT_EQ(ManySendReceiveTestMF_messageLength1, messageLength);
        EXPECT_EQ(Mtype_Diagnostics, mtype);

        OSInterfaceLogInfo("ManySendReceiveTestMF_N_USData_FF_indication_cb", "First call");
    }
    else if (ManySendReceiveTestMF_N_USData_FF_indication_cb_calls == 2)
    {
        EXPECT_EQ_N_AI(expectedNAi, nAi);
        ASSERT_EQ(ManySendReceiveTestMF_messageLength2, messageLength);
        EXPECT_EQ(Mtype_Diagnostics, mtype);

        OSInterfaceLogInfo("ManySendReceiveTestMF_N_USData_FF_indication_cb", "Second call");
    }
}

TEST(ISOTP_SystemTests, ManySendReceiveTestMF)
{
    constexpr uint32_t TIMEOUT = 10000;
    senderKeepRunning          = true;
    receiverKeepRunning        = true;

    LocalCANNetwork network;
    CANInterface*   senderInterface   = network.newCANInterfaceConnection();
    CANInterface*   receiverInterface = network.newCANInterfaceConnection();
    ISOTP*       senderISOTP =
        new ISOTP(1, 2000, ManySendReceiveTestMF_N_USData_confirm_cb, ManySendReceiveTestMF_N_USData_indication_cb,
                     ManySendReceiveTestMF_N_USData_FF_indication_cb, osInterface, *senderInterface, 2,
                     ISOTP_DefaultSTmin, "senderISOTP");
    ISOTP* receiverISOTP =
        new ISOTP(2, 2000, ManySendReceiveTestMF_N_USData_confirm_cb, ManySendReceiveTestMF_N_USData_indication_cb,
                     ManySendReceiveTestMF_N_USData_FF_indication_cb, osInterface, *receiverInterface, 2,
                     ISOTP_DefaultSTmin, "receiverISOTP");

    uint32_t initialTime = osInterface.osMillis();
    uint32_t step        = 0;
    while ((senderKeepRunning || receiverKeepRunning) && osInterface.osMillis() - initialTime < TIMEOUT)
    {
        senderISOTP->runStep();
        senderISOTP->canMessageACKQueueRunStep();
        receiverISOTP->runStep();
        receiverISOTP->canMessageACKQueueRunStep();

        if (step == 5)
        {
            EXPECT_TRUE(
                senderISOTP->N_USData_request(2, N_TATYPE_5_CAN_CLASSIC_29bit_Physical,
                                                 reinterpret_cast<const uint8_t*>(ManySendReceiveTestMF_message1),
                                                 ManySendReceiveTestMF_messageLength1, Mtype_Diagnostics));
            EXPECT_TRUE(
                senderISOTP->N_USData_request(2, N_TATYPE_5_CAN_CLASSIC_29bit_Physical,
                                                 reinterpret_cast<const uint8_t*>(ManySendReceiveTestMF_message2),
                                                 ManySendReceiveTestMF_messageLength2, Mtype_Diagnostics));
        }
        step++;
    }
    uint32_t elapsedTime = osInterface.osMillis() - initialTime;

    EXPECT_EQ(2, ManySendReceiveTestMF_N_USData_FF_indication_cb_calls);
    EXPECT_EQ(2, ManySendReceiveTestMF_N_USData_confirm_cb_calls);
    EXPECT_EQ(2, ManySendReceiveTestMF_N_USData_indication_cb_calls);

    ASSERT_LT(elapsedTime, TIMEOUT) << "Test took too long: " << elapsedTime << " ms, Timeout was: " << TIMEOUT;

    delete senderISOTP;
    delete receiverISOTP;
    delete senderInterface;
    delete receiverInterface;
}
// END ManySendReceiveTestMF

// NullCharSendReceiveTestSF
constexpr char     NullCharSendReceiveTestSF_message[]     = "pa"
                                                             "\0"
                                                             "a"
                                                             "\0"
                                                             "a";
constexpr uint32_t NullCharSendReceiveTestSF_messageLength = 7;

static uint32_t NullCharSendReceiveTestSF_N_USData_confirm_cb_calls = 0;
void            NullCharSendReceiveTestSF_N_USData_confirm_cb(N_AI nAi, N_Result nResult, Mtype mtype)
{
    NullCharSendReceiveTestSF_N_USData_confirm_cb_calls++;

    N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 1};
    EXPECT_EQ_N_AI(expectedNAi, nAi);
    EXPECT_EQ(N_OK, nResult);
    EXPECT_EQ(Mtype_Diagnostics, mtype);

    OSInterfaceLogInfo("NullCharSendReceiveTestSF_N_USData_confirm_cb", "SenderKeepRunning set to false");
    senderKeepRunning = false;
}

static uint32_t NullCharSendReceiveTestSF_N_USData_indication_cb_calls = 0;
void NullCharSendReceiveTestSF_N_USData_indication_cb(N_AI nAi, const uint8_t* messageData, uint32_t messageLength,
                                                      N_Result nResult, Mtype mtype)
{
    NullCharSendReceiveTestSF_N_USData_indication_cb_calls++;
    N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 1};
    EXPECT_EQ(N_OK, nResult);
    EXPECT_EQ(Mtype_Diagnostics, mtype);
    EXPECT_EQ_N_AI(expectedNAi, nAi);
    ASSERT_EQ(NullCharSendReceiveTestSF_messageLength, messageLength);
    ASSERT_NE(nullptr, messageData);
    EXPECT_EQ_ARRAY(NullCharSendReceiveTestSF_message, messageData, NullCharSendReceiveTestSF_messageLength);

    OSInterfaceLogInfo("NullCharSendReceiveTestSF_N_USData_indication_cb", "ReceiverKeepRunning set to false");
    receiverKeepRunning = false;
}

static uint32_t NullCharSendReceiveTestSF_N_USData_FF_indication_cb_calls = 0;
void            NullCharSendReceiveTestSF_N_USData_FF_indication_cb(const N_AI nAi, const uint32_t messageLength,
                                                                    const Mtype mtype)
{
    NullCharSendReceiveTestSF_N_USData_FF_indication_cb_calls++;
}

TEST(ISOTP_SystemTests, NullCharSendReceiveTestSF)
{
    constexpr uint32_t TIMEOUT = 10000; // 10 seconds
    senderKeepRunning          = true;
    receiverKeepRunning        = true;

    LocalCANNetwork network;
    CANInterface*   senderInterface   = network.newCANInterfaceConnection("senderInterface");
    CANInterface*   receiverInterface = network.newCANInterfaceConnection("receiverInterface");
    ISOTP*       senderISOTP    = new ISOTP(1, 2000, NullCharSendReceiveTestSF_N_USData_confirm_cb,
                                                     NullCharSendReceiveTestSF_N_USData_indication_cb,
                                                     NullCharSendReceiveTestSF_N_USData_FF_indication_cb, osInterface,
                                                     *senderInterface, 2, ISOTP_DefaultSTmin, "senderISOTP");
    ISOTP*       receiverISOTP  = new ISOTP(2, 2000, NullCharSendReceiveTestSF_N_USData_confirm_cb,
                                                     NullCharSendReceiveTestSF_N_USData_indication_cb,
                                                     NullCharSendReceiveTestSF_N_USData_FF_indication_cb, osInterface,
                                                     *receiverInterface, 2, ISOTP_DefaultSTmin, "receiverISOTP");

    uint32_t initialTime = osInterface.osMillis();
    uint32_t step        = 0;
    while ((senderKeepRunning || receiverKeepRunning) && osInterface.osMillis() - initialTime < TIMEOUT)
    {
        senderISOTP->runStep();
        senderISOTP->canMessageACKQueueRunStep();
        receiverISOTP->runStep();
        receiverISOTP->canMessageACKQueueRunStep();

        if (step == 5)
        {
            EXPECT_TRUE(
                senderISOTP->N_USData_request(2, N_TATYPE_5_CAN_CLASSIC_29bit_Physical,
                                                 reinterpret_cast<const uint8_t*>(NullCharSendReceiveTestSF_message),
                                                 NullCharSendReceiveTestSF_messageLength, Mtype_Diagnostics));
        }

        step++;
    }
    uint32_t elapsedTime = osInterface.osMillis() - initialTime;

    EXPECT_EQ(0, NullCharSendReceiveTestSF_N_USData_FF_indication_cb_calls);
    EXPECT_EQ(1, NullCharSendReceiveTestSF_N_USData_confirm_cb_calls);
    EXPECT_EQ(1, NullCharSendReceiveTestSF_N_USData_indication_cb_calls);

    ASSERT_LT(elapsedTime, TIMEOUT) << "Test took too long: " << elapsedTime << " ms, Timeout was: " << TIMEOUT;

    delete senderISOTP;
    delete receiverISOTP;
    delete senderInterface;
    delete receiverInterface;
}
// END NullCharSendReceiveTestSF

// NullCharSendReceiveTestMF
constexpr char     NullCharSendReceiveTestMF_message[]     = "01"
                                                             "\0"
                                                             "345678901234567"
                                                             "\0"
                                                             "9";
constexpr uint32_t NullCharSendReceiveTestMF_messageLength = 21;

static uint32_t NullCharSendReceiveTestMF_N_USData_confirm_cb_calls = 0;
void            NullCharSendReceiveTestMF_N_USData_confirm_cb(N_AI nAi, N_Result nResult, Mtype mtype)
{
    NullCharSendReceiveTestMF_N_USData_confirm_cb_calls++;

    N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 1};
    EXPECT_EQ_N_AI(expectedNAi, nAi);
    EXPECT_EQ(N_OK, nResult);
    EXPECT_EQ(Mtype_Diagnostics, mtype);

    OSInterfaceLogInfo("NullCharSendReceiveTestMF_N_USData_confirm_cb", "SenderKeepRunning set to false");
    senderKeepRunning = false;
}

static uint32_t NullCharSendReceiveTestMF_N_USData_indication_cb_calls = 0;
void NullCharSendReceiveTestMF_N_USData_indication_cb(N_AI nAi, const uint8_t* messageData, uint32_t messageLength,
                                                      N_Result nResult, Mtype mtype)
{
    NullCharSendReceiveTestMF_N_USData_indication_cb_calls++;
    N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 1};
    EXPECT_EQ(N_OK, nResult);
    EXPECT_EQ(Mtype_Diagnostics, mtype);
    EXPECT_EQ_N_AI(expectedNAi, nAi);
    ASSERT_EQ(NullCharSendReceiveTestMF_messageLength, messageLength);
    ASSERT_NE(nullptr, messageData);
    ASSERT_EQ_ARRAY(NullCharSendReceiveTestMF_message, messageData, NullCharSendReceiveTestMF_messageLength);

    OSInterfaceLogInfo("NullCharSendReceiveTestMF_N_USData_indication_cb", "ReceiverKeepRunning set to false");
    receiverKeepRunning = false;
}

static uint32_t NullCharSendReceiveTestMF_N_USData_FF_indication_cb_calls = 0;
void            NullCharSendReceiveTestMF_N_USData_FF_indication_cb(const N_AI nAi, const uint32_t messageLength,
                                                                    const Mtype mtype)
{
    NullCharSendReceiveTestMF_N_USData_FF_indication_cb_calls++;
    N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 1};
    EXPECT_EQ_N_AI(expectedNAi, nAi);
    ASSERT_EQ(NullCharSendReceiveTestMF_messageLength, messageLength);
    EXPECT_EQ(Mtype_Diagnostics, mtype);
}

TEST(ISOTP_SystemTests, NullCharSendReceiveTestMF)
{
    constexpr uint32_t TIMEOUT = 10000;
    senderKeepRunning          = true;
    receiverKeepRunning        = true;

    LocalCANNetwork network;
    CANInterface*   senderInterface   = network.newCANInterfaceConnection();
    CANInterface*   receiverInterface = network.newCANInterfaceConnection();
    ISOTP*       senderISOTP    = new ISOTP(1, 2000, NullCharSendReceiveTestMF_N_USData_confirm_cb,
                                                     NullCharSendReceiveTestMF_N_USData_indication_cb,
                                                     NullCharSendReceiveTestMF_N_USData_FF_indication_cb, osInterface,
                                                     *senderInterface, 2, ISOTP_DefaultSTmin, "senderISOTP");
    ISOTP*       receiverISOTP  = new ISOTP(2, 2000, NullCharSendReceiveTestMF_N_USData_confirm_cb,
                                                     NullCharSendReceiveTestMF_N_USData_indication_cb,
                                                     NullCharSendReceiveTestMF_N_USData_FF_indication_cb, osInterface,
                                                     *receiverInterface, 2, ISOTP_DefaultSTmin, "receiverISOTP");

    uint32_t initialTime = osInterface.osMillis();
    uint32_t step        = 0;
    while ((senderKeepRunning || receiverKeepRunning) && osInterface.osMillis() - initialTime < TIMEOUT)
    {
        senderISOTP->runStep();
        senderISOTP->canMessageACKQueueRunStep();
        receiverISOTP->runStep();
        receiverISOTP->canMessageACKQueueRunStep();

        if (step == 5)
        {
            EXPECT_TRUE(
                senderISOTP->N_USData_request(2, N_TATYPE_5_CAN_CLASSIC_29bit_Physical,
                                                 reinterpret_cast<const uint8_t*>(NullCharSendReceiveTestMF_message),
                                                 NullCharSendReceiveTestMF_messageLength, Mtype_Diagnostics));
        }
        step++;
    }
    uint32_t elapsedTime = osInterface.osMillis() - initialTime;

    EXPECT_EQ(1, NullCharSendReceiveTestMF_N_USData_FF_indication_cb_calls);
    EXPECT_EQ(1, NullCharSendReceiveTestMF_N_USData_confirm_cb_calls);
    EXPECT_EQ(1, NullCharSendReceiveTestMF_N_USData_indication_cb_calls);

    ASSERT_LT(elapsedTime, TIMEOUT) << "Test took too long: " << elapsedTime << " ms, Timeout was: " << TIMEOUT;

    delete senderISOTP;
    delete receiverISOTP;
    delete senderInterface;
    delete receiverInterface;
}
// END NullCharSendReceiveTestMF

// LowMemorySenderTestSF
constexpr char     LowMemorySenderTestSF_message[]     = "patata";
constexpr uint32_t LowMemorySenderTestSF_messageLength = 7;

static uint32_t LowMemorySenderTestSF_N_USData_confirm_cb_calls = 0;
void            LowMemorySenderTestSF_N_USData_confirm_cb(N_AI nAi, N_Result nResult, Mtype mtype)
{
    LowMemorySenderTestSF_N_USData_confirm_cb_calls++;

    N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 1};
    EXPECT_EQ_N_AI(expectedNAi, nAi);
    EXPECT_EQ(N_OK, nResult);
    EXPECT_EQ(Mtype_Diagnostics, mtype);

    OSInterfaceLogInfo("LowMemorySenderTestSF_N_USData_confirm_cb", "SenderKeepRunning set to false");
    senderKeepRunning = false;
}

static uint32_t LowMemorySenderTestSF_N_USData_indication_cb_calls = 0;
void LowMemorySenderTestSF_N_USData_indication_cb(N_AI nAi, const uint8_t* messageData, uint32_t messageLength,
                                                  N_Result nResult, Mtype mtype)
{
    LowMemorySenderTestSF_N_USData_indication_cb_calls++;
    N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 1};
    EXPECT_EQ(N_OK, nResult);
    EXPECT_EQ(Mtype_Diagnostics, mtype);
    EXPECT_EQ_N_AI(expectedNAi, nAi);
    ASSERT_EQ(LowMemorySenderTestSF_messageLength, messageLength);
    ASSERT_NE(nullptr, messageData);
    EXPECT_EQ_ARRAY(LowMemorySenderTestSF_message, messageData, LowMemorySenderTestSF_messageLength);

    OSInterfaceLogInfo("LowMemorySenderTestSF_N_USData_indication_cb", "ReceiverKeepRunning set to false");
    receiverKeepRunning = false;
}

static uint32_t LowMemorySenderTestSF_N_USData_FF_indication_cb_calls = 0;
void LowMemorySenderTestSF_N_USData_FF_indication_cb(const N_AI nAi, const uint32_t messageLength, const Mtype mtype)
{
    LowMemorySenderTestSF_N_USData_FF_indication_cb_calls++;
}

TEST(ISOTP_SystemTests, LowMemorySenderTestSF)
{
    constexpr uint32_t TIMEOUT = 10000; // 10 seconds
    senderKeepRunning          = true;
    receiverKeepRunning        = true;

    LocalCANNetwork network;
    CANInterface*   senderInterface   = network.newCANInterfaceConnection("senderInterface");
    CANInterface*   receiverInterface = network.newCANInterfaceConnection("receiverInterface");
    ISOTP*       senderISOTP =
        new ISOTP(1, 100, LowMemorySenderTestSF_N_USData_confirm_cb, LowMemorySenderTestSF_N_USData_indication_cb,
                     LowMemorySenderTestSF_N_USData_FF_indication_cb, osInterface, *senderInterface, 2,
                     ISOTP_DefaultSTmin, "senderISOTP");
    ISOTP* receiverISOTP =
        new ISOTP(2, 2000, LowMemorySenderTestSF_N_USData_confirm_cb, LowMemorySenderTestSF_N_USData_indication_cb,
                     LowMemorySenderTestSF_N_USData_FF_indication_cb, osInterface, *receiverInterface, 2,
                     ISOTP_DefaultSTmin, "receiverISOTP");

    uint32_t initialTime = osInterface.osMillis();
    uint32_t step        = 0;
    while ((senderKeepRunning || receiverKeepRunning) && osInterface.osMillis() - initialTime < TIMEOUT)
    {
        senderISOTP->runStep();
        senderISOTP->canMessageACKQueueRunStep();
        receiverISOTP->runStep();
        receiverISOTP->canMessageACKQueueRunStep();

        if (step == 5)
        {
            EXPECT_FALSE(
                senderISOTP->N_USData_request(2, N_TATYPE_5_CAN_CLASSIC_29bit_Physical,
                                                 reinterpret_cast<const uint8_t*>(LowMemorySenderTestSF_message),
                                                 LowMemorySenderTestSF_messageLength, Mtype_Diagnostics));
        }
        if (step == 10)
        {
            senderKeepRunning   = false;
            receiverKeepRunning = false;
        }

        step++;
    }
    uint32_t elapsedTime = osInterface.osMillis() - initialTime;

    EXPECT_EQ(0, LowMemorySenderTestSF_N_USData_FF_indication_cb_calls);
    EXPECT_EQ(0, LowMemorySenderTestSF_N_USData_confirm_cb_calls);
    EXPECT_EQ(0, LowMemorySenderTestSF_N_USData_indication_cb_calls);

    ASSERT_LT(elapsedTime, TIMEOUT) << "Test took too long: " << elapsedTime << " ms, Timeout was: " << TIMEOUT;

    delete senderISOTP;
    delete receiverISOTP;
    delete senderInterface;
    delete receiverInterface;
}
// END LowMemorySenderTestSF

// LowMemoryReceiverTestSF
constexpr char     LowMemoryReceiverTestSF_message[]     = "patata";
constexpr uint32_t LowMemoryReceiverTestSF_messageLength = 7;

static uint32_t LowMemoryReceiverTestSF_N_USData_confirm_cb_calls = 0;
void            LowMemoryReceiverTestSF_N_USData_confirm_cb(N_AI nAi, N_Result nResult, Mtype mtype)
{
    LowMemoryReceiverTestSF_N_USData_confirm_cb_calls++;

    N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 1};
    EXPECT_EQ_N_AI(expectedNAi, nAi);
    EXPECT_EQ(N_OK, nResult);
    EXPECT_EQ(Mtype_Diagnostics, mtype);

    OSInterfaceLogInfo("LowMemoryReceiverTestSF_N_USData_confirm_cb", "SenderKeepRunning set to false");
    senderKeepRunning = false;
}

static uint32_t LowMemoryReceiverTestSF_N_USData_indication_cb_calls = 0;
void LowMemoryReceiverTestSF_N_USData_indication_cb(N_AI nAi, const uint8_t* messageData, uint32_t messageLength,
                                                    N_Result nResult, Mtype mtype)
{
    LowMemoryReceiverTestSF_N_USData_indication_cb_calls++;
    N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 1};
    EXPECT_EQ(N_ERROR, nResult);
    EXPECT_EQ(Mtype_Diagnostics, mtype);
    EXPECT_EQ_N_AI(expectedNAi, nAi);
    ASSERT_EQ(LowMemoryReceiverTestSF_messageLength, messageLength);
    ASSERT_EQ(nullptr, messageData);

    OSInterfaceLogInfo("LowMemoryReceiverTestSF_N_USData_indication_cb", "ReceiverKeepRunning set to false");
    receiverKeepRunning = false;
}

static uint32_t LowMemoryReceiverTestSF_N_USData_FF_indication_cb_calls = 0;
void LowMemoryReceiverTestSF_N_USData_FF_indication_cb(const N_AI nAi, const uint32_t messageLength, const Mtype mtype)
{
    LowMemoryReceiverTestSF_N_USData_FF_indication_cb_calls++;
}

TEST(ISOTP_SystemTests, LowMemoryReceiverTestSF)
{
    constexpr uint32_t TIMEOUT = 10000; // 10 seconds
    senderKeepRunning          = true;
    receiverKeepRunning        = true;

    LocalCANNetwork network;
    CANInterface*   senderInterface   = network.newCANInterfaceConnection("senderInterface");
    CANInterface*   receiverInterface = network.newCANInterfaceConnection("receiverInterface");
    ISOTP*       senderISOTP =
        new ISOTP(1, 2000, LowMemoryReceiverTestSF_N_USData_confirm_cb,
                     LowMemoryReceiverTestSF_N_USData_indication_cb, LowMemoryReceiverTestSF_N_USData_FF_indication_cb,
                     osInterface, *senderInterface, 2, ISOTP_DefaultSTmin, "senderISOTP");
    ISOTP* receiverISOTP =
        new ISOTP(2, 100, LowMemoryReceiverTestSF_N_USData_confirm_cb,
                     LowMemoryReceiverTestSF_N_USData_indication_cb, LowMemoryReceiverTestSF_N_USData_FF_indication_cb,
                     osInterface, *receiverInterface, 2, ISOTP_DefaultSTmin, "receiverISOTP");

    uint32_t initialTime = osInterface.osMillis();
    uint32_t step        = 0;
    while ((senderKeepRunning || receiverKeepRunning) && osInterface.osMillis() - initialTime < TIMEOUT)
    {
        senderISOTP->runStep();
        senderISOTP->canMessageACKQueueRunStep();
        receiverISOTP->runStep();
        receiverISOTP->canMessageACKQueueRunStep();

        if (step == 5)
        {
            EXPECT_TRUE(
                senderISOTP->N_USData_request(2, N_TATYPE_5_CAN_CLASSIC_29bit_Physical,
                                                 reinterpret_cast<const uint8_t*>(LowMemoryReceiverTestSF_message),
                                                 LowMemoryReceiverTestSF_messageLength, Mtype_Diagnostics));
        }

        step++;
    }
    uint32_t elapsedTime = osInterface.osMillis() - initialTime;

    EXPECT_EQ(0, LowMemoryReceiverTestSF_N_USData_FF_indication_cb_calls);
    EXPECT_EQ(1, LowMemoryReceiverTestSF_N_USData_confirm_cb_calls);
    EXPECT_EQ(1, LowMemoryReceiverTestSF_N_USData_indication_cb_calls);

    ASSERT_LT(elapsedTime, TIMEOUT) << "Test took too long: " << elapsedTime << " ms, Timeout was: " << TIMEOUT;

    delete senderISOTP;
    delete receiverISOTP;
    delete senderInterface;
    delete receiverInterface;
}
// END LowMemoryReceiverTestSF

// LowMemorySenderTestMF
constexpr char     LowMemorySenderTestMF_message[]     = "01234567890123456789";
constexpr uint32_t LowMemorySenderTestMF_messageLength = 21;

static uint32_t LowMemorySenderTestMF_N_USData_confirm_cb_calls = 0;
void            LowMemorySenderTestMF_N_USData_confirm_cb(N_AI nAi, N_Result nResult, Mtype mtype)
{
    LowMemorySenderTestMF_N_USData_confirm_cb_calls++;

    N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 1};
    EXPECT_EQ_N_AI(expectedNAi, nAi);
    EXPECT_EQ(N_OK, nResult);
    EXPECT_EQ(Mtype_Diagnostics, mtype);

    OSInterfaceLogInfo("LowMemorySenderTestMF_N_USData_confirm_cb", "SenderKeepRunning set to false");
    senderKeepRunning = false;
}

static uint32_t LowMemorySenderTestMF_N_USData_indication_cb_calls = 0;
void LowMemorySenderTestMF_N_USData_indication_cb(N_AI nAi, const uint8_t* messageData, uint32_t messageLength,
                                                  N_Result nResult, Mtype mtype)
{
    LowMemorySenderTestMF_N_USData_indication_cb_calls++;
    N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 1};
    EXPECT_EQ(N_OK, nResult);
    EXPECT_EQ(Mtype_Diagnostics, mtype);
    EXPECT_EQ_N_AI(expectedNAi, nAi);
    ASSERT_EQ(LowMemorySenderTestMF_messageLength, messageLength);
    ASSERT_NE(nullptr, messageData);
    ASSERT_EQ_ARRAY(LowMemorySenderTestMF_message, messageData, LowMemorySenderTestMF_messageLength);

    OSInterfaceLogInfo("LowMemorySenderTestMF_N_USData_indication_cb", "ReceiverKeepRunning set to false");
    receiverKeepRunning = false;
}

static uint32_t LowMemorySenderTestMF_N_USData_FF_indication_cb_calls = 0;
void LowMemorySenderTestMF_N_USData_FF_indication_cb(const N_AI nAi, const uint32_t messageLength, const Mtype mtype)
{
    LowMemorySenderTestMF_N_USData_FF_indication_cb_calls++;
    N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 1};
    EXPECT_EQ_N_AI(expectedNAi, nAi);
    ASSERT_EQ(LowMemorySenderTestMF_messageLength, messageLength);
    EXPECT_EQ(Mtype_Diagnostics, mtype);
}

TEST(ISOTP_SystemTests, LowMemorySenderTestMF)
{
    constexpr uint32_t TIMEOUT = 10000;
    senderKeepRunning          = true;
    receiverKeepRunning        = true;

    LocalCANNetwork network;
    CANInterface*   senderInterface   = network.newCANInterfaceConnection();
    CANInterface*   receiverInterface = network.newCANInterfaceConnection();
    ISOTP*       senderISOTP =
        new ISOTP(1, 100, LowMemorySenderTestMF_N_USData_confirm_cb, LowMemorySenderTestMF_N_USData_indication_cb,
                     LowMemorySenderTestMF_N_USData_FF_indication_cb, osInterface, *senderInterface, 2,
                     ISOTP_DefaultSTmin, "senderISOTP");
    ISOTP* receiverISOTP =
        new ISOTP(2, 2000, LowMemorySenderTestMF_N_USData_confirm_cb, LowMemorySenderTestMF_N_USData_indication_cb,
                     LowMemorySenderTestMF_N_USData_FF_indication_cb, osInterface, *receiverInterface, 2,
                     ISOTP_DefaultSTmin, "receiverISOTP");

    uint32_t initialTime = osInterface.osMillis();
    uint32_t step        = 0;
    while ((senderKeepRunning || receiverKeepRunning) && osInterface.osMillis() - initialTime < TIMEOUT)
    {
        senderISOTP->runStep();
        senderISOTP->canMessageACKQueueRunStep();
        receiverISOTP->runStep();
        receiverISOTP->canMessageACKQueueRunStep();

        if (step == 5)
        {
            EXPECT_FALSE(
                senderISOTP->N_USData_request(2, N_TATYPE_5_CAN_CLASSIC_29bit_Physical,
                                                 reinterpret_cast<const uint8_t*>(LowMemorySenderTestMF_message),
                                                 LowMemorySenderTestMF_messageLength, Mtype_Diagnostics));
        }

        if (step == 10)
        {
            senderKeepRunning   = false;
            receiverKeepRunning = false;
        }
        step++;
    }
    uint32_t elapsedTime = osInterface.osMillis() - initialTime;

    EXPECT_EQ(0, LowMemorySenderTestMF_N_USData_FF_indication_cb_calls);
    EXPECT_EQ(0, LowMemorySenderTestMF_N_USData_confirm_cb_calls);
    EXPECT_EQ(0, LowMemorySenderTestMF_N_USData_indication_cb_calls);

    ASSERT_LT(elapsedTime, TIMEOUT) << "Test took too long: " << elapsedTime << " ms, Timeout was: " << TIMEOUT;

    delete senderISOTP;
    delete receiverISOTP;
    delete senderInterface;
    delete receiverInterface;
}
// END LowMemorySenderTestMF

// LowMemoryReceiverTestMF
constexpr char     LowMemoryReceiverTestMF_message[]     = "01234567890123456789";
constexpr uint32_t LowMemoryReceiverTestMF_messageLength = 21;

static uint32_t LowMemoryReceiverTestMF_N_USData_confirm_cb_calls = 0;
void            LowMemoryReceiverTestMF_N_USData_confirm_cb(N_AI nAi, N_Result nResult, Mtype mtype)
{
    LowMemoryReceiverTestMF_N_USData_confirm_cb_calls++;

    N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 1};
    EXPECT_EQ_N_AI(expectedNAi, nAi);
    EXPECT_EQ(N_BUFFER_OVFLW, nResult);
    EXPECT_EQ(Mtype_Diagnostics, mtype);

    OSInterfaceLogInfo("LowMemoryReceiverTestMF_N_USData_confirm_cb", "SenderKeepRunning set to false");
    senderKeepRunning = false;
}

static uint32_t LowMemoryReceiverTestMF_N_USData_indication_cb_calls = 0;
void LowMemoryReceiverTestMF_N_USData_indication_cb(N_AI nAi, const uint8_t* messageData, uint32_t messageLength,
                                                    N_Result nResult, Mtype mtype)
{
    LowMemoryReceiverTestMF_N_USData_indication_cb_calls++;
    N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 1};
    EXPECT_EQ(N_ERROR, nResult);
    EXPECT_EQ(Mtype_Diagnostics, mtype);
    EXPECT_EQ_N_AI(expectedNAi, nAi);
    ASSERT_EQ(LowMemoryReceiverTestMF_messageLength, messageLength);
    ASSERT_EQ(nullptr, messageData);

    OSInterfaceLogInfo("LowMemoryReceiverTestMF_N_USData_indication_cb", "ReceiverKeepRunning set to false");
    receiverKeepRunning = false;
}

static uint32_t LowMemoryReceiverTestMF_N_USData_FF_indication_cb_calls = 0;
void LowMemoryReceiverTestMF_N_USData_FF_indication_cb(const N_AI nAi, const uint32_t messageLength, const Mtype mtype)
{
    LowMemoryReceiverTestMF_N_USData_FF_indication_cb_calls++;
    N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 1};
    EXPECT_EQ_N_AI(expectedNAi, nAi);
    ASSERT_EQ(LowMemoryReceiverTestMF_messageLength, messageLength);
    EXPECT_EQ(Mtype_Diagnostics, mtype);
}

TEST(ISOTP_SystemTests, LowMemoryReceiverTestMF)
{
    constexpr uint32_t TIMEOUT = 10000;
    senderKeepRunning          = true;
    receiverKeepRunning        = true;

    LocalCANNetwork network;
    CANInterface*   senderInterface   = network.newCANInterfaceConnection();
    CANInterface*   receiverInterface = network.newCANInterfaceConnection();
    ISOTP*       senderISOTP =
        new ISOTP(1, 2000, LowMemoryReceiverTestMF_N_USData_confirm_cb,
                     LowMemoryReceiverTestMF_N_USData_indication_cb, LowMemoryReceiverTestMF_N_USData_FF_indication_cb,
                     osInterface, *senderInterface, 2, ISOTP_DefaultSTmin, "senderISOTP");
    ISOTP* receiverISOTP =
        new ISOTP(2, 100, LowMemoryReceiverTestMF_N_USData_confirm_cb,
                     LowMemoryReceiverTestMF_N_USData_indication_cb, LowMemoryReceiverTestMF_N_USData_FF_indication_cb,
                     osInterface, *receiverInterface, 2, ISOTP_DefaultSTmin, "receiverISOTP");

    uint32_t initialTime = osInterface.osMillis();
    uint32_t step        = 0;
    while ((senderKeepRunning || receiverKeepRunning) && osInterface.osMillis() - initialTime < TIMEOUT)
    {
        senderISOTP->runStep();
        senderISOTP->canMessageACKQueueRunStep();
        receiverISOTP->runStep();
        receiverISOTP->canMessageACKQueueRunStep();

        if (step == 5)
        {
            EXPECT_TRUE(
                senderISOTP->N_USData_request(2, N_TATYPE_5_CAN_CLASSIC_29bit_Physical,
                                                 reinterpret_cast<const uint8_t*>(LowMemoryReceiverTestMF_message),
                                                 LowMemoryReceiverTestMF_messageLength, Mtype_Diagnostics));
        }
        step++;
    }
    uint32_t elapsedTime = osInterface.osMillis() - initialTime;

    EXPECT_EQ(0, LowMemoryReceiverTestMF_N_USData_FF_indication_cb_calls);
    EXPECT_EQ(1, LowMemoryReceiverTestMF_N_USData_confirm_cb_calls);
    EXPECT_EQ(1, LowMemoryReceiverTestMF_N_USData_indication_cb_calls);

    ASSERT_LT(elapsedTime, TIMEOUT) << "Test took too long: " << elapsedTime << " ms, Timeout was: " << TIMEOUT;

    delete senderISOTP;
    delete receiverISOTP;
    delete senderInterface;
    delete receiverInterface;
}
// END LowMemoryReceiverTestMF

// ManySenderToOneTargetSF
constexpr char     ManySenderToOneTargetSF_message1[]     = "patata";
constexpr uint32_t ManySenderToOneTargetSF_messageLength1 = 7;
constexpr char     ManySenderToOneTargetSF_message2[]     = "cocida";
constexpr uint32_t ManySenderToOneTargetSF_messageLength2 = 7;

static uint32_t ManySenderToOneTargetSF_N_USData_confirm_cb_calls = 0;
void            ManySenderToOneTargetSF_N_USData_confirm_cb(N_AI nAi, N_Result nResult, Mtype mtype)
{
    ManySenderToOneTargetSF_N_USData_confirm_cb_calls++;

    if (ManySenderToOneTargetSF_N_USData_confirm_cb_calls == 1)
    {
        OSInterfaceLogInfo("ManySenderToOneTargetSF_N_USData_confirm_cb", "First call");
    }
    else if (ManySenderToOneTargetSF_N_USData_confirm_cb_calls == 2)
    {
        OSInterfaceLogInfo("ManySenderToOneTargetSF_N_USData_confirm_cb", "SenderKeepRunning set to false");
        senderKeepRunning = false;
    }

    if (nAi.N_SA == 1)
    {
        N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 1};
        EXPECT_EQ(N_OK, nResult);
        EXPECT_EQ(Mtype_Diagnostics, mtype);
        EXPECT_EQ_N_AI(expectedNAi, nAi);
    }
    else if (nAi.N_SA == 3)
    {
        N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 3};
        EXPECT_EQ(N_OK, nResult);
        EXPECT_EQ(Mtype_Diagnostics, mtype);
        EXPECT_EQ_N_AI(expectedNAi, nAi);
    }
    else
    {
        EXPECT_TRUE(false) << "Unexpected N_SA value (!= 1; != 3): " << nAi.N_SA;
    }
}

static uint32_t ManySenderToOneTargetSF_N_USData_indication_cb_calls = 0;
void ManySenderToOneTargetSF_N_USData_indication_cb(N_AI nAi, const uint8_t* messageData, uint32_t messageLength,
                                                    N_Result nResult, Mtype mtype)
{
    ManySenderToOneTargetSF_N_USData_indication_cb_calls++;

    if (ManySenderToOneTargetSF_N_USData_indication_cb_calls == 2)
    {
        OSInterfaceLogInfo("ManySenderToOneTargetSF_N_USData_indication_cb", "ReceiverKeepRunning set to false");
        receiverKeepRunning = false;
    }
    else if (ManySenderToOneTargetSF_N_USData_indication_cb_calls == 1)
    {
        OSInterfaceLogInfo("ManySenderToOneTargetSF_N_USData_indication_cb", "First call");
    }

    if (nAi.N_SA == 1)
    {
        N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 1};
        EXPECT_EQ(N_OK, nResult);
        EXPECT_EQ(Mtype_Diagnostics, mtype);
        EXPECT_EQ_N_AI(expectedNAi, nAi);
        ASSERT_EQ(ManySenderToOneTargetSF_messageLength1, messageLength);
        ASSERT_NE(nullptr, messageData);
        EXPECT_EQ_ARRAY(ManySenderToOneTargetSF_message1, messageData, ManySenderToOneTargetSF_messageLength1);
    }
    else if (nAi.N_SA == 3)
    {
        N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 3};
        EXPECT_EQ(N_OK, nResult);
        EXPECT_EQ(Mtype_Diagnostics, mtype);
        EXPECT_EQ_N_AI(expectedNAi, nAi);
        ASSERT_EQ(ManySenderToOneTargetSF_messageLength2, messageLength);
        ASSERT_NE(nullptr, messageData);
        EXPECT_EQ_ARRAY(ManySenderToOneTargetSF_message2, messageData, ManySenderToOneTargetSF_messageLength2);
    }
    else
    {
        EXPECT_TRUE(false) << "Unexpected N_SA value (!= 1; != 3): " << nAi.N_SA;
    }
}

static uint32_t ManySenderToOneTargetSF_N_USData_FF_indication_cb_calls = 0;
void ManySenderToOneTargetSF_N_USData_FF_indication_cb(const N_AI nAi, const uint32_t messageLength, const Mtype mtype)
{
    ManySenderToOneTargetSF_N_USData_FF_indication_cb_calls++;
}

TEST(ISOTP_SystemTests, ManySenderToOneTargetSF)
{
    constexpr uint32_t TIMEOUT = 10000; // 10 seconds
    senderKeepRunning          = true;
    receiverKeepRunning        = true;

    LocalCANNetwork network;
    CANInterface*   senderInterface1  = network.newCANInterfaceConnection("senderInterface1");
    CANInterface*   senderInterface2  = network.newCANInterfaceConnection("senderInterface2");
    CANInterface*   receiverInterface = network.newCANInterfaceConnection("receiverInterface");
    ISOTP*       senderISOTP1 =
        new ISOTP(1, 2000, ManySenderToOneTargetSF_N_USData_confirm_cb,
                     ManySenderToOneTargetSF_N_USData_indication_cb, ManySenderToOneTargetSF_N_USData_FF_indication_cb,
                     osInterface, *senderInterface1, 2, ISOTP_DefaultSTmin, "senderISOTP1");
    ISOTP* senderISOTP2 =
        new ISOTP(3, 2000, ManySenderToOneTargetSF_N_USData_confirm_cb,
                     ManySenderToOneTargetSF_N_USData_indication_cb, ManySenderToOneTargetSF_N_USData_FF_indication_cb,
                     osInterface, *senderInterface2, 2, ISOTP_DefaultSTmin, "senderISOTP2");
    ISOTP* receiverISOTP =
        new ISOTP(2, 2000, ManySenderToOneTargetSF_N_USData_confirm_cb,
                     ManySenderToOneTargetSF_N_USData_indication_cb, ManySenderToOneTargetSF_N_USData_FF_indication_cb,
                     osInterface, *receiverInterface, 2, ISOTP_DefaultSTmin, "receiverISOTP");

    uint32_t initialTime = osInterface.osMillis();
    uint32_t step        = 0;
    while ((senderKeepRunning || receiverKeepRunning) && osInterface.osMillis() - initialTime < TIMEOUT)
    {
        senderISOTP1->runStep();
        senderISOTP1->canMessageACKQueueRunStep();
        receiverISOTP->runStep();
        receiverISOTP->canMessageACKQueueRunStep();
        senderISOTP2->runStep();
        senderISOTP2->canMessageACKQueueRunStep();

        if (step == 5)
        {
            EXPECT_TRUE(
                senderISOTP1->N_USData_request(2, N_TATYPE_5_CAN_CLASSIC_29bit_Physical,
                                                  reinterpret_cast<const uint8_t*>(ManySenderToOneTargetSF_message1),
                                                  ManySenderToOneTargetSF_messageLength1, Mtype_Diagnostics));
            EXPECT_TRUE(
                senderISOTP2->N_USData_request(2, N_TATYPE_5_CAN_CLASSIC_29bit_Physical,
                                                  reinterpret_cast<const uint8_t*>(ManySenderToOneTargetSF_message2),
                                                  ManySenderToOneTargetSF_messageLength2, Mtype_Diagnostics));
        }

        step++;
    }
    uint32_t elapsedTime = osInterface.osMillis() - initialTime;

    EXPECT_EQ(0, ManySenderToOneTargetSF_N_USData_FF_indication_cb_calls);
    EXPECT_EQ(2, ManySenderToOneTargetSF_N_USData_confirm_cb_calls);
    EXPECT_EQ(2, ManySenderToOneTargetSF_N_USData_indication_cb_calls);

    ASSERT_LT(elapsedTime, TIMEOUT) << "Test took too long: " << elapsedTime << " ms, Timeout was: " << TIMEOUT;

    delete senderISOTP1;
    delete senderISOTP2;
    delete receiverISOTP;
    delete senderInterface1;
    delete senderInterface2;
    delete receiverInterface;
}
// END ManySenderToOneTargetSF

// ManySenderToOneTargetBroadcast
constexpr char     ManySenderToOneTargetBroadcast_message1[]     = "patata";
constexpr uint32_t ManySenderToOneTargetBroadcast_messageLength1 = 7;
constexpr char     ManySenderToOneTargetBroadcast_message2[]     = "cocida";
constexpr uint32_t ManySenderToOneTargetBroadcast_messageLength2 = 7;

static uint32_t ManySenderToOneTargetBroadcast_N_USData_confirm_cb_calls = 0;
void            ManySenderToOneTargetBroadcast_N_USData_confirm_cb(N_AI nAi, N_Result nResult, Mtype mtype)
{
    ManySenderToOneTargetBroadcast_N_USData_confirm_cb_calls++;

    if (ManySenderToOneTargetBroadcast_N_USData_confirm_cb_calls == 1)
    {
        OSInterfaceLogInfo("ManySenderToOneTargetBroadcast_N_USData_confirm_cb", "First call");
    }
    else if (ManySenderToOneTargetBroadcast_N_USData_confirm_cb_calls == 2)
    {
        OSInterfaceLogInfo("ManySenderToOneTargetBroadcast_N_USData_confirm_cb", "SenderKeepRunning set to false");
        senderKeepRunning = false;
    }

    if (nAi.N_SA == 1)
    {
        N_AI expectedNAi = {.N_TAtype = N_TATYPE_6_CAN_CLASSIC_29bit_Functional, .N_TA = 2, .N_SA = 1};
        EXPECT_EQ(N_OK, nResult);
        EXPECT_EQ(Mtype_Diagnostics, mtype);
        EXPECT_EQ_N_AI(expectedNAi, nAi);
    }
    else if (nAi.N_SA == 3)
    {
        N_AI expectedNAi = {.N_TAtype = N_TATYPE_6_CAN_CLASSIC_29bit_Functional, .N_TA = 2, .N_SA = 3};
        EXPECT_EQ(N_OK, nResult);
        EXPECT_EQ(Mtype_Diagnostics, mtype);
        EXPECT_EQ_N_AI(expectedNAi, nAi);
    }
    else
    {
        EXPECT_TRUE(false) << "Unexpected N_SA value (!= 1; != 3): " << nAi.N_SA;
    }
}

static uint32_t ManySenderToOneTargetBroadcast_N_USData_indication_cb_calls = 0;
void ManySenderToOneTargetBroadcast_N_USData_indication_cb(N_AI nAi, const uint8_t* messageData, uint32_t messageLength,
                                                           N_Result nResult, Mtype mtype)
{
    ManySenderToOneTargetBroadcast_N_USData_indication_cb_calls++;

    if (ManySenderToOneTargetBroadcast_N_USData_indication_cb_calls == 2)
    {
        OSInterfaceLogInfo("ManySenderToOneTargetBroadcast_N_USData_indication_cb", "ReceiverKeepRunning set to false");
        receiverKeepRunning = false;
    }
    else if (ManySenderToOneTargetBroadcast_N_USData_indication_cb_calls == 1)
    {
        OSInterfaceLogInfo("ManySenderToOneTargetBroadcast_N_USData_indication_cb", "First call");
    }

    if (nAi.N_SA == 1)
    {
        N_AI expectedNAi = {.N_TAtype = N_TATYPE_6_CAN_CLASSIC_29bit_Functional, .N_TA = 2, .N_SA = 1};
        EXPECT_EQ(N_OK, nResult);
        EXPECT_EQ(Mtype_Diagnostics, mtype);
        EXPECT_EQ_N_AI(expectedNAi, nAi);
        ASSERT_EQ(ManySenderToOneTargetBroadcast_messageLength1, messageLength);
        ASSERT_NE(nullptr, messageData);
        EXPECT_EQ_ARRAY(ManySenderToOneTargetBroadcast_message1, messageData,
                        ManySenderToOneTargetBroadcast_messageLength1);
    }
    else if (nAi.N_SA == 3)
    {
        N_AI expectedNAi = {.N_TAtype = N_TATYPE_6_CAN_CLASSIC_29bit_Functional, .N_TA = 2, .N_SA = 3};
        EXPECT_EQ(N_OK, nResult);
        EXPECT_EQ(Mtype_Diagnostics, mtype);
        EXPECT_EQ_N_AI(expectedNAi, nAi);
        ASSERT_EQ(ManySenderToOneTargetBroadcast_messageLength2, messageLength);
        ASSERT_NE(nullptr, messageData);
        EXPECT_EQ_ARRAY(ManySenderToOneTargetBroadcast_message2, messageData,
                        ManySenderToOneTargetBroadcast_messageLength2);
    }
    else
    {
        EXPECT_TRUE(false) << "Unexpected N_SA value (!= 1; != 3): " << nAi.N_SA;
    }
}

static uint32_t ManySenderToOneTargetBroadcast_N_USData_FF_indication_cb_calls = 0;
void            ManySenderToOneTargetBroadcast_N_USData_FF_indication_cb(const N_AI nAi, const uint32_t messageLength,
                                                                         const Mtype mtype)
{
    ManySenderToOneTargetBroadcast_N_USData_FF_indication_cb_calls++;
}

TEST(ISOTP_SystemTests, ManySenderToOneTargetBroadcast)
{
    constexpr uint32_t TIMEOUT = 10000; // 10 seconds
    senderKeepRunning          = true;
    receiverKeepRunning        = true;

    LocalCANNetwork network;
    CANInterface*   senderInterface1  = network.newCANInterfaceConnection("senderInterface1");
    CANInterface*   senderInterface2  = network.newCANInterfaceConnection("senderInterface2");
    CANInterface*   receiverInterface = network.newCANInterfaceConnection("receiverInterface");
    ISOTP*       senderISOTP1   = new ISOTP(1, 2000, ManySenderToOneTargetBroadcast_N_USData_confirm_cb,
                                                     ManySenderToOneTargetBroadcast_N_USData_indication_cb,
                                                     ManySenderToOneTargetBroadcast_N_USData_FF_indication_cb, osInterface,
                                                     *senderInterface1, 2, ISOTP_DefaultSTmin, "senderISOTP1");
    ISOTP*       senderISOTP2   = new ISOTP(3, 2000, ManySenderToOneTargetBroadcast_N_USData_confirm_cb,
                                                     ManySenderToOneTargetBroadcast_N_USData_indication_cb,
                                                     ManySenderToOneTargetBroadcast_N_USData_FF_indication_cb, osInterface,
                                                     *senderInterface2, 2, ISOTP_DefaultSTmin, "senderISOTP2");
    ISOTP*       receiverISOTP  = new ISOTP(2, 2000, ManySenderToOneTargetBroadcast_N_USData_confirm_cb,
                                                     ManySenderToOneTargetBroadcast_N_USData_indication_cb,
                                                     ManySenderToOneTargetBroadcast_N_USData_FF_indication_cb, osInterface,
                                                     *receiverInterface, 2, ISOTP_DefaultSTmin, "receiverISOTP");

    receiverISOTP->addAcceptedFunctionalN_TA(2);

    uint32_t initialTime = osInterface.osMillis();
    uint32_t step        = 0;
    while ((senderKeepRunning || receiverKeepRunning) && osInterface.osMillis() - initialTime < TIMEOUT)
    {
        senderISOTP1->runStep();
        senderISOTP1->canMessageACKQueueRunStep();
        receiverISOTP->runStep();
        receiverISOTP->canMessageACKQueueRunStep();
        senderISOTP2->runStep();
        senderISOTP2->canMessageACKQueueRunStep();

        if (step == 5)
        {
            EXPECT_TRUE(senderISOTP1->N_USData_request(
                2, N_TATYPE_6_CAN_CLASSIC_29bit_Functional,
                reinterpret_cast<const uint8_t*>(ManySenderToOneTargetBroadcast_message1),
                ManySenderToOneTargetBroadcast_messageLength1, Mtype_Diagnostics));
            EXPECT_TRUE(senderISOTP2->N_USData_request(
                2, N_TATYPE_6_CAN_CLASSIC_29bit_Functional,
                reinterpret_cast<const uint8_t*>(ManySenderToOneTargetBroadcast_message2),
                ManySenderToOneTargetBroadcast_messageLength2, Mtype_Diagnostics));
        }

        step++;
    }
    uint32_t elapsedTime = osInterface.osMillis() - initialTime;

    EXPECT_EQ(0, ManySenderToOneTargetBroadcast_N_USData_FF_indication_cb_calls);
    EXPECT_EQ(2, ManySenderToOneTargetBroadcast_N_USData_confirm_cb_calls);
    EXPECT_EQ(2, ManySenderToOneTargetBroadcast_N_USData_indication_cb_calls);

    ASSERT_LT(elapsedTime, TIMEOUT) << "Test took too long: " << elapsedTime << " ms, Timeout was: " << TIMEOUT;

    delete senderISOTP1;
    delete senderISOTP2;
    delete receiverISOTP;
    delete senderInterface1;
    delete senderInterface2;
    delete receiverInterface;
}
// END ManySenderToOneTargetBroadcast

// ManySenderToOneTargetMF
constexpr char     ManySenderToOneTargetMF_message1[]     = "01234567890123456789";
constexpr uint32_t ManySenderToOneTargetMF_messageLength1 = 21;
constexpr char     ManySenderToOneTargetMF_message2[]     = "98765432109876543210";
constexpr uint32_t ManySenderToOneTargetMF_messageLength2 = 21;

static uint32_t ManySenderToOneTargetMF_N_USData_confirm_cb_calls = 0;
void            ManySenderToOneTargetMF_N_USData_confirm_cb(N_AI nAi, N_Result nResult, Mtype mtype)
{
    ManySenderToOneTargetMF_N_USData_confirm_cb_calls++;

    if (ManySenderToOneTargetMF_N_USData_confirm_cb_calls == 1)
    {
        OSInterfaceLogInfo("ManySenderToOneTargetMF_N_USData_confirm_cb", "First call");
    }
    else if (ManySenderToOneTargetMF_N_USData_confirm_cb_calls == 2)
    {
        OSInterfaceLogInfo("ManySenderToOneTargetMF_N_USData_confirm_cb", "SenderKeepRunning set to false");
        senderKeepRunning = false;
    }

    if (nAi.N_SA == 1)
    {
        N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 1};
        EXPECT_EQ_N_AI(expectedNAi, nAi);
        EXPECT_EQ(N_OK, nResult);
        EXPECT_EQ(Mtype_Diagnostics, mtype);
    }
    else if (nAi.N_SA == 3)
    {
        N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 3};
        EXPECT_EQ_N_AI(expectedNAi, nAi);
        EXPECT_EQ(N_OK, nResult);
        EXPECT_EQ(Mtype_Diagnostics, mtype);
    }
    else
    {
        EXPECT_TRUE(false) << "Unexpected N_SA value (!= 1; != 3): " << nAi.N_SA;
    }
}

static uint32_t ManySenderToOneTargetMF_N_USData_indication_cb_calls = 0;
void ManySenderToOneTargetMF_N_USData_indication_cb(N_AI nAi, const uint8_t* messageData, uint32_t messageLength,
                                                    N_Result nResult, Mtype mtype)
{
    ManySenderToOneTargetMF_N_USData_indication_cb_calls++;

    if (ManySenderToOneTargetMF_N_USData_indication_cb_calls == 1)
    {
        OSInterfaceLogInfo("ManySenderToOneTargetMF_N_USData_indication_cb", "First call");
    }
    else if (ManySenderToOneTargetMF_N_USData_indication_cb_calls == 2)
    {
        OSInterfaceLogInfo("ManySenderToOneTargetMF_N_USData_indication_cb", "ReceiverKeepRunning set to false");
        receiverKeepRunning = false;
    }

    if (nAi.N_SA == 1)
    {
        N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 1};
        EXPECT_EQ(N_OK, nResult);
        EXPECT_EQ(Mtype_Diagnostics, mtype);
        EXPECT_EQ_N_AI(expectedNAi, nAi);
        ASSERT_EQ(ManySenderToOneTargetMF_messageLength1, messageLength);
        ASSERT_NE(nullptr, messageData);
        ASSERT_EQ_ARRAY(ManySenderToOneTargetMF_message1, messageData, ManySenderToOneTargetMF_messageLength1);
    }
    else if (nAi.N_SA == 3)
    {
        N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 3};
        EXPECT_EQ(N_OK, nResult);
        EXPECT_EQ(Mtype_Diagnostics, mtype);
        EXPECT_EQ_N_AI(expectedNAi, nAi);
        ASSERT_EQ(ManySenderToOneTargetMF_messageLength2, messageLength);
        ASSERT_NE(nullptr, messageData);
        ASSERT_EQ_ARRAY(ManySenderToOneTargetMF_message2, messageData, ManySenderToOneTargetMF_messageLength2);
    }
    else
    {
        EXPECT_TRUE(false) << "Unexpected N_SA value (!= 1; != 3): " << nAi.N_SA;
    }
}

static uint32_t ManySenderToOneTargetMF_N_USData_FF_indication_cb_calls = 0;
void ManySenderToOneTargetMF_N_USData_FF_indication_cb(const N_AI nAi, const uint32_t messageLength, const Mtype mtype)
{
    ManySenderToOneTargetMF_N_USData_FF_indication_cb_calls++;

    if (ManySenderToOneTargetMF_N_USData_FF_indication_cb_calls == 1)
    {
        OSInterfaceLogInfo("ManySenderToOneTargetMF_N_USData_FF_indication_cb", "First call");
    }
    else if (ManySenderToOneTargetMF_N_USData_FF_indication_cb_calls == 2)
    {
        OSInterfaceLogInfo("ManySenderToOneTargetMF_N_USData_FF_indication_cb", "Second call");
    }

    if (nAi.N_SA == 1)
    {
        N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 1};
        EXPECT_EQ_N_AI(expectedNAi, nAi);
        ASSERT_EQ(ManySenderToOneTargetMF_messageLength1, messageLength);
        EXPECT_EQ(Mtype_Diagnostics, mtype);
    }
    else if (nAi.N_SA == 3)
    {
        N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 3};
        EXPECT_EQ_N_AI(expectedNAi, nAi);
        ASSERT_EQ(ManySenderToOneTargetMF_messageLength2, messageLength);
        EXPECT_EQ(Mtype_Diagnostics, mtype);
    }
    else
    {
        EXPECT_TRUE(false) << "Unexpected N_SA value (!= 1; != 3): " << nAi.N_SA;
    }
}

TEST(ISOTP_SystemTests, ManySenderToOneTargetMF)
{
    constexpr uint32_t TIMEOUT = 10000;
    senderKeepRunning          = true;
    receiverKeepRunning        = true;

    LocalCANNetwork network;
    CANInterface*   senderInterface1  = network.newCANInterfaceConnection();
    CANInterface*   senderInterface2  = network.newCANInterfaceConnection();
    CANInterface*   receiverInterface = network.newCANInterfaceConnection();
    ISOTP*       senderISOTP1 =
        new ISOTP(1, 2000, ManySenderToOneTargetMF_N_USData_confirm_cb,
                     ManySenderToOneTargetMF_N_USData_indication_cb, ManySenderToOneTargetMF_N_USData_FF_indication_cb,
                     osInterface, *senderInterface1, 2, ISOTP_DefaultSTmin, "senderISOTP1");
    ISOTP* senderISOTP2 =
        new ISOTP(3, 2000, ManySenderToOneTargetMF_N_USData_confirm_cb,
                     ManySenderToOneTargetMF_N_USData_indication_cb, ManySenderToOneTargetMF_N_USData_FF_indication_cb,
                     osInterface, *senderInterface2, 2, ISOTP_DefaultSTmin, "senderISOTP2");
    ISOTP* receiverISOTP =
        new ISOTP(2, 2000, ManySenderToOneTargetMF_N_USData_confirm_cb,
                     ManySenderToOneTargetMF_N_USData_indication_cb, ManySenderToOneTargetMF_N_USData_FF_indication_cb,
                     osInterface, *receiverInterface, 2, ISOTP_DefaultSTmin, "receiverISOTP");

    uint32_t initialTime = osInterface.osMillis();
    uint32_t step        = 0;
    while ((senderKeepRunning || receiverKeepRunning) && osInterface.osMillis() - initialTime < TIMEOUT)
    {
        senderISOTP1->runStep();
        senderISOTP1->canMessageACKQueueRunStep();
        senderISOTP2->runStep();
        senderISOTP2->canMessageACKQueueRunStep();
        receiverISOTP->runStep();
        receiverISOTP->canMessageACKQueueRunStep();

        if (step == 5)
        {
            EXPECT_TRUE(
                senderISOTP1->N_USData_request(2, N_TATYPE_5_CAN_CLASSIC_29bit_Physical,
                                                  reinterpret_cast<const uint8_t*>(ManySenderToOneTargetMF_message1),
                                                  ManySenderToOneTargetMF_messageLength1, Mtype_Diagnostics));
            EXPECT_TRUE(
                senderISOTP2->N_USData_request(2, N_TATYPE_5_CAN_CLASSIC_29bit_Physical,
                                                  reinterpret_cast<const uint8_t*>(ManySenderToOneTargetMF_message2),
                                                  ManySenderToOneTargetMF_messageLength2, Mtype_Diagnostics));
        }
        step++;
    }
    uint32_t elapsedTime = osInterface.osMillis() - initialTime;

    EXPECT_EQ(2, ManySenderToOneTargetMF_N_USData_FF_indication_cb_calls);
    EXPECT_EQ(2, ManySenderToOneTargetMF_N_USData_confirm_cb_calls);
    EXPECT_EQ(2, ManySenderToOneTargetMF_N_USData_indication_cb_calls);

    ASSERT_LT(elapsedTime, TIMEOUT) << "Test took too long: " << elapsedTime << " ms, Timeout was: " << TIMEOUT;

    delete senderISOTP1;
    delete senderISOTP2;
    delete receiverISOTP;
    delete senderInterface1;
    delete senderInterface2;
    delete receiverInterface;
}
// END ManySenderToOneTargetMF

// ManySenderToOneTargetMIX
constexpr char     ManySenderToOneTargetMIX_message1[]     = "patata";
constexpr uint32_t ManySenderToOneTargetMIX_messageLength1 = 7;
constexpr char     ManySenderToOneTargetMIX_message2[]     = "cocida";
constexpr uint32_t ManySenderToOneTargetMIX_messageLength2 = 7;
constexpr char     ManySenderToOneTargetMIX_message3[]     = "98765432109876543210";
constexpr uint32_t ManySenderToOneTargetMIX_messageLength3 = 21;

static uint32_t ManySenderToOneTargetMIX_N_USData_confirm_cb_calls = 0;
void            ManySenderToOneTargetMIX_N_USData_confirm_cb(N_AI nAi, N_Result nResult, Mtype mtype)
{
    ManySenderToOneTargetMIX_N_USData_confirm_cb_calls++;

    N_AI expectedNAi;

    OSInterfaceLogInfo("ManySenderToOneTargetMIX_N_USData_confirm_cb", "Callback called (%d)",
                       ManySenderToOneTargetMIX_N_USData_confirm_cb_calls);

    if (ManySenderToOneTargetMIX_N_USData_confirm_cb_calls == 3)
    {
        OSInterfaceLogInfo("ManySenderToOneTargetMIX_N_USData_confirm_cb", "SenderKeepRunning set to false");
        senderKeepRunning = false;
    }

    switch (nAi.N_SA)
    {
        case 1:
            expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 1};
            break;
        case 3:
            expectedNAi = {.N_TAtype = N_TATYPE_6_CAN_CLASSIC_29bit_Functional, .N_TA = 2, .N_SA = 3};
            break;
        case 4:
            expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 4};
            break;
        default:
            EXPECT_TRUE(false) << "Unexpected N_SA value (!= 1; != 3; != 4): " << nAi.N_SA;
    }

    EXPECT_EQ_N_AI(expectedNAi, nAi);
    EXPECT_EQ(N_OK, nResult);
    EXPECT_EQ(Mtype_Diagnostics, mtype);
}

static uint32_t ManySenderToOneTargetMIX_N_USData_indication_cb_calls = 0;
void ManySenderToOneTargetMIX_N_USData_indication_cb(N_AI nAi, const uint8_t* messageData, uint32_t messageLength,
                                                     N_Result nResult, Mtype mtype)
{
    ManySenderToOneTargetMIX_N_USData_indication_cb_calls++;
    N_AI expectedNAi;

    OSInterfaceLogInfo("ManySenderToOneTargetMIX_N_USData_indication_cb", "Callback called (%d)",
                       ManySenderToOneTargetMIX_N_USData_indication_cb_calls);

    if (ManySenderToOneTargetMIX_N_USData_indication_cb_calls == 3)
    {
        OSInterfaceLogInfo("ManySenderToOneTargetMIX_N_USData_indication_cb", "ReceiverKeepRunning set to false");
        receiverKeepRunning = false;
    }

    switch (nAi.N_SA)
    {
        case 1:
            expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 1};
            EXPECT_EQ(N_OK, nResult);
            EXPECT_EQ(Mtype_Diagnostics, mtype);
            EXPECT_EQ_N_AI(expectedNAi, nAi);
            ASSERT_EQ(ManySenderToOneTargetMIX_messageLength1, messageLength);
            ASSERT_NE(nullptr, messageData);
            ASSERT_EQ_ARRAY(ManySenderToOneTargetMIX_message1, messageData, ManySenderToOneTargetMIX_messageLength1);
            break;
        case 3:
            expectedNAi = {.N_TAtype = N_TATYPE_6_CAN_CLASSIC_29bit_Functional, .N_TA = 2, .N_SA = 3};
            EXPECT_EQ(N_OK, nResult);
            EXPECT_EQ(Mtype_Diagnostics, mtype);
            EXPECT_EQ_N_AI(expectedNAi, nAi);
            ASSERT_EQ(ManySenderToOneTargetMIX_messageLength2, messageLength);
            ASSERT_NE(nullptr, messageData);
            ASSERT_EQ_ARRAY(ManySenderToOneTargetMIX_message2, messageData, ManySenderToOneTargetMIX_messageLength2);
            break;
        case 4:
            expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 4};
            EXPECT_EQ(N_OK, nResult);
            EXPECT_EQ(Mtype_Diagnostics, mtype);
            EXPECT_EQ_N_AI(expectedNAi, nAi);
            ASSERT_EQ(ManySenderToOneTargetMIX_messageLength3, messageLength);
            ASSERT_NE(nullptr, messageData);
            ASSERT_EQ_ARRAY(ManySenderToOneTargetMIX_message3, messageData, ManySenderToOneTargetMIX_messageLength3);
            break;
        default:
            EXPECT_TRUE(false) << "Unexpected N_SA value (!= 1; != 3; != 4): " << nAi.N_SA;
    }
}

static uint32_t ManySenderToOneTargetMIX_N_USData_FF_indication_cb_calls = 0;
void ManySenderToOneTargetMIX_N_USData_FF_indication_cb(const N_AI nAi, const uint32_t messageLength, const Mtype mtype)
{
    ManySenderToOneTargetMIX_N_USData_FF_indication_cb_calls++;

    OSInterfaceLogInfo("ManySenderToOneTargetMIX_N_USData_FF_indication_cb", "Callback called (%d)",
                       ManySenderToOneTargetMIX_N_USData_FF_indication_cb_calls);

    N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 4};
    EXPECT_EQ(Mtype_Diagnostics, mtype);
    EXPECT_EQ_N_AI(expectedNAi, nAi);
    ASSERT_EQ(ManySenderToOneTargetMIX_messageLength3, messageLength);
}

TEST(ISOTP_SystemTests, ManySenderToOneTargetMIX)
{
    constexpr uint32_t TIMEOUT = 10000;
    senderKeepRunning          = true;
    receiverKeepRunning        = true;

    LocalCANNetwork network;
    CANInterface*   senderInterface1  = network.newCANInterfaceConnection();
    CANInterface*   senderInterface2  = network.newCANInterfaceConnection();
    CANInterface*   senderInterface3  = network.newCANInterfaceConnection();
    CANInterface*   receiverInterface = network.newCANInterfaceConnection();
    ISOTP*       senderISOTP1   = new ISOTP(1, 2000, ManySenderToOneTargetMIX_N_USData_confirm_cb,
                                                     ManySenderToOneTargetMIX_N_USData_indication_cb,
                                                     ManySenderToOneTargetMIX_N_USData_FF_indication_cb, osInterface,
                                                     *senderInterface1, 2, ISOTP_DefaultSTmin, "senderISOTP1");
    ISOTP*       senderISOTP2   = new ISOTP(3, 2000, ManySenderToOneTargetMIX_N_USData_confirm_cb,
                                                     ManySenderToOneTargetMIX_N_USData_indication_cb,
                                                     ManySenderToOneTargetMIX_N_USData_FF_indication_cb, osInterface,
                                                     *senderInterface2, 2, ISOTP_DefaultSTmin, "senderISOTP2");
    ISOTP*       senderISOTP3   = new ISOTP(4, 2000, ManySenderToOneTargetMIX_N_USData_confirm_cb,
                                                     ManySenderToOneTargetMIX_N_USData_indication_cb,
                                                     ManySenderToOneTargetMIX_N_USData_FF_indication_cb, osInterface,
                                                     *senderInterface3, 2, ISOTP_DefaultSTmin, "senderISOTP3");
    ISOTP*       receiverISOTP  = new ISOTP(2, 2000, ManySenderToOneTargetMIX_N_USData_confirm_cb,
                                                     ManySenderToOneTargetMIX_N_USData_indication_cb,
                                                     ManySenderToOneTargetMIX_N_USData_FF_indication_cb, osInterface,
                                                     *receiverInterface, 2, ISOTP_DefaultSTmin, "receiverISOTP");

    receiverISOTP->addAcceptedFunctionalN_TA(2);

    uint32_t initialTime = osInterface.osMillis();
    uint32_t step        = 0;
    while ((senderKeepRunning || receiverKeepRunning) && osInterface.osMillis() - initialTime < TIMEOUT)
    {
        senderISOTP1->runStep();
        senderISOTP1->canMessageACKQueueRunStep();
        senderISOTP2->runStep();
        senderISOTP2->canMessageACKQueueRunStep();
        senderISOTP3->runStep();
        senderISOTP3->canMessageACKQueueRunStep();
        receiverISOTP->runStep();
        receiverISOTP->canMessageACKQueueRunStep();

        if (step == 5)
        {
            EXPECT_TRUE(
                senderISOTP1->N_USData_request(2, N_TATYPE_5_CAN_CLASSIC_29bit_Physical,
                                                  reinterpret_cast<const uint8_t*>(ManySenderToOneTargetMIX_message1),
                                                  ManySenderToOneTargetMIX_messageLength1, Mtype_Diagnostics));
            EXPECT_TRUE(
                senderISOTP2->N_USData_request(2, N_TATYPE_6_CAN_CLASSIC_29bit_Functional,
                                                  reinterpret_cast<const uint8_t*>(ManySenderToOneTargetMIX_message2),
                                                  ManySenderToOneTargetMIX_messageLength2, Mtype_Diagnostics));
            EXPECT_TRUE(
                senderISOTP3->N_USData_request(2, N_TATYPE_5_CAN_CLASSIC_29bit_Physical,
                                                  reinterpret_cast<const uint8_t*>(ManySenderToOneTargetMIX_message3),
                                                  ManySenderToOneTargetMIX_messageLength3, Mtype_Diagnostics));
        }
        step++;
    }
    uint32_t elapsedTime = osInterface.osMillis() - initialTime;

    EXPECT_EQ(1, ManySenderToOneTargetMIX_N_USData_FF_indication_cb_calls);
    EXPECT_EQ(3, ManySenderToOneTargetMIX_N_USData_confirm_cb_calls);
    EXPECT_EQ(3, ManySenderToOneTargetMIX_N_USData_indication_cb_calls);

    ASSERT_LT(elapsedTime, TIMEOUT) << "Test took too long: " << elapsedTime << " ms, Timeout was: " << TIMEOUT;

    delete senderISOTP1;
    delete senderISOTP2;
    delete senderISOTP3;
    delete receiverISOTP;
    delete senderInterface1;
    delete senderInterface2;
    delete senderInterface3;
    delete receiverInterface;
}
// END ManySenderToOneTargetMIX

// MessageExchangeSF
constexpr char     MessageExchangeSF_message1[]     = "patata";
constexpr uint32_t MessageExchangeSF_messageLength1 = 7;
constexpr char     MessageExchangeSF_message2[]     = "cocida";
constexpr uint32_t MessageExchangeSF_messageLength2 = 7;

static uint32_t MessageExchangeSF_N_USData_confirm_cb_calls = 0;
void            MessageExchangeSF_N_USData_confirm_cb(N_AI nAi, N_Result nResult, Mtype mtype)
{
    MessageExchangeSF_N_USData_confirm_cb_calls++;

    if (MessageExchangeSF_N_USData_confirm_cb_calls == 1)
    {
        OSInterfaceLogInfo("MessageExchangeSF_N_USData_confirm_cb", "First call");
    }
    else if (MessageExchangeSF_N_USData_confirm_cb_calls == 2)
    {
        OSInterfaceLogInfo("MessageExchangeSF_N_USData_confirm_cb", "SenderKeepRunning set to false");
        senderKeepRunning = false;
    }

    if (nAi.N_SA == 1)
    {
        N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 1};
        EXPECT_EQ_N_AI(expectedNAi, nAi);
        EXPECT_EQ(N_OK, nResult);
        EXPECT_EQ(Mtype_Diagnostics, mtype);
    }
    else if (nAi.N_SA == 2)
    {
        N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 1, .N_SA = 2};
        EXPECT_EQ_N_AI(expectedNAi, nAi);
        EXPECT_EQ(N_OK, nResult);
        EXPECT_EQ(Mtype_Diagnostics, mtype);
    }
    else
    {
        EXPECT_TRUE(false) << "Unexpected N_SA value (!= 1; != 2): " << nAi.N_SA;
    }
}

static uint32_t MessageExchangeSF_N_USData_indication_cb_calls = 0;
void            MessageExchangeSF_N_USData_indication_cb(N_AI nAi, const uint8_t* messageData, uint32_t messageLength,
                                                         N_Result nResult, Mtype mtype)
{
    MessageExchangeSF_N_USData_indication_cb_calls++;

    if (MessageExchangeSF_N_USData_indication_cb_calls == 1)
    {
        OSInterfaceLogInfo("MessageExchangeSF_N_USData_indication_cb", "First call");
    }
    else if (MessageExchangeSF_N_USData_indication_cb_calls == 2)
    {
        OSInterfaceLogInfo("MessageExchangeSF_N_USData_indication_cb", "ReceiverKeepRunning set to false");
        receiverKeepRunning = false;
    }

    if (nAi.N_SA == 1)
    {
        N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 1};
        EXPECT_EQ(N_OK, nResult);
        EXPECT_EQ(Mtype_Diagnostics, mtype);
        EXPECT_EQ_N_AI(expectedNAi, nAi);
        ASSERT_EQ(MessageExchangeSF_messageLength1, messageLength);
        ASSERT_NE(nullptr, messageData);
        ASSERT_EQ_ARRAY(MessageExchangeSF_message1, messageData, MessageExchangeSF_messageLength1);
    }
    else if (nAi.N_SA == 2)
    {
        N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 1, .N_SA = 2};
        EXPECT_EQ(N_OK, nResult);
        EXPECT_EQ(Mtype_Diagnostics, mtype);
        EXPECT_EQ_N_AI(expectedNAi, nAi);
        ASSERT_EQ(MessageExchangeSF_messageLength2, messageLength);
        ASSERT_NE(nullptr, messageData);
        ASSERT_EQ_ARRAY(MessageExchangeSF_message2, messageData, MessageExchangeSF_messageLength2);
    }
    else
    {
        EXPECT_TRUE(false) << "Unexpected N_SA value (!= 1; != 2): " << nAi.N_SA;
    }
}

static uint32_t MessageExchangeSF_N_USData_FF_indication_cb_calls = 0;
void MessageExchangeSF_N_USData_FF_indication_cb(const N_AI nAi, const uint32_t messageLength, const Mtype mtype)
{
    MessageExchangeSF_N_USData_FF_indication_cb_calls++;

    if (MessageExchangeSF_N_USData_FF_indication_cb_calls == 1)
    {
        OSInterfaceLogInfo("MessageExchangeSF_N_USData_FF_indication_cb", "First call");
    }
    else if (MessageExchangeSF_N_USData_FF_indication_cb_calls == 2)
    {
        OSInterfaceLogInfo("MessageExchangeSF_N_USData_FF_indication_cb", "Second call");
    }

    if (nAi.N_SA == 1)
    {
        N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 1};
        EXPECT_EQ_N_AI(expectedNAi, nAi);
        ASSERT_EQ(MessageExchangeSF_messageLength1, messageLength);
        EXPECT_EQ(Mtype_Diagnostics, mtype);
    }
    else if (nAi.N_SA == 2)
    {
        N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 1, .N_SA = 2};
        EXPECT_EQ_N_AI(expectedNAi, nAi);
        ASSERT_EQ(MessageExchangeSF_messageLength2, messageLength);
        EXPECT_EQ(Mtype_Diagnostics, mtype);
    }
    else
    {
        EXPECT_TRUE(false) << "Unexpected N_SA value (!= 1; != 2): " << nAi.N_SA;
    }
}

TEST(ISOTP_SystemTests, MessageExchangeSF)
{
    constexpr uint32_t TIMEOUT = 10000;
    senderKeepRunning          = true;
    receiverKeepRunning        = true;

    LocalCANNetwork network;
    CANInterface*   interface1 = network.newCANInterfaceConnection();
    CANInterface*   interface2 = network.newCANInterfaceConnection();
    ISOTP*       ISOTP1  = new ISOTP(
        1, 2000, MessageExchangeSF_N_USData_confirm_cb, MessageExchangeSF_N_USData_indication_cb,
        MessageExchangeSF_N_USData_FF_indication_cb, osInterface, *interface1, 2, ISOTP_DefaultSTmin, "ISOTP1");
    ISOTP* ISOTP2 = new ISOTP(
        2, 2000, MessageExchangeSF_N_USData_confirm_cb, MessageExchangeSF_N_USData_indication_cb,
        MessageExchangeSF_N_USData_FF_indication_cb, osInterface, *interface2, 2, ISOTP_DefaultSTmin, "ISOTP2");

    uint32_t initialTime = osInterface.osMillis();
    uint32_t step        = 0;
    while ((senderKeepRunning || receiverKeepRunning) && osInterface.osMillis() - initialTime < TIMEOUT)
    {
        ISOTP1->runStep();
        ISOTP1->canMessageACKQueueRunStep();
        ISOTP2->runStep();
        ISOTP2->canMessageACKQueueRunStep();

        if (step == 5)
        {
            EXPECT_TRUE(ISOTP1->N_USData_request(2, N_TATYPE_5_CAN_CLASSIC_29bit_Physical,
                                                    reinterpret_cast<const uint8_t*>(MessageExchangeSF_message1),
                                                    MessageExchangeSF_messageLength1, Mtype_Diagnostics));
            EXPECT_TRUE(ISOTP2->N_USData_request(1, N_TATYPE_5_CAN_CLASSIC_29bit_Physical,
                                                    reinterpret_cast<const uint8_t*>(MessageExchangeSF_message2),
                                                    MessageExchangeSF_messageLength2, Mtype_Diagnostics));
        }
        step++;
    }
    uint32_t elapsedTime = osInterface.osMillis() - initialTime;

    EXPECT_EQ(0, MessageExchangeSF_N_USData_FF_indication_cb_calls);
    EXPECT_EQ(2, MessageExchangeSF_N_USData_confirm_cb_calls);
    EXPECT_EQ(2, MessageExchangeSF_N_USData_indication_cb_calls);

    ASSERT_LT(elapsedTime, TIMEOUT) << "Test took too long: " << elapsedTime << " ms, Timeout was: " << TIMEOUT;

    delete ISOTP1;
    delete ISOTP2;
    delete interface1;
    delete interface2;
}
// END MessageExchangeSF

// MessageExchangeMF
constexpr char     MessageExchangeMF_message1[]     = "01234567890123456789";
constexpr uint32_t MessageExchangeMF_messageLength1 = 21;
constexpr char     MessageExchangeMF_message2[]     = "98765432109876543210";
constexpr uint32_t MessageExchangeMF_messageLength2 = 21;

static uint32_t MessageExchangeMF_N_USData_confirm_cb_calls = 0;
void            MessageExchangeMF_N_USData_confirm_cb(N_AI nAi, N_Result nResult, Mtype mtype)
{
    MessageExchangeMF_N_USData_confirm_cb_calls++;

    if (MessageExchangeMF_N_USData_confirm_cb_calls == 1)
    {
        OSInterfaceLogInfo("MessageExchangeMF_N_USData_confirm_cb", "First call");
    }
    else if (MessageExchangeMF_N_USData_confirm_cb_calls == 2)
    {
        OSInterfaceLogInfo("MessageExchangeMF_N_USData_confirm_cb", "SenderKeepRunning set to false");
        senderKeepRunning = false;
    }

    if (nAi.N_SA == 1)
    {
        N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 1};
        EXPECT_EQ_N_AI(expectedNAi, nAi);
        EXPECT_EQ(N_OK, nResult);
        EXPECT_EQ(Mtype_Diagnostics, mtype);
    }
    else if (nAi.N_SA == 2)
    {
        N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 1, .N_SA = 2};
        EXPECT_EQ_N_AI(expectedNAi, nAi);
        EXPECT_EQ(N_OK, nResult);
        EXPECT_EQ(Mtype_Diagnostics, mtype);
    }
    else
    {
        EXPECT_TRUE(false) << "Unexpected N_SA value (!= 1; != 2): " << nAi.N_SA;
    }
}

static uint32_t MessageExchangeMF_N_USData_indication_cb_calls = 0;
void            MessageExchangeMF_N_USData_indication_cb(N_AI nAi, const uint8_t* messageData, uint32_t messageLength,
                                                         N_Result nResult, Mtype mtype)
{
    MessageExchangeMF_N_USData_indication_cb_calls++;

    if (MessageExchangeMF_N_USData_indication_cb_calls == 1)
    {
        OSInterfaceLogInfo("MessageExchangeMF_N_USData_indication_cb", "First call");
    }
    else if (MessageExchangeMF_N_USData_indication_cb_calls == 2)
    {
        OSInterfaceLogInfo("MessageExchangeMF_N_USData_indication_cb", "ReceiverKeepRunning set to false");
        receiverKeepRunning = false;
    }

    if (nAi.N_SA == 1)
    {
        N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 1};
        EXPECT_EQ(N_OK, nResult);
        EXPECT_EQ(Mtype_Diagnostics, mtype);
        EXPECT_EQ_N_AI(expectedNAi, nAi);
        ASSERT_EQ(MessageExchangeMF_messageLength1, messageLength);
        ASSERT_NE(nullptr, messageData);
        ASSERT_EQ_ARRAY(MessageExchangeMF_message1, messageData, MessageExchangeMF_messageLength1);
    }
    else if (nAi.N_SA == 2)
    {
        N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 1, .N_SA = 2};
        EXPECT_EQ(N_OK, nResult);
        EXPECT_EQ(Mtype_Diagnostics, mtype);
        EXPECT_EQ_N_AI(expectedNAi, nAi);
        ASSERT_EQ(MessageExchangeMF_messageLength2, messageLength);
        ASSERT_NE(nullptr, messageData);
        ASSERT_EQ_ARRAY(MessageExchangeMF_message2, messageData, MessageExchangeMF_messageLength2);
    }
    else
    {
        EXPECT_TRUE(false) << "Unexpected N_SA value (!= 1; != 2): " << nAi.N_SA;
    }
}

static uint32_t MessageExchangeMF_N_USData_FF_indication_cb_calls = 0;
void MessageExchangeMF_N_USData_FF_indication_cb(const N_AI nAi, const uint32_t messageLength, const Mtype mtype)
{
    MessageExchangeMF_N_USData_FF_indication_cb_calls++;

    if (MessageExchangeMF_N_USData_FF_indication_cb_calls == 1)
    {
        OSInterfaceLogInfo("MessageExchangeMF_N_USData_FF_indication_cb", "First call");
    }
    else if (MessageExchangeMF_N_USData_FF_indication_cb_calls == 2)
    {
        OSInterfaceLogInfo("MessageExchangeMF_N_USData_FF_indication_cb", "Second call");
    }

    if (nAi.N_SA == 1)
    {
        N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 2, .N_SA = 1};
        EXPECT_EQ_N_AI(expectedNAi, nAi);
        ASSERT_EQ(MessageExchangeMF_messageLength1, messageLength);
        EXPECT_EQ(Mtype_Diagnostics, mtype);
    }
    else if (nAi.N_SA == 2)
    {
        N_AI expectedNAi = {.N_TAtype = N_TATYPE_5_CAN_CLASSIC_29bit_Physical, .N_TA = 1, .N_SA = 2};
        EXPECT_EQ_N_AI(expectedNAi, nAi);
        ASSERT_EQ(MessageExchangeMF_messageLength2, messageLength);
        EXPECT_EQ(Mtype_Diagnostics, mtype);
    }
    else
    {
        EXPECT_TRUE(false) << "Unexpected N_SA value (!= 1; != 2): " << nAi.N_SA;
    }
}

TEST(ISOTP_SystemTests, MessageExchangeMF)
{
    constexpr uint32_t TIMEOUT = 10000;
    senderKeepRunning          = true;
    receiverKeepRunning        = true;

    LocalCANNetwork network;
    CANInterface*   interface1 = network.newCANInterfaceConnection();
    CANInterface*   interface2 = network.newCANInterfaceConnection();
    ISOTP*       ISOTP1  = new ISOTP(
        1, 2000, MessageExchangeMF_N_USData_confirm_cb, MessageExchangeMF_N_USData_indication_cb,
        MessageExchangeMF_N_USData_FF_indication_cb, osInterface, *interface1, 2, ISOTP_DefaultSTmin, "ISOTP1");
    ISOTP* ISOTP2 = new ISOTP(
        2, 2000, MessageExchangeMF_N_USData_confirm_cb, MessageExchangeMF_N_USData_indication_cb,
        MessageExchangeMF_N_USData_FF_indication_cb, osInterface, *interface2, 2, ISOTP_DefaultSTmin, "ISOTP2");

    uint32_t initialTime = osInterface.osMillis();
    uint32_t step        = 0;
    while ((senderKeepRunning || receiverKeepRunning) && osInterface.osMillis() - initialTime < TIMEOUT)
    {
        ISOTP1->runStep();
        ISOTP1->canMessageACKQueueRunStep();
        ISOTP2->runStep();
        ISOTP2->canMessageACKQueueRunStep();

        if (step == 5)
        {
            EXPECT_TRUE(ISOTP1->N_USData_request(2, N_TATYPE_5_CAN_CLASSIC_29bit_Physical,
                                                    reinterpret_cast<const uint8_t*>(MessageExchangeMF_message1),
                                                    MessageExchangeMF_messageLength1, Mtype_Diagnostics));
            EXPECT_TRUE(ISOTP2->N_USData_request(1, N_TATYPE_5_CAN_CLASSIC_29bit_Physical,
                                                    reinterpret_cast<const uint8_t*>(MessageExchangeMF_message2),
                                                    MessageExchangeMF_messageLength2, Mtype_Diagnostics));
        }
        step++;
    }
    uint32_t elapsedTime = osInterface.osMillis() - initialTime;

    EXPECT_EQ(2, MessageExchangeMF_N_USData_FF_indication_cb_calls);
    EXPECT_EQ(2, MessageExchangeMF_N_USData_confirm_cb_calls);
    EXPECT_EQ(2, MessageExchangeMF_N_USData_indication_cb_calls);

    ASSERT_LT(elapsedTime, TIMEOUT) << "Test took too long: " << elapsedTime << " ms, Timeout was: " << TIMEOUT;

    delete ISOTP1;
    delete ISOTP2;
    delete interface1;
    delete interface2;
}
// END MessageExchangeMF
