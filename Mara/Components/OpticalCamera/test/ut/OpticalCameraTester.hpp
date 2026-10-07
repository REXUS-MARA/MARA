// ======================================================================
// \title  OpticalCameraTester.hpp
// \author fsowa
// \brief  hpp file for OpticalCamera component test harness implementation class
//
// OpticalCamera starts "ffmpeg" with posix_spawnp, which searches PATH. The tester puts a
// fake ffmpeg (a shell script) first on PATH. The fake logs its arguments and lifecycle
// to a file, writes into the primary output file like a recording would, and exits 0 on
// SIGINT like ffmpeg finalising its files. FAKE_FFMPEG_MODE selects other behaviour.
// ======================================================================

#ifndef Mara_OpticalCameraTester_HPP
#define Mara_OpticalCameraTester_HPP

#include "Mara/Components/OpticalCamera/OpticalCamera.hpp"
#include "Mara/Components/OpticalCamera/OpticalCameraGTestBase.hpp"

#include <string>
#include <vector>

namespace Mara {

class OpticalCameraTester final : public OpticalCameraGTestBase {
  public:
    // ----------------------------------------------------------------------
    // Constants
    // ----------------------------------------------------------------------

    // Maximum size of histories storing events, telemetry, and port outputs
    static const FwSizeType MAX_HISTORY_SIZE = 100;

    // Instance ID supplied to the component instance under test
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    // Queue depth supplied to the component instance under test
    static const FwSizeType TEST_INSTANCE_QUEUE_DEPTH = 10;

  public:
    // ----------------------------------------------------------------------
    // Construction and destruction
    // ----------------------------------------------------------------------

    //! Construct object OpticalCameraTester
    OpticalCameraTester();

    //! Destroy object OpticalCameraTester. Restores PATH; the component stops any recorder.
    ~OpticalCameraTester();

  public:
    // ----------------------------------------------------------------------
    // Tests
    // ----------------------------------------------------------------------

    //! ON starts ffmpeg with the parameters; OFF sends SIGINT and waits
    void testOnOffArguments();

    //! A second ON while recording starts nothing; OFF then ON starts segment 2
    void testOnIdempotentAndSegments();

    //! OFF while not recording does nothing
    void testOffWhenIdle();

    //! ffmpeg exiting with an error is reported once on the next ping
    void testExitedEarly();

    //! ffmpeg exiting with 0 (MAX_SECONDS reached) is not a warning
    void testExitedNormally();

    //! An output file that stops growing is reported after 5 pings, once
    void testStall();

    //! A growing output file never reports a stall
    void testNoStallWhileGrowing();

    //! No ffmpeg on PATH: SpawnFailed, and the component keeps working
    void testSpawnFailed();

    //! Path of the fake's lifecycle log (for checks after the tester is gone)
    std::string logPath() const { return m_dir + "/ffmpeg.log"; }

    //! Turn the camera on and wait until the fake is writing
    void startRecording();

  private:
    // ----------------------------------------------------------------------
    // Helper functions
    // ----------------------------------------------------------------------

    //! Connect ports
    void connectPorts();

    //! Initialize components
    void initComponents();

    //! Invoke the async ports and dispatch
    void on();
    void off();
    void ping();

    //! Select the fake's behaviour: "record" (default), "exit0", "exit1", "stall"
    void setMode(const char* mode);

    //! Lines of the fake's lifecycle log ("start", "sigint", ...)
    std::vector<std::string> logLines() const;

    //! Arguments of the most recent fake run
    std::vector<std::string> lastArgs() const;

    //! Wait until a file exists and is non-empty
    bool waitForFile(const std::string& path, U32 timeoutMs) const;

    static void sleepMs(U32 ms);

  private:
    // ----------------------------------------------------------------------
    // Member variables
    // ----------------------------------------------------------------------

    //! The component under test
    OpticalCamera component;

    //! Temp dir: fake ffmpeg, its log, and the two output dirs
    std::string m_dir;
    std::string m_primary;
    std::string m_backup;

    //! PATH before the test, restored in the destructor
    std::string m_savedPath;
};

}  // namespace Mara

#endif
