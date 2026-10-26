// ======================================================================
// \title  ReconnectingUartDriverTester.cpp
// \author fsowa
// \brief  cpp file for ReconnectingUartDriver component test harness implementation class
// ======================================================================

#include "ReconnectingUartDriverTester.hpp"

#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <chrono>
#include <cstdlib>
#include <thread>

namespace Mara {

// Out-of-line definitions: C++14 needs them when gtest binds the constants by reference
constexpr FwSizeType ReconnectingUartDriverTester::BUFFER_SIZE;
constexpr FwSizeType ReconnectingUartDriverTester::BUFFER_COUNT;

namespace {
//! Longest the driver should need to notice a plug or unplug: 1 s retry or poll interval, plus margin
constexpr U32 DETECT_MS = 3000;
}  // namespace

// ----------------------------------------------------------------------
// Construction and destruction
// ----------------------------------------------------------------------

ReconnectingUartDriverTester ::ReconnectingUartDriverTester()
    : ReconnectingUartDriverGTestBase("ReconnectingUartDriverTester", ReconnectingUartDriverTester::MAX_HISTORY_SIZE),
      component("ReconnectingUartDriver") {
    this->initComponents();
    this->connectPorts();

    std::string dirTemplate = "/tmp/mara_uart_ut_XXXXXX";
    std::vector<char> dir(dirTemplate.begin(), dirTemplate.end());
    dir.push_back('\0');
    EXPECT_NE(::mkdtemp(dir.data()), nullptr);
    m_dir = dir.data();
    m_link = m_dir + "/adapter";

    this->component.configure(m_link.c_str(), ReconnectingUartDriver::BAUD_115K, ReconnectingUartDriver::NO_FLOW,
                              ReconnectingUartDriver::PARITY_NONE, BUFFER_SIZE);
}

ReconnectingUartDriverTester ::~ReconnectingUartDriverTester() {
    (void)this->stop();
    this->unplug();
    (void)::rmdir(m_dir.c_str());
}

// ----------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------

void ReconnectingUartDriverTester ::testNotConnectedAtStart() {
    this->start();
    sleepMs(3500);  // three or four reconnect attempts

    ASSERT_EQ(this->notConnectedCount(), 1U) << "AdapterNotConnected must be logged once per disconnected period";
    ASSERT_EQ(this->connectedCount(), 0U);
    {
        std::lock_guard<std::recursive_mutex> lock(m_lock);
        ASSERT_EQ(m_allocated, 0U) << "no buffers should be taken while disconnected";
    }
    ASSERT_EQ(this->send("ping"), Drv::ByteStreamStatus::OTHER_ERROR);
}

void ReconnectingUartDriverTester ::testConnectReceiveSend() {
    this->start();
    sleepMs(1200);
    ASSERT_EQ(this->notConnectedCount(), 1U);

    this->plug();
    ASSERT_TRUE(this->waitFor([this]() { return this->eventHistory_AdapterConnected->size() == 1; }, DETECT_MS));
    {
        std::lock_guard<std::recursive_mutex> lock(m_lock);
        ASSERT_EQ(m_readyCount, 1U);
    }

    // Adapter -> driver
    this->writeToDriver("HELLO");
    ASSERT_TRUE(this->waitFor([this]() { return m_received.size() >= 5; }, DETECT_MS));
    {
        std::lock_guard<std::recursive_mutex> lock(m_lock);
        ASSERT_EQ(m_received, "HELLO");
        ASSERT_EQ(m_recvNotOk, 0U);
        ASSERT_GE(this->tlmHistory_BytesReceived->size(), 1U);
        ASSERT_EQ(this->tlmHistory_BytesReceived->at(this->tlmHistory_BytesReceived->size() - 1).arg, 5U);
    }

    // Driver -> adapter: a full 13-byte Waveshare frame, including 0x00 bytes
    const std::string frame("\xAA\xC8\x01\x06\x2B\x40\x60\x00\x0F\x00\x00\x00\x55", 13);
    ASSERT_EQ(this->send(frame), Drv::ByteStreamStatus::OP_OK);
    ASSERT_EQ(this->readFromDriver(frame.size(), 1000), frame);
    {
        std::lock_guard<std::recursive_mutex> lock(m_lock);
        ASSERT_GE(this->tlmHistory_BytesSent->size(), 1U);  // guard: History::at asserts on a bad index
        ASSERT_EQ(this->tlmHistory_BytesSent->at(this->tlmHistory_BytesSent->size() - 1).arg, 13U);
    }

    (void)this->stop();
    std::lock_guard<std::recursive_mutex> lock(m_lock);
    ASSERT_EQ(m_allocated, m_deallocated) << "every receive buffer must be returned";
}

void ReconnectingUartDriverTester ::testUnplugReplugCycles() {
    this->plug();
    this->start();
    ASSERT_TRUE(this->waitFor([this]() { return this->eventHistory_AdapterConnected->size() == 1; }, DETECT_MS));
    ASSERT_EQ(this->notConnectedCount(), 0U);

    for (FwSizeType cycle = 1; cycle <= 3; cycle++) {
        this->unplug();
        ASSERT_TRUE(
            this->waitFor([this, cycle]() { return this->eventHistory_AdapterDisconnected->size() == cycle; }, DETECT_MS))
            << "cycle " << cycle << ": no AdapterDisconnected";
        ASSERT_TRUE(
            this->waitFor([this, cycle]() { return this->eventHistory_AdapterNotConnected->size() == cycle; }, DETECT_MS))
            << "cycle " << cycle << ": no AdapterNotConnected";

        // Stay unplugged for a while: no event flood
        sleepMs(2500);
        ASSERT_EQ(this->disconnectedCount(), cycle);
        ASSERT_EQ(this->notConnectedCount(), cycle);
        ASSERT_EQ(this->send("x"), Drv::ByteStreamStatus::OTHER_ERROR);

        this->plug();
        ASSERT_TRUE(
            this->waitFor([this, cycle]() { return this->eventHistory_AdapterConnected->size() == cycle + 1; }, DETECT_MS))
            << "cycle " << cycle << ": no AdapterConnected after replug";

        // Data path works again, both ways
        const std::string marker = "cycle" + std::to_string(cycle);
        this->writeToDriver(marker);
        ASSERT_TRUE(this->waitFor(
            [this, &marker]() {
                return m_received.size() >= marker.size() &&
                       m_received.compare(m_received.size() - marker.size(), marker.size(), marker) == 0;
            },
            DETECT_MS));
        ASSERT_EQ(this->send(marker), Drv::ByteStreamStatus::OP_OK);
        ASSERT_EQ(this->readFromDriver(marker.size(), 1000), marker);
    }

    (void)this->stop();
    std::lock_guard<std::recursive_mutex> lock(m_lock);
    ASSERT_EQ(m_allocated, m_deallocated) << "every receive buffer must be returned";
    ASSERT_EQ(m_recvNotOk, 0U);
}

void ReconnectingUartDriverTester ::testNoBuffers() {
    {
        std::lock_guard<std::recursive_mutex> lock(m_lock);
        m_allocateValid = false;
    }
    this->plug();
    this->start();
    ASSERT_TRUE(this->waitFor([this]() { return this->eventHistory_AdapterConnected->size() == 1; }, DETECT_MS));

    // Data waiting but no buffer: one warning, even though the driver keeps retrying
    this->writeToDriver("AB");
    ASSERT_TRUE(this->waitFor([this]() { return this->eventHistory_NoBuffers->size() == 1; }, DETECT_MS));
    sleepMs(1000);
    ASSERT_EQ(this->noBuffersCount(), 1U);
    ASSERT_EQ(this->received(), "");

    // Buffers come back: the waiting data is delivered
    {
        std::lock_guard<std::recursive_mutex> lock(m_lock);
        m_allocateValid = true;
    }
    ASSERT_TRUE(this->waitFor([this]() { return m_received == "AB"; }, DETECT_MS));

    // A new episode warns again (the throttle was cleared when a buffer was available)
    {
        std::lock_guard<std::recursive_mutex> lock(m_lock);
        m_allocateValid = false;
    }
    this->writeToDriver("C");
    ASSERT_TRUE(this->waitFor([this]() { return this->eventHistory_NoBuffers->size() == 2; }, DETECT_MS));
    {
        std::lock_guard<std::recursive_mutex> lock(m_lock);
        m_allocateValid = true;
    }
    ASSERT_TRUE(this->waitFor([this]() { return m_received == "ABC"; }, DETECT_MS));

    (void)this->stop();
    std::lock_guard<std::recursive_mutex> lock(m_lock);
    ASSERT_EQ(m_allocated, m_deallocated);
}

void ReconnectingUartDriverTester ::testShutdown(bool connected) {
    if (connected) {
        this->plug();
    }
    this->start();
    if (connected) {
        ASSERT_TRUE(this->waitFor([this]() { return this->eventHistory_AdapterConnected->size() == 1; }, DETECT_MS));
    } else {
        sleepMs(1500);
    }

    // At most one poll interval or reconnect delay (1 s), plus margin
    const U32 elapsed = this->stop();
    ASSERT_LT(elapsed, 2000U) << "read task took " << elapsed << " ms to stop";
}

void ReconnectingUartDriverTester ::testEmptySend() {
    Fw::Buffer empty;
    ASSERT_EQ(this->invoke_to_send(0, empty), Drv::ByteStreamStatus::OTHER_ERROR);

    U8 byte = 0;
    Fw::Buffer zeroSize(&byte, 0);
    ASSERT_EQ(this->invoke_to_send(0, zeroSize), Drv::ByteStreamStatus::OTHER_ERROR);
}

void ReconnectingUartDriverTester ::testAllSettingsConnect(ReconnectingUartDriver::UartBaudRate baud,
                                                           ReconnectingUartDriver::UartParity parity,
                                                           ReconnectingUartDriver::UartFlowControl flowControl) {
    this->component.configure(m_link.c_str(), baud, flowControl, parity, BUFFER_SIZE);
    this->plug();
    this->start();
    ASSERT_TRUE(this->waitFor([this]() { return this->eventHistory_AdapterConnected->size() == 1; }, DETECT_MS))
        << "baud " << static_cast<U32>(baud) << ", parity " << parity << ", flow " << flowControl;
    ASSERT_EQ(this->notConnectedCount(), 0U);
}

void ReconnectingUartDriverTester ::testNotASerialPort() {
    // Opens fine, but has no terminal attributes
    ASSERT_EQ(::symlink("/dev/null", m_link.c_str()), 0);
    this->start();
    sleepMs(2500);
    ASSERT_EQ(this->notConnectedCount(), 1U);
    ASSERT_EQ(this->connectedCount(), 0U);
    ASSERT_EQ(this->send("x"), Drv::ByteStreamStatus::OTHER_ERROR);
}

void ReconnectingUartDriverTester ::testUnsupportedBaud() {
    this->component.configure(m_link.c_str(), static_cast<ReconnectingUartDriver::UartBaudRate>(12345),
                              ReconnectingUartDriver::NO_FLOW, ReconnectingUartDriver::PARITY_NONE, BUFFER_SIZE);
    this->plug();
    this->start();
    sleepMs(2500);
    ASSERT_EQ(this->notConnectedCount(), 1U);
    ASSERT_EQ(this->connectedCount(), 0U);
}

void ReconnectingUartDriverTester ::testSendBufferFull() {
    this->plug();
    this->start();
    ASSERT_TRUE(this->waitFor([this]() { return this->eventHistory_AdapterConnected->size() == 1; }, DETECT_MS));

    // Nobody reads the "adapter": the driver's non-blocking writes eventually fail
    const std::string frame(13, 'F');
    FwSizeType sent = 0;
    Drv::ByteStreamStatus status = Drv::ByteStreamStatus::OP_OK;
    while (status == Drv::ByteStreamStatus::OP_OK && sent < 1000000) {
        status = this->send(frame);
        sent += (status == Drv::ByteStreamStatus::OP_OK) ? 1 : 0;
        if (sent % 500 == 0) {
            // Each send writes BytesSent telemetry; a Linux pty buffers far more than the
            // history holds. Only this thread writes telemetry here (no data is incoming).
            std::lock_guard<std::recursive_mutex> lock(m_lock);
            this->clearTlm();
        }
    }
    ASSERT_EQ(status, Drv::ByteStreamStatus::OTHER_ERROR) << "output buffer never filled";
    ASSERT_GT(sent, 0U);

    // A send failure is the caller's to report: no event, and the device stays connected
    sleepMs(1500);
    ASSERT_EQ(this->disconnectedCount(), 0U);
    ASSERT_EQ(this->notConnectedCount(), 0U);

    // Drain the "adapter": sending works again
    (void)this->readFromDriver(sent * frame.size(), 2000);
    ASSERT_EQ(this->send(frame), Drv::ByteStreamStatus::OP_OK);
    ASSERT_EQ(this->readFromDriver(frame.size(), 1000), frame);
}

// ----------------------------------------------------------------------
// Handlers for typed from ports
// ----------------------------------------------------------------------

Fw::Buffer ReconnectingUartDriverTester ::from_allocate_handler(FwIndexType portNum, FwSizeType size) {
    (void)portNum;
    std::lock_guard<std::recursive_mutex> lock(m_lock);
    EXPECT_EQ(size, BUFFER_SIZE);
    if (!m_allocateValid) {
        return Fw::Buffer();
    }
    for (FwSizeType i = 0; i < BUFFER_COUNT; i++) {
        if (!m_inUse[i]) {
            m_inUse[i] = true;
            m_allocated++;
            return Fw::Buffer(m_pool[i].data(), BUFFER_SIZE, static_cast<U32>(i));
        }
    }
    ADD_FAILURE() << "receive buffer pool exhausted: buffers are not being returned";
    return Fw::Buffer();
}

void ReconnectingUartDriverTester ::from_deallocate_handler(FwIndexType portNum, Fw::Buffer& fwBuffer) {
    (void)portNum;
    std::lock_guard<std::recursive_mutex> lock(m_lock);
    const U32 slot = fwBuffer.getContext();
    EXPECT_LT(slot, BUFFER_COUNT);
    if (slot < BUFFER_COUNT) {
        EXPECT_TRUE(m_inUse[slot]) << "buffer returned twice";
        m_inUse[slot] = false;
    }
    m_deallocated++;
}

void ReconnectingUartDriverTester ::from_ready_handler(FwIndexType portNum) {
    (void)portNum;
    std::lock_guard<std::recursive_mutex> lock(m_lock);
    m_readyCount++;
}

void ReconnectingUartDriverTester ::from_recv_handler(FwIndexType portNum,
                                                      Fw::Buffer& buffer,
                                                      const Drv::ByteStreamStatus& status) {
    (void)portNum;
    std::lock_guard<std::recursive_mutex> lock(m_lock);
    if (status == Drv::ByteStreamStatus::OP_OK) {
        m_received.append(reinterpret_cast<const char*>(buffer.getData()), static_cast<size_t>(buffer.getSize()));
    } else {
        m_recvNotOk++;
    }
    // Like a real consumer, give the buffer back
    this->invoke_to_recvReturnIn(0, buffer);
}

// ----------------------------------------------------------------------
// Event and text log handlers
// ----------------------------------------------------------------------

void ReconnectingUartDriverTester ::logIn_WARNING_HI_AdapterNotConnected(const Fw::StringBase& device) {
    std::lock_guard<std::recursive_mutex> lock(m_lock);
    EXPECT_STREQ(device.toChar(), m_link.c_str());
    ReconnectingUartDriverTesterBase::logIn_WARNING_HI_AdapterNotConnected(device);
}

void ReconnectingUartDriverTester ::logIn_ACTIVITY_HI_AdapterConnected(const Fw::StringBase& device) {
    std::lock_guard<std::recursive_mutex> lock(m_lock);
    ReconnectingUartDriverTesterBase::logIn_ACTIVITY_HI_AdapterConnected(device);
}

void ReconnectingUartDriverTester ::logIn_WARNING_HI_AdapterDisconnected(const Fw::StringBase& device, I32 err) {
    std::lock_guard<std::recursive_mutex> lock(m_lock);
    ReconnectingUartDriverTesterBase::logIn_WARNING_HI_AdapterDisconnected(device, err);
}

void ReconnectingUartDriverTester ::logIn_WARNING_HI_NoBuffers(const Fw::StringBase& device) {
    std::lock_guard<std::recursive_mutex> lock(m_lock);
    ReconnectingUartDriverTesterBase::logIn_WARNING_HI_NoBuffers(device);
}

void ReconnectingUartDriverTester ::textLogIn(FwEventIdType id,
                                              const Fw::Time& timeTag,
                                              const Fw::LogSeverity severity,
                                              const Fw::TextLogString& text) {
    std::lock_guard<std::recursive_mutex> lock(m_lock);
    ReconnectingUartDriverTesterBase::textLogIn(id, timeTag, severity, text);
}

// ----------------------------------------------------------------------
// Helper functions
// ----------------------------------------------------------------------

void ReconnectingUartDriverTester ::start() {
    ASSERT_EQ(this->component.start(), Os::Task::OP_OK);
    m_started = true;
}

U32 ReconnectingUartDriverTester ::stop() {
    if (!m_started || m_stopped) {
        return 0;
    }
    const auto begin = std::chrono::steady_clock::now();
    this->component.quitReadThread();
    (void)this->component.join();
    m_stopped = true;
    return static_cast<U32>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - begin).count());
}

void ReconnectingUartDriverTester ::plug() {
    ASSERT_LT(m_master, 0) << "already plugged in";
    const int master = ::posix_openpt(O_RDWR | O_NOCTTY);
    ASSERT_GE(master, 0);
    ASSERT_EQ(::grantpt(master), 0);
    ASSERT_EQ(::unlockpt(master), 0);
    const char* slave = ::ptsname(master);
    ASSERT_NE(slave, nullptr);
    ASSERT_EQ(::symlink(slave, m_link.c_str()), 0);
    m_master = master;
}

void ReconnectingUartDriverTester ::unplug() {
    (void)::unlink(m_link.c_str());  // first, so the driver can't reopen the old pty
    if (m_master >= 0) {
        (void)::close(m_master);  // hangs up the driver's side
        m_master = -1;
    }
}

void ReconnectingUartDriverTester ::writeToDriver(const std::string& bytes) {
    ASSERT_GE(m_master, 0);
    ASSERT_EQ(::write(m_master, bytes.data(), bytes.size()), static_cast<ssize_t>(bytes.size()));
}

std::string ReconnectingUartDriverTester ::readFromDriver(FwSizeType count, U32 timeoutMs) {
    std::string out;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (out.size() < count && std::chrono::steady_clock::now() < deadline) {
        struct pollfd pfd = {m_master, POLLIN, 0};
        if (::poll(&pfd, 1, 50) > 0 && (pfd.revents & POLLIN) != 0) {
            std::array<char, 64> chunk{};
            const ssize_t n = ::read(m_master, chunk.data(), chunk.size());
            if (n > 0) {
                out.append(chunk.data(), static_cast<size_t>(n));
            }
        }
    }
    return out;
}

Drv::ByteStreamStatus ReconnectingUartDriverTester ::send(const std::string& bytes) {
    std::vector<U8> data(bytes.begin(), bytes.end());
    Fw::Buffer buffer(data.data(), data.size());
    return this->invoke_to_send(0, buffer);
}

bool ReconnectingUartDriverTester ::waitFor(const std::function<bool()>& predicate, U32 timeoutMs) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (true) {
        {
            std::lock_guard<std::recursive_mutex> lock(m_lock);
            if (predicate()) {
                return true;
            }
        }
        if (std::chrono::steady_clock::now() >= deadline) {
            return false;
        }
        sleepMs(20);
    }
}

void ReconnectingUartDriverTester ::sleepMs(U32 ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

FwSizeType ReconnectingUartDriverTester ::notConnectedCount() {
    std::lock_guard<std::recursive_mutex> lock(m_lock);
    return this->eventHistory_AdapterNotConnected->size();
}

FwSizeType ReconnectingUartDriverTester ::connectedCount() {
    std::lock_guard<std::recursive_mutex> lock(m_lock);
    return this->eventHistory_AdapterConnected->size();
}

FwSizeType ReconnectingUartDriverTester ::disconnectedCount() {
    std::lock_guard<std::recursive_mutex> lock(m_lock);
    return this->eventHistory_AdapterDisconnected->size();
}

FwSizeType ReconnectingUartDriverTester ::noBuffersCount() {
    std::lock_guard<std::recursive_mutex> lock(m_lock);
    return this->eventHistory_NoBuffers->size();
}

std::string ReconnectingUartDriverTester ::received() {
    std::lock_guard<std::recursive_mutex> lock(m_lock);
    return m_received;
}

}  // namespace Mara
