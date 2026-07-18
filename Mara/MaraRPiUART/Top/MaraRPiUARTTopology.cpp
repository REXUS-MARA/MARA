// ======================================================================
// \title  MaraRPiUARTTopology.cpp
// \brief cpp file containing the topology instantiation code
//
// ======================================================================
// Provides access to autocoded functions
#include <Mara/MaraRPiUART/Top/MaraRPiUARTTopologyAc.hpp>
// Note: Uncomment when using Svc:TlmPacketizer
//#include <Mara/MaraRPiUART/Top/MaraRPiUARTPacketsAc.hpp>

// Necessary project-specified types
#include <Fw/Types/MallocAllocator.hpp>
#include <Fw/Logger/Logger.hpp>

// Public functions for use in main program are namespaced with deployment module Mara
// This is also the namespace where the topology components are instantiated by FPP.
namespace Mara {

// Instantiate a malloc allocator for cmdSeq buffer allocation
Fw::MallocAllocator mallocator;

// The reference topology divides the incoming clock signal (1Hz) into sub-signals: 1Hz, 1/2Hz, and 1/4Hz with 0 offset
// For our current needs, I set up a clock signal of 100Hz (you can change that in the Main.cpp file)
// That will get spili into sub signals - 1Hz (100 / 100), 20hz (100/5), 1/4Hz, 100Hz (100/1)
Svc::RateGroupDriver::DividerSet rateGroupDivisorsSet{{{100, 0}, {5, 0}, {400, 0}, {1, 0}}};

// Rate groups may supply a context token to each of the attached children whose purpose is set by the project. The
// reference topology sets each token to zero as these contexts are unused in this project.
U32 rateGroup1Context[Svc::ActiveRateGroup::CONNECTION_COUNT_MAX] = {};
U32 rateGroup2Context[Svc::ActiveRateGroup::CONNECTION_COUNT_MAX] = {};
U32 rateGroup3Context[Svc::ActiveRateGroup::CONNECTION_COUNT_MAX] = {};
U32 rateGroup4Context[Svc::ActiveRateGroup::CONNECTION_COUNT_MAX] = {};

enum TopologyConstants {
    COMM_PRIORITY = 34,
};

/**
 * \brief configure/setup components in project-specific way
 *
 * This is a *helper* function which configures/sets up each component requiring project specific input. This includes
 * allocating resources, passing-in arguments, etc. This function may be inlined into the topology setup function if
 * desired, but is extracted here for clarity.
 */
void configureTopology() {
    // Rate group driver needs a divisor list
    rateGroupDriver.configure(rateGroupDivisorsSet);

    // Rate groups require context arrays.
    rateGroup1.configure(rateGroup1Context, FW_NUM_ARRAY_ELEMENTS(rateGroup1Context));
    rateGroup2.configure(rateGroup2Context, FW_NUM_ARRAY_ELEMENTS(rateGroup2Context));
    rateGroup3.configure(rateGroup3Context, FW_NUM_ARRAY_ELEMENTS(rateGroup3Context));
    rateGroup4.configure(rateGroup4Context, FW_NUM_ARRAY_ELEMENTS(rateGroup4Context));

    // Command sequencer needs to allocate memory to hold contents of command sequences
    cmdSeq.allocateBuffer(0, mallocator, 5 * 1024);


    if (not I2CDriver.open("/dev/i2c-1")) {
        Fw::Logger::log("[ERROR] I2C driver open failed\n");
    }
    else {
        Fw::Logger::log("[INFO] I2C driver open successful\n");
    }

    ltrManager.configure(0x29); // Device I2C address from datasheet

    Os::File::Status EODSstatus =
        EODSgpioDriver.open("/dev/gpiochip4", EODSGpioPin, Drv::LinuxGpioDriver::GpioConfiguration::GPIO_INPUT);
    if (EODSstatus != Os::File::Status::OP_OK) {
        Fw::Logger::log("[ERROR] Failed to open GPIO pin for EODS line\n");
    }

    Os::File::Status LOstatus =
        LOgpioDriver.open("/dev/gpiochip4", LOGpioPin, Drv::LinuxGpioDriver::GpioConfiguration::GPIO_INPUT);
    if (LOstatus != Os::File::Status::OP_OK) {
        Fw::Logger::log("[ERROR] Failed to open GPIO pin for LO line\n");
    }

    Os::File::Status SOEstatus =
        SOEgpioDriver.open("/dev/gpiochip4", SOEGpioPin, Drv::LinuxGpioDriver::GpioConfiguration::GPIO_INPUT);
    if (SOEstatus != Os::File::Status::OP_OK) {
        Fw::Logger::log("[ERROR] Failed to open GPIO pin for SOE line\n");
    }
}

void setupTopology(const TopologyState& state) {
    // Autocoded initialization. Function provided by autocoder.
    initComponents(state);
    // Autocoded id setup. Function provided by autocoder.
    setBaseIds();
    // Autocoded connection wiring. Function provided by autocoder.
    connectComponents();
    // Autocoded command registration. Function provided by autocoder.
    regCommands();
    // Autocoded configuration. Function provided by autocoder.
    configComponents(state);
    // Project-specific component configuration. Function provided above. May be inlined, if desired.
    configureTopology();
    // Autocoded parameter loading. Function provided by autocoder.
    loadParameters();
    // Autocoded task kick-off (active components). Function provided by autocoder.
    startTasks(state);
    if (state.uartDevice != nullptr) {
        Os::TaskString name("ReceiveTask");
        // Uplink is configured for receive so a socket task is started
        if (comDriver.open(state.uartDevice, static_cast<Drv::LinuxUartDriver::UartBaudRate>(state.baudRate), 
                           Drv::LinuxUartDriver::NO_FLOW, Drv::LinuxUartDriver::PARITY_NONE, 2048)) {
            comDriver.start(COMM_PRIORITY, Default::STACK_SIZE);
        } else {
            printf("Failed to open UART device %s at baud rate %" PRIu32 "\n", state.uartDevice, state.baudRate);
        }
    }
}

void startRateGroups(const Fw::TimeInterval& interval) {
    // The timer component drives the fundamental tick rate of the system.
    // Svc::RateGroupDriver will divide this down to the slower rate groups.
    // This call will block until the stopRateGroups() call is made.
    // For this Linux demo, that call is made from a signal handler.
    timer.startTimer(interval);
}

void stopRateGroups() {
    timer.quit();
}

void teardownTopology(const TopologyState& state) {
    // Autocoded (active component) task clean-up. Functions provided by topology autocoder.
    stopTasks(state);
    freeThreads(state);

    // Other task clean-up.
    comDriver.quitReadThread();
    (void)comDriver.join();

    // Resource deallocation
    cmdSeq.deallocateBuffer(mallocator);

    tearDownComponents(state);
}
};  // namespace Mara
