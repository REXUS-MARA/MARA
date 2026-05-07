module Mara {
    @ Ltr has 2 channels
    struct LtrData {
        channel_1 : U16
        channel_0 : U16
        calculated_lux : U16
    }

    struct LtrDataTimed{
        time_stamp: Fw.TimeValue
        data: LtrData
    }

    @ Gain of the Ltr, hex values represent the values in the register
    enum LtrGain : U8 {
        GAIN_1X =  0x00
        GAIN_2X =  0x04
        GAIN_4X =  0x08
        GAIN_8X =  0x0C
        GAIN_48X = 0x18
        GAIN_96X = 0x1C
    }
}