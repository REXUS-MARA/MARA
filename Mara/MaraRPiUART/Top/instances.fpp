module Mara {

  # ----------------------------------------------------------------------
  # Base ID Convention
  # ----------------------------------------------------------------------
  #
  # All Base IDs follow the 8-digit hex format: 0xDSSCCxxx
  #
  # Where:
  #   D   = Deployment digit (1 for this deployment)
  #   SS  = Subtopology digits (00 for main topology, 01-05 for subtopologies)
  #   CC  = Component digits (00, 01, 02, etc.)
  #   xxx = Reserved for internal component items (events, commands, telemetry)
  #

  # ----------------------------------------------------------------------
  # Defaults
  # ----------------------------------------------------------------------

  module Default {
    constant QUEUE_SIZE = 10
    constant STACK_SIZE = 256 * 1024
  }

  # ----------------------------------------------------------------------
  # Active component instances
  # ----------------------------------------------------------------------

  instance rateGroup1: Svc.ActiveRateGroup base id 0x10001000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 43

  instance rateGroup2: Svc.ActiveRateGroup base id 0x10002000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 42

  instance rateGroup3: Svc.ActiveRateGroup base id 0x10003000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 41

  instance rateGroup4: Svc.ActiveRateGroup base id 0x10004000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 40

  instance cmdSeq: Svc.CmdSequencer base id 0x10005000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 39

  instance orchestrator: Mara.Orchestrator base id 0x10006000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 38
  # ----------------------------------------------------------------------
  # Queued component instances
  # ----------------------------------------------------------------------

  # ----------------------------------------------------------------------
  # Passive component instances
  # ----------------------------------------------------------------------

  instance chronoTime: Svc.ChronoTime base id 0x10030000

  instance rateGroupDriver: Svc.RateGroupDriver base id 0x10031000

  instance systemResources: Svc.SystemResources base id 0x10032000

  instance timer: Svc.LinuxTimer base id 0x10033000

  instance comDriver: Drv.LinuxUartDriver base id 0x10034000

  instance adxl345Manager: Mara.ADXL345Manager base id 0x10035000
  
  instance I2CDriver: Drv.LinuxI2cDriver base id 0x10036000

  instance gpioWatcher: Mara.GPIOWatcher base id 0x10037000

  instance LOgpioDriver: Drv.LinuxGpioDriver base id 0x10038000
  
  instance SOEgpioDriver: Drv.LinuxGpioDriver base id 0x10039000

  instance EODSgpioDriver: Drv.LinuxGpioDriver base id 0x10040000

}
