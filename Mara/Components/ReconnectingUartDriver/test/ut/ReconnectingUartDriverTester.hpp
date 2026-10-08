// ======================================================================
// \title  ReconnectingUartDriverTester.hpp
// \author fsowa
// \brief  hpp file for ReconnectingUartDriver component test harness implementation class
//
// The driver's read task runs for real against pseudo-terminals. The "adapter" is a
// pty behind a symlink in a temp dir: plug = new pty + link, unplug = remove the link
// and close the pty (a hangup, as when a USB serial adapter is pulled).
//
// Everything the read task calls in the tester (ports, events, text log) takes m_lock,
// and the test thread only inspects state under the same lock, so the two threads
// never race on the histories.
// ======================================================================

#ifndef Mara_ReconnectingUartDriverTester_HPP
#define Mara_ReconnectingUartDriverTester_HPP

#include "Mara/Components/ReconnectingUartDriver/ReconnectingUartDriver.hpp"
#include "Mara/Components/ReconnectingUartDriver/ReconnectingUartDriverGTestBase.hpp"

#include <array>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace Mara {

class ReconnectingUartDriverTester final : public ReconnectingUartDriverGTestBase {
  public:
    // ----------------------------------------------------------------------
    // Constants
    // ----------------------------------------------------------------------

    // Maximum size of histories storing events, telemetry, and port outputs
    static const FwSizeType MAX_HISTORY_SIZE = 1000;

    // Instance ID supplied to the component instance under test
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    //! Size of each receive buffer handed to the driver
    static constexpr FwSizeType BUFFER_SIZE = 64;

    //! Number of receive buffers in the tester's pool
    static constexpr FwSizeType BUFFER_COUNT = 8;

  public:
    // ----------------------------------------------------------------------
    // Construction and destruction
    // ----------------------------------------------------------------------

    //! Construct object ReconnectingUartDriverTester
    ReconnectingUartDriverTester();

    //! Destroy object ReconnectingUartDriverTester. Always stops the read task.
    ~ReconnectingUartDriverTester();

  public:
    // ----------------------------------------------------------------------
    // Tests
    // ----------------------------------------------------------------------

    //! No device at start: one AdapterNotConnected, sends fail, nothing allocated
    void testNotConnectedAtStart();

    //! Plugging in connects; data flows both ways; telemetry counts it
    void testConnectReceiveSend();

    //! Three unplug/replug cycles: one event each way per cycle, no flood, data works again
    void testUnplugReplugCycles();

    //! No buffers: one NoBuffers per episode; data is delivered once buffers come back
    void testNoBuffers();

    //! quitReadThread + join return promptly, connected or not
    void testShutdown(bool connected);

    //! An empty send is rejected
    void testEmptySend();

    //! The device opens with every supported baud rate, parity and flow control setting
    void testAllSettingsConnect(ReconnectingUartDriver::UartBaudRate baud,
                                ReconnectingUartDriver::UartParity parity,
                                ReconnectingUartDriver::UartFlowControl flowControl);

    //! A device path that is not a serial port never connects, and nothing asserts
    void testNotASerialPort();

    //! An unsupported baud rate never connects, and nothing asserts
    void testUnsupportedBaud();

    //! A full output buffer makes $send fail without an event or a disconnect; it recovers
    void testSendBufferFull();

  private:
    // ----------------------------------------------------------------------
    // Handlers for typed from ports (called on the driver's read task)
    // ----------------------------------------------------------------------

    Fw::Buffer from_allocate_handler(FwIndexType portNum, FwSizeType size) override;
    void from_deallocate_handler(FwIndexType portNum, Fw::Buffer& fwBuffer) override;
    void from_ready_handler(FwIndexType portNum) override;
    void from_recv_handler(FwIndexType portNum, Fw::Buffer& buffer, const Drv::ByteStreamStatus& status) override;

    // ----------------------------------------------------------------------
    // Event and text log handlers (also called on the read task)
    // ----------------------------------------------------------------------

    void logIn_WARNING_HI_AdapterNotConnected(const Fw::StringBase& device) override;
    void logIn_ACTIVITY_HI_AdapterConnected(const Fw::StringBase& device) override;
    void logIn_WARNING_HI_AdapterDisconnected(const Fw::StringBase& device, I32 err) override;
    void logIn_WARNING_HI_NoBuffers(const Fw::StringBase& device) override;
    void textLogIn(FwEventIdType id,
                   const Fw::Time& timeTag,
                   const Fw::LogSeverity severity,
                   const Fw::TextLogString& text) override;

  private:
    // ----------------------------------------------------------------------
    // Helper functions
    // ----------------------------------------------------------------------

    //! Connect ports
    void connectPorts();

    //! Initialize components
    void initComponents();

    //! Start the driver's read task
    void start();

    //! Stop and join the read task (once). \return how long it took, in ms
    U32 stop();

    //! Create a new pty and link it at the configured device path
    void plug();

    //! Remove the link and close the pty
    void unplug();

    //! Write bytes into the "adapter" (they arrive at the driver)
    void writeToDriver(const std::string& bytes);

    //! Read bytes the driver sent, waiting up to timeoutMs
    std::string readFromDriver(FwSizeType count, U32 timeoutMs);

    //! Send bytes through the driver's $send port
    Drv::ByteStreamStatus send(const std::string& bytes);

    //! Wait until predicate (evaluated under m_lock) holds. \return whether it did in time
    bool waitFor(const std::function<bool()>& predicate, U32 timeoutMs);

    //! Sleep without holding the lock
    static void sleepMs(U32 ms);

    //! Event counts and received data, read under m_lock
    FwSizeType notConnectedCount();
    FwSizeType connectedCount();
    FwSizeType disconnectedCount();
    FwSizeType noBuffersCount();
    std::string received();

  private:
    // ----------------------------------------------------------------------
    // Member variables
    // ----------------------------------------------------------------------

    //! The component under test
    ReconnectingUartDriver component;

    //! Guards everything the read task touches in the tester
    std::recursive_mutex m_lock;

    //! Temp dir holding the device link, and the link path the driver opens
    std::string m_dir;
    std::string m_link;

    //! pty master of the plugged-in "adapter", or -1
    int m_master = -1;

    //! Read task lifecycle
    bool m_started = false;
    bool m_stopped = false;

    //! Receive buffer pool
    std::array<std::array<U8, BUFFER_SIZE>, BUFFER_COUNT> m_pool{};
    std::array<bool, BUFFER_COUNT> m_inUse{};
    bool m_allocateValid = true;
    FwSizeType m_allocated = 0;
    FwSizeType m_deallocated = 0;

    //! What arrived on $recv
    std::string m_received;
    FwSizeType m_recvNotOk = 0;
    FwSizeType m_readyCount = 0;
};

}  // namespace Mara

#endif
