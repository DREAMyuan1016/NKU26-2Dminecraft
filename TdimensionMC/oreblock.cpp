#include "oreblock.h"
#include <QPainter>
#include <QtGlobal>

OreBlock::OreBlock(Kind kind)
    : m_kind(kind)
    , m_hitPoints((kind == Kind::Gold || kind == Kind::Green) ? 5 : 4)
{

    switch(kind){
    case Kind::Gold: m_orePixmap.load("D:/develop/QTproject/TdimensionMC/assets/blocks/gold.png"); break;
    case Kind::Green: m_orePixmap.load("D:/develop/QTproject/TdimensionMC/assets/blocks/emerald.png"); break;
    case Kind::Red: m_orePixmap.load("D:/develop/QTproject/TdimensionMC/assets/blocks/redstone.png"); break;
    case Kind::Blue: m_orePixmap.load("D:/develop/QTproject/TdimensionMC/assets/blocks/diamond.png"); break;
    }
}

bool OreBlock::dig(int damage)
{
    m_hitPoints -= qMax(1, damage);
    return isDestroyed();
}

bool OreBlock::isDestroyed() const
{
    return m_hitPoints <= 0;
}

OreBlock::Kind OreBlock::kind() const
{
    return m_kind;
}

void OreBlock::paint(QPainter &painter, const QRect &rect) const
{

    if(!m_orePixmap.isNull()){
        painter.drawPixmap(rect, m_orePixmap);
    }

    else{
        painter.fillRect(rect, Qt::red);
    }




}
