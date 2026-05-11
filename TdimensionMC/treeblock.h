#ifndef TREEBLOCK_H
#define TREEBLOCK_H

#include <QPainter>
#include <QRect>
#include <QPixmap>

class TreeBlock
{
public:
    enum class Kind {
        Trunk,
        Leaves
    };

    explicit TreeBlock(Kind kind = Kind::Trunk);

    Kind kind() const;
    bool dig(int damage = 1);
    bool isDestroyed() const;
    void paint(QPainter &painter, const QRect &rect) const;

private:
    Kind m_kind;
    int m_hitPoints;
        QPixmap m_treePixmap;
};

#endif // TREEBLOCK_H
