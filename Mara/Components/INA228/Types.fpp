module Mara {
    @ INA-228 has many outputs, but according to the SED, only current and voltage will be used.
    struct INAData {
        voltage : F32
        current : F32
    };

    struct INADataTimed{
        time_stamp: Fw.TimeValue
        data: INAData
    };
}
