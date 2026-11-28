#pragma once

#include <QRectF>
#include <QWidget>

class CropOverlay final : public QWidget
{
    Q_OBJECT

public:
    explicit CropOverlay(QWidget *parent = nullptr);

    void setAspectRatio(double ratio);
    void resetSelection();

signals:
    void cropChanged(const QRectF &normalizedRect);
    void confirmed();
    void cancelled();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QRect selectionPixels() const;
    QRectF normalizedSelection() const;
    void applyAspectRatio();
    enum class Handle {
        None, Move,
        TopLeft, Top, TopRight, Right,
        BottomRight, Bottom, BottomLeft, Left
    };
    Handle handleAt(const QPoint &point) const;
    QRectF m_selection;
    Handle m_activeHandle = Handle::None;
    QPoint m_dragOffset;
    double m_aspectRatio = 0.0;
};
