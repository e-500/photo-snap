#pragma once

#include "ImageAdjustments.h"

#include <QImage>
#include <QObject>
#include <QString>

class ImageDocument final : public QObject
{
    Q_OBJECT

public:
    explicit ImageDocument(QObject *parent = nullptr);

    void clear();
    bool load(const QString &path);
    bool setImage(const QImage &image, const QString &path = {});

    const QImage &originalImage() const;
    const ImageAdjustments &adjustments() const;
    const QString &path() const;
    bool isModified() const;

    void setRotation(double degrees);
    void setCropRect(const QRectF &cropRect);
    void setBrightness(double brightness);
    void setContrast(double contrast);
    void setSaturation(double saturation);
    void setGrayscale(bool enabled);
    void setSepia(bool enabled);
    void setTemperature(double temperature);
    void setSharpness(double sharpness);
    void resetAdjustments();

signals:
    void documentChanged();
    void adjustmentsChanged();

private:
    bool isAdjustmentNeutral() const;

    QImage m_originalImage;
    ImageAdjustments m_adjustments;
    QString m_path;
};
