// ======================================================================
// \title  LtrTypes.hpp
// \author fsowa
// \brief  hpp file for defining constants and types for LtrHelpers.cpp
// ======================================================================

#ifndef Ltr_LtrTypes_HPP
#define Ltr_LtrTypes_HPP
#include "Fw/FPrimeBasicTypes.hpp"

namespace Mara {
    namespace Ltr{
        static constexpr U8 DEVICE_DEFAULT_ADDRESS = 0x29;
        static constexpr U8 RESET_VALUE_MASK = (1 << 1);
    }
}
#endif