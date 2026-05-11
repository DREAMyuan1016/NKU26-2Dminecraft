#ifndef GAMEWIDGET_H
#define GAMEWIDGET_H

#include "dirtblock.h"
#include "oreblock.h"
#include "treeblock.h"

#include <QColor>
#include <QElapsedTimer>
#include <QKeyEvent>
#include <QMap>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPoint>
#include <QPointF>
#include <QRandomGenerator>
#include <QRect>
#include <QRectF>
#include <QSet>
#include <QUrl>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QWheelEvent>
#include <QWidget>

class QMediaPlayer;
class QTimer;

class GameWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GameWidget(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:

    void drawBlockEdges(QPainter &painter, const QRect &rect, int column, int row) const;
    bool isOccupiedTile(int column, int row) const;



    struct Player {
        QRectF rect { 0.0, 0.0, 24.0, 52.0 };
        QPointF velocity { 0.0, 0.0 };
        double hurtTimer = 0.0;
        double knockbackTimer = 0.0;
        double crouchTimer = 0.0;
        double burnTimeRemaining = 0.0;
        double burnTickTimer = 0.0;
        double walkPhase = 0.0;
        double eatTimer = 0.0;
        qreal knockbackDirection = 0.0;
        bool crouching = false;
        bool onGround = false;
        bool facingLeft = false;
        double slowTimer = 0.0;      // 减速剩余时�?�?
        double slowMultiplier = 1.0; // 当前移动速度倍率�?.0 = 正常�?
    };


    struct Projectile {
        QPointF position;
        QPointF velocity;
        qreal radius = 6.0;
        int damage = 10;
        bool hostile = false;
        bool decorative = false;
        bool appliesBurn = false;
        double burnDuration = 0.0;
        double burnTickInterval = 0.0;
        double gravity = 0.0;
        double lifetime = -1.0;
        QColor color { 255, 78, 78 };
        QColor highlight { 255, 190, 190 };
    };

    enum class EnemyKind {
        Zombie,
        Skeleton,
        Blaze,
        ZombifiedPiglin,
        FogStalker
    };

    enum class SwordAttackType {
        None,
        Sweep,
        Downslash
    };

    struct Enemy {
        EnemyKind kind = EnemyKind::Zombie;
        QRectF rect;
        QPointF velocity { 0.0, 0.0 };
        int health = 100;
        int fogStalkerPhaseState = 1;
        double attackCooldown = 0.0;
        double sunlightTick = 0.0;
        double hurtTimer = 0.0;
        double knockbackTimer = 0.0;
        double blazeChargeTimer = 0.0;
        double blazeFloatPhase = 0.0;
        double angerTimeRemaining = 0.0;
        double lavaDamageTimer = 0.0;
        double attackWindupTimer = 0.0;
        double deathTimer = 0.0;
        double walkPhase = 0.0;
        double airbornePeakY = 0.0;
        double visibility = 0.9;
        double targetVisibility = 0.9;
        double teleportTimer = 0.0;
        double phantomTimer = 0.0;
        double fogStalkerStunTimer = 0.0;
        double fogStalkerTeleportFlashTimer = 0.0;
        double fogStalkerAfterimageTimer = 0.0;
        double fogStalkerCloneLifeTimer = 0.0;
        double fogStalkerLightningTimer = 0.0;
        QPointF fogStalkerTeleportTarget;
        qreal knockbackDirection = 0.0;
        qreal patrolOriginX = 0.0;
        qreal patrolDirection = 1.0;
        bool onGround = false;
        bool facingLeft = true;
        bool countedDeath = false;
        bool isPhantomClone = false;
        bool fogStalkerTeleportQueued = false;
        double turnCooldown = 0.0;  // 转向冷却
        double randomTurnTimer = 0.0;   // 空闲随机转向计时�?
    };

    struct LootFx {
        QPointF screenPos;
        QPointF velocity;
        qreal scale = 1.0;
        qreal alpha = 1.0;
        bool active = true;
    };

    enum class MiningTargetType {
        None,
        Dirt,
        Stone,
        Tree,
        Ore
    };

    struct MiningState {
        MiningTargetType type = MiningTargetType::None;
        QPoint tile { -1, -1 };
        double elapsedSeconds = 0.0;
        double requiredSeconds = 0.0;
        bool active = false;
    };

    enum class HotbarItem {
        Dirt = 0,
        Bow,
        Sword,
        Pickaxe,
        GoldenApple
    };

    enum class DepthLayer {
        Surface,
        ShallowCavern,
        DeepCavern
    };

    enum class GameEndState {
        None,
        Failed,
        Succeeded
    };

    struct Portal {
        QRect tiles;
        QPoint safeTile;
        DepthLayer destinationLayer = DepthLayer::Surface;
    };

    static constexpr int kTileSize = 32;
    static constexpr int kWorldColumns = 168;
    static constexpr int kWorldRows = 150;
    static constexpr int kSurfaceRow = 12;

    void initializeWorld();
    void generateTrees();
    void generateOreVeins();
    void generateWaterPools();
    void generateCavernLights();
    void generateNetherTransitionWalls();
    void carveEllipseCavern(const QPoint &center, int radiusX, int radiusY, double edgeNoiseChance = 0.0);
    void carveWideTunnel(const QPoint &from, const QPoint &to, int radius);
    int randomBetween(int minInclusive, int maxExclusive);
    bool randomChance(int percent);
    int surfaceRowAt(int column) const;
    void tick();
    void updateHorizontalMovement(double deltaSeconds);
    void updateVerticalMovement(double deltaSeconds);
    void updateProjectiles(double deltaSeconds);
    void updateEnemies(double deltaSeconds);
    void updateGoldenAppleEffect(double deltaSeconds);
    void updateDepthLayer(double deltaSeconds);
    void updatePortalTravel(double deltaSeconds);
    void checkPortalEntry();
    void resolveHorizontalCollisions(const QRectF &previousRect);
    void resolveVerticalCollisions(const QRectF &previousRect);
    void clampPlayerToWorld();
    void updateCamera();
    int tileKey(int column, int row) const;
    int tileKey(const QPoint &tile) const;
    QPointF screenToWorld(const QPoint &screenPos) const;
    QPointF playerWeaponAnchor() const;
    int experienceToNextLevel() const;
    void addExperience(int amount);
    double movementSpeedMultiplier() const;
    double miningSpeedMultiplier() const;
    MiningTargetType miningTargetTypeAt(const QPoint &tile) const;
    double miningDurationForTile(const QPoint &tile, MiningTargetType type) const;
    void startMining(const QPoint &tile);
    void stopMining();
    bool completeMiningTarget();
    void updateMining(double deltaSeconds);
    bool placeDirtBlock(const QPoint &screenPos);
    void useGoldenApple();
    void fireProjectile(const QPoint &screenPos);
    void performSwordAttack();
    void applySwordAttackDamage();
    void drawBackground(QPainter &painter);
    void drawWorld(QPainter &painter);
    void drawProjectiles(QPainter &painter);
    void drawEnemies(QPainter &painter);
    void drawLootFx(QPainter &painter);
    void drawWeapon(QPainter &painter, const QPointF &anchor, qreal angleDegrees, bool backArm = false);
    void drawTorch(QPainter &painter, const QRect &rect);
    void drawPlayer(QPainter &painter);
    void drawHud(QPainter &painter);
    void drawEndScreen(QPainter &painter);
    void restartGame();
    QRect endPrimaryButtonRect() const;
    QRect endSecondaryButtonRect() const;
    QRect hotbarSlotRect(int index) const;
    QRect blazeCounterRect() const;
    QString hotbarLabel(HotbarItem item) const;
    bool isSolidTile(int column, int row) const;
    bool isWaterTile(int column, int row) const;
    bool isLavaTile(int column, int row) const;
    bool isPlayerInWater() const;
    bool isPlayerInLava() const;
    bool isPlayerInFluid() const;
    bool rectIntersectsLava(const QRectF &rect) const;
    QRect tileRect(int column, int row) const;
    QPoint screenToTile(const QPoint &screenPos) const;
    bool isTileReachable(const QPoint &tile) const;
    int playerTileRow() const;
    DepthLayer depthLayerForRow(int tileRow) const;
    QString depthLayerTitle(DepthLayer layer) const;
    QString depthLayerMonsterHint(DepthLayer layer) const;
    void drawPortal(QPainter &painter, const QRect &rect) const;
    void generateSurfaceMobs();
    void generateNetherBlazes();
    void generateZombifiedPiglins();
    void generateFogStalker();
    bool teleportFogStalkerToRandomSafePosition(Enemy &enemy, qreal minTiles, qreal maxTiles, bool allowHighAir);
    bool hasFogStalkerClone() const;
    void spawnFogStalkerClone(const Enemy &source);
    bool isTileExposedToSky(int column, int row) const;
    void startLayerGracePeriod();
    void applyPurpleLandingZone(const QPoint &originTile, bool faceLeft);
    void updatePurpleBeam(double deltaSeconds);
    void updateLootFx(double deltaSeconds);
    void spawnBlazeRodLoot(const QPointF &worldPos);
    void enrageNearbyPiglins(const QPointF &worldCenter);
    void applyDamage(int damage, qreal sourceDirection = 0.0, bool isBlaze = false, bool playHurtSound = true);
    void applyEnemyDamage(Enemy &enemy, int damage, qreal sourceDirection);
    void playSound(const QString &relativePath, bool monsterSound = false);
    void playRandomSound(const QStringList &relativePaths, bool monsterSound = false);
    void playSurfaceMobSound(const QString &relativePath);
    void playRandomSurfaceMobSound(const QStringList &relativePaths);
    void stopSurfaceMobSounds();
    void stopMonsterSounds();
    void drawMiningCracks(QPainter &painter, const QRect &rect, double progress,
                          const QColor &color = QColor(24, 24, 24, 190)) const;

    QTimer *m_timer;
    QElapsedTimer m_frameClock;
    Player m_player;
    QMap<int, DirtBlock> m_dirtBlocks;
    QMap<int, OreBlock> m_oreBlocks;
    QMap<int, TreeBlock> m_treeBlocks;
    QSet<int> m_torchTiles;
    QSet<int> m_waterTiles;
    QSet<int> m_lavaTiles;
    QSet<int> m_pressedKeys;
    QSet<int> m_magmaTiles;   // 岩浆块位�?
    QRandomGenerator m_worldRng;
    QVector<QPoint> m_largeCavernCenters;
    QVector<int> m_surfaceRows;
    QVector<Projectile> m_projectiles;
    QVector<Enemy> m_enemies;
    QVector<LootFx> m_lootFx;
    QVector<QMediaPlayer *> m_soundPlayers;
    QSet<QMediaPlayer *> m_monsterSoundPlayers;
    QSet<QMediaPlayer *> m_surfaceMobSoundPlayers;
    QPointF m_cursorWorldPos;
    HotbarItem m_selectedHotbarItem = HotbarItem::Bow;
    int m_playerHealth = 100;
    int m_playerMaxHealth = 100;
    int m_playerLevel = 1;
    int m_levelExperience = 0;
    int m_defeatedEnemyCount = 0;
    int m_blazeDefeatedCount = 0;
    int m_zombiePigmanDefeatedCount = 0;
    int m_goldenHealth = 0;
    int m_goldenHealthMax = 50;
    int m_fogStalkerMaxHealth = 300;
    double m_swordSwingTimer = 0.0;
    double m_swordAttackCooldown = 0.0;
    double m_shootCooldown = 0.0;
    double m_miningFinishTimer = 0.0;
    double m_cameraX = 0.0;
    double m_cameraY = 0.0;
    double m_airbornePeakY = 0.0;
    double m_totalRunTime = 0.0;
    double m_goldenEffectTimeRemaining = 0.0;
    double m_goldenRegenTickAccumulator = 0.0;
    double m_layerHintTimer = 0.0;
    double m_portalCooldown = 0.0;
    double m_layerGraceTimer = 0.0;
    double m_fearDebuffTimer = 0.0;
    double m_fearFogBoost = 0.0;
    double m_phaseFlashTimer = 0.0;
    double m_playerOutlineFlashTimer = 0.0;
    double m_fogClearTimer = 0.0;
    double m_deepLayerSpawnTimer = -1.0;
    double m_victoryBannerTimer = 0.0;
    double m_victorySummaryTimer = 0.0;
    int m_fearDebuffStacks = 0;
    double m_playerFacingSameDirectionTimer = 0.0;
    double m_stepSoundTimer = 0.0;
    double m_zombieAmbientTimer = 0.0;
    double m_piglinAmbientTimer = 0.0;
    bool m_lastPlayerFacingLeft = false;
    bool m_goldenRegenActive = false;
    bool m_leftMousePressed = false;
    bool m_swordAttackHitApplied = false;
    bool m_swordAttackFacingLeft = false;
    bool m_pendingGoldenAppleUse = false;
    bool m_deepLayerBossSpawned = false;
    bool m_deepLayerBossDefeated = false;
    bool m_deepLayerEntryRegistered = false;
    bool m_controlsLocked = false;
    QColor m_screenFog = QColor(235, 245, 255, 18);
    double m_globalBrightness = 1.0;
    DepthLayer m_currentDepthLayer = DepthLayer::Surface;
    QString m_currentLayerHint;
    QString m_currentMonsterProfile = QStringLiteral("Normal mobs");
    QSet<int> m_discoveredLayerIds;
    QVector<Portal> m_portals;
    QSet<int> m_purpleTiles;
    QSet<int> m_purpleBeamTiles;

    // 第二层浅层传送门（需要烈焰人+猪灵击杀达标�?
    QSet<int> m_netherPortalTiles;
    QSet<int> m_netherPortalBeamTiles;
    bool m_netherPortalActive = false;
    QPoint m_netherPortalOrigin;   // 入口方块坐标（用于生成竖井）
    bool m_netherPortalLit = false; // 是否已点亮（达标�?

    // 黑方块传送门（向下直达第三层�?
    QSet<int> m_blackPortalTiles;         // 底座黑方块所在行（y=68�?
    QSet<int> m_blackPortalBeamTiles;     // 向下光束通道
    QPoint m_blackPortalOrigin;           // 底座左上角坐�?
    bool m_blackPortalActive = true;      // 是否激活（默认永久开启）


    MiningState m_miningState;
    SwordAttackType m_swordAttackType = SwordAttackType::None;
    bool m_inPurpleBeam = false;
    bool m_gameWon = false;
    GameEndState m_endState = GameEndState::None;
    QPixmap m_failScreenBg;
    QPixmap m_successScreenBg;
    QPointF m_deepLayerSpawnAnchor;

    QPixmap m_skyBg;
    QVector<QPixmap> m_bgList;
    QPixmap m_kuangdongBg;
    QPixmap m_netherBgUpper;
    QPixmap m_netherBgLower;
    QPixmap m_upperGapDirtBg;
    QPixmap m_upperGapStoneBg;
    QPixmap m_upperGapDeepStoneBg;
};



#endif // GAMEWIDGET_H
