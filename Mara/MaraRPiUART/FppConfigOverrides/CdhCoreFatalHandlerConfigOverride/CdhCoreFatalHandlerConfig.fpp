module CdhCore {

    # MARA: FATALs are logged and survived instead of aborting (see Mara/Components/FatalHandler)
    instance fatalHandler: Mara.FatalHandler base id CdhCoreConfig.BASE_ID + 0x07000

}
