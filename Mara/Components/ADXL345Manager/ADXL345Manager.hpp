// ======================================================================
// \title  ADXL345Manager.hpp
// \author imanol
// \brief  hpp file for ADXL345Manager component implementation class
// ======================================================================

#ifndef Mara_ADXL345Manager_HPP
#define Mara_ADXL345Manager_HPP

#include "Mara/Components/ADXL345Manager/ADXL345ManagerComponentAc.hpp"
#include "Mara/Components/ADXL345Manager/AccelDataTimedSerializableAc.hpp"

namespace Mara {

class ADXL345Manager final : public ADXL345ManagerComponentBase {

  public:
    ADXL345Manager(const char* const compName);
    ~ADXL345Manager();

  private:
    static constexpr FwSizeType RECORD_COUNT = 100;
    FwSizeType m_count;
    DpContainer m_container;
    bool m_containerValid;

    void run_handler(FwIndexType portNum, U32 context) override;
    void ADXL345_SET_RANGE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, U8 range) override;
    void ADXL345_SET_RATE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, U8 rate) override;

    Drv::I2cStatus writeRegister(U8 reg, U8 value);
    Drv::I2cStatus readRegisters(U8 startReg, U8* buffer, U32 size);

    // if it was an active component, we could do it in the preamble
    // we have to initialize outside of the constructor
    Drv::I2cStatus initialize_helper();

    U8 getI2cAddr();
    bool m_initialized = false;
};

}  // namespace Mara
#endif