#ifndef PID_H
#define PID_H

#include "Definitions.h"

class PID
{
public:
    enum pidMode {DIFFERENCE_NORMAL, DIFFERENCE_ANGLE};

    double Kp, Ki, Kd;

    double data0;

	double yMin;
    double yMax;

    double PMin;
    double PMax;

    double IMin;
    double IMax;

    double DMin;
    double DMax;

    double P, I, D;

    double IUse;

    enum _angleDifferenceType{TNONE, T1, T2, T3};
    int angleDifferenceType, angleDifferenceTypePrev;

private:

    double eIntegral;
    double ePrev;

    double data0Prev;

    //temp
    double e, eDerivative; //error
    double y; //correction

    void prepare(double, double);
    void iteration();

    pidMode mode;

public:
    PID(pidMode nMode = PID::DIFFERENCE_NORMAL);

    void setKpKiKd(double, double, double);

    void setPMinMax(double);
    void setIMinMax(double);
    void setDMinMax(double);

    void setYMinYMax(double);

    void reset();
    double getY(double, double); // iteration
    double getY(double, double, double); // iteration

    void setIUse(bool a);
};

#endif // PID_H
