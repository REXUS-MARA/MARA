// ======================================================================
// \title  ADXL345Manager.hpp
// \author imanol
// \brief  hpp file for ADXL345Manager component implementation class
// ======================================================================

#ifndef Mara_ADXL345Manager_HPP
#define Mara_ADXL345Manager_HPP

#include "Mara/Components/ADXL345Manager/ADXL345ManagerComponentAc.hpp"

namespace Mara {

class ADXL345Manager final : public ADXL345ManagerComponentBase {

  public:
    ADXL345Manager(const char* const compName);
    ~ADXL345Manager();

  private:
    void run_handler(FwIndexType portNum, U32 context) override;
    void ADXL345_INIT_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;
    void ADXL345_SET_RANGE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, U8 range) override;
    void ADXL345_SET_RATE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, U8 rate) override;

    Drv::I2cStatus writeRegister(U8 reg, U8 value);
    Drv::I2cStatus readRegisters(U8 startReg, U8* buffer, U32 size);

    U8 getI2cAddr();

    bool m_initialized = false;
};

}  // namespace Mara
#endif