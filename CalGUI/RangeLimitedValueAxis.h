#ifndef RANGELIMITEDVALUEAXIS_H
#define RANGELIMITEDVALUEAXIS_H

#include <QtCharts/QValueAxis>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
QT_CHARTS_USE_NAMESPACE
#endif


class RangeLimitedValueAxis : public QValueAxis
{
public:
    RangeLimitedValueAxis(QObject *parent = nullptr);

    void setLowerLimit(qreal value);
    void setUpperLimit(qreal value);

    void disableLowerLimit();
    void disableUpperLimit();

    bool isLowerRangeLimited() const;
    bool isUpperRangeLimited() const;

    qreal lowerLimit() const;
    qreal upperLimit() const;

protected:
    bool m_limitLowerRange = false;
    bool m_limitUpperRange = false;

    qreal m_lowLimit = 0.0;
    qreal m_upLimit = 0.0;
};

#endif // RANGELIMITEDVALUEAXIS_H
