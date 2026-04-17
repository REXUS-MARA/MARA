module Mara {
    @ INA-228 has many outputs, but according to the SED, only current and voltage will be used.
    @ TODO: double check expected readings with electronics team.
    struct INAData {
        voltage : I32
        current : I32
    }
}
