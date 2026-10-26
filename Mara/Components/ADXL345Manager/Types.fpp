module Mara {
    struct AccelData {
        accelX : F32
        accelY : F32
        accelZ : F32
    }
    struct AccelDataTimed {
        time_stamp : Fw.TimeValue
        data : AccelData
    }
}