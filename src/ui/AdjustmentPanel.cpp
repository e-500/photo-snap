#include "AdjustmentPanel.h"

#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>
#include <QCheckBox>

AdjustmentPanel::AdjustmentPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 6, 8, 6);

    auto *brightnessLayout = new QVBoxLayout;
    brightnessLayout->addWidget(new QLabel(tr("Brightness"), this));
    m_brightness = createSlider(-100, 100);
    m_brightnessSpin = createSpin(-1.0, 1.0, 0.01);
    brightnessLayout->addWidget(m_brightness);
    brightnessLayout->addWidget(m_brightnessSpin);
    layout->addLayout(brightnessLayout);

    auto *contrastLayout = new QVBoxLayout;
    contrastLayout->addWidget(new QLabel(tr("Contrast"), this));
    m_contrast = createSlider(50, 150);
    m_contrastSpin = createSpin(0.5, 1.5, 0.01);
    contrastLayout->addWidget(m_contrast);
    contrastLayout->addWidget(m_contrastSpin);
    layout->addLayout(contrastLayout);

    auto *saturationLayout = new QVBoxLayout;
    saturationLayout->addWidget(new QLabel(tr("Saturation"), this));
    m_saturation = createSlider(0, 200);
    m_saturationSpin = createSpin(0.0, 2.0, 0.01);
    saturationLayout->addWidget(m_saturation);
    saturationLayout->addWidget(m_saturationSpin);
    layout->addLayout(saturationLayout);

    auto *effectsLayout = new QVBoxLayout;
    m_grayscale = new QCheckBox(tr("Black and white"), this);
    m_sepia = new QCheckBox(tr("Sepia"), this);
    effectsLayout->addWidget(m_grayscale);
    effectsLayout->addWidget(m_sepia);
    layout->addLayout(effectsLayout);

    auto *temperatureLayout = new QVBoxLayout;
    temperatureLayout->addWidget(new QLabel(tr("Temperature"), this));
    m_temperatureSpin = createSpin(-1.0, 1.0, 0.01);
    temperatureLayout->addWidget(m_temperatureSpin);
    layout->addLayout(temperatureLayout);

    auto *sharpnessLayout = new QVBoxLayout;
    sharpnessLayout->addWidget(new QLabel(tr("Sharpness"), this));
    m_sharpnessSpin = createSpin(-1.0, 1.0, 0.01);
    sharpnessLayout->addWidget(m_sharpnessSpin);
    layout->addLayout(sharpnessLayout);

    auto *resetButton = new QPushButton(tr("Reset adjustments"), this);
    layout->addWidget(resetButton);

    connect(m_brightness, &QSlider::valueChanged, this, [this](int value) {
        const double brightness = value / 100.0;
        if (!qFuzzyCompare(m_brightnessSpin->value(), brightness))
            m_brightnessSpin->setValue(brightness);
    });
    connect(m_brightnessSpin, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, [this](double value) {
        const int sliderValue = qRound(value * 100.0);
        if (m_brightness->value() != sliderValue)
            m_brightness->setValue(sliderValue);
        emit brightnessChanged(value);
    });

    connect(m_contrast, &QSlider::valueChanged, this, [this](int value) {
        const double contrast = value / 100.0;
        if (!qFuzzyCompare(m_contrastSpin->value(), contrast))
            m_contrastSpin->setValue(contrast);
    });
    connect(m_contrastSpin, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, [this](double value) {
        const int sliderValue = qRound(value * 100.0);
        if (m_contrast->value() != sliderValue)
            m_contrast->setValue(sliderValue);
        emit contrastChanged(value);
    });

    connect(m_saturation, &QSlider::valueChanged, this, [this](int value) {
        const double saturation = value / 100.0;
        if (!qFuzzyCompare(m_saturationSpin->value(), saturation))
            m_saturationSpin->setValue(saturation);
    });
    connect(m_saturationSpin, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, [this](double value) {
        const int sliderValue = qRound(value * 100.0);
        if (m_saturation->value() != sliderValue)
            m_saturation->setValue(sliderValue);
        emit saturationChanged(value);
    });

    connect(resetButton, &QPushButton::clicked, this, &AdjustmentPanel::resetRequested);
    connect(m_grayscale, &QCheckBox::toggled,
            this, &AdjustmentPanel::grayscaleChanged);
    connect(m_sepia, &QCheckBox::toggled,
            this, &AdjustmentPanel::sepiaChanged);
    connect(m_temperatureSpin, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &AdjustmentPanel::temperatureChanged);
    connect(m_sharpnessSpin, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &AdjustmentPanel::sharpnessChanged);
    resetValues();
}

QSlider *AdjustmentPanel::createSlider(int minimum, int maximum)
{
    auto *slider = new QSlider(Qt::Horizontal, this);
    slider->setRange(minimum, maximum);
    slider->setMinimumWidth(140);
    return slider;
}

QDoubleSpinBox *AdjustmentPanel::createSpin(double minimum, double maximum, double step)
{
    auto *spin = new QDoubleSpinBox(this);
    spin->setRange(minimum, maximum);
    spin->setDecimals(2);
    spin->setSingleStep(step);
    spin->setMinimumWidth(80);
    return spin;
}

void AdjustmentPanel::resetValues()
{
    m_brightness->setValue(0);
    m_brightnessSpin->setValue(0.0);
    m_contrast->setValue(100);
    m_contrastSpin->setValue(1.0);
    m_saturation->setValue(100);
    m_saturationSpin->setValue(1.0);
    m_grayscale->setChecked(false);
    m_sepia->setChecked(false);
    m_temperatureSpin->setValue(0.0);
    m_sharpnessSpin->setValue(0.0);
}
