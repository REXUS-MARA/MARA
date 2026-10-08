module Mara {
    @ Compensated BMP280 reading
    struct Bmp280Data {
        @ Pressure in Pa (clamped by Bosch to 30000..110000, see status)
        pressure: F32
        @ Temperature in degrees Celsius (clamped by Bosch to -40..85, see status)
        temperature: F32
    }

    @ One BMP280 data product record
    struct Bmp280DataTimed {
        time_stamp: Fw.TimeValue
        data: Bmp280Data
        @ Raw 20-bit pressure ADC value, for recomputing pressure below 300 hPa on the ground
        rawPressure: U32
        @ Raw 20-bit temperature ADC value
        rawTemperature: U32
        @ Bosch compensation result: 0 OK, 1..4 clamped (BMP2_W_*), <0 error (BMP2_E_*)
        status: I8
    }

    @ Factory trimming parameters read from the BMP280 NVM (datasheet section 3.11.2)
    struct Bmp280Calib {
        t1: U16
        t2: I16
        t3: I16
        p1: U16
        p2: I16
        p3: I16
        p4: I16
        p5: I16
        p6: I16
        p7: I16
        p8: I16
        p9: I16
    }

    @ Sensor configuration the readings of a data product container were taken with
    struct Bmp280Config {
        oversampling: Bmp280OsMode
        filter: Bmp280Filter
        standby: Bmp280Standby
    }

    @ Oversampling presets of the Bosch API (values are BMP2_OS_MODE_*)
    enum Bmp280OsMode : U8 {
        @ Pressure x1, temperature x1
        ULTRA_LOW_POWER = 0
        @ Pressure x2, temperature x1
        LOW_POWER = 1
        @ Pressure x4, temperature x1
        STANDARD_RESOLUTION = 2
        @ Pressure x8, temperature x1
        HIGH_RESOLUTION = 3
        @ Pressure x16, temperature x2
        ULTRA_HIGH_RESOLUTION = 4
    }

    @ IIR filter coefficient (values are BMP2_FILTER_*)
    enum Bmp280Filter : U8 {
        OFF = 0
        COEFF_2 = 1
        COEFF_4 = 2
        COEFF_8 = 3
        COEFF_16 = 4
    }

    @ Standby time between measurements in normal mode (values are BMP2_ODR_*)
    enum Bmp280Standby : U8 {
        MS_0_5 = 0
        MS_62_5 = 1
        MS_125 = 2
        MS_250 = 3
        MS_500 = 4
        MS_1000 = 5
        MS_2000 = 6
        MS_4000 = 7
    }
}
