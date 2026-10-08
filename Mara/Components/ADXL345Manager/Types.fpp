module Mara {
    struct AccelData {
        accelX : F32
        accelY : F32
        accelZ : F32
    }
    @ Sensor configuration the readings of a data product container were taken with
    struct ADXL345Config {
        @ Measurement range (0=2g, 1=4g, 2=8g, 3=16g)
        range: U8
        @ BW_RATE register value (output data rate code, 0x0A = 100 Hz)
        rate: U8
    }
    struct AccelDataTimed {
        time_stamp : Fw.TimeValue
        data : AccelData
    }
}