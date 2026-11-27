module Mara {

    @ One MAX31855 reading
    struct ThermoData {
        thermocoupleC: F32 
        internalC: F32     
        faults: U8         
    }

    @ store timestamp
    struct ThermoDataTimed {
        time_stamp: Fw.TimeValue
        data: ThermoData
    }

}
