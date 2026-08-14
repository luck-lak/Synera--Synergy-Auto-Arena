#ifndef EQUIPMENTITEM_H
#define EQUIPMENTITEM_H

#include <QGraphicsObject>
#include "equipment/equipment.h"

class EquipmentItem : public QGraphicsObject
{
    Q_OBJECT

public:
    explicit EquipmentItem(const Equipment& eq, QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override;

    const Equipment& equipment() const { return m_eq; }

signals:
    void dragStarted(EquipmentItem* item, const QPointF& scenePos);
    void dragMoved(EquipmentItem* item, const QPointF& scenePos);
    void dragDropped(EquipmentItem* item, const QPointF& scenePos);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
    Equipment m_eq;
    bool m_dragging = false;
    QPointF m_dragOffset;
};

#endif // EQUIPMENTITEM_H
