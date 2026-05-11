#include "dirtblock.h"
#include <QPainter>
#include <QtGlobal>

DirtBlock::DirtBlock(Kind kind)
    : m_kind(kind)
    , m_hitPoints(kind == Kind::Dirt || kind == Kind::Grassdirt ? 1 : 3)
{
    if(kind == Kind::Dirt)
        m_blockPixmap.load("D:/develop/QTproject/TdimensionMC/assets/blocks/dirt.png");
    else if(kind == Kind::Grassdirt)
        m_blockPixmap.load("D:/develop/QTproject/TdimensionMC/assets/blocks/grassdirt.png");
    else if(kind == Kind::Stone)
        m_blockPixmap.load("D:/develop/QTproject/TdimensionMC/assets/blocks/stone.png");
    else if(kind == Kind::Black)   // 新增
        m_blockPixmap.load("D:/develop/QTproject/TdimensionMC/assets/blocks/black.png");
    else if(kind == Kind::Netherrack)
        m_blockPixmap.load("D:/develop/QTproject/TdimensionMC/assets/blocks/diyuyan.png");
    else if(kind == Kind::Magma)
        m_blockPixmap.load("D:/develop/QTproject/TdimensionMC/assets/blocks/yanjiangkuai.png");
    else if(kind == Kind::Quartz)
        m_blockPixmap.load("D:/develop/QTproject/TdimensionMC/assets/blocks/shiying.png");
}

bool DirtBlock::isDestroyed() const
{
    return m_hitPoints <= 0;
}

void DirtBlock::reset()
{
    m_hitPoints = (m_kind == Kind::Dirt || m_kind == Kind::Grassdirt) ? 1 : 3;
}

bool DirtBlock::dig(int damage)
{
    m_hitPoints -= qMax(1, damage);
    return isDestroyed();
}

DirtBlock::Kind DirtBlock::kind() const
{
    return m_kind;
}

void DirtBlock::paint(QPainter &painter, const QRect &rect) const
{
    if(!m_blockPixmap.isNull() && m_blockPixmap.width() > 0 && m_blockPixmap.height() > 0){
        painter.drawPixmap(rect, m_blockPixmap);
    }
    else{
        painter.fillRect(rect, Qt::red);
    }





}
