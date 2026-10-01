// ======================================================================
// \title  DrillMotor.hpp
// \author john
// \brief  hpp file for DrillMotor component implementation class
//
//   ON_PIN   -> HIGH = motor on at 330 rpm, LOW = motor off
//   DIR_PIN  -> HIGH = clockwise, LOW = counter-clockwise
//   READ_PIN -> input, status pin read back on demand / periodically
// ======================================================================

#ifndef Mara_DrillMotor_HPP
#define Mara_DrillMotor_HPP

#include "Mara/Components/DrillMotor/DrillMotorComponentAc.hpp"

namespace Mara {

class DrillMotor final : public DrillMotorComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct DrillMotor object
    DrillMotor(const char* const compName  //!< The component name
    );
 
    //! Destroy DrillMotor object
    ~DrillMotor();

  
  private:
  
    //! Handler implementation for MOTOR_ON
    void MOTOR_ON_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;
 
    //! Handler implementation for MOTOR_OFF
    void MOTOR_OFF_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;
 
    //! Handler implementation for SET_DIRECTION
    void SET_DIRECTION_cmdHandler(FwOpcodeType opCode,
                                   U32 cmdSeq,
                                   DrillMotor_Direction direction) override;
 
    //! Handler implementation for READ_STATUS
    void READ_STATUS_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;
 
    void schedIn_handler(FwIndexType portNum,  //!< The port number
                          U32 context           //!< The call order
                          ) override;
 

    void readAndReportStatus();
 
    bool m_motorOn{false};
    DrillMotor_Direction m_direction{DrillMotor_Direction::CW};


};

}  // namespace Mara

#endif
