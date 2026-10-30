module Mara {

  # ----------------------------------------------------------------------
  # Symbolic constants for port numbers
  # ----------------------------------------------------------------------

  enum Ports_RateGroups {
    # 1 Hz
    rateGroup1
    # 20 Hz
    rateGroup2
    # 1/4 Hz
    rateGroup3
    # 100 Hz
    rateGroup4
  }

  topology MaraRPiUART {

  # ----------------------------------------------------------------------
  # Subtopology imports
  # ----------------------------------------------------------------------
    import CdhCore.Subtopology
    import ComCcsds.Subtopology
    import DataProducts.Subtopology
    import FileHandling.Subtopology
    
  # ----------------------------------------------------------------------
  # Instances used in the topology
  # ----------------------------------------------------------------------
    instance chronoTime
    instance rateGroup1
    instance rateGroup2
    instance rateGroup3
    instance rateGroup4
    instance rateGroupDriver
    instance systemResources
    instance timer
    instance comDriver
    instance cmdSeq
    instance adxl345Manager
    instance I2CDriver
    instance orchestrator
    instance gpioWatcher
    instance LOgpioDriver
    instance EODSgpioDriver
    instance SOEgpioDriver
    instance drillMotorOnDriver
    instance drillMotorDirDriver
    instance drillMotorStatusDriver
    instance drillMotor
    instance opticalCamera
    instance platformMotor
    instance motorUart
    instance motorBufferManager
    instance ThermalCamera
    instance ina228supply
    instance ina228drill
    instance ina228platform
    instance bmp280Manager
    instance tc1
    instance tc2
    instance spiDriver1
    instance spiDriver2

  # ----------------------------------------------------------------------
  # Pattern graph specifiers
  # ----------------------------------------------------------------------

    command connections instance CdhCore.cmdDisp
    event connections instance CdhCore.events
    telemetry connections instance CdhCore.tlmSend
    text event connections instance CdhCore.textLogger
    health connections instance CdhCore.$health
    param connections instance FileHandling.prmDb
    time connections instance chronoTime

  # ----------------------------------------------------------------------
  # Telemetry packets (only used when TlmPacketizer is used)
  # ----------------------------------------------------------------------

    # include "MaraRPiUARTPackets.fppi"

  # ----------------------------------------------------------------------
  # Direct graph specifiers
  # ----------------------------------------------------------------------

    connections ComCcsds_CdhCore {
      # Core events and telemetry to communication queue
      CdhCore.events.PktSend -> ComCcsds.comQueue.comPacketQueueIn[ComCcsds.Ports_ComPacketQueue.EVENTS]
      CdhCore.tlmSend.PktSend -> ComCcsds.comQueue.comPacketQueueIn[ComCcsds.Ports_ComPacketQueue.TELEMETRY]

      # Router to Command Dispatcher
      ComCcsds.fprimeRouter.commandOut -> CdhCore.cmdDisp.seqCmdBuff
      CdhCore.cmdDisp.seqCmdStatus -> ComCcsds.fprimeRouter.cmdResponseIn
      
    }

    connections ComCcsds_FileHandling {
      # File Downlink to Communication Queue
      FileHandling.fileDownlink.bufferSendOut -> ComCcsds.comQueue.bufferQueueIn[ComCcsds.Ports_ComBufferQueue.FILE]
      ComCcsds.comQueue.bufferReturnOut[ComCcsds.Ports_ComBufferQueue.FILE] -> FileHandling.fileDownlink.bufferReturn

      # Router to File Uplink
      ComCcsds.fprimeRouter.fileOut -> FileHandling.fileUplink.bufferSendIn
      FileHandling.fileUplink.bufferSendOut -> ComCcsds.fprimeRouter.fileBufferReturnIn
    }

    connections Communications {
      # ComDriver buffer allocations
      comDriver.allocate      -> ComCcsds.commsBufferManager.bufferGetCallee
      comDriver.deallocate    -> ComCcsds.commsBufferManager.bufferSendIn
      
      # ComDriver <-> ComStub (Uplink)
      comDriver.$recv                     -> ComCcsds.comStub.drvReceiveIn
      ComCcsds.comStub.drvReceiveReturnOut -> comDriver.recvReturnIn
      
      # ComStub <-> ComDriver (Downlink)
      ComCcsds.comStub.drvSendOut      -> comDriver.$send
      comDriver.ready         -> ComCcsds.comStub.drvConnected
    }

    connections FileHandling_DataProducts {
      # Data Products to File Downlink
      DataProducts.dpCat.fileOut -> FileHandling.fileDownlink.SendFile
      FileHandling.fileDownlink.FileComplete -> DataProducts.dpCat.fileDone
    }

    connections RateGroups {
      # timer to drive rate group
      timer.CycleOut -> rateGroupDriver.CycleIn

      # Rate group 1
      rateGroupDriver.CycleOut[Ports_RateGroups.rateGroup1] -> rateGroup1.CycleIn
      rateGroup1.RateGroupMemberOut[0] -> CdhCore.tlmSend.Run
      rateGroup1.RateGroupMemberOut[1] -> FileHandling.fileDownlink.Run
      rateGroup1.RateGroupMemberOut[2] -> systemResources.run
      rateGroup1.RateGroupMemberOut[3] -> ComCcsds.comQueue.run
      rateGroup1.RateGroupMemberOut[4] -> ComCcsds.aggregator.timeout
      rateGroup1.RateGroupMemberOut[5] -> cmdSeq.schedIn
      rateGroup1.RateGroupMemberOut[7] -> orchestrator.schedIn
      rateGroup1.RateGroupMemberOut[8] -> tc1.run
      rateGroup1.RateGroupMemberOut[9] -> tc2.run
      

      # Rate group 2
      rateGroupDriver.CycleOut[Ports_RateGroups.rateGroup2] -> rateGroup2.CycleIn
      rateGroup2.RateGroupMemberOut[0] -> gpioWatcher.schedIn
      rateGroup2.RateGroupMemberOut[1] -> drillMotor.schedIn
      rateGroup2.RateGroupMemberOut[3] -> ina228drill.run
      rateGroup2.RateGroupMemberOut[4] -> ina228platform.run
      rateGroup2.RateGroupMemberOut[5] -> ina228supply.run
      rateGroup2.RateGroupMemberOut[2] -> bmp280Manager.run



      # Rate group 3
      rateGroupDriver.CycleOut[Ports_RateGroups.rateGroup3] -> rateGroup3.CycleIn
      rateGroup3.RateGroupMemberOut[0] -> CdhCore.$health.Run
      rateGroup3.RateGroupMemberOut[1] -> ComCcsds.commsBufferManager.schedIn
      rateGroup3.RateGroupMemberOut[2] -> DataProducts.dpBufferManager.schedIn
      rateGroup3.RateGroupMemberOut[3] -> DataProducts.dpWriter.schedIn
      rateGroup3.RateGroupMemberOut[4] -> DataProducts.dpMgr.schedIn

      # Rate group 4
      rateGroupDriver.CycleOut[Ports_RateGroups.rateGroup4] -> rateGroup4.CycleIn
      rateGroup4.RateGroupMemberOut[0] -> adxl345Manager.run

    }

    connections CdhCore_cmdSeq {
      # Command Sequencer
      cmdSeq.comCmdOut -> CdhCore.cmdDisp.seqCmdBuff
      CdhCore.cmdDisp.seqCmdStatus -> cmdSeq.cmdResponseIn
    }

    connections MaraRPiUART {
      gpioWatcher.EODSHigh         -> orchestrator.EODSHigh
      gpioWatcher.LOHigh           -> orchestrator.LOHigh
      gpioWatcher.SOEHigh          -> orchestrator.SOEHigh

      gpioWatcher.EODSPinRead      -> EODSgpioDriver.gpioRead
      gpioWatcher.LOPinRead        -> LOgpioDriver.gpioRead
      gpioWatcher.SOEPinRead       -> SOEgpioDriver.gpioRead

      orchestrator.OpticalCameraON -> opticalCamera.Camera_ON
      orchestrator.OpticalCameraOFF -> opticalCamera.Camera_OFF

      orchestrator.PlatformEnable -> platformMotor.enable
      orchestrator.PlatformMoveTo -> platformMotor.moveTo
      orchestrator.PlatformStop   -> platformMotor.stop
    }

    # Platform motor drive over CANopen, through the USB-CAN adapter's UART
    connections PlatformMotor {
      platformMotor.toByteStreamDriver         -> motorUart.$send
      motorUart.$recv                          -> platformMotor.fromByteStreamDriver
      platformMotor.fromByteStreamDriverReturn -> motorUart.recvReturnIn
      motorUart.ready                          -> platformMotor.byteStreamDriverReady

      motorUart.allocate   -> motorBufferManager.bufferGetCallee
      motorUart.deallocate -> motorBufferManager.bufferSendIn
    }

    # Connect ADXL345 to I2C driver
    connections ADXL345 {
        adxl345Manager.i2cReadWrite -> I2CDriver.writeRead
        adxl345Manager.i2cWrite -> I2CDriver.write
        adxl345Manager.productGetOut  -> DataProducts.dpMgr.productGetIn
        adxl345Manager.productSendOut -> DataProducts.dpMgr.productSendIn
    }

    connections ina228drill {
      ina228drill.busWriteRead      -> I2CDriver.writeRead
      ina228drill.busWrite          -> I2CDriver.write
      ina228drill.productGetOut     -> DataProducts.dpMgr.productGetIn
      ina228drill.productSendOut    -> DataProducts.dpMgr.productSendIn
    }
    connections ina228platform {
      ina228platform.busWriteRead      -> I2CDriver.writeRead
      ina228platform.busWrite          -> I2CDriver.write
      ina228platform.productGetOut     -> DataProducts.dpMgr.productGetIn
      ina228platform.productSendOut    -> DataProducts.dpMgr.productSendIn
    }
    connections ina228supply {
      ina228supply.busWriteRead      -> I2CDriver.writeRead
      ina228supply.busWrite          -> I2CDriver.write
      ina228supply.productGetOut     -> DataProducts.dpMgr.productGetIn
      ina228supply.productSendOut    -> DataProducts.dpMgr.productSendIn
    }

    # Connect BMP280 to I2C driver
    connections BMP280 {
        bmp280Manager.busWriteRead   -> I2CDriver.writeRead
        bmp280Manager.busWrite       -> I2CDriver.write
        bmp280Manager.productGetOut  -> DataProducts.dpMgr.productGetIn
        bmp280Manager.productSendOut -> DataProducts.dpMgr.productSendIn
    }

    connections thermocouple1 {
        tc1.spiWriteRead -> spiDriver1.SpiWriteRead
        tc1.productGetOut -> DataProducts.dpMgr.productGetIn
        tc1.productSendOut -> DataProducts.dpMgr.productSendIn
    }    
    
    connections thermocouple2 {
        tc2.spiWriteRead -> spiDriver2.SpiWriteRead
        tc2.productGetOut -> DataProducts.dpMgr.productGetIn
        tc2.productSendOut -> DataProducts.dpMgr.productSendIn
    }    
    
    connections DrillMotor {
        drillMotor.motorPinWrite     -> drillMotorOnDriver.gpioWrite
        drillMotor.directionPinWrite -> drillMotorDirDriver.gpioWrite
        drillMotorStatusDriver.gpioInterrupt -> drillMotor.tachPulseIn
        drillMotor.productGetOut  -> DataProducts.dpMgr.productGetIn
        drillMotor.productSendOut -> DataProducts.dpMgr.productSendIn
    }

  }

}
