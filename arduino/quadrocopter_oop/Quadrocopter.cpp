#include "Quadrocopter.h"
#include "Arduino.h"
#include "PID.h"
#ifdef _arch_avr_
    #include <avr/delay.h>
#endif

Quadrocopter::Quadrocopter()
{
    needPCTx = false;

    flying = false;

    serialReadN = 21; // 3 + 12 + 6

#ifdef PID_USE_YAW_ANGLE
    serialReadN += 6;
#endif

    DefaultVSensorPin = A4;
    reactionType = ReactionNone;

#ifdef _arch_avr_
    #ifdef DEBUG_DAC
        DefaultMotorPins[0] = 3;
        DefaultMotorPins[1] = 5;
        DefaultMotorPins[2] = 6;
        DefaultMotorPins[3] = 9;
    #else
        DefaultMotorPins[0] = 3;
        DefaultMotorPins[1] = 9;
        DefaultMotorPins[2] = 10;
        DefaultMotorPins[3] = 11;
    #endif
#endif
#ifdef _arch_arm_
        DefaultMotorPins[0] = 6;
        DefaultMotorPins[1] = 7;
        DefaultMotorPins[2] = 8;
        DefaultMotorPins[3] = 9;
#endif

#ifdef DEBUG_SERIAL_SECOND
    DEBUG_SERIAL_SECOND.begin(115200);
#endif

    MSerial = new MySerial;
    MController = new MotorController(DefaultMotorPins);
    VSensor = new VoltageSensor(DefaultVSensorPin, DefaultVSensorMaxVoltage);
    MyMPU = new MPU6050DMP;

    pidAngleX = PID(PID::DIFFERENCE_ANGLE);
    pidAngleY = PID(PID::DIFFERENCE_ANGLE);

#ifdef PID_USE_YAW_ANGLE
    pidAngleZ = PID(PID::DIFFERENCE_ANGLE);
#endif

#ifdef DEBUG_FREQ_PIN
    freqLed = InfoLED(DEBUG_FREQ_PIN, InfoLED::DIGITAL);
#endif

#ifdef DEBUG_MPUBYTES_PIN
    mpuBytesLed = InfoLED(DEBUG_MPUBYTES_PIN, InfoLED::DIGITAL);
#endif

    MController->calibrate();

    forceOverrideValue = 0;
    forceOverride = 1;

    this->reset();

    MyMPU->initialize();

#ifdef USE_COMPASS
    MyCompass = new HMC5883L;
    MyCompass->initialize();
#endif

    Joystick = new PWMJoystick;

#ifdef DEBUG_DAC
    myLed = MyMPU->myLed;
    myLed.setState(0);
#endif

#ifdef _arch_avr_
    interrupts();
#endif
}

void Quadrocopter::reset()
{
    angleZLPF.setPeriod(0.05);
    flyingTime = 0;
    flying = false;

    angleOffsetPC = RVector3D();
    angle = RVector3D();
    torqueAutomaticCorrection = RVector3D();
    angleManualCorrection = RVector3D();

    MController->setForce(0);
    MController->setTorque(RVector3D());

    pidAngleX.reset();
    pidAngleX.setYMinYMax(angleMaxCorrection);
    pidAngleY.reset();
    pidAngleY.setYMinYMax(angleMaxCorrection);

#ifdef PID_USE_YAW
    pidAngularVelocityZ.reset();
    pidAngularVelocityZ.setYMinYMax(angularVelocityMaxCorrection);
#endif

#ifdef PID_USE_YAW_ANGLE
    pidAngleZ.reset();
    pidAngleZ.setYMinYMax(angleMaxCorrection);
#endif

    voltage = 0;
    dtMax = 0;

    MyMPU->resetFIFO();
}

void Quadrocopter::processCorrection()
{
    torqueAutomaticCorrection = RVector3D();

    switch(reactionType)
    {
    //    case ReactionAngularVelocity:
    //        torqueAutomaticCorrection = getAngularVelocityCorrection(angularVelocity, DeltaT.getTimeDifferenceSeconds());
    //        break;

    //    case ReactionAcceleration:
    //        torqueAutomaticCorrection = getAccelerationCorrection(angle, accelDataRaw);
    //        break;

    case ReactionAngle:
        torqueAutomaticCorrection = getAngleCorrection(angle, DeltaT.getTimeDifferenceSeconds());
        break;
    }
}

void Quadrocopter::processMotors()
{
    if(MController->getForce() >= MINIMUM_FLYING_THROTTLE)
        flyingTime += dt;
    if(flyingTime >= MINIMUM_FLYING_TIME)
        flying = 1;

    MController->setTorque(getTorques());
}

void Quadrocopter::iteration()
{
    if(dtMax < dt) dtMax = dt;

    if(MyMPU->getNewData()) // on each MPU data packet
    {

#ifdef DEBUG_FREQ_PIN
    freqLed.changeDigitalState();
#endif

#ifdef DEBUG_MPUBYTES_PIN
    mpuBytesLed.setState(MyMPU->bytesAvailableFIFO() > MyMPU->getPacketSize());
#endif

#ifdef DEBUG_DAC
    myLed.setState(0);
#endif

        { // Serial
            processSerialGetCommand();
            myLed.setState(5);
            processSerialDoCommand();
        }

#ifdef DEBUG_DAC
        myLed.setState(20);
#endif

        tCount.setTime();
        { // Sensors
            MyMPU->iteration();
            processSensorsData();
        }
        sensorsTime = tCount.getTimeDifferenceSeconds();

#ifdef DEBUG_DAC
        myLed.setState(80);
#endif


        { // Joystick

            processJoystickRx();
        }

        tCount.setTime();
        { // Corrections, Motors
            dt = DeltaT.getTimeDifferenceSeconds();

            processCorrection();
            DeltaT.setTime();
            processMotors();
        }
        calculationsTime = tCount.getTimeDifferenceSeconds();


#ifdef DEBUG_DAC
        myLed.setState(100);
#endif

        MyMPU->resetNewData();
    }
}

void Quadrocopter::MPUInterrupt()
{
    MyMPU->processInterrupt();
}

RVector3D Quadrocopter::getTorques()
{
    return(torqueAutomaticCorrection);
}
