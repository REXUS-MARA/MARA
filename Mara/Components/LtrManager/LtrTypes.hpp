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
static constexpr U8 ALS_MEAS_RATE = 0x85;

static constexpr U8 ALS_INTEGRATION_TIME_50MS = 0b00001000;
static constexpr U8 ALS_MEASUREMENT_RATE_50MS = 0b00000000;

// every channel will return 4 bytes
// the whole channel is to be read in a group
// and the channels should be read separately
static constexpr U8 DATA_LEN_CHANNEL = 4 * sizeof(U8);

// in the ALS_CONTR we have 8 bits
// bits 7:5 have to be always set to 000
// bits 4:2 are responsible for the gain
// below we define the actual VALUES that are needed to be put
// into the field
static constexpr U8 GAIN_CONFIG_1X =  0b00000000;
static constexpr U8 GAIN_CONFIG_2X =  0b00000100;
static constexpr U8 GAIN_CONFIG_4X =  0b00001000;
static constexpr U8 GAIN_CONFIG_8X =  0b00001100;
static constexpr U8 GAIN_CONFIG_48X = 0b00011000;
static constexpr U8 GAIN_CONFIG_96X = 0b00011100;
// bit 1 is telling us whether the board is actually doing
// what it supposed to be doing - we're reading that, so it's supposed
// to be used as a mask
static constexpr U8 RESET_VALUE_MASK = 0b00000010;
// and bit 0 is set by us - when we set it to 1
// the als is active - if it's set to 0 there are no readings
static constexpr U8 ALS_ACTIVE_MODE_MASK = 0b00000001;


//! RawLtrData: basic structure of als data as read from the device
//! To make sense of that, it still need to be converted to something meaningful
struct RawLtrData {
    U16 ch_1;
    U16 ch_0;
};
}  // namespace Ltr
}  // namespace Mara
#endif