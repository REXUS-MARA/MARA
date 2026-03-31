// ======================================================================
// \title  LtrTypes.hpp
// \author fsowa
// \brief  hpp file for defining constants and types for LtrHelpers.cpp
// ======================================================================

#ifndef Mara_LtrTypes_HPP
#define Mara_LtrTypes_HPP
#include "Fw/FPrimeBasicTypes.hpp"

namespace Mara {
namespace Ltr {
static constexpr U8 DEVICE_DEFAULT_ADDRESS = 0x29;

static constexpr U8 ALS_CONTR_REGISTER = 0x80;
static constexpr U8 ALS_DATA_CH1_0_REGISTER = 0x88;
// static constexpr U8 ALS_DATA_CH1_1_REGISTER = 0x89;
// static constexpr U8 ALS_DATA_CH0_0_REGISTER = 0x8A;
// static constexpr U8 ALS_DATA_CH0_1_REGISTER = 0x8B;

// every channel will return 4 bytes
// the whole channel is to be read in a group
// and the channels should be read separately
static constexpr U8 DATA_LEN_CHANNEL = 4 * sizeof(U8);

static constexpr U8 RESET_VALUE_MASK = 0x02;
static constexpr U8 ALS_ACTIVE_MODE_MASK = 0x01;

//! RawLtrData: basic structure of als data as read from the device
//! To make sense of that, it still need to be converted to something meaningful
struct RawLtrData {
    U16 ch_1;
    U16 ch_0;
};
}  // namespace Ltr
}  // namespace Mara
#endif