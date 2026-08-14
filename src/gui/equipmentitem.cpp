#include "equipmentitem.h"
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QFont>

EquipmentItem::EquipmentItem(const Equipment& eq, QGraphicsItem* parent)
    : QGraphicsObject(parent)
    , m_eq(eq)
{
    setAcceptedMouseButtons(Qt::LeftButton);
}

QRectF EquipmentItem::boundingRect() const
{
    return QRectF(-22, -22, 44, 44);
}

void EquipmentItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->setRenderHint(QPainter::Antialiasing);

    // 背景
    painter->setPen(QPen(QColor(120, 120, 120), 1.5));
    painter->setBrush(QColor(45, 45, 45));
    painter->drawRoundedRect(QRectF(-20, -20, 40, 40), 4, 4);

    // 名字
    painter->setPen(QColor(220, 220, 220));
    QFont font = painter->font();
    font.setPointSize(8);
    painter->setFont(font);
    painter->drawText(QRectF(-18, -18, 36, 16), Qt::AlignCenter, m_eq.name);

    // 属性摘要
    QStringList stats;
    if (m_eq.hpBonus)    stats << QString("HP+%1").arg(m_eq.hpBonus);
    if (m_eq.atkBonus)   stats << QString("ATK+%1").arg(m_eq.atkBonus);
    if (m_eq.atkSpeedPct != 0)
        stats << QString("攻速+%1%").arg(static_cast<int>(-m_eq.atkSpeedPct * 100));
    if (m_eq.moveSpeedPct != 0)
        stats << QString("移速+%1%").arg(static_cast<int>(-m_eq.moveSpeedPct * 100));

    painter->setPen(QColor(160, 160, 160));
    font.setPointSize(6);
    painter->setFont(font);
    painter->drawText(QRectF(-18, 2, 36, 16), Qt::AlignCenter, stats.join(" "));
}

void EquipmentItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) return;

    m_dragging = true;
    m_dragOffset = event->pos();
    setZValue(10);
    emit dragStarted(this, event->scenePos());
    event->accept();
}

void EquipmentItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (!m_dragging) return;

    QPointF delta = event->scenePos() - event->lastScenePos();
    setPos(pos() + delta);
    emit dragMoved(this, event->scenePos());
    event->accept();
}

void EquipmentItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (!m_dragging || event->button() != Qt::LeftButton) return;

    m_dragging = false;
    setZValue(0);
    emit dragDropped(this, event->scenePos());
    event->accept();
}
