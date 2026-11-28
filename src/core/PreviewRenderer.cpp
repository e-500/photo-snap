#include "PreviewRenderer.h"

#include "ImageTransform.h"
#include "ImageFilters.h"

PreviewRenderer::PreviewRenderer(QObject *parent)
    : QObject(parent)
{
    m_timer.setSingleShot(true);
    m_timer.setInterval(30);
    connect(&m_timer, &QTimer::timeout,
            this, &PreviewRenderer::deliverScheduledPreview);
}

QImage PreviewRenderer::render(const QImage &original, const ImageAdjustments &adjustments) const
{
    if (original.isNull())
        return {};

    QImage result = ImageTransform::rotate(original, adjustments.rotation);
    result = ImageTransform::cropNormalized(result, adjustments.cropRect);
    result = ImageFilters::apply(result, adjustments);

    return result;
}

quint64 PreviewRenderer::requestPreview(const QImage &original, const ImageAdjustments &adjustments)
{
    const quint64 requestId = ++m_requestId;
    m_pendingOriginal = original;
    m_pendingAdjustments = adjustments;
    m_timer.start();
    return requestId;
}

void PreviewRenderer::deliverScheduledPreview()
{
    const quint64 requestId = m_requestId;
    if (requestId == m_deliveredRequestId)
        return;

    m_deliveredRequestId = requestId;
    QImage preview = render(m_pendingOriginal, m_pendingAdjustments);
    m_cache.insert(requestId, preview);
    emit previewReady(requestId, preview);
}

void PreviewRenderer::clearCache()
{
    m_cache.clear();
    m_deliveredRequestId = 0;
}
