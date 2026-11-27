// ======================================================================
// \title  PlatformMotor.hpp
// \author fsowa
// \brief  hpp file for PlatformMotor component implementation class
// ======================================================================

#ifndef Mara_PlatformMotor_HPP
#define Mara_PlatformMotor_HPP

#include "Mara/Components/PlatformMotor/PlatformMotorComponentAc.hpp"
#include "Fw/Types/String.hpp"

#include <initializer_list>
#include <array>

namespace Mara {

class PlatformMotor final : public PlatformMotorComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct PlatformMotor object
    PlatformMotor(const char* const compName  //!< The component name
    );

    //! Destroy PlatformMotor object
    ~PlatformMotor();

    //! Directory where position is stored
    void configure(const char* pathA, const char* pathB);

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for byteStreamDriverReady
    //!
    //! Port for receiving ready signals from the driver
    //! Sample connection: byteStreamDriver.ready -> byteStreamDriverClient.byteStreamDriverReady
    void byteStreamDriverReady_handler(FwIndexType portNum  //!< The port number
                                       ) override;

    //! Handler implementation for enable
    //!
    //! Enable the drive. Also clears a drive fault.
    void enable_handler(FwIndexType portNum  //!< The port number
                        ) override;

    //! Handler implementation for fromByteStreamDriver
    //!
    //! Port for receiving data from the driver
    //! Sample connection: byteStreamDriver.$recv -> byteStreamDriverClient.fromDriver
    void fromByteStreamDriver_handler(FwIndexType portNum,  //!< The port number
                                      Fw::Buffer& buffer,
                                      const Drv::ByteStreamStatus& status) override;

    //! Handler implementation for moveTo
    //!
    //! Move to an absolute position in encoder counts, clamped to [MIN_POSITION, MAX_POSITION].
    //! 0 is the position at drive power-up.
    void moveTo_handler(FwIndexType portNum,  //!< The port number
                        I32 position) override;

    //! Handler implementation for pingIn
    //!
    //! Health ping
    void pingIn_handler(FwIndexType portNum,  //!< The port number
                        U32 key               //!< Value to return to pinger
                        ) override;

    //! Handler implementation for runTestCycle
    //!
    //! Run the test cycle: advance, pause, retract, pause, advance and hold
    void runTestCycle_handler(FwIndexType portNum  //!< The port number
                              ) override;
 

    //! Handler implementation for schedIn
    //!
    //! Rate group tick: polls the status word during moves and times the test cycle pauses
    void schedIn_handler(FwIndexType portNum,  //!< The port number
                         U32 context           //!< The call order
                         ) override;
 


    //! Handler implementation for stop
    //!
    //! Stop and hold position
    void stop_handler(FwIndexType portNum  //!< The port number
                      ) override;

  private:
    // ----------------------------------------------------------------------
    // Helpers
    // ----------------------------------------------------------------------

       //! Status word read back from the drive
    void statusWordReceived_internalInterfaceHandler(U16 statusWord) override;
 
    //! Emergency message from the drive
    void emergencyReceived_internalInterfaceHandler(U16 errorCode, U8 errorRegister) override;
 
    //! The drive rejected an SDO request
    void sdoAbortReceived_internalInterfaceHandler(U16 index, U32 abortCode) override;
 
 //! Actual position read back from the drive
    void positionReceived_internalInterfaceHandler(I32 drivePosition) override;
 
    //! The drive sent its boot-up message
    void driveBooted_internalInterfaceHandler() override;
 
  private:
    // ----------------------------------------------------------------------
    // Handler implementations for commands
    // ----------------------------------------------------------------------
 
    //! Handler implementation for command SET_POSITION
    void SET_POSITION_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                                 U32 cmdSeq,           //!< The command sequence number
                                 I32 position) override;

    //! One value to write into the drive's object dictionary
    struct SdoWrite {
        U16 index;
        U32 value;
        U8 size;  //!< value size in bytes: 1, 2 or 4
    };

    //! Send CANopen SDO downloads in order. Stops at the first failed send and logs one SendFailed.
    //! \return true if every write was sent
    bool sdoWriteAll(std::initializer_list<SdoWrite> writes);

        //! Send one SDO request frame to the drive
    //! \return true if it was sent
    bool sendSdo(U8 command, U16 index, U32 value);
 
    //! Decode one CAN frame received from the adapter. Runs on the driver's thread.
    void handleCanFrame(U32 canId, const U8* data, U8 length);
 
    //! The drive reported the target reached: report it and continue a running test cycle
    void onTargetReached();
 
    //! Go to the next test cycle step
    void advanceCycle();
 
    //! Send the Waveshare USB-CAN-A its settings (500 kbps, standard frames, normal mode)
    //! \return true if the settings frame was sent
    bool configureAdapter();
 
    //! Set the mode and acceleration, then step the drive to Operation enabled
    //! \return true if every write was sent
    bool enableDrive();
 
    //! Clamp the target, set the advance or retract velocity and start an absolute move
    //! \return true if the move was sent
    bool startMove(I32 position);
 
    //! Test cycle steps. Each move step lasts until the drive reports its target reached, plus the pause if any.
    enum class CycleStep : U8 { IDLE, ADVANCE_1, RETRACT, ADVANCE_2 };
 
    //! Start one cycle move. The next step follows when the target is reached, after the pause if any.
    //! Aborts the cycle if the move can't be sent.
    void cycleMove(I32 position, CycleStep step, bool pauseAfter);
 
    //! Cancel a running test cycle and log CycleAborted
    void abortCycle();
 
    //! Estimated duration of a trapezoidal move, used to time out a move that never arrives
    F64 estimateMoveSeconds(U32 distance, U32 velocity, U32 acceleration) const;
 
    //! Current time in seconds
    F64 nowSeconds();

    void tryRestorePosition();
 
    //! Write the current platform position and offset to the next of the two position files
    void savePosition();
 
    //! Read both position files and return the newest valid record
    //! \return true if one was valid
    bool loadPosition(I32& platformPosition, I32& offset);
 
    I32 m_offset = 0;              //!< platform position = drive position + m_offset
    bool m_positionKnown = false;  //!< The platform position is known (restored or set); moves are allowed
    bool m_restorePending = true;  //!< Waiting for the first drive replies to restore the stored position
    bool m_uartReady = false;      //!< The serial port to the adapter is open
    bool m_haveStatus = false;     //!< A status word has arrived since start-up
    bool m_haveDrivePos = false;   //!< A drive position has arrived since start-up
    U16 m_lastStatus = 0;          //!< Last status word
    I32 m_drivePos = 0;            //!< Last drive position (drive counts)
    I32 m_platformPos = 0;         //!< Last platform position (drive position + offset)
    I32 m_savedPos = 0;            //!< Platform position in the last saved record
    U32 m_saveSeq = 0;             //!< Sequence number of the last saved record
    bool m_sawMoving = false;      //!< A status poll showed the current move running
    Fw::String m_pathA;            //!< Position file A
    Fw::String m_pathB;            //!< Position file B
 
    I32 m_target = 0;  //!< Last commanded target; 0 is the power-up position
 
    bool m_moveActive = false;   //!< A move was started and its target is not reached yet
    U32 m_pollsSinceStart = 0;   //!< Status polls sent since the move started; replies before the first are stale
    F64 m_moveDeadline = 0.0;    //!< Time after which the move is reported as timed out, in seconds
 
    CycleStep m_cycleStep = CycleStep::IDLE;  //!< Current test cycle step
    bool m_cycleWaitingForMove = false;       //!< The current step waits for its move to finish
    bool m_cyclePauseAfter = false;           //!< The current step pauses after its move
    F64 m_stepDeadline = 0.0;                 //!< End of the current pause, in seconds
 
    //! Bytes received from the adapter that do not form a complete frame yet. Driver thread only.
    std::array<U8, 64> m_rx{};
    FwSizeType m_rxLength = 0;

};

}  // namespace Mara

#endif
