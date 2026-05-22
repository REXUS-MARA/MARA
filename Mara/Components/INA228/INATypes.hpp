// ======================================================================
// \title  INATypes.hpp
// \author aurahs
// \brief  hpp file for defining constants and types for INA.cpp
// ======================================================================

#ifndef Mara_INATypes_HPP
#define Mara_INATypes_HPP
#include "Fw/FPrimeBasicTypes.hpp"

namespace Mara {
namespace INA {
static constexpr U8 DEVICE_DEFAULT_ADDRESS = 0x40;

/* Register addresses */
static constexpr U8 SHUNT_CAL = 0x2;
static constexpr U8 VBUS = 0x5;
static constexpr U8 CURRENT = 0x7;


/* Misc configuration */
static constexpr U8 COMMANDSIZE = 3; // all commands are addr + 16 bits (2 bytes)
static constexpr U8 DATASIZE = 3; // all data read is 24 bits (3 bytes)
static constexpr U16 SHUNT_CAL_VAL = 3750; // refer to the datasheet

}  // namespace INA
}  // namespace Mara
#endif
