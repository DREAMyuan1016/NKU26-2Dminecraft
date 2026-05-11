#ifndef DIRTBLOCK_H
#define DIRTBLOCK_H

#include <QColor>
#include <QPainter>
#include <QRect>
#include <QPixmap>

class DirtBlock
{
public:
    enum class Kind {
        Dirt,
        Stone,
        Grassdirt,
        Black,
        Netherrack, // 地狱岩
        Magma,      // 岩浆块
        Quartz      // 石英块
    };

    explicit DirtBlock(Kind kind = Kind::Stone);

    bool isDestroyed() const;
    int remainingHitPoints() const;
    void reset();
    bool dig(int damage = 1);
    Kind kind() const;
    void paint(QPainter &painter, const QRect &rect) const;

private:
    Kind m_kind;
    QColor m_baseColor;
    QColor m_shadowColor;
    QColor m_stoneSpeckColor;
    int m_hitPoints;
    QPixmap m_blockPixmap;
};

#endif // DIRTBLOCK_H
