// ======================================================================
// \title  ReconnectingUartDriver.hpp
// \author fsowa
// \brief  hpp file for ReconnectingUartDriver component implementation class
// ======================================================================

#ifndef Mara_ReconnectingUartDriver_HPP
#define Mara_ReconnectingUartDriver_HPP

#include "Mara/Components/ReconnectingUartDriver/ReconnectingUartDriverComponentAc.hpp"

#include "Fw/Types/String.hpp"
#include "Os/Mutex.hpp"
#include "Os/Task.hpp"

#include <atomic>

namespace Mara {

class ReconnectingUartDriver final : public ReconnectingUartDriverComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct ReconnectingUartDriver object
    ReconnectingUartDriver(const char* const compName  //!< The component name
    );

    //! Destroy ReconnectingUartDriver object
    ~ReconnectingUartDriver();

    //! Baud rates. Rates above 230400 only exist on Linux.
    enum UartBaudRate {
        BAUD_9600 = 9600,
        BAUD_19200 = 19200,
        BAUD_38400 = 38400,
        BAUD_57600 = 57600,
        BAUD_115K = 115200,
        BAUD_230K = 230400,
#ifdef TGT_OS_TYPE_LINUX
        BAUD_460K = 460800,
        BAUD_921K = 921600,
        BAUD_1000K = 1000000,
        BAUD_1500K = 1500000,
        BAUD_2000K = 2000000,
#endif
    };

    enum UartFlowControl { NO_FLOW, HW_FLOW };

    enum UartParity { PARITY_NONE, PARITY_ODD, PARITY_EVEN };

    //! Store the serial settings. Does not open the device: the read task does that,
    //! and keeps retrying until the device is present.
    void configure(const char* device,
                   UartBaudRate baud,
                   UartFlowControl flowControl,
                   UartParity parity,
                   FwSizeType allocationSize  //!< size of each receive buffer
    );

    //! Start the read task, which connects, reconnects and receives
    Os::Task::Status start(FwTaskPriorityType priority = Os::Task::TASK_PRIORITY_DEFAULT,
                           Os::Task::ParamType stackSize = Os::Task::TASK_DEFAULT);

    //! Ask the read task to stop. It notices within one poll interval (1 s).
    void quitReadThread();

    //! Wait for the read task to stop
    Os::Task::Status join();

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for recvReturnIn
    //!
    //! Port receiving back ownership of data sent out on $recv port
    void recvReturnIn_handler(FwIndexType portNum,  //!< The port number
                              Fw::Buffer& fwBuffer  //!< The buffer
                              ) override;

    //! Handler implementation for send
    //!
    //! Invoke this port to send data out the driver (synchronous)
    //! Status is returned, and ownership of the buffer is retained by the caller
    Drv::ByteStreamStatus send_handler(FwIndexType portNum,    //!< The port number
                                       Fw::Buffer& sendBuffer  //!< Data to send
                                       ) override;

  private:
    // ----------------------------------------------------------------------
    // Helpers
    // ----------------------------------------------------------------------

    //! Read task entry point
    static void readTaskEntry(void* ptr);

    //! Read task body: connect, then receive until the device goes away; repeat
    void readLoop();

    //! Open and configure the device. Logs nothing.
    //! \return the file descriptor, or -1
    int tryOpen() const;

    //! Close the device after it went away
    void closeDevice();

    Fw::String m_device;                    //!< device path
    UartBaudRate m_baud = BAUD_9600;        //!< baud rate
    UartFlowControl m_flowControl = NO_FLOW;
    UartParity m_parity = PARITY_NONE;
    FwSizeType m_allocationSize = 0;        //!< size of each receive buffer

    Os::Mutex m_fdLock;  //!< guards m_fd: opened/closed by the read task, written by send
    int m_fd = -1;       //!< open device, or -1 when not connected

    Os::Task m_readTask;                       //!< connects and receives
    std::atomic<bool> m_quitReadThread{false}; //!< asks the read task to stop
    std::atomic<FwSizeType> m_bytesSent{0};
    std::atomic<FwSizeType> m_bytesReceived{0};
};

}  // namespace Mara

#endif
