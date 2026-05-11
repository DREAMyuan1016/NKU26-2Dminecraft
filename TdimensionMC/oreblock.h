#ifndef OREBLOCK_H
#define OREBLOCK_H

#include <QPainter>
#include <QRect>
#include <QPixmap>

class OreBlock
{
public:
    enum class Kind {
        Gold,
        Green,
        Red,
        Blue
    };

    explicit OreBlock(Kind kind = Kind::Gold);

    bool dig(int damage = 1);
    bool isDestroyed() const;
    Kind kind() const;
    void paint(QPainter &painter, const QRect &rect) const;

private:
    Kind m_kind;
    int m_hitPoints;
    QPixmap m_orePixmap;
};

#endif // OREBLOCK_H
