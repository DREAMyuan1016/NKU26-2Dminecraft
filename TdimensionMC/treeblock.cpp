#include "treeblock.h"
#include <QPainter>
#include <QtGlobal>

TreeBlock::TreeBlock(Kind kind)
    : m_kind(kind)
    , m_hitPoints(kind == Kind::Trunk ? 2 : 1)
{

    if(kind == Kind::Trunk)
        m_treePixmap.load("D:/develop/QtProject/TdimensionMC/assets/blocks/trunk.png");
    else
        m_treePixmap.load("D:/develop/QtProject/TdimensionMC/assets/blocks/leaves.png");
}

TreeBlock::Kind TreeBlock::kind() const
{
    return m_kind;
}

bool TreeBlock::dig(int damage)
{
    m_hitPoints -= qMax(1, damage);
    return isDestroyed();
}

bool TreeBlock::isDestroyed() const
{
    return m_hitPoints <= 0;
}

void TreeBlock::paint(QPainter &painter, const QRect &rect) const
{
    if(!m_treePixmap.isNull()){
        painter.drawPixmap(rect, m_treePixmap);
    }
    else{
        painter.fillRect(rect, Qt::red);
    }




}
