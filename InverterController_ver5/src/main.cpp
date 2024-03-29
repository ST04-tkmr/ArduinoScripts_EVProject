#include <Arduino.h>
#include "Inverter.hpp"
#include "Accel.hpp"
#include "Switch.hpp"
#include "IO_dfs.hpp"

/**
 * CHECK LIST
 * > Argument of runInverter()
 * > Accel_dfs.hpp
 *  - MAXIMUM_TORQUE
 * > What MCU is used
 *  > Inverter_dfs.hpp
 *      - #define ARDUINO_UNO_R4 or #define ARDUINO_MEGA
*/

#ifdef ARDUINO_UNO_R4
#include "AGTimerR4.hpp"
#else
#include <MsTimer2.h>
#endif

Inverter *inverter = new Inverter();
Accel *accel = new Accel();
int val[2] = {0, 0};
float torque = 0;
Switch *driveSW = new Switch();
Switch *shutdownDetect = new Switch();

/**
 * flags[0] = airFlag
 * flags[1] = torqueControlFlag
 * flags[2] = shutdownFlag
 * flags[3] = driveFlag
 */
unsigned char flags[4] = {0, 0, 0, 0};

unsigned char setupFlag = 0;
unsigned char airFlag = 0;
unsigned char torqueControlFlag = 0;

unsigned long deltaTime = 0;
unsigned long lastTime = 0;
unsigned long nowTime = 0;

unsigned short accVol = 370;

float deviation = 100;

unsigned char CANErrorCount = 0;

void timerCallback(void);

void setup()
{
    inverter->init();

    pinMode(ACCEL_SENSOR1, INPUT);
    pinMode(ACCEL_SENSOR2, INPUT);

    pinMode(READY_TO_DRIVE_SW, INPUT);
    pinMode(READY_TO_DRIVE_LED, OUTPUT);
    digitalWrite(READY_TO_DRIVE_LED, LOW);

    pinMode(AIR_PLUS_SIG, OUTPUT);
    digitalWrite(AIR_PLUS_SIG, LOW);
    pinMode(AIR_MINUS_SIG, OUTPUT);
    digitalWrite(AIR_MINUS_SIG, LOW);

    pinMode(SHUTDOWN_DETECT, INPUT);

#ifdef ARDUINO_UNO_R4
    AGTimer.init(500000, timerCallback);
    AGTimer.start();
#else
    MsTimer2::set(500, timerCallback);
    MsTimer2::start();
#endif
}

void loop()
{
    unsigned long id = inverter->readMsgFromInverter(0);

    /**
     * Measuring the period of CAN Message coming from MG_ECU1.
     * If the period exceeds 50ms, Shutdown.
    */
    nowTime = millis();
    deltaTime = nowTime - lastTime;

    if (id == MG_ECU1_ID)
    {
        lastTime = nowTime;
    }
    else if (deltaTime > 50)
    {
        setupFlag = 1;
        shutdownDetect->setFlag();
    }

    /**
     * Transmitting CAN Massage.
     * If the number of massage sending failures exceeds 3, shutdown.
    */
    unsigned char result = inverter->sendMsgToInverter(0);

#ifdef ARDUINO_UNO_R4
    if (result < 0)
    {
        CANErrorCount++;
    }
    else
    {
        CANErrorCount = 0;
    }
#else
    if (result != 0)
    {
        CANErrorCount++;
    }
    else
    {
        CANErrorCount = 0;
    }
#endif

    if (CANErrorCount > 3)
    {
        setupFlag = 1;
        shutdownDetect->setFlag();
    }

    /**
     * Read accel pedal position sensors.
     * The number of Accel Pedal Position Sensors is 2.
     * Set value of sensors to accel object.
     * Range of sensors is configured in "Accel_dfs.hpp".
    */
    val[0] = analogRead(ACCEL_SENSOR1);
    val[1] = analogRead(ACCEL_SENSOR2);
    accel->setValue(val[0], val[1]);

    deviation = accel->getDev();

    /**
     * If Torque Control Flag is 1, calculate torque from accelerator opening.
    */
    torque = torqueControlFlag ? accel->getTorque() : 0;

    /**
     * SHUTDOWN_DETECT Port is pulldown.
     * When Shutdown Circuit is OPEN, digitalRead(SHUTdOWN_DETECT) will return 0.
    */
    shutdownDetect->updateState(~digitalRead(SHUTDOWN_DETECT));

    /**
     * Initial value of setupFlag is 0.
     * When Low Voltage is ON, Shutdown Circuit is OPEN.
     * In other words, shutdownFlag(flags[2]) is 1.
     * If setupFlag is 0 and shutdownFlag is 1,
     * this programs set setupFlag to 1 and reset shutdownFlag to 0.
     * When Shutdown Circuit is CLOSED, SETUP is complete.
     */
    if (!setupFlag && shutdownDetect->getSWFlag())
    {
        shutdownDetect->resetFlag();
        setupFlag = 1;
    }

    /**
     * READY_TO_DRIVE_SW Port is pulldown.
     * When Ready to Drive SW is pushed,
     * digitalRead(READY_TO_DRIVE_SW) will return 1.
    */
    driveSW->updateState(digitalRead(READY_TO_DRIVE_SW));

    /**
     * If driveFlag is set to 1 before airFlag and torqueControlFlag,
     * driveFlag is reset.
     * Although Ready to Drive Switch(RtDSW) was pushed,
     * Ignition is off (In other words, TSMS is off)
     * or Precharge is not completed.
     * When Precharge is completed, airFlag is set to 1.
     * When Ignition is ON, torqueControlFlag is set to 1.
     * Sequence is below.
     * GLVMS -> AMS, BSPD, IMD Reset -> TSMS -> Precharge -> RtDSW
    */
    if (driveSW->getSWFlag() && !(airFlag && torqueControlFlag))
    {
        driveSW->resetFlag();
        /*
        if (accel->getValue(0) * 0.0049f >= MINIMUM_SENSOR_VOLTAGE && accel->getValue(1) * 0.0049f >= MINIMUM_SENSOR_VOLTAGE)
        {
        }
        */
    }
    /*
    else if (!(flags[3] && flags[0] && flags[1]))
    {
        flags[3] = 0;
        driveSW->resetFlag();
    }
    */

    flags[2] = shutdownDetect->getSWFlag();
    flags[3] = driveSW->getSWFlag();

    inverter->runInverter(flags, accVol, -torque);

    airFlag = flags[0];
    torqueControlFlag = flags[1];

    digitalWrite(AIR_PLUS_SIG, airFlag);

    digitalWrite(AIR_MINUS_SIG, !(shutdownDetect->getSWFlag()));

    digitalWrite(READY_TO_DRIVE_LED, driveSW->getSWFlag());

    //delay(100);
}

void timerCallback(void)
{
    Serial.print("airFlag : ");
    Serial.println(airFlag);
    Serial.print("torqueControlFlag : ");
    Serial.println(torqueControlFlag);
    Serial.print("shutdownFlag : ");
    Serial.println(shutdownDetect->getSWFlag());
    Serial.print("driveFlag : ");
    Serial.println(driveSW->getSWFlag());
    Serial.print("Accel1 : ");
    Serial.println(accel->getValue(0));
    Serial.print("Accel2 : ");
    Serial.println(accel->getValue(1));
    Serial.print("Deviation : ");
    Serial.println(accel->getDev());
    Serial.print("Torque : ");
    Serial.println(torque);
    Serial.print("MGECU1 MSG Period : ");
    Serial.println(deltaTime);
    Serial.print("Deviation of Accel Sensors : ");
    Serial.println(deviation);
    inverter->checkMsg(MG_ECU1_ID);
    inverter->checkMsg(MG_ECU2_ID);
}