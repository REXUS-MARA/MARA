// ======================================================================
// \title  ReconnectingUartDriver.cpp
// \author fsowa
// \brief  cpp file for ReconnectingUartDriver component implementation class
// ======================================================================

#include "Mara/Components/ReconnectingUartDriver/ReconnectingUartDriver.hpp"

#include "Os/TaskString.hpp"

#include <fcntl.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>
#include <cerrno>

namespace Mara {

namespace {
//! How long the read task waits between attempts to open a missing device
const Fw::TimeInterval RECONNECT_INTERVAL(1, 0);
//! How long the read task waits when no receive buffer is available
const Fw::TimeInterval NO_BUFFER_RETRY(0, 50 * 1000);
//! How long one poll() waits for data; also how quickly quitReadThread() takes effect
constexpr int POLL_TIMEOUT_MS = 1000;
}  // namespace

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

ReconnectingUartDriver ::ReconnectingUartDriver(const char* const compName)
    : ReconnectingUartDriverComponentBase(compName) {}

ReconnectingUartDriver ::~ReconnectingUartDriver() {
    if (m_fd >= 0) {
        (void)::close(m_fd);
    }
}

void ReconnectingUartDriver ::configure(const char* device,
                                        UartBaudRate baud,
                                        UartFlowControl flowControl,
                                        UartParity parity,
                                        FwSizeType allocationSize) {
    m_device = device;
    m_baud = baud;
    m_flowControl = flowControl;
    m_parity = parity;
    m_allocationSize = allocationSize;
}

Os::Task::Status ReconnectingUartDriver ::start(FwTaskPriorityType priority, Os::Task::ParamType stackSize) {
    Os::TaskString name("UartReconnect");
    Os::Task::Arguments arguments(name, readTaskEntry, this, priority, stackSize, Os::Task::TASK_DEFAULT);
    return m_readTask.start(arguments);
}

void ReconnectingUartDriver ::quitReadThread() {
    m_quitReadThread = true;
}

Os::Task::Status ReconnectingUartDriver ::join() {
    return m_readTask.join();
}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void ReconnectingUartDriver ::recvReturnIn_handler(FwIndexType portNum, Fw::Buffer& fwBuffer) {
    (void)portNum;
    this->deallocate_out(0, fwBuffer);
}

Drv::ByteStreamStatus ReconnectingUartDriver ::send_handler(FwIndexType portNum, Fw::Buffer& sendBuffer) {
    (void)portNum;
    if (sendBuffer.getData() == nullptr || sendBuffer.getSize() == 0) {
        return Drv::ByteStreamStatus::OTHER_ERROR;
    }

    // Hold the lock through the write, so the read task can't close the fd (and the
    // number be reused by another file) while we write to it
    Os::ScopeLock lock(m_fdLock);
    if (m_fd < 0) {
        return Drv::ByteStreamStatus::OTHER_ERROR;  // not connected; the caller reports it
    }

    const size_t size = static_cast<size_t>(sendBuffer.getSize());
    const ssize_t written = ::write(m_fd, sendBuffer.getData(), size);
    if (written < 0 || static_cast<size_t>(written) != size) {
        // No close here: a dead device also fails the read task's poll, which handles reconnection
        return Drv::ByteStreamStatus::OTHER_ERROR;
    }

    m_bytesSent += static_cast<FwSizeType>(written);
    this->tlmWrite_BytesSent(m_bytesSent);
    return Drv::ByteStreamStatus::OP_OK;
}

// ----------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------

void ReconnectingUartDriver ::readTaskEntry(void* ptr) {
    static_cast<ReconnectingUartDriver*>(ptr)->readLoop();
}

void ReconnectingUartDriver ::readLoop() {
    while (!m_quitReadThread) {
        int fd = -1;
        {
            Os::ScopeLock lock(m_fdLock);
            fd = m_fd;
        }

        // Not connected: try to open. AdapterNotConnected is throttled to once per disconnected period.
        if (fd < 0) {
            fd = tryOpen();
            if (fd < 0) {
                this->log_WARNING_HI_AdapterNotConnected(m_device);
                (void)Os::Task::delay(RECONNECT_INTERVAL);
                continue;
            }
            {
                Os::ScopeLock lock(m_fdLock);
                m_fd = fd;
            }
            this->log_WARNING_HI_AdapterNotConnected_ThrottleClear();
            this->log_ACTIVITY_HI_AdapterConnected(m_device);
            if (this->isConnected_ready_OutputPort(0)) {
                this->ready_out(0);
            }
        }

        // Wait for data. poll() reports a hangup explicitly; a plain read() on a
        // hung-up tty returns 0 forever, which looks like "no data yet".
        struct pollfd pfd = {fd, POLLIN, 0};
        const int ready = ::poll(&pfd, 1, POLL_TIMEOUT_MS);
        if (ready == 0) {
            continue;  // timeout: check the quit flag and wait again
        }
        if (ready < 0) {
            if (errno != EINTR) {
                this->log_WARNING_HI_AdapterDisconnected(m_device, errno);
                closeDevice();
            }
            continue;
        }
        if ((pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
            this->log_WARNING_HI_AdapterDisconnected(m_device, 0);  // hangup: no errno
            closeDevice();
            continue;
        }

        Fw::Buffer buffer = this->allocate_out(0, m_allocationSize);
        if (buffer.getData() == nullptr) {
            this->log_WARNING_HI_NoBuffers(m_device);  // throttled to once until a buffer is available
            (void)Os::Task::delay(NO_BUFFER_RETRY);
            continue;
        }
        this->log_WARNING_HI_NoBuffers_ThrottleClear();

        const ssize_t received = ::read(fd, buffer.getData(), static_cast<size_t>(buffer.getSize()));
        if (received > 0) {
            buffer.setSize(static_cast<FwSizeType>(received));
            m_bytesReceived += static_cast<FwSizeType>(received);
            this->tlmWrite_BytesReceived(m_bytesReceived);
            this->recv_out(0, buffer, Drv::ByteStreamStatus::OP_OK);
            continue;
        }

        this->deallocate_out(0, buffer);
        if (received == 0 || (errno != EAGAIN && errno != EINTR)) {
            // Readable but nothing to read (EOF) or a read error: the device is gone
            this->log_WARNING_HI_AdapterDisconnected(m_device, (received == 0) ? 0 : errno);
            closeDevice();
        }
    }
}

int ReconnectingUartDriver ::tryOpen() const {
    speed_t speed = B0;
    switch (m_baud) {
        case BAUD_9600:
            speed = B9600;
            break;
        case BAUD_19200:
            speed = B19200;
            break;
        case BAUD_38400:
            speed = B38400;
            break;
        case BAUD_57600:
            speed = B57600;
            break;
        case BAUD_115K:
            speed = B115200;
            break;
        case BAUD_230K:
            speed = B230400;
            break;
#ifdef TGT_OS_TYPE_LINUX
        case BAUD_460K:
            speed = B460800;
            break;
        case BAUD_921K:
            speed = B921600;
            break;
        case BAUD_1000K:
            speed = B1000000;
            break;
        case BAUD_1500K:
            speed = B1500000;
            break;
        case BAUD_2000K:
            speed = B2000000;
            break;
#endif
        default:
            return -1;
    }

    // O_NONBLOCK so open() can't hang on a serial line; poll() does the waiting
    const int fd = ::open(m_device.toChar(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        return -1;
    }

    struct termios cfg;
    if (::tcgetattr(fd, &cfg) != 0) {
        (void)::close(fd);
        return -1;
    }

    // Raw mode: 8 data bits, no parity, no echo, no line editing or character translation
    ::cfmakeraw(&cfg);
    // Receiver on, ignore modem control lines
    cfg.c_cflag |= CLOCAL | CREAD;
    cfg.c_cflag &= static_cast<tcflag_t>(~CRTSCTS);
    if (m_parity != PARITY_NONE) {
        cfg.c_cflag |= PARENB;
        cfg.c_iflag |= INPCK;
        if (m_parity == PARITY_ODD) {
            cfg.c_cflag |= PARODD;
        }
    }
    if (m_flowControl == HW_FLOW) {
        cfg.c_cflag |= CRTSCTS;
    }
    // read() returns what is available; poll() does the waiting
    cfg.c_cc[VMIN] = 0;
    cfg.c_cc[VTIME] = 0;

    if (::cfsetispeed(&cfg, speed) != 0 || ::cfsetospeed(&cfg, speed) != 0 ||
        ::tcsetattr(fd, TCSANOW, &cfg) != 0) {
        (void)::close(fd);
        return -1;
    }
    (void)::tcflush(fd, TCIFLUSH);
    return fd;
}

void ReconnectingUartDriver ::closeDevice() {
    Os::ScopeLock lock(m_fdLock);
    if (m_fd >= 0) {
        (void)::close(m_fd);
        m_fd = -1;
    }
}

}  // namespace Mara
