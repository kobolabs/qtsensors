/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtSensors module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include "qwhipsensorgesturerecognizer.h"

#define _USE_MATH_DEFINES
#include <QtCore/qmath.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288419717
#endif
#ifndef M_PI_2
#define M_PI_2  1.57079632679489661923
#endif


QT_BEGIN_NAMESPACE

inline qreal calcYaw(double Ax, double Ay, double Az)
{
    return (float)atan2(Az, (sqrt(Ax * Ax + Ay * Ay)));
}

QWhipSensorGestureRecognizer::QWhipSensorGestureRecognizer(QObject *parent) :
    QSensorGestureRecognizer(parent), whipIt(0), lastX(0)
{
}

QWhipSensorGestureRecognizer::~QWhipSensorGestureRecognizer()
{
}

void QWhipSensorGestureRecognizer::create()
{
    accel = new QAccelerometer(this);
    accel->connectToBackend();

    orientation = new QOrientationSensor(this);
    orientation->connectToBackend();

    timer = new QTimer(this);

    qoutputrangelist outputranges = accel->outputRanges();

    if (outputranges.count() > 0)
        accelRange = (int)(outputranges.at(0).maximum);
    else
        accelRange = 44; //this should never happen

    connect(timer,SIGNAL(timeout()),this,SLOT(timeout()));
    timer->setSingleShot(true);
    timer->setInterval(750);

}

QString QWhipSensorGestureRecognizer::id() const
{
    return QString("QtSensors.whip");
}

bool QWhipSensorGestureRecognizer::start()
{
    connect(accel,SIGNAL(readingChanged()),this,SLOT(accelChanged()));
    active = accel->start();
    orientation->start();
    return active;
}

bool QWhipSensorGestureRecognizer::stop()
{
    accel->stop();
    active = accel->isActive();
    orientation->stop();
    disconnect(accel,SIGNAL(readingChanged()),this,SLOT(accelChanged()));
    return !active;
}

bool QWhipSensorGestureRecognizer::isActive()
{
    return active;
}

#define WHIP_THRESHOLD_FACTOR 0.95 //37
#define WHIP_DETECTION_FACTOR 0.3 // 11.7
#define WHIP_DEGREES 25

void QWhipSensorGestureRecognizer::accelChanged()
{
    qreal x = accel->reading()->x();
    qreal difference = lastX - x;
    if (abs(difference) < 1)
        return;

    qreal y = accel->reading()->y();
    qreal z = accel->reading()->z();

    qreal degreesZ = calc(calcYaw(x,y,z));

    if (whipIt) {
        if (((!wasNegative && difference > accelRange * WHIP_THRESHOLD_FACTOR)
                || (wasNegative && difference < -accelRange * WHIP_THRESHOLD_FACTOR))
                && abs(degreesZ) < WHIP_DEGREES
                && abs(detectedX) < abs(x)) {
            Q_EMIT whip();
            Q_EMIT detected("whip");
            whipIt = false;
        }

    } else if (((difference > 0 && difference < accelRange * WHIP_DETECTION_FACTOR)
                 || (difference < 0 && difference > -accelRange * WHIP_DETECTION_FACTOR))
               && abs(degreesZ) < WHIP_DEGREES
               && orientation->reading()->orientation() != QOrientationReading::FaceUp) {

        detectedX = x;
//        start of gesture
        timer->start();
        whipIt = true;
        if (difference > 0)
            wasNegative = false;
        else
            wasNegative = true;
    }
    lastX = x;
}

void QWhipSensorGestureRecognizer::timeout()
{
    whipIt = false;
}

qreal QWhipSensorGestureRecognizer::calc(qreal yrot)
{
    qreal aG = 1 * sin(yrot);
    qreal aK = 1 * cos(yrot);

    yrot = atan2(aG, aK);
    if (yrot > M_PI_2)
        yrot = M_PI - yrot;
    else if (yrot < -M_PI_2)
        yrot = -(M_PI + yrot);

    return yrot * 180 / M_PI;
}

QT_END_NAMESPACE