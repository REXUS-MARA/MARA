module Mara {
    struct AccelData {
        accelX : I16
        accelY : I16
        accelZ : I16
    }
    struct AccelDataTimed {
        time_stamp : Fw.TimeValue
        data : AccelData
    }
}