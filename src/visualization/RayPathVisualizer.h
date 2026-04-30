#pragma once
#include <QQuickPaintedItem>
#include <QPainter>
#include <cmath>

namespace visualization {

class RayPathVisualizer : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(double angle READ angle WRITE setAngle NOTIFY angleChanged)
    Q_PROPERTY(double offset READ offset WRITE setOffset NOTIFY offsetChanged)

public:
    explicit RayPathVisualizer(QQuickItem* parent = nullptr) : QQuickPaintedItem(parent) {
        setAntialiasing(true);
    }

    double angle() const { return m_angle; }
    void setAngle(double a) {
        if (m_angle != a) {
            m_angle = a;
            emit angleChanged();
            update();
        }
    }

    double offset() const { return m_offset; }
    void setOffset(double o) {
        if (m_offset != o) {
            m_offset = o;
            emit offsetChanged();
            update();
        }
    }

    void paint(QPainter* painter) override {
        double w = width();
        double h = height();
        double cx = w / 2.0;
        double cy = h / 2.0;

        // Draw grid (the object)
        painter->setPen(QPen(QColor(100, 116, 139, 100), 1));
        int gridSize = 10;
        double step = qMin(w, h) * 0.8 / gridSize;
        double gridW = step * gridSize;
        double startX = cx - gridW / 2.0;
        double startY = cy - gridW / 2.0;

        for (int i = 0; i <= gridSize; ++i) {
            painter->drawLine(startX + i * step, startY, startX + i * step, startY + gridW);
            painter->drawLine(startX, startY + i * step, startX + gridW, startY + i * step);
        }

        // Draw phantom object (a circle)
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(56, 189, 248, 150));
        painter->drawEllipse(QPointF(cx, cy), gridW * 0.3, gridW * 0.3);

        // Calculate ray
        painter->setPen(QPen(Qt::green, 3));
        double radAngle = m_angle * M_PI / 180.0;
        
        // Ray origin on a circle around the object
        double radius = qMin(w, h) * 0.45;
        
        // The detector array is opposite to the source.
        // offset shifts the ray along the detector array perpendicular to the angle.
        double dirX = std::cos(radAngle);
        double dirY = std::sin(radAngle);
        
        // Perpendicular for offset
        double perpX = -std::sin(radAngle);
        double perpY = std::cos(radAngle);
        
        // Source position
        double srcX = cx - dirX * radius + perpX * m_offset;
        double srcY = cy - dirY * radius + perpY * m_offset;
        
        // Detector position
        double detX = cx + dirX * radius + perpX * m_offset;
        double detY = cy + dirY * radius + perpY * m_offset;

        painter->drawLine(QPointF(srcX, srcY), QPointF(detX, detY));

        // Draw source dot
        painter->setBrush(Qt::yellow);
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(QPointF(srcX, srcY), 5, 5);

        // Draw detector dot
        painter->setBrush(Qt::red);
        painter->drawEllipse(QPointF(detX, detY), 5, 5);
    }

signals:
    void angleChanged();
    void offsetChanged();

private:
    double m_angle = 0.0;
    double m_offset = 0.0;
};

} // namespace visualization
