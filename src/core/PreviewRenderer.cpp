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

    const QString key = cacheKey(original, adjustments);
    if (const auto it = m_cache.constFind(key);
        it != m_cache.constEnd()) {
        return it.value();
    }

    QImage result = ImageTransform::rotate(original, adjustments.rotation);
    result = ImageTransform::cropNormalized(result, adjustments.cropRect);
    result = ImageFilters::apply(result, adjustments);
    if (m_cache.size() >= 4)
        m_cache.clear();
    m_cache.insert(key, result);

    return result;
}

QImage PreviewRenderer::renderPreview(const QImage &original,
                                      const ImageAdjustments &adjustments) const
{
    if (original.isNull())
        return {};

    QImage previewSource = original;
    if (previewSource.width() > 1920 || previewSource.height() > 1080) {
        previewSource = previewSource.scaled(
            QSize(1920, 1080),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation);
    }
    QImage preview = render(previewSource, adjustments);
    if (preview.width() > 1920 || preview.height() > 1080) {
        preview = preview.scaled(
            QSize(1920, 1080),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation);
    }
    return preview;
}

QString PreviewRenderer::cacheKey(const QImage &original,
                                  const ImageAdjustments &adjustments) const
{
    return QStringLiteral("%1|%2x%3|%4|%5|%6|%7|%8|%9|%10|%11")
        .arg(original.cacheKey())
        .arg(original.width())
        .arg(original.height())
        .arg(adjustments.rotation, 0, 'g', 10)
        .arg(adjustments.cropRect.left(), 0, 'g', 10)
        .arg(adjustments.cropRect.top(), 0, 'g', 10)
        .arg(adjustments.cropRect.width(), 0, 'g', 10)
        .arg(adjustments.cropRect.height(), 0, 'g', 10)
        .arg(adjustments.brightness, 0, 'g', 10)
        .arg(adjustments.contrast, 0, 'g', 10)
        .arg(adjustments.saturation, 0, 'g', 10)
        + QStringLiteral("|%1|%2|%3|%4")
            .arg(adjustments.grayscale)
            .arg(adjustments.sepia)
            .arg(adjustments.temperature, 0, 'g', 10)
            .arg(adjustments.sharpness, 0, 'g', 10);
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
    QImage preview = renderPreview(m_pendingOriginal, m_pendingAdjustments);
    m_cache.insert(cacheKey(m_pendingOriginal, m_pendingAdjustments), preview);
    emit previewReady(requestId, preview);
}

void PreviewRenderer::clearCache()
{
    m_cache.clear();
    m_deliveredRequestId = 0;
}
