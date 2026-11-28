#pragma once

#include "../core/ImageAdjustments.h"

#include <QWidget>

class QDoubleSpinBox;
class QSlider;
class QToolButton;
class QCheckBox;

class AdjustmentPanel final : public QWidget
{
    Q_OBJECT

public:
    explicit AdjustmentPanel(QWidget *parent = nullptr);
    void resetValues();
    void setValues(const ImageAdjustments &adjustments);

signals:
    void brightnessChanged(double value);
    void contrastChanged(double value);
    void saturationChanged(double value);
    void grayscaleChanged(bool enabled);
    void sepiaChanged(bool enabled);
    void temperatureChanged(double value);
    void sharpnessChanged(double value);
    void resetRequested();

private:
    QSlider *createSlider(int minimum, int maximum);
    QDoubleSpinBox *createSpin(double minimum, double maximum, double step);

    QSlider *m_brightness = nullptr;
    QSlider *m_contrast = nullptr;
    QSlider *m_saturation = nullptr;
    QDoubleSpinBox *m_brightnessSpin = nullptr;
    QDoubleSpinBox *m_contrastSpin = nullptr;
    QDoubleSpinBox *m_saturationSpin = nullptr;
    QCheckBox *m_grayscale = nullptr;
    QCheckBox *m_sepia = nullptr;
    QDoubleSpinBox *m_temperatureSpin = nullptr;
    QDoubleSpinBox *m_sharpnessSpin = nullptr;
};
