#pragma once

#include "ImageAdjustments.h"

#include <QImage>
#include <QObject>
#include <QTimer>
#include <QHash>

class PreviewRenderer final : public QObject
{
    Q_OBJECT

public:
    explicit PreviewRenderer(QObject *parent = nullptr);
    QImage render(const QImage &original, const ImageAdjustments &adjustments) const;
    quint64 requestPreview(const QImage &original, const ImageAdjustments &adjustments);
    void clearCache();

signals:
    void previewReady(quint64 requestId, const QImage &preview);

private slots:
    void deliverScheduledPreview();

private:
    QTimer m_timer;
    quint64 m_requestId = 0;
    quint64 m_deliveredRequestId = 0;
    QImage m_pendingOriginal;
    ImageAdjustments m_pendingAdjustments;
    QHash<quint64, QImage> m_cache;
};
