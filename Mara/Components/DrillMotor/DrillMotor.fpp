module Mara {
    


  @ Controls a drill motor via GPIO: combined on/off+rpm pin, a direction
  @ pin, and a read-only status pin. Uses libgpiod v2 directly (no shell
  @ calls) so it can run inside a real-time F Prime deployment.
  passive component DrillMotor {
      @ Rotation direction for the motor
    enum Direction {
        CW = 0
        CCW = 1
    }

    # ------------------------------------------------------------
    # GPIO ports
    # ------------------------------------------------------------

    @ Combined on/off + rpm pin. HIGH = motor on at 330 rpm, LOW = off.
    output port motorPinWrite: Drv.GpioWrite

    @ Direction pin. HIGH = clockwise, LOW = counter-clockwise.
    output port directionPinWrite: Drv.GpioWrite

    @ Status pin, read-only.
    output port statusPinRead: Drv.GpioRead

    # ------------------------------------------------------------------
    # Commands
    # ------------------------------------------------------------------

    @ Turn the motor on (330 rpm)
    sync command MOTOR_ON()

    @ Turn the motor off
    sync command MOTOR_OFF()

    @ Set rotation direction
    sync command SET_DIRECTION(
      direction: Direction
    )

    @ Read the status GPIO pin and report via telemetry/event
    sync command READ_STATUS()

    # ------------------------------------------------------------------
    # Telemetry
    # ------------------------------------------------------------------

    @ Current commanded motor on/off state
    telemetry MotorState: bool

    @ Current commanded direction
    telemetry MotorDirection: Direction

    @ Last read state of the status input pin
    telemetry StatusPinState: bool

    # ------------------------------------------------------------------
    # Events
    # ------------------------------------------------------------------

    @ Motor was turned on
    event MotorOn() \
      severity activity high \
      format "Motor turned ON (330 rpm)"

    @ Motor was turned off
    event MotorOff() \
      severity activity high \
      format "Motor turned OFF"

    @ Direction was changed
    event DirectionChanged(
      direction: Direction
    ) \
      severity activity high \
      format "Direction set to {}"

    @ Status pin read
    event StatusPinRead(
      pinState: bool    
    ) \
      severity activity low \
      format "Status pin is {}"

    @ A GPIO operation failed
    event GpioError(
      operation: string size 40
      errorCode: I32
    ) \
      severity warning high \
      format "GPIO operation '{}' failed (code {})"


    @ Scheduler input for periodic status-pin polling
    sync input port schedIn: Svc.Sched

    # ------------------------------------------------------------------
    # Special/standard ports
    # ------------------------------------------------------------------

    @ Command receive port
    command recv port cmdIn

    @ Command registration port
    command reg port cmdRegOut

    @ Command response port
    command resp port cmdResponseOut
    
    time get port timeCaller

    @ Enables event handling
    import Fw.Event

    @ Enables telemetry channels handling
    import Fw.Channel

  }
}