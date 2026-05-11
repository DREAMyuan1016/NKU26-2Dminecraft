#include "gamewidget.h"
#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QKeyEvent>
#include <QLineF>
#include <QLinearGradient>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QMediaContent>
#else
#include <QAudioOutput>
#endif
#include <QMediaPlayer>
#include <QMouseEvent>
#include <QPainter>
#include <QRandomGenerator>
#include <QString>
#include <QTimer>
#include <QTransform>
#include <QVector>
#include <QWheelEvent>
#include <QtMath>

#include <algorithm>
#include <cmath>
#include <utility>

namespace {
// 物理常量
constexpr double kGravity = 1800.0;
constexpr double kMoveSpeed = 230.0;
constexpr double kJumpSpeed = 480.0;
constexpr double kMaxFallSpeed = 980.0;
constexpr double kDigRangeTiles = 3.0;

// 水中移动参数
constexpr double kWaterMoveSpeedFactor = 0.45;
constexpr double kWaterGravityFactor = 0.28;
constexpr double kWaterMaxFallSpeed = 220.0;
constexpr double kWaterJumpSpeed = 360.0;

// 投射物参数
constexpr double kProjectileSpeed = 680.0;
constexpr double kShootCooldownSeconds = 0.18;

// 剑攻击参数
constexpr double kSwordSwingDurationSeconds = 0.25;
constexpr double kSwordAttackCooldownSeconds = 0.5;

// 数量上限
constexpr int kMaxProjectiles = 24;
constexpr int kMaxEnemies = 10;

// 快捷栏
constexpr int kHotbarSlotCount = 5;

// 生命值显示
constexpr int kHeartCount = 10;
constexpr int kGoldenHeartCount = 5;
constexpr int kHeartHitPoints = 10;

// 金苹果效果
constexpr double kGoldenAppleEffectSeconds = 5.0;
constexpr double kGoldenAppleRegenIntervalSeconds = 1.0;
constexpr int kGoldenAppleRegenAmount = 5;
constexpr double kGoldenAppleEatSeconds = 0.85;

// 挖掘动画
constexpr double kMiningFinishSwingSeconds = 0.16;

// 数学常量
constexpr double kPi = 3.14159265358979323846;

// 世界种子与基岩厚度
constexpr quint32 kFixedWorldSeed = 20260419u;
constexpr int kBedrockThickness = 3;

// 层级提示
constexpr double kLayerHintDurationSeconds = 2.0;

// 敌人通用尺寸
constexpr qreal kEnemyWidthTiles = 0.6;
constexpr qreal kEnemyHeightTiles = 1.8;
constexpr int kEnemyDamage = 2;
constexpr int kEnemyHealth = 100;
constexpr double kEnemyDeathAnimSeconds = 0.8;

// 烈焰人专属
constexpr int kBlazeHealth = 60;
constexpr int kBlazeFireballDamage = 2;
constexpr int kPiglinHealth = 20;
constexpr int kPiglinDamage = 2;

// 玩家攻击伤害
constexpr int kPlayerArrowDamage = 35;
constexpr int kSwordDamage = 50;

// 敌人生成距离
constexpr int kEnemySpawnMinDistanceTiles = 20;
constexpr int kEnemySpawnMaxDistanceTiles = 40;

// 剑攻击各阶段时间
constexpr double kSwordPreSwingSeconds = kSwordSwingDurationSeconds * (2.0 / 9.0);
constexpr double kSwordSwingPhaseSeconds = kSwordSwingDurationSeconds * (4.0 / 9.0);
constexpr double kSwordPostSwingSeconds = kSwordSwingDurationSeconds * (3.0 / 9.0);

// 受伤效果
constexpr double kHurtEffectSeconds = 0.5;
constexpr double kLayerGraceSeconds = 3.0;

// 击退参数
constexpr qreal kHitKnockbackSpeed = 64.0;
constexpr qreal kPlayerHitKnockbackScale = 0.7;
constexpr qreal kHitJumpSpeed = 340.0;
constexpr qreal kEnemyJumpSpeed = kJumpSpeed + 70.0;

// 敌人侦查距离
constexpr qreal kZombieAggroRangeTiles = 12.8;
constexpr qreal kSkeletonAggroRangeTiles = 16.0;

// 蹲伏
constexpr double kCrouchDurationSeconds = 0.5;
constexpr qreal kStandingPlayerHeight = 52.0;
constexpr qreal kCrouchingPlayerHeight = 48.0;

// 紫色传送光束参数
constexpr qreal kPurpleBeamFallSpeed = 46.0;
constexpr qreal kPurpleBeamFastFallSpeed = 136.0;
constexpr int kDefeatTargetCount = 25;

// 烈焰人移动
constexpr qreal kBlazeMoveSpeed = kMoveSpeed * 0.5;
constexpr qreal kBlazeAggroRangeTiles = 20.0;
constexpr qreal kBlazeLoseAggroRangeTiles = 24.0;
constexpr qreal kBlazeOptimalRangeTiles = 12.0;
constexpr qreal kBlazeMinSafeRangeTiles = 8.0;
constexpr double kBlazeChargeSeconds = 1.0;
constexpr double kBlazeCooldownSeconds = 2.0;
constexpr double kBlazeBurnDurationSeconds = 3.0;
constexpr double kBlazeBurnTickSeconds = 3.0;
constexpr double kBlazeHurtFlashSeconds = 0.1;
constexpr int kNetherTopOffsetRows = 50;
constexpr int kNetherBottomOffsetRows = 97;
constexpr qreal kBlazeHeightTiles = 3.6;
constexpr qreal kBlazeMinStoneClearanceTiles = 4.0;

// 僵尸猪灵
constexpr qreal kPiglinWidthTiles = 0.6;
constexpr qreal kPiglinHeightTiles = 1.9;
constexpr qreal kPiglinMoveSpeed = kMoveSpeed * 0.7;
constexpr qreal kPiglinAggroRangeTiles = 16.0;
constexpr qreal kPiglinChaseRangeTiles = 52.0;
constexpr qreal kPiglinLoseFocusTiles = 28.0;
constexpr qreal kPiglinPatrolRadiusTiles = 200.0;
constexpr double kPiglinAngerDurationSeconds = 20.0;
constexpr double kPiglinAttackCooldownSeconds = 1.2;
constexpr double kPiglinAttackWindupSeconds = 0.22;
constexpr double kZombieAttackSwingSeconds = 0.32;
constexpr double kSkeletonBowChargeSeconds = 1.0;
constexpr double kPiglinDeathAnimSeconds = 0.32;
constexpr qreal kPiglinJumpSpeed = kJumpSpeed + 90.0;

// 雾行者 (Boss)
constexpr int kFogStalkerHealth = 300;
constexpr qreal kFogStalkerWidthTiles = 0.7;
constexpr qreal kFogStalkerHeightTiles = 2.2;
constexpr qreal kFogStalkerBaseSpeed = kMoveSpeed * 1.1;
constexpr qreal kFogStalkerPhase2Speed = kMoveSpeed * 1.3;
constexpr qreal kFogStalkerPhase3Speed = kMoveSpeed * 1.5;
constexpr qreal kFogStalkerFearRangeTiles = 3.0;
constexpr int kFogStalkerFearDamage = 5;
constexpr double kFogStalkerFearTickSeconds = 0.5;
constexpr double kFogStalkerFearDebuffSeconds = 3.0;
constexpr int kFogStalkerFearMaxStacks = 3;
constexpr double kFogStalkerPhaseFlashSeconds = 0.35;
constexpr double kFogStalkerFogClearSeconds = 3.0;
constexpr double kFogStalkerDeathFadeSeconds = 0.6;
constexpr double kFogStalkerSpawnDelaySeconds = 5.0;
constexpr double kFogStalkerCloneIntervalSeconds = 5.0;
constexpr double kFogStalkerRandomTeleportSeconds = 4.0;
constexpr double kFogStalkerStunSeconds = 0.2;
constexpr double kFogStalkerTeleportFlashSeconds = 0.1;
constexpr double kFogStalkerAfterimageSeconds = 0.5;
constexpr double kFogStalkerSameFacingPunishSeconds = 3.0;
constexpr double kFogStalkerLightningStrikeSeconds = 5.0;
constexpr double kFogStalkerCloneLifetimeSeconds = 3.0;
constexpr qreal kFogStalkerCloneHealth = kSwordDamage * 3.0;
constexpr qreal kFogStalkerCloneDiveSpeed = kMoveSpeed * 2.0;
constexpr int kFogStalkerLightningDamage = 20;
constexpr qreal kFogStalkerTeleportMinTiles = 8.0;
constexpr qreal kFogStalkerTeleportMaxTiles = 10.0;
constexpr qreal kFogStalkerSpawnAheadTiles = 20.0;
constexpr qreal kFogStalkerRecoveryTeleportTiles = 10.0;
constexpr double kFogStalkerTeleportPhase2 = 3.0;
constexpr double kFogStalkerTeleportPhase3 = 2.0;
constexpr double kVictoryBannerSeconds = 5.0;
constexpr double kVictorySummaryDelaySeconds = 3.0;

// 资源路径宏定义
#ifndef TDIMENSIONMC_ASSET_ROOT
#define TDIMENSIONMC_ASSET_ROOT "D:/develop/Qtproject/TdimensionMC/assets"
#endif


QString assetPath(const char *relativePath)
{
    const QString relative = QString::fromLatin1(relativePath);
    const QString appAssetPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + QStringLiteral("/assets/") + relative);
    if (QFileInfo::exists(appAssetPath)) {
        return appAssetPath;
    }
    return QString::fromUtf8(TDIMENSIONMC_ASSET_ROOT) + QLatin1Char('/') + relative;
}

QString utf8Hex(const char *hex)
{
    return QString::fromUtf8(QByteArray::fromHex(hex));
}

/**
 * @brief 随机返回下界方块种类：70% 地狱岩，20% 岩浆块，10% 石英。
 */
DirtBlock::Kind randomNetherBlockKind(QRandomGenerator &rng)
{
    const double r = rng.generateDouble();
    if (r < 0.70) {
        return DirtBlock::Kind::Netherrack;
    }
    if (r < 0.90) {
        return DirtBlock::Kind::Magma;
    }
    return DirtBlock::Kind::Quartz;
}
} // 匿名命名空间结束


GameWidget::GameWidget(QWidget *parent)
    : QWidget(parent)
    , m_timer(new QTimer(this))
    , m_worldRng(kFixedWorldSeed)
{
    // 预加载背景图片
    m_bgList.append(QPixmap(assetPath("blocks/backgroundFirst.png")));
    m_bgList.append(QPixmap(assetPath("blocks/backgroundSecond.png")));
    m_kuangdongBg.load(assetPath("blocks/kuangdong.png"));
    m_netherBgUpper.load(assetPath("blocks/Uf.png"));
    m_netherBgLower.load(assetPath("blocks/Us.png"));
    m_upperGapDirtBg.load(assetPath("blocks/dirtsepng.png"));
    m_upperGapStoneBg.load(assetPath("blocks/stonese.png"));
    m_upperGapDeepStoneBg.load(assetPath("blocks/stonethi.png"));
    m_failScreenBg.load(assetPath("blocks/fail.png"));
    m_successScreenBg.load(assetPath("blocks/success.png"));
    m_stepSoundTimer = 0.0;
    m_zombieAmbientTimer = 0.0;
    m_piglinAmbientTimer = 0.0;

    // 设置焦点策略，允许接收键盘事件
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setFocus();

    // 初始化世界
    initializeWorld();
    generateOreVeins();
    generateWaterPools();
    generateCavernLights();
    generateTrees();
    generateNetherBlazes();
    generateZombifiedPiglins();

    // 移除出生点附近的水（避免初始困住）
    for (int col = 44; col <= 56; ++col) {
        for (int row = 43; row <= 50; ++row) {
            int key = tileKey(col, row);
            m_waterTiles.remove(key);
        }
    }

    // 设置玩家初始位置
    m_player.rect.moveTo(8.0 * kTileSize, (kSurfaceRow - 2) * kTileSize - m_player.rect.height());
    m_cursorWorldPos = m_player.rect.center() + QPointF(80.0, -18.0);
    m_airbornePeakY = m_player.rect.top();
    startLayerGracePeriod();
    applyPurpleLandingZone(QPoint(qFloor(m_player.rect.right() / kTileSize),
                                  qFloor(m_player.rect.bottom() / kTileSize) - 1),
                           false);
    updateDepthLayer(0.0);

    // 启动游戏主循环
    connect(m_timer, &QTimer::timeout, this, &GameWidget::tick);
    m_timer->start(16);
    m_frameClock.start();
}



// 音效播放系统
void GameWidget::playSound(const QString &relativePath, bool monsterSound)
{
    QMediaPlayer *player = new QMediaPlayer(this);
    m_soundPlayers.append(player);
    if (monsterSound) {
        m_monsterSoundPlayers.insert(player);
    }
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    connect(player, static_cast<void (QMediaPlayer::*)(QMediaPlayer::State)>(&QMediaPlayer::stateChanged),
            this, [this, player](QMediaPlayer::State state) {
                if (state == QMediaPlayer::StoppedState) {
                    m_soundPlayers.removeOne(player);
                    m_monsterSoundPlayers.remove(player);
                    m_surfaceMobSoundPlayers.remove(player);
                    player->deleteLater();
                }
            });
    player->setMedia(QMediaContent(QUrl::fromLocalFile(assetPath(relativePath.toLatin1().constData()))));
    player->setVolume(75);
#else
    auto *audioOutput = new QAudioOutput(player);
    audioOutput->setVolume(0.75);
    player->setAudioOutput(audioOutput);
    connect(player, &QMediaPlayer::playbackStateChanged,
            this, [this, player](QMediaPlayer::PlaybackState state) {
                if (state == QMediaPlayer::StoppedState) {
                    m_soundPlayers.removeOne(player);
                    m_monsterSoundPlayers.remove(player);
                    m_surfaceMobSoundPlayers.remove(player);
                    player->deleteLater();
                }
            });
    player->setSource(QUrl::fromLocalFile(assetPath(relativePath.toLatin1().constData())));
#endif
    player->play();
}


void GameWidget::playRandomSound(const QStringList &relativePaths, bool monsterSound)
{
    if (relativePaths.isEmpty()) {
        return;
    }
    playSound(relativePaths.at(m_worldRng.bounded(relativePaths.size())), monsterSound);
}


void GameWidget::playSurfaceMobSound(const QString &relativePath)
{
    playSound(relativePath, true);
    if (!m_soundPlayers.isEmpty()) {
        m_surfaceMobSoundPlayers.insert(m_soundPlayers.last());
    }
}

void GameWidget::playRandomSurfaceMobSound(const QStringList &relativePaths)
{
    if (relativePaths.isEmpty()) {
        return;
    }
    playSurfaceMobSound(relativePaths.at(m_worldRng.bounded(relativePaths.size())));
}


void GameWidget::stopSurfaceMobSounds()
{
    for (QMediaPlayer *player : std::as_const(m_surfaceMobSoundPlayers)) {
        if (player != nullptr) {
            player->stop();
            player->deleteLater();
        }
    }
    m_soundPlayers.erase(std::remove_if(m_soundPlayers.begin(), m_soundPlayers.end(), [this](QMediaPlayer *player) {
        return m_surfaceMobSoundPlayers.contains(player);
    }), m_soundPlayers.end());
    for (QMediaPlayer *player : std::as_const(m_surfaceMobSoundPlayers)) {
        m_monsterSoundPlayers.remove(player);
    }
    m_surfaceMobSoundPlayers.clear();
}


void GameWidget::stopMonsterSounds()
{
    for (QMediaPlayer *player : std::as_const(m_soundPlayers)) {
        if (!m_monsterSoundPlayers.contains(player)) {
            continue;
        }
        player->stop();
        player->deleteLater();
    }
    m_soundPlayers.erase(std::remove_if(m_soundPlayers.begin(), m_soundPlayers.end(), [this](QMediaPlayer *player) {
        return m_monsterSoundPlayers.contains(player);
    }), m_soundPlayers.end());
    m_surfaceMobSoundPlayers.clear();
    m_monsterSoundPlayers.clear();
}

//  绘制事件
void GameWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    drawBackground(painter);

    painter.save();
    painter.translate(-m_cameraX, -m_cameraY);
    drawWorld(painter);
    drawProjectiles(painter);
    drawEnemies(painter);
    drawPlayer(painter);
    painter.restore();
    drawLootFx(painter);

    // 计算玩家屏幕中心，用于视野特效
    const QPointF playerScreenCenter = m_player.rect.center() - QPointF(m_cameraX, m_cameraY);
    qreal visibleHalfWidthTiles = 6.0;
    qreal visibleHalfHeightTiles = 4.0;

    // 恐惧 Debuff 减小视野
    if (m_fearDebuffStacks > 0) {
        const qreal debuffScale = qMax<qreal>(0.45, 1.0 - 0.15 * m_fearDebuffStacks);
        visibleHalfWidthTiles *= debuffScale;
        visibleHalfHeightTiles *= debuffScale;
    }

    // 深层根据 Boss 生命进一步缩小视野
    if (m_currentDepthLayer == DepthLayer::DeepCavern) {
        for (const Enemy &enemy : std::as_const(m_enemies)) {
            if (enemy.kind != EnemyKind::FogStalker || enemy.isPhantomClone) {
                continue;
            }
            const qreal healthRatio = qreal(enemy.health) / qreal(m_fogStalkerMaxHealth);
            if (healthRatio <= 0.3) {
                visibleHalfWidthTiles = 1.8;
                visibleHalfHeightTiles = 1.4;
            } else if (healthRatio <= 0.7) {
                visibleHalfWidthTiles = 2.0;
                visibleHalfHeightTiles = 2.0;
            }
            break;
        }
    }

    const qreal visibleRadius = qMin(visibleHalfWidthTiles, visibleHalfHeightTiles) * kTileSize;
    const QRectF visibleRect(playerScreenCenter.x() - visibleRadius,
                             playerScreenCenter.y() - visibleRadius,
                             visibleRadius * 2.0,
                             visibleRadius * 2.0);

    // 全局暗度效果
    if (m_globalBrightness < 0.999) {
        const int darknessAlpha = qRound((1.0 - m_globalBrightness) * 180.0);
        QRadialGradient darknessGrad(playerScreenCenter, visibleRadius * 1.7);
        darknessGrad.setColorAt(0.0, QColor(0, 0, 0, 0));
        darknessGrad.setColorAt(0.62, QColor(0, 0, 0, 0));
        darknessGrad.setColorAt(0.82, QColor(0, 0, 0, darknessAlpha / 2));
        darknessGrad.setColorAt(1.0, QColor(0, 0, 0, darknessAlpha));
        painter.fillRect(rect(), darknessGrad);
    }

    // 雾效绘制：中心清晰，向外渐进变浓
    if (m_screenFog.alpha() > 0) {
        qreal maxRadius = qMax(width(), height()) * 0.75;
        QPointF center = playerScreenCenter;

        // 微小平移制造雾气飘动感
        double time = m_frameClock.elapsed() / 1000.0;
        double wobbleX = qCos(time * 0.7) * 15.0;
        double wobbleY = qSin(time * 0.5) * 12.0;
        center += QPointF(wobbleX, wobbleY);

        QRadialGradient fogGrad(center, maxRadius);
        fogGrad.setColorAt(0.0, QColor(m_screenFog.red(), m_screenFog.green(), m_screenFog.blue(), 0));
        fogGrad.setColorAt(0.3, QColor(m_screenFog.red(), m_screenFog.green(), m_screenFog.blue(), 0));
        fogGrad.setColorAt(0.5, QColor(m_screenFog.red(), m_screenFog.green(), m_screenFog.blue(), m_screenFog.alpha() / 3));
        fogGrad.setColorAt(0.8, QColor(m_screenFog.red(), m_screenFog.green(), m_screenFog.blue(), m_screenFog.alpha() * 2 / 3));
        fogGrad.setColorAt(1.0, QColor(m_screenFog.red(), m_screenFog.green(), m_screenFog.blue(), m_screenFog.alpha()));

        QPainterPath fogPath;
        fogPath.addRect(rect());
        QPainterPath visiblePath;
        visiblePath.addEllipse(visibleRect.adjusted(-20, -20, 20, 20));
        fogPath = fogPath.subtracted(visiblePath);   // 在可见区域挖一个洞

        painter.fillPath(fogPath, fogGrad);
    }

    // 阶段切换闪光效果 (全屏白色)
    if (m_phaseFlashTimer > 0.0) {
        const qreal flashRatio = qBound(0.0, m_phaseFlashTimer / kFogStalkerPhaseFlashSeconds, 1.0);
        painter.fillRect(rect(), QColor(255, 255, 255, qRound(185.0 * flashRatio)));
    }

    // 玩家轮廓高亮 (雷电打击等)
    if (m_playerOutlineFlashTimer > 0.0) {
        const qreal flashRatio = qBound(0.0, m_playerOutlineFlashTimer / 0.35, 1.0);
        const QRectF playerScreenRect = m_player.rect.translated(-m_cameraX, -m_cameraY).adjusted(-18.0, -18.0, 18.0, 18.0);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 255, 255, qRound(205.0 * flashRatio)));
        painter.drawRoundedRect(playerScreenRect, 10.0, 10.0);
    }

    drawHud(painter);
    drawEndScreen(painter);
}

// 键盘事件处理
void GameWidget::keyPressEvent(QKeyEvent *event)
{
    if (m_controlsLocked) {
        event->accept();
        return;
    }
    if (event->isAutoRepeat()) {
        event->ignore();
        return;
    }

    m_pressedKeys.insert(event->key());

    // W 键跳跃
    if (event->key() == Qt::Key_W && (m_player.onGround || isPlayerInFluid())) {
        m_player.velocity.setY(isPlayerInFluid() ? -kWaterJumpSpeed : -kJumpSpeed);
        m_player.onGround = false;
    }

    // Shift 键蹲伏
    if (event->key() == Qt::Key_Shift && !m_inPurpleBeam) {
        m_player.crouchTimer = kCrouchDurationSeconds;
        if (!m_player.crouching) {
            const qreal previousBottom = m_player.rect.bottom();
            m_player.rect.setHeight(kCrouchingPlayerHeight);
            m_player.rect.moveBottom(previousBottom);
            m_player.crouching = true;
        }
    }

    // 数字键 1-5 切换快捷栏
    if (event->key() >= Qt::Key_1 && event->key() <= Qt::Key_5) {
        m_selectedHotbarItem = static_cast<HotbarItem>(event->key() - Qt::Key_1);
        stopMining();
    }

    event->accept();
}

void GameWidget::keyReleaseEvent(QKeyEvent *event)
{
    if (m_controlsLocked) {
        event->accept();
        return;
    }
    if (event->isAutoRepeat()) {
        event->ignore();
        return;
    }

    m_pressedKeys.remove(event->key());
    event->accept();
}

// 鼠标事件处理
void GameWidget::mousePressEvent(QMouseEvent *event)
{
    // 结束画面按钮点击
    if (m_endState != GameEndState::None && event->button() == Qt::LeftButton) {
        if (endPrimaryButtonRect().contains(event->pos())) {
            if (m_endState == GameEndState::Failed) {
                restartGame();
            } else {
                window()->close();
            }
            event->accept();
            return;
        }
        if (m_endState == GameEndState::Failed && endSecondaryButtonRect().contains(event->pos())) {
            window()->close();
            event->accept();
            return;
        }
    }
    if (m_controlsLocked) {
        event->accept();
        return;
    }

    m_cursorWorldPos = screenToWorld(event->pos());

    // 右键：使用当前快捷栏物品
    if (event->button() == Qt::RightButton) {
        switch (m_selectedHotbarItem) {
        case HotbarItem::Dirt:         placeDirtBlock(event->pos()); break;
        case HotbarItem::Bow:          fireProjectile(event->pos()); break;
        case HotbarItem::Sword:        performSwordAttack(); break;
        case HotbarItem::Pickaxe:      break;
        case HotbarItem::GoldenApple:  useGoldenApple(); break;
        }
        update();
        event->accept();
        return;
    }

    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    // 左键按下：开始挖掘或敲掉火把
    m_leftMousePressed = true;

    const QPoint tile = screenToTile(event->pos());
    if (!isTileReachable(tile)) {
        stopMining();
        event->accept();
        return;
    }

    const int key = tileKey(tile);
    if (m_torchTiles.contains(key)) {
        m_torchTiles.remove(key);
        stopMining();
        update();
        event->accept();
        return;
    }

    startMining(tile);
    update();
    event->accept();
}

void GameWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_controlsLocked) {
        event->accept();
        return;
    }
    if (event->button() == Qt::LeftButton) {
        m_leftMousePressed = false;
        stopMining();
        update();
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void GameWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_controlsLocked) {
        event->accept();
        return;
    }
    m_cursorWorldPos = screenToWorld(event->pos());

    // 根据鼠标位置决定朝向
    if (m_swordSwingTimer <= 0.0) {
        m_player.facingLeft = m_cursorWorldPos.x() < m_player.rect.center().x();
    }

    // 如果鼠标移出正在挖掘的方块，停止挖掘
    if (m_miningState.active && screenToTile(event->pos()) != m_miningState.tile) {
        stopMining();
    }
    update();
    event->accept();
}

void GameWidget::wheelEvent(QWheelEvent *event)
{
    if (m_controlsLocked) {
        event->accept();
        return;
    }
    const int delta = event->angleDelta().y();
    if (delta == 0) {
        event->ignore();
        return;
    }

    int selectedIndex = static_cast<int>(m_selectedHotbarItem);
    if (delta > 0) {
        selectedIndex = (selectedIndex - 1 + kHotbarSlotCount) % kHotbarSlotCount;
    } else {
        selectedIndex = (selectedIndex + 1) % kHotbarSlotCount;
    }

    m_selectedHotbarItem = static_cast<HotbarItem>(selectedIndex);
    stopMining();
    update();
    event->accept();
}

// 工具函数
int GameWidget::randomBetween(int minInclusive, int maxExclusive)
{
    return m_worldRng.bounded(minInclusive, maxExclusive);
}

bool GameWidget::randomChance(int percent)
{
    return m_worldRng.bounded(100) < percent;
}

int GameWidget::surfaceRowAt(int column) const
{
    if (column < 0 || column >= m_surfaceRows.size()) {
        return kSurfaceRow;
    }
    return m_surfaceRows.at(column);
}

int GameWidget::playerTileRow() const
{
    return qFloor(m_player.rect.center().y() / kTileSize);
}

GameWidget::DepthLayer GameWidget::depthLayerForRow(int tileRow) const
{
    if (tileRow < kSurfaceRow + 50) return DepthLayer::Surface;
    if (tileRow < kSurfaceRow + 100) return DepthLayer::ShallowCavern;
    return DepthLayer::DeepCavern;
}

QString GameWidget::depthLayerTitle(DepthLayer layer) const
{
    switch (layer) {
    case DepthLayer::Surface:       return QStringLiteral("Overworld");
    case DepthLayer::ShallowCavern: return QStringLiteral("Nether");
    case DepthLayer::DeepCavern:    return QStringLiteral("Forbidden Depths");
    }
    return QStringLiteral("Unknown Zone");
}

QString GameWidget::depthLayerMonsterHint(DepthLayer layer) const
{
    switch (layer) {
    case DepthLayer::Surface:       return QStringLiteral("Normal mobs");
    case DepthLayer::ShallowCavern: return QStringLiteral("Nether mobs");
    case DepthLayer::DeepCavern:    return QStringLiteral("Boss mobs");
    }
    return QStringLiteral("Unknown mobs");
}

// 深度层级更新
void GameWidget::updateDepthLayer(double deltaSeconds)
{
    if (m_layerHintTimer > 0.0) m_layerHintTimer = qMax(0.0, m_layerHintTimer - deltaSeconds);
    if (m_layerGraceTimer > 0.0) m_layerGraceTimer = qMax(0.0, m_layerGraceTimer - deltaSeconds);

    const DepthLayer layer = depthLayerForRow(playerTileRow());
    if (layer != m_currentDepthLayer) {
        m_currentDepthLayer = layer;
        startLayerGracePeriod();
        // 首次进入深层且 Boss 未生成时，启动生成计时器
        if (layer == DepthLayer::DeepCavern && !m_deepLayerBossSpawned && !m_deepLayerBossDefeated
            && !m_deepLayerEntryRegistered) {
            m_deepLayerEntryRegistered = true;
            m_deepLayerSpawnTimer = kFogStalkerSpawnDelaySeconds;
            m_deepLayerSpawnAnchor = m_player.rect.center();
        }
        const int layerId = static_cast<int>(layer);
        if (!m_discoveredLayerIds.contains(layerId)) {
            m_discoveredLayerIds.insert(layerId);
            m_currentLayerHint = QStringLiteral("Entered %1").arg(depthLayerTitle(layer));
            m_layerHintTimer = kLayerHintDurationSeconds;
        }
    }

    // 根据当前层级设置雾气和亮度
    switch (m_currentDepthLayer) {
    case DepthLayer::Surface:
        m_screenFog = QColor(235, 245, 255, 18);
        m_globalBrightness = 1.0;
        break;
    case DepthLayer::ShallowCavern:
        m_screenFog = QColor(126, 138, 155, 72);
        m_globalBrightness = 0.78;
        break;
    case DepthLayer::DeepCavern:
        m_screenFog = QColor(200, 208, 220, 179);
        m_globalBrightness = 0.2;
        // 根据 Boss 生命值动态调整雾气与亮度
        for (const Enemy &enemy : std::as_const(m_enemies)) {
            if (enemy.kind != EnemyKind::FogStalker || enemy.isPhantomClone) continue;
            const qreal healthRatio = qreal(enemy.health) / qreal(m_fogStalkerMaxHealth);
            if (healthRatio <= 0.3) {
                m_screenFog = QColor(200, 208, 220, 230);
                m_globalBrightness = 0.1;
            } else if (healthRatio <= 0.7) {
                m_screenFog = QColor(200, 208, 220, 204);
                m_globalBrightness = 0.14;
            } else {
                m_screenFog = QColor(200, 208, 220, 179);
                m_globalBrightness = 0.2;
            }
            break;
        }
        if (m_fearFogBoost > 0.0)
            m_screenFog.setAlpha(qMin(242, m_screenFog.alpha() + qRound(m_fearFogBoost * 255.0)));
        if (m_phaseFlashTimer > 0.0) m_screenFog.setAlpha(242);
        if (m_fogClearTimer > 0.0) {
            const qreal clearProgress = 1.0 - qBound(0.0, m_fogClearTimer / kFogStalkerFogClearSeconds, 1.0);
            const int targetAlpha = qRound(230.0 + (51.0 - 230.0) * clearProgress);
            m_screenFog.setAlpha(qMin(m_screenFog.alpha(), targetAlpha));
            m_globalBrightness = qMax(m_globalBrightness, 0.2 + clearProgress * 0.55);
        }
        break;
    }
    m_currentMonsterProfile = depthLayerMonsterHint(m_currentDepthLayer);
}

void GameWidget::startLayerGracePeriod()
{
    m_layerGraceTimer = kLayerGraceSeconds;
}

// 紫色着陆区设置
void GameWidget::applyPurpleLandingZone(const QPoint &originTile, bool faceLeft)
{
    m_purpleTiles.clear();
    m_purpleBeamTiles.clear();
    const int startColumn = faceLeft ? originTile.x() - 4 : originTile.x() + 1;
    for (int dx = 0; dx < 4; ++dx) {
        const int column = startColumn + dx;

        // 清空上方紧邻的方块
        m_dirtBlocks.remove(tileKey(column, originTile.y() + 6));
        m_oreBlocks.remove(tileKey(column, originTile.y() + 6));
        m_treeBlocks.remove(tileKey(column, originTile.y() + 6));

        // 清空更上方的方块（多清两层）
        for (int up = 1; up <= 2; ++up) {
            int clearRow = originTile.y() + 6 - up;
            if (clearRow >= 0) {
                m_dirtBlocks.remove(tileKey(column, clearRow));
                m_oreBlocks.remove(tileKey(column, clearRow));
                m_treeBlocks.remove(tileKey(column, clearRow));
                m_waterTiles.remove(tileKey(column, clearRow));
                m_lavaTiles.remove(tileKey(column, clearRow));
            }
        }

        // 清空下方一行，并放置紫色石头
        m_dirtBlocks.remove(tileKey(column, originTile.y() + 9));
        m_oreBlocks.remove(tileKey(column, originTile.y() + 9));
        m_treeBlocks.remove(tileKey(column, originTile.y() + 9));

        for (int dy = 0; dy < 2; ++dy) {
            const int row = originTile.y() + dy + 7;
            if (column < 0 || column >= kWorldColumns || row < 0 || row >= kWorldRows - kBedrockThickness)
                continue;
            const int key = tileKey(column, row);
            m_waterTiles.remove(key);
            m_lavaTiles.remove(key);
            m_oreBlocks.remove(key);
            m_dirtBlocks.insert(key, DirtBlock(DirtBlock::Kind::Stone));
            m_purpleTiles.insert(key);   // 标记为紫色方块（用于渲染和传送判定）
        }
    }
}

// 传送光束更新
void GameWidget::updatePurpleBeam(double deltaSeconds)
{
    m_purpleBeamTiles.clear();
    m_netherPortalBeamTiles.clear();
    m_inPurpleBeam = false;

    // 地表传送门：击败一定数量敌人后激活
    if (m_defeatedEnemyCount >= kDefeatTargetCount && !m_purpleTiles.isEmpty()) {
        // 找出紫色方块的列范围和最低行
        int minColumn = kWorldColumns;
        int maxColumn = -1;
        int maxRow = -1;
        for (int key : m_purpleTiles) {
            const int column = key % kWorldColumns;
            const int row = key / kWorldColumns;
            minColumn = qMin(minColumn, column);
            maxColumn = qMax(maxColumn, column);
            maxRow = qMax(maxRow, row);
        }

        // 从最下行向下延伸光束，直到碰到固体方块
        for (int column = minColumn; column <= maxColumn; ++column) {
            for (int row = maxRow + 1; row < kWorldRows - kBedrockThickness; ++row) {
                if (m_dirtBlocks.contains(tileKey(column, row)) || m_oreBlocks.contains(tileKey(column, row)))
                    break;
                m_purpleBeamTiles.insert(tileKey(column, row));
            }
        }

        // 检测玩家是否站在光束内
        const int pMinCol = qFloor(m_player.rect.left() / kTileSize);
        const int pMaxCol = qFloor((m_player.rect.right() - 1.0) / kTileSize);
        const int pMinRow = qFloor(m_player.rect.top() / kTileSize);
        const int pMaxRow = qFloor((m_player.rect.bottom() - 1.0) / kTileSize);
        for (int row = pMinRow; row <= pMaxRow; ++row) {
            for (int column = pMinCol; column <= pMaxCol; ++column) {
                const int key = tileKey(column, row);
                if (m_purpleTiles.contains(key) || m_purpleBeamTiles.contains(key)) {
                    m_inPurpleBeam = true;
                    break;
                }
            }
            if (m_inPurpleBeam) break;
        }

        // 如果在光束中，启用垂直下落
        if (m_inPurpleBeam) {
            const qreal fallSpeed = m_pressedKeys.contains(Qt::Key_Shift)
            ? kPurpleBeamFastFallSpeed : kPurpleBeamFallSpeed;
            const QRectF previousRect = m_player.rect;
            m_player.velocity.setY(fallSpeed);
            m_player.rect.translate(0.0, fallSpeed * deltaSeconds);
            m_player.onGround = false;
            resolveVerticalCollisions(previousRect);
            if (m_player.onGround) m_player.velocity.setY(0.0);
        }
    }

    // 第二层传送门（Nether Portal）：需要击败特定怪物后点亮
    if (m_netherPortalLit && !m_netherPortalTiles.isEmpty()) {
        int minCol = kWorldColumns, maxCol = -1, topRow = kWorldRows;
        for (int key : m_netherPortalTiles) {
            int col = key % kWorldColumns;
            int row = key / kWorldColumns;
            minCol = qMin(minCol, col);
            maxCol = qMax(maxCol, col);
            topRow = qMin(topRow, row);
        }

        // 生成向下的光束，直到遇到固体
        for (int col = minCol; col <= maxCol; ++col) {
            for (int row = topRow + 1; row < kWorldRows - kBedrockThickness; ++row) {
                int key = tileKey(col, row);
                if (m_dirtBlocks.contains(key) || m_oreBlocks.contains(key)) break;
                m_netherPortalBeamTiles.insert(key);
            }
        }

        bool inNetherBeam = false;
        const int pMinCol2 = qFloor(m_player.rect.left() / kTileSize);
        const int pMaxCol2 = qFloor((m_player.rect.right() - 1.0) / kTileSize);
        const int pMinRow2 = qFloor(m_player.rect.top() / kTileSize);
        const int pMaxRow2 = qFloor((m_player.rect.bottom() - 1.0) / kTileSize);
        for (int row = pMinRow2; row <= pMaxRow2; ++row) {
            for (int col = pMinCol2; col <= pMaxCol2; ++col) {
                int key = tileKey(col, row);
                if (m_netherPortalTiles.contains(key) || m_netherPortalBeamTiles.contains(key)) {
                    inNetherBeam = true;
                    break;
                }
            }
            if (inNetherBeam) break;
        }

        if (inNetherBeam) {
            m_inPurpleBeam = true;
            const qreal fallSpeed = m_pressedKeys.contains(Qt::Key_Shift)
                                        ? kPurpleBeamFastFallSpeed : kPurpleBeamFallSpeed;
            const QRectF prevRect = m_player.rect;
            m_player.velocity.setY(fallSpeed);
            m_player.rect.translate(0.0, fallSpeed * deltaSeconds);
            m_player.onGround = false;
            resolveVerticalCollisions(prevRect);
            if (m_player.onGround) m_player.velocity.setY(0.0);
        }
    }

    // 黑方块传送门：永久存在，触碰即下落
    if (m_blackPortalActive && !m_blackPortalTiles.isEmpty()) {
        bool inBlackBeam = false;
        const int pMinCol = qFloor(m_player.rect.left() / kTileSize);
        const int pMaxCol = qFloor((m_player.rect.right() - 1.0) / kTileSize);
        const int pMinRow = qFloor(m_player.rect.top() / kTileSize);
        const int pMaxRow = qFloor((m_player.rect.bottom() - 1.0) / kTileSize);
        for (int row = pMinRow; row <= pMaxRow; ++row) {
            for (int col = pMinCol; col <= pMaxCol; ++col) {
                int key = tileKey(col, row);
                if (m_blackPortalTiles.contains(key) || m_blackPortalBeamTiles.contains(key)) {
                    inBlackBeam = true;
                    break;
                }
            }
            if (inBlackBeam) break;
        }

        if (inBlackBeam) {
            m_inPurpleBeam = true;
            const qreal fallSpeed = m_pressedKeys.contains(Qt::Key_Shift)
                                        ? kPurpleBeamFastFallSpeed : kPurpleBeamFallSpeed;
            const QRectF prevRect = m_player.rect;
            m_player.velocity.setY(fallSpeed);
            m_player.rect.translate(0.0, fallSpeed * deltaSeconds);
            m_player.onGround = false;
            resolveVerticalCollisions(prevRect);
            if (m_player.onGround) m_player.velocity.setY(0.0);
        }
    }
}
/////////////////////////////////////////////////////





void GameWidget::updatePortalTravel(double deltaSeconds)
{
    m_portalCooldown = qMax(0.0, m_portalCooldown - deltaSeconds);
}


bool GameWidget::isTileExposedToSky(int column, int row) const
{
    if (column < 0 || column >= kWorldColumns || row < 0 || row >= kWorldRows) {
        return false;
    }

    // 从该方块上方一行开始检查，直到最顶部
    for (int checkRow = row - 1; checkRow >= 0; --checkRow) {
        if (isSolidTile(column, checkRow)) {
            return false; // 遇到固体方块就不算暴露
        }
    }

    return true;
}


 //在地表生成敌对生物（僵尸和骷髅）。会限制同时存在的地表怪物数量，并根据玩家位置在周围一定距离的地表尝试生成，
//确保生成点不在视野内、不被方块阻挡，且与已有怪物保持一定距离。

void GameWidget::generateSurfaceMobs()
{
    // 统计现有地表怪物（僵尸和骷髅）数量
    int surfaceMobCount = 0;
    for (const Enemy &enemy : std::as_const(m_enemies)) {
        if (enemy.kind == EnemyKind::Zombie || enemy.kind == EnemyKind::Skeleton) {
            ++surfaceMobCount;
        }
    }

    if (surfaceMobCount >= kMaxEnemies) {
        return;
    }

    const int playerColumn = qBound(0, qFloor(m_player.rect.center().x() / kTileSize), kWorldColumns - 1);
    const int playerRow = qFloor(m_player.rect.center().y() / kTileSize);
    if (playerRow >= 50) {
        stopSurfaceMobSounds();
        return;
    }
    const int playerSurfaceRow = surfaceRowAt(playerColumn);
    // 只有玩家在地表层或非常接近地表时才生成地表怪物
    if (depthLayerForRow(playerRow) != DepthLayer::Surface || playerRow > playerSurfaceRow + 6) {
        return;
    }

    const qreal enemyWidth = kTileSize * kEnemyWidthTiles;
    const qreal enemyHeight = kTileSize * kEnemyHeightTiles;
    const QRectF viewportWorldRect(m_cameraX, m_cameraY, width(), height());
    // Lambda：检查指定矩形区域是否被固体方块或树阻挡
    const auto spawnRectBlocked = [this](const QRectF &rect) {
        const int minColumn = qFloor(rect.left() / kTileSize);
        const int maxColumn = qFloor((rect.right() - 1.0) / kTileSize);
        const int minRow = qFloor(rect.top() / kTileSize);
        const int maxRow = qFloor((rect.bottom() - 1.0) / kTileSize);

        for (int row = minRow; row <= maxRow; ++row) {
            for (int column = minColumn; column <= maxColumn; ++column) {
                if (column < 0 || column >= kWorldColumns || row < 0 || row >= kWorldRows) {
                    return true;
                }

                const int key = tileKey(column, row);
                const bool blockedTile = isSolidTile(column, row) || m_treeBlocks.contains(key);
                if (blockedTile && rect.intersects(QRectF(tileRect(column, row)))) {
                    return true;
                }
            }
        }

        return false;
    };

    // 构建候选列列表：以玩家列为中心，在最小和最大生成距离之间左右收集
    QVector<int> candidateColumns;
    candidateColumns.reserve((kEnemySpawnMaxDistanceTiles - kEnemySpawnMinDistanceTiles + 1) * 2);

    for (int distance = kEnemySpawnMinDistanceTiles; distance <= kEnemySpawnMaxDistanceTiles; ++distance) {
        const int leftColumn = playerColumn - distance;
        const int rightColumn = playerColumn + distance;
        if (leftColumn >= 2) {
            candidateColumns.append(leftColumn);
        }
        if (rightColumn <= kWorldColumns - 3) {
            candidateColumns.append(rightColumn);
        }
    }

    // 随机打乱候选列顺序，使生成更自然
    for (int i = candidateColumns.size() - 1; i > 0; --i) {
        const int swapIndex = m_worldRng.bounded(i + 1);
        candidateColumns.swapItemsAt(i, swapIndex);
    }

    // 遍历候选列尝试生成
    for (int column : std::as_const(candidateColumns)) {
        if (surfaceMobCount >= kMaxEnemies) {
            break;
        }

        const int row = surfaceRowAt(column);
        const qreal left = column * kTileSize + (kTileSize - enemyWidth) * 0.5;
        const qreal top = row * kTileSize - enemyHeight;
        const QRectF spawnRect(left, top, enemyWidth, enemyHeight);

        // 不在当前视口内生成，避免突兀出现
        if (viewportWorldRect.intersects(spawnRect)) {
            continue;
        }

        // 检查生成区域是否被方块阻挡
        if (spawnRectBlocked(spawnRect)) {
            continue;
        }

        // 确保与已有怪物保持一定距离
        bool overlapsExistingEnemy = false;
        for (const Enemy &enemy : std::as_const(m_enemies)) {
            const qreal horizontalTiles = qAbs(enemy.rect.center().x() - spawnRect.center().x()) / kTileSize;
            const qreal verticalTiles = qAbs(enemy.rect.center().y() - spawnRect.center().y()) / kTileSize;
            if (horizontalTiles < 4.0 && verticalTiles < 3.0) {
                overlapsExistingEnemy = true;
                break;
            }
        }

        if (overlapsExistingEnemy) {
            continue;
        }

        Enemy enemy;
        // 随机决定生成骷髅（45%概率）还是僵尸
        enemy.kind = (m_worldRng.bounded(100) < 45) ? EnemyKind::Skeleton : EnemyKind::Zombie;
        enemy.health = kEnemyHealth;
        enemy.rect = spawnRect;
        enemy.facingLeft = enemy.rect.center().x() > m_player.rect.center().x();
        m_enemies.append(enemy);
        ++surfaceMobCount;
    }
}


 //生成下界烈焰人。
 // 最多生成2个，有70%的概率实际进行生成尝试。在固定深度范围内随机选点，
 //要求生成区域无固体方块、不与已有烈焰人重叠，且不占据特殊方块（如紫色方块、传送门框架等）。

void GameWidget::generateNetherBlazes()
{
    int blazeCount = 0;
    for (const Enemy &enemy : std::as_const(m_enemies)) {
        if (enemy.kind == EnemyKind::Blaze) ++blazeCount;
    }
    const int maxBlazes = 2;
    if (blazeCount >= maxBlazes) return;
    if (!randomChance(70)) return;

    const qreal enemyWidth = kTileSize * kEnemyWidthTiles;
    const qreal enemyHeight = kTileSize * kBlazeHeightTiles;

    for (int attempt = 0; attempt < 30 && blazeCount < maxBlazes; ++attempt) {
        int col = randomBetween(10, kWorldColumns - 10);
        int row = randomBetween(kSurfaceRow + 65, kSurfaceRow + 95);

        // 检查生成区域是否有足够的空位（无固体方块）
        bool spaceClear = true;
        const int checkWidth = qCeil(enemyWidth / kTileSize) + 1;
        const int checkHeight = qCeil(enemyHeight / kTileSize) + 4;
        for (int dx = 0; dx < checkWidth && spaceClear; ++dx) {
            for (int dy = 0; dy < checkHeight; ++dy) {
                int c = col + dx;
                int r = row - dy;
                if (c < 0 || c >= kWorldColumns || r < 0) continue;
                if (isSolidTile(c, r)) {
                    spaceClear = false;
                    break;
                }
            }
        }
        if (!spaceClear) continue;

        // 检查是否与已有的烈焰人重叠
        QRectF spawnRect(col * kTileSize, row * kTileSize - enemyHeight, enemyWidth, enemyHeight);
        bool overlaps = false;
        for (const Enemy &other : std::as_const(m_enemies)) {
            if (other.kind != EnemyKind::Blaze) continue;
            if (other.rect.intersects(spawnRect.adjusted(-kTileSize * 2, -kTileSize * 2, kTileSize * 2, kTileSize * 2))) {
                overlaps = true;
                break;
            }
        }
        if (overlaps) continue;

        Enemy blaze;
        blaze.kind = EnemyKind::Blaze;
        blaze.health = kBlazeHealth;
        blaze.rect = spawnRect;
        blaze.facingLeft = m_worldRng.bounded(2) == 0;
        blaze.blazeFloatPhase = m_worldRng.generateDouble() * 6.283185307179586;
        blaze.onGround = false;
        blaze.airbornePeakY = blaze.rect.top();

        // 避免生成在特殊方块（紫色方块、传送门框架等）上方
        int checkCmin = qFloor(spawnRect.left() / kTileSize);
        int checkCmax = qFloor((spawnRect.right() - 1) / kTileSize);
        int checkRmin = qFloor(spawnRect.top() / kTileSize);
        int checkRmax = qFloor((spawnRect.bottom() - 1) / kTileSize);
        bool blocked = false;
        for (int c = checkCmin; c <= checkCmax && !blocked; ++c)
            for (int r = checkRmin; r <= checkRmax && !blocked; ++r) {
                int k = tileKey(c, r);
                if (m_purpleTiles.contains(k) || m_purpleBeamTiles.contains(k) ||
                    m_netherPortalTiles.contains(k) || m_netherPortalBeamTiles.contains(k) ||
                    m_blackPortalTiles.contains(k) || m_blackPortalBeamTiles.contains(k))
                    blocked = true;
            }
        if (blocked) continue;

        m_enemies.append(blaze);
        ++blazeCount;
    }
}


 //生成僵尸猪灵。
 //最多生成7个，在世界一定深度范围内寻找合适的位置，要求不与其他猪灵过于接近，
 // 且不占据特殊方块（紫色方块、传送门框架等）。生成后设置其巡逻原点与方向。

void GameWidget::generateZombifiedPiglins()
{
    int piglinCount = 0;
    for (const Enemy &enemy : std::as_const(m_enemies)) {
        if (enemy.kind == EnemyKind::ZombifiedPiglin) {
            ++piglinCount;
        }
    }

    if (piglinCount >= 7) {
        return;
    }

    const qreal enemyWidth = kTileSize * kPiglinWidthTiles;
    const qreal enemyHeight = kTileSize * kPiglinHeightTiles;

    // 收集一些有效的列：在指定深度范围内至少有两格非固体方块
    QVector<int> randomColumns;
    for (int attempt = 0; attempt < 50 && randomColumns.size() < 7; ++attempt) {
        int col = randomBetween(10, kWorldColumns - 10);
        bool valid = false;
        for (int r = kSurfaceRow + 50; r < kSurfaceRow + 95; ++r) {
            if (!isSolidTile(col, r) && !isSolidTile(col, r - 1)) {
                valid = true;
                break;
            }
        }
        if (valid && !randomColumns.contains(col))
            randomColumns.append(col);
    }
    // 如果仍未凑够列，用预定义位置填充
    while (randomColumns.size() < 7)
        randomColumns.append(kWorldColumns - 50 - randomColumns.size() * 5);

    for (int column : std::as_const(randomColumns)) {
        if (piglinCount >= 7) {
            break;
        }

        // 检查是否与已有猪灵过于接近
        bool occupied = false;
        for (const Enemy &enemy : std::as_const(m_enemies)) {
            if (enemy.kind != EnemyKind::ZombifiedPiglin) {
                continue;
            }
            if (qAbs(enemy.rect.center().x() / kTileSize - column) < 2.0) {
                occupied = true;
                break;
            }
        }
        if (occupied) {
            continue;
        }

        // 向下寻找地面行
        int floorRow = kSurfaceRow + 92;
        for (int row = kSurfaceRow + 70; row < kWorldRows - kBedrockThickness; ++row) {
            if (isSolidTile(column, row)) {
                floorRow = row;
                break;
            }
        }

        Enemy piglin;
        piglin.kind = EnemyKind::ZombifiedPiglin;
        piglin.health = kPiglinHealth;
        piglin.rect = QRectF(column * kTileSize + (kTileSize - enemyWidth) * 0.5,
                             floorRow * kTileSize - enemyHeight,
                             enemyWidth,
                             enemyHeight);
        piglin.facingLeft = m_worldRng.bounded(2) == 0;
        piglin.patrolOriginX = piglin.rect.center().x();
        piglin.patrolDirection = piglin.facingLeft ? -1.0 : 1.0;
        piglin.onGround = true;
        piglin.airbornePeakY = piglin.rect.top();

        // 避免生成在特殊方块上
        int checkCmin = qFloor(piglin.rect.left() / kTileSize);
        int checkCmax = qFloor((piglin.rect.right() - 1) / kTileSize);
        int checkRmin = qFloor(piglin.rect.top() / kTileSize);
        int checkRmax = qFloor((piglin.rect.bottom() - 1) / kTileSize);
        bool blocked = false;
        for (int c = checkCmin; c <= checkCmax && !blocked; ++c)
            for (int r = checkRmin; r <= checkRmax && !blocked; ++r) {
                int k = tileKey(c, r);
                if (m_purpleTiles.contains(k) || m_purpleBeamTiles.contains(k) ||
                    m_netherPortalTiles.contains(k) || m_netherPortalBeamTiles.contains(k) ||
                    m_blackPortalTiles.contains(k) || m_blackPortalBeamTiles.contains(k))
                    blocked = true;
            }
        if (blocked) continue;

        m_enemies.append(piglin);
        ++piglinCount;
    }
}


//生成深窟Boss“雾行者”（Fog Stalker）。仅在深窟层、Boss未生成且未击败、并已注册深窟进入条件时才会尝试生成。
 //在玩家前方一定距离搜索空旷区域放置Boss，若搜索失败则使用备用位置。
 //设置Boss的各项初始状态、阶段计时器，并播放提示音效。

void GameWidget::generateFogStalker()
{
    if (m_currentDepthLayer != DepthLayer::DeepCavern || m_deepLayerBossSpawned || m_deepLayerBossDefeated
        || !m_deepLayerEntryRegistered || m_deepLayerSpawnTimer > 0.0) {
        return;
    }

    // 如果已经存在雾行者（本体），则不再生成
    for (const Enemy &enemy : std::as_const(m_enemies)) {
        if (enemy.kind == EnemyKind::FogStalker && !enemy.isPhantomClone) {
            return;
        }
    }

    const qreal bossWidth = kTileSize * kFogStalkerWidthTiles;
    const qreal bossHeight = kTileSize * kFogStalkerHeightTiles;
    QRectF spawnRect(0.0, 0.0, bossWidth, bossHeight);
    // Lambda：检查矩形区域是否在深窟范围内且没有固体方块阻挡
    const auto isClearRect = [this](const QRectF &rect) {
        const int minColumn = qFloor(rect.left() / kTileSize) - 1;
        const int maxColumn = qFloor((rect.right() - 1.0) / kTileSize) + 1;
        const int minRow = qFloor(rect.top() / kTileSize) - 1;
        const int maxRow = qFloor((rect.bottom() - 1.0) / kTileSize) + 1;
        const int deepMinRow = kSurfaceRow + 100;
        const int deepMaxRow = kWorldRows - kBedrockThickness - 1;

        if (rect.top() < deepMinRow * kTileSize || rect.bottom() > (deepMaxRow + 1) * kTileSize) {
            return false;
        }

        for (int row = minRow; row <= maxRow; ++row) {
            for (int column = minColumn; column <= maxColumn; ++column) {
                if (column < 0 || column >= kWorldColumns || row < deepMinRow || row > deepMaxRow) {
                    return false;
                }
                if (isSolidTile(column, row) && rect.intersects(QRectF(tileRect(column, row)))) {
                    return false;
                }
            }
        }

        return true;
    };

    // 搜索玩家前方合适的位置
    bool foundSpawn = false;
    for (int offset = int(kFogStalkerSpawnAheadTiles); offset <= 36 && !foundSpawn; offset += 2) {
        for (int directionSign : { m_player.facingLeft ? -1 : 1, m_player.facingLeft ? 1 : -1 }) {
            for (int rowOffset = 0; rowOffset <= 4 && !foundSpawn; ++rowOffset) {
                for (int verticalSign : { 1, -1 }) {
                    QRectF candidate(0.0, 0.0, bossWidth, bossHeight);
                    candidate.moveCenter(QPointF(m_player.rect.center().x() + directionSign * offset * kTileSize,
                                                 m_player.rect.center().y() + verticalSign * rowOffset * kTileSize));
                    if (!isClearRect(candidate)) {
                        continue;
                    }
                    spawnRect = candidate;
                    foundSpawn = true;
                    break;
                }
            }
        }
    }

    // 若未找到，尝试使用紧急备用位置
    if (!foundSpawn) {
        QRectF emergencyRect(0.0, 0.0, bossWidth, bossHeight);
        emergencyRect.moveCenter(QPointF(m_player.rect.center().x() + (m_player.facingLeft ? -1.0 : 1.0) * 12.0 * kTileSize,
                                         m_player.rect.center().y()));
        if (isClearRect(emergencyRect)) {
            spawnRect = emergencyRect;
            foundSpawn = true;
        }
    }

    if (!foundSpawn) {
        m_deepLayerSpawnTimer = 1.0;
        return;
    }

    Enemy boss;
    boss.kind = EnemyKind::FogStalker;
    boss.health = kFogStalkerHealth;
    boss.rect = spawnRect;
    boss.visibility = 0.9;
    boss.targetVisibility = 0.9;
    boss.teleportTimer = kFogStalkerTeleportPhase2;
    boss.phantomTimer = kFogStalkerCloneIntervalSeconds;
    boss.fogStalkerLightningTimer = kFogStalkerLightningStrikeSeconds;
    boss.fogStalkerPhaseState = 1;
    boss.fogStalkerTeleportTarget = boss.rect.center();
    boss.onGround = false;
    boss.facingLeft = boss.rect.center().x() < m_player.rect.center().x();
    m_enemies.append(boss);
    m_deepLayerBossSpawned = true;
    m_currentLayerHint = QStringLiteral("The fog is watching...");
    playSound(QStringLiteral("sounds/fogwatching.mp3"));
    m_layerHintTimer = 2.5;
}


 // 检查当前是否已经存在雾行者分身（Phantom Clone）。 如果存在分身则返回true

bool GameWidget::hasFogStalkerClone() const
{
    for (const Enemy &enemy : m_enemies) {
        if (enemy.kind == EnemyKind::FogStalker && enemy.isPhantomClone) {
            return true;
        }
    }
    return false;
}


 //尝试将指定的雾行者敌人（本体或分身）传送到以玩家为中心的随机安全位置。
// 在深窟层范围内随机选取角度和距离，检查目标位置是否合法（在边界内、无固体方块）。
//若allowHighAir为true，可以添加额外的垂直随机偏移。
//传送成功后更新敌人的位置、速度、可见性和相关视觉计时器。
//要传送的敌人引用
bool GameWidget::teleportFogStalkerToRandomSafePosition(Enemy &enemy, qreal minTiles, qreal maxTiles, bool allowHighAir)
{
    const int deepMinRow = kSurfaceRow + 100;
    const int deepMaxRow = kWorldRows - kBedrockThickness - 1;
    const qreal minRadius = minTiles * kTileSize;
    const qreal maxRadius = maxTiles * kTileSize;

    for (int attempt = 0; attempt < 48; ++attempt) {
        const qreal angle = m_worldRng.generateDouble() * kPi * 2.0;
        const qreal radius = minRadius + m_worldRng.generateDouble() * (maxRadius - minRadius);
        const qreal verticalOffset = allowHighAir ? (m_worldRng.generateDouble() * 8.0 - 5.0) * kTileSize : 0.0;
        QRectF candidate = enemy.rect;
        candidate.moveCenter(QPointF(m_player.rect.center().x() + qCos(angle) * radius,
                                     m_player.rect.center().y() + qSin(angle) * radius + verticalOffset));

        const int minColumn = qFloor(candidate.left() / kTileSize) - 1;
        const int maxColumn = qFloor((candidate.right() - 1.0) / kTileSize) + 1;
        const int minRow = qFloor(candidate.top() / kTileSize) - 1;
        const int maxRow = qFloor((candidate.bottom() - 1.0) / kTileSize) + 1;
        bool blocked = candidate.top() < deepMinRow * kTileSize
                       || candidate.bottom() > (deepMaxRow + 1) * kTileSize
                       || candidate.left() < 0.0
                       || candidate.right() > kWorldColumns * kTileSize;
        for (int row = minRow; row <= maxRow && !blocked; ++row) {
            for (int column = minColumn; column <= maxColumn; ++column) {
                if (column < 0 || column >= kWorldColumns || row < deepMinRow || row > deepMaxRow) {
                    blocked = true;
                    break;
                }
                if (isSolidTile(column, row) && candidate.intersects(QRectF(tileRect(column, row)))) {
                    blocked = true;
                    break;
                }
            }
        }

        if (!blocked) {
            // 找到安全位置，更新敌人状态
            enemy.rect = candidate;
            enemy.velocity = QPointF(0.0, 0.0);
            enemy.visibility = 0.08;
            enemy.targetVisibility = enemy.isPhantomClone ? 0.52 : 0.9;
            enemy.fogStalkerAfterimageTimer = kFogStalkerAfterimageSeconds;
            enemy.fogStalkerTeleportFlashTimer = kFogStalkerTeleportFlashSeconds;
            m_phaseFlashTimer = kFogStalkerPhaseFlashSeconds;
            return true;
        }
    }
    return false;
}

 //生成雾行者分身（如果当前还没有分身）。
 //复制源敌人的大部分属性，但将生命值、可见性、计时器等修改为分身特有的设定，并尝试将其传送到随机安全位置或源敌人附近。

void GameWidget::spawnFogStalkerClone(const Enemy &source)
{
    if (hasFogStalkerClone()) {
        return;
    }

    Enemy clone = source;
    clone.isPhantomClone = true;
    clone.health = int(kFogStalkerCloneHealth);
    clone.visibility = 0.52;
    clone.targetVisibility = 0.52;
    clone.attackCooldown = 0.0;
    clone.teleportTimer = kFogStalkerRandomTeleportSeconds;
    clone.phantomTimer = 0.0;
    clone.deathTimer = 0.0;
    clone.countedDeath = false;
    clone.fogStalkerStunTimer = 0.0;
    clone.fogStalkerCloneLifeTimer = kFogStalkerCloneLifetimeSeconds;
    clone.fogStalkerLightningTimer = 0.0;
    clone.fogStalkerTeleportQueued = false;
    clone.fogStalkerTeleportFlashTimer = 0.0;
    clone.fogStalkerAfterimageTimer = 0.0;
    // 尝试传送到随机安全位置，失败则放到源旁边
    if (!teleportFogStalkerToRandomSafePosition(clone, 8.0, 12.0, true)) {
        clone.rect.moveCenter(source.rect.center() + QPointF(source.facingLeft ? -8.0 * kTileSize : 8.0 * kTileSize, -5.0 * kTileSize));
    }
    m_enemies.append(clone);
}


//激怒在指定世界坐标周围一定范围内的所有僵尸猪灵，使它们进入愤怒状态。
//激怒中心的世界坐标（通常是玩家攻击猪灵的位置）

void GameWidget::enrageNearbyPiglins(const QPointF &worldCenter)
{
    for (int enemyIndex = 0; enemyIndex < m_enemies.size(); ++enemyIndex) {
        Enemy &enemy = m_enemies[enemyIndex];
        if (enemy.kind != EnemyKind::ZombifiedPiglin) {
            continue;
        }

        const qreal distanceTiles = QLineF(enemy.rect.center(), worldCenter).length() / kTileSize;
        if (distanceTiles <= kPiglinAggroRangeTiles) {
            enemy.angerTimeRemaining = kPiglinAngerDurationSeconds;
        }
    }
}


//检查玩家是否进入传送门。如果玩家的底部中心点位于某个传送门的方块列表中，则执行传送：
//更新玩家位置、速度，重置状态，播放音效，应用到达地带特效，并更新深度层提示。
 //首次进入深窟时会设置Boss生成计时器。

void GameWidget::checkPortalEntry()
{
    if (m_portalCooldown > 0.0) {
        return;
    }

    const QPoint tile = QPoint(qFloor(m_player.rect.center().x() / kTileSize),
                               qFloor((m_player.rect.bottom() - 4.0) / kTileSize));
    for (const Portal &portal : std::as_const(m_portals)) {
        if (!portal.tiles.contains(tile)) {
            continue;
        }

        // 传送玩家
        m_player.rect.moveTo(portal.safeTile.x() * kTileSize,
                             portal.safeTile.y() * kTileSize - m_player.rect.height());
        m_player.velocity = QPointF(0.0, 0.0);
        m_player.onGround = false;
        m_portalCooldown = 0.8;
        stopMonsterSounds();
        playSound(QStringLiteral("sounds/travel.mp3"));
        startLayerGracePeriod();
        applyPurpleLandingZone(portal.safeTile, false);
        stopMining();
        updateDepthLayer(0.0);
        m_currentLayerHint = QStringLiteral("Entered %1").arg(depthLayerTitle(portal.destinationLayer));
        m_layerHintTimer = kLayerHintDurationSeconds;
        // 首次进入深窟时注册并启动Boss生成倒计时
        if (portal.destinationLayer == DepthLayer::DeepCavern && !m_deepLayerBossSpawned && !m_deepLayerBossDefeated
            && !m_deepLayerEntryRegistered) {
            m_deepLayerEntryRegistered = true;
            m_deepLayerSpawnTimer = kFogStalkerSpawnDelaySeconds;
            m_deepLayerSpawnAnchor = m_player.rect.center();
        }
        break;
    }
}

// 绘制传送门的外观，使用多层矩形和不同颜色来表现传送门的视觉效果。
void GameWidget::drawPortal(QPainter &painter, const QRect &rect) const
{
    painter.fillRect(rect, QColor(22, 12, 30));
    painter.fillRect(rect.adjusted(2, 2, -2, -2), QColor(94, 45, 148));
    painter.fillRect(rect.adjusted(6, 6, -6, -6), QColor(189, 120, 255, 215));
}


void GameWidget::carveEllipseCavern(const QPoint &center, int radiusX, int radiusY, double edgeNoiseChance)
{
    m_largeCavernCenters.append(center);

    for (int row = center.y() - radiusY - 1; row <= center.y() + radiusY + 1; ++row) {
        for (int column = center.x() - radiusX - 1; column <= center.x() + radiusX + 1; ++column) {
            if (column < 0 || column >= kWorldColumns || row < 0 || row >= kWorldRows) {
                continue;
            }

            // 不破坏接近地表的方块
            if (row < surfaceRowAt(column) + 2) {
                continue;
            }

            const double normalizedX = double(column - center.x()) / qMax(1, radiusX);
            const double normalizedY = double(row - center.y()) / qMax(1, radiusY);
            const double ellipseValue = normalizedX * normalizedX + normalizedY * normalizedY;
            const int key = tileKey(column, row);

            if (ellipseValue <= 1.0) {
                m_dirtBlocks.remove(key);
                continue;
            }

            // 在椭圆边缘附近，以一定概率移除方块，形成不规则边缘
            if (ellipseValue <= 1.22 && edgeNoiseChance > 0.0
                && m_worldRng.generateDouble() < edgeNoiseChance) {
                m_dirtBlocks.remove(key);
            }
        }
    }
}


void GameWidget::carveWideTunnel(const QPoint &from, const QPoint &to, int radius)
{
    const int steps = qMax(qAbs(to.x() - from.x()), qAbs(to.y() - from.y())) * 3 + 1;
    for (int step = 0; step <= steps; ++step) {
        const double t = steps == 0 ? 0.0 : double(step) / double(steps);
        const double curveLift = qSin(t * kPi) * 1.5; // 使用正弦曲线制造起伏
        const int centerColumn = qRound(from.x() + (to.x() - from.x()) * t);
        const int centerRow = qRound(from.y() + (to.y() - from.y()) * t - curveLift);

        for (int row = centerRow - radius - 1; row <= centerRow + radius + 1; ++row) {
            for (int column = centerColumn - radius - 2; column <= centerColumn + radius + 2; ++column) {
                if (column < 0 || column >= kWorldColumns || row < 0 || row >= kWorldRows) {
                    continue;
                }

                // 不破坏接近地表的方块
                if (row < surfaceRowAt(column) + 2) {
                    continue;
                }

                const double dx = double(column - centerColumn) / qMax(1, radius + 1);
                const double dy = double(row - centerRow) / qMax(1, radius);
                const double distance = dx * dx + dy * dy;
                // 在半径内的方块移除，边缘以一定概率移除产生自然感
                if (distance <= 1.0 || (distance <= 1.18 && randomChance(65))) {
                    m_dirtBlocks.remove(tileKey(column, row));
                }
            }
        }
    }
}




/////////////////////////////////

void GameWidget::drawWorld(QPainter &painter)
{
    const int minColumn = qMax(0, qFloor(m_cameraX / kTileSize) - 1);
    const int maxColumn = qMin(kWorldColumns - 1, qFloor((m_cameraX + width()) / kTileSize) + 1);
    const int minRow = qMax(0, qFloor(m_cameraY / kTileSize) - 1);
    const int maxRow = qMin(kWorldRows - 1, qFloor((m_cameraY + height()) / kTileSize) + 1);
    const QRectF viewRect(m_cameraX, m_cameraY, width(), height());
    const auto drawPixmapCover = [&painter](const QRectF &targetRect, const QPixmap &pixmap) {
        if (pixmap.isNull() || targetRect.width() <= 0.0 || targetRect.height() <= 0.0) {
            return;
        }

        const qreal targetAspect = targetRect.width() / targetRect.height();
        const qreal sourceAspect = qreal(pixmap.width()) / qreal(pixmap.height());
        QRectF sourceRect(0.0, 0.0, pixmap.width(), pixmap.height());
        if (sourceAspect > targetAspect) {
            const qreal sourceWidth = pixmap.height() * targetAspect;
            sourceRect.setLeft((pixmap.width() - sourceWidth) * 0.5);
            sourceRect.setWidth(sourceWidth);
        } else {
            const qreal sourceHeight = pixmap.width() / targetAspect;
            sourceRect.setTop((pixmap.height() - sourceHeight) * 0.5);
            sourceRect.setHeight(sourceHeight);
        }

        painter.drawPixmap(targetRect, pixmap, sourceRect);
    };

    const qreal netherBandTop = 68.0 * kTileSize;
    const qreal netherBandBottom = (kSurfaceRow + 92) * kTileSize;
    const QRectF netherBandRect(0.0, netherBandTop, kWorldColumns * kTileSize, netherBandBottom - netherBandTop);
    if (viewRect.intersects(netherBandRect)) {
        const qreal halfWorldWidth = (kWorldColumns * kTileSize) * 0.5;
        const QRectF upperRect(0.0, netherBandTop, halfWorldWidth, netherBandRect.height());
        const QRectF lowerRect(halfWorldWidth, netherBandTop, kWorldColumns * kTileSize - halfWorldWidth, netherBandRect.height());
        if (!m_netherBgLower.isNull()) {
            drawPixmapCover(upperRect, m_netherBgLower);
        }
        if (!m_netherBgUpper.isNull()) {
            drawPixmapCover(lowerRect, m_netherBgUpper);
        }
    }


    for (int row = minRow; row <= maxRow; ++row) {
        for (int column = minColumn; column <= maxColumn; ++column) {
            const auto treeIt = m_treeBlocks.constFind(tileKey(column, row));
            if (treeIt == m_treeBlocks.constEnd()) {
                continue;
            }

            treeIt->paint(painter, tileRect(column, row));

             drawBlockEdges(painter, tileRect(column, row), column, row);

            if (m_miningState.active && m_miningState.tile == QPoint(column, row) && m_miningState.requiredSeconds > 0.0) {
                drawMiningCracks(painter, tileRect(column, row), m_miningState.elapsedSeconds / m_miningState.requiredSeconds);
            }
        }
    }

    for (int row = minRow; row <= maxRow; ++row) {
        for (int column = minColumn; column <= maxColumn; ++column) {
            const QRect currentTile = tileRect(column, row);

            if (row >= kSurfaceRow && (row < 68 || row > kSurfaceRow + 92)) {
                painter.fillRect(currentTile, QColor(43, 30, 20));
            }

            if (row >= 11 && row <= 68) {
                const int key = tileKey(column, row);
                const bool emptyTile = !m_dirtBlocks.contains(key) && !m_oreBlocks.contains(key) && !m_treeBlocks.contains(key)
                                       && !m_waterTiles.contains(key) && !m_lavaTiles.contains(key) && !m_torchTiles.contains(key)
                                       && !m_purpleTiles.contains(key) && !m_purpleBeamTiles.contains(key)
                                       && !m_netherPortalTiles.contains(key) && !m_netherPortalBeamTiles.contains(key)
                                       && !m_blackPortalTiles.contains(key) && !m_blackPortalBeamTiles.contains(key);
                if (emptyTile) {
                    const QPixmap *gapPixmap = nullptr;
                    if (row <= 12) {
                        gapPixmap = &m_upperGapDirtBg;
                    } else if (row <= 50) {
                        gapPixmap = &m_upperGapStoneBg;
                    } else {
                        gapPixmap = &m_upperGapDeepStoneBg;
                    }
                    if (gapPixmap != nullptr && !gapPixmap->isNull()) {
                        painter.drawPixmap(currentTile, *gapPixmap);
                    }
                }
            }

            if (row >= kWorldRows - kBedrockThickness) {
                painter.fillRect(currentTile, QColor(58, 58, 62));
                painter.fillRect(currentTile.adjusted(2, 2, -2, -2), QColor(92, 92, 98));
                painter.fillRect(QRect(currentTile.left() + 4, currentTile.top() + 5, 8, 6), QColor(122, 122, 128));
                painter.fillRect(QRect(currentTile.right() - 11, currentTile.top() + 15, 6, 5), QColor(34, 34, 38));
                continue;
            }

            bool portalTile = false;
            for (const Portal &portal : std::as_const(m_portals)) {
                if (portal.tiles.contains(column, row)) {
                    drawPortal(painter, currentTile);
                    portalTile = true;
                    break;
                }
            }
            if (portalTile) {
                continue;
            }

            if (m_waterTiles.contains(tileKey(column, row))) {
                painter.fillRect(currentTile, QColor(46, 122, 214, 210));
            }
            if (m_lavaTiles.contains(tileKey(column, row))) {
                painter.fillRect(currentTile, QColor(214, 82, 24, 220));
                painter.fillRect(QRect(currentTile.left() + 6, currentTile.top() + 8, 4, 4), QColor(244, 206, 92, 210));
                painter.fillRect(QRect(currentTile.left() + 18, currentTile.top() + 17, 5, 5), QColor(255, 220, 116, 190));
                painter.fillRect(QRect(currentTile.left() + 12, currentTile.top() + 23, 3, 3), QColor(255, 236, 164, 180));

                const int tileKeyValue = tileKey(column, row);
                if ((tileKeyValue + int(m_totalRunTime * 10.0)) % 3 == 0) {
                    const int flicker = (tileKeyValue + int(m_totalRunTime * 18.0)) % 9;
                    const int flameHeight = 8 + (flicker % 4);
                    painter.fillRect(QRect(currentTile.left() + 9, currentTile.top() - flameHeight + 2, 5, flameHeight),
                                     QColor(255, 189, 72, 165));
                    painter.fillRect(QRect(currentTile.left() + 11, currentTile.top() - flameHeight - 4, 3, flameHeight + 2),
                                     QColor(255, 234, 132, 150));
                }
            }

            const auto oreIt = m_oreBlocks.constFind(tileKey(column, row));
            if (oreIt != m_oreBlocks.constEnd()) {
                oreIt->paint(painter, currentTile);

                 drawBlockEdges(painter, currentTile, column, row);

                if (m_miningState.active && m_miningState.tile == QPoint(column, row) && m_miningState.requiredSeconds > 0.0) {
                    drawMiningCracks(painter, currentTile, m_miningState.elapsedSeconds / m_miningState.requiredSeconds,
                                     QColor(255, 255, 255, 160));
                }
                continue;
            }

            const auto blockIt = m_dirtBlocks.constFind(tileKey(column, row));
            if (blockIt != m_dirtBlocks.constEnd()) {
                blockIt->paint(painter, currentTile);


                if (m_purpleTiles.contains(tileKey(column, row))) {
                    const bool activePurple = m_defeatedEnemyCount >= kDefeatTargetCount;
                    painter.fillRect(currentTile, activePurple ? QColor(148, 78, 224, 190) : QColor(130, 68, 196, 150));
                    painter.fillRect(currentTile.adjusted(3, 3, -3, -18),
                                     activePurple ? QColor(242, 215, 255, 130) : QColor(210, 180, 255, 85));
                }
                if (m_miningState.active && m_miningState.tile == QPoint(column, row) && m_miningState.requiredSeconds > 0.0) {
                    drawMiningCracks(painter, currentTile, m_miningState.elapsedSeconds / m_miningState.requiredSeconds);
                }
                continue;
            }
            if (m_torchTiles.contains(tileKey(column, row))) {
                drawTorch(painter, currentTile);
            }
        }   
    }

    for (int row = minRow; row <= maxRow; ++row) {
        for (int column = minColumn; column <= maxColumn; ++column) {
            if (!m_purpleBeamTiles.contains(tileKey(column, row))) {
                continue;
            }

            const QRect currentTile = tileRect(column, row);
            painter.fillRect(currentTile, QColor(134, 78, 224, 54));
            painter.fillRect(currentTile.adjusted(8, 0, -8, 0), QColor(218, 188, 255, 70));
        }
    }

    for (int row = minRow; row <= maxRow; ++row) {
        for (int column = minColumn; column <= maxColumn; ++column) {
            if (!m_netherPortalBeamTiles.contains(tileKey(column, row)))
                continue;
            const QRect currentTile = tileRect(column, row);
            painter.fillRect(currentTile, QColor(134, 78, 224, 54));
            painter.fillRect(currentTile.adjusted(8, 0, -8, 0), QColor(218, 188, 255, 70));
        }
    }


    for (int row = minRow; row <= maxRow; ++row) {
        for (int column = minColumn; column <= maxColumn; ++column) {
            if (!m_netherPortalTiles.contains(tileKey(column, row)))
                continue;
            const QRect currentTile = tileRect(column, row);
            painter.fillRect(currentTile, QColor(148, 78, 224, 190));
            painter.fillRect(currentTile.adjusted(3, 3, -3, -18), QColor(242, 215, 255, 130));
        }
    }


    for (int row = minRow; row <= maxRow; ++row) {
        for (int column = minColumn; column <= maxColumn; ++column) {
            if (!m_blackPortalBeamTiles.contains(tileKey(column, row)))
                continue;

            const QRect currentTile = tileRect(column, row);


            const int topRow = 68;
          const int bottomRow = 90;
            double t = (double)(row - topRow) / (double)(bottomRow - topRow);
            t = qBound(0.0, t, 1.0);


            int alpha = qRound(140.0 * (1.0 - t) + 30.0 * t);
            QColor beamColor(134, 78, 224, alpha);
            QColor highlightColor(218, 188, 255, qMin(255, alpha + 20));

            painter.fillRect(currentTile, QColor(10, 5, 20));
            painter.fillRect(currentTile, beamColor);
            painter.fillRect(currentTile.adjusted(8, 0, -8, 0), highlightColor);
        }
    }





}

void GameWidget::generateOreVeins()
{
    auto placeOre = [this](const QPoint &tile, OreBlock::Kind kind) {
        if (tile.x() < 0 || tile.x() >= kWorldColumns || tile.y() < kSurfaceRow + 4 || tile.y() >= kWorldRows)
            return false;
        const int netherTopRow = kSurfaceRow + kNetherTopOffsetRows;
        const int netherBottomRow = kSurfaceRow + kNetherBottomOffsetRows;
        if (tile.y() >= netherTopRow && tile.y() <= netherBottomRow)
            return false;
        int key = tileKey(tile);
        if (!m_dirtBlocks.contains(key)) return false;
        m_dirtBlocks.remove(key);
        m_oreBlocks.insert(key, OreBlock(kind));
        return true;
    };

    auto growLongVein = [&](OreBlock::Kind kind, int veinCount, int minRow, int maxRow) {
        for (int i = 0; i < veinCount; ++i) {
            QPoint cursor(randomBetween(4, kWorldColumns - 4),
                          randomBetween(minRow, maxRow));
            int steps = randomBetween(14, 26);
            for (int step = 0; step < steps; ++step) {
                int radius = randomChance(70) ? 1 : 2;
                for (int row = cursor.y() - radius; row <= cursor.y() + radius; ++row) {
                    for (int column = cursor.x() - radius; column <= cursor.x() + radius; ++column) {
                        int dx = column - cursor.x(), dy = row - cursor.y();
                        if (dx * dx + dy * dy > radius * radius + 1) continue;
                        if (randomChance(88))
                            placeOre(QPoint(column, row), kind);
                    }
                }
                cursor.rx() += randomBetween(-1, 2);
                cursor.ry() += randomBetween(-1, 2);
                cursor.setX(qBound(2, cursor.x(), kWorldColumns - 3));
                cursor.setY(qBound(minRow, cursor.y(), maxRow - 1));
            }
        }
    };

    auto growClusteredOre = [&](OreBlock::Kind kind, int clusterCount, int minRow, int maxRow) {
        for (int i = 0; i < clusterCount; ++i) {
            QPoint center(randomBetween(4, kWorldColumns - 4),
                          randomBetween(minRow, maxRow));
            QVector<QPoint> frontier { center };
            QSet<int> reservedTiles;
            for (int placed = 0; placed < 6 && !frontier.isEmpty(); ++placed) {
                int idx = randomBetween(0, frontier.size());
                QPoint tile = frontier.takeAt(idx);
                int key = tileKey(tile);
                if (reservedTiles.contains(key)) { --placed; continue; }
                reservedTiles.insert(key);
                if (!placeOre(tile, kind)) { --placed; continue; }
                QVector<QPoint> neighbors = {
                    QPoint(tile.x()-1,tile.y()), QPoint(tile.x()+1,tile.y()),
                    QPoint(tile.x(),tile.y()-1), QPoint(tile.x(),tile.y()+1)
                };
                for (auto &n : neighbors) {
                    if (n.x()<0||n.x()>=kWorldColumns||n.y()<minRow||n.y()>=maxRow) continue;
                    if (!reservedTiles.contains(tileKey(n)) && randomChance(80))
                        frontier.append(n);
                }
            }
        }
    };

    growLongVein(OreBlock::Kind::Green, 5, kSurfaceRow + 5, kSurfaceRow + 18);
    growLongVein(OreBlock::Kind::Gold, 6, kSurfaceRow + 10, kWorldRows - 3);
    growClusteredOre(OreBlock::Kind::Blue, 4, kSurfaceRow + 9, kWorldRows - 5);
    growClusteredOre(OreBlock::Kind::Red, 4, kSurfaceRow + 14, kWorldRows - 4);
}
void GameWidget::generateTrees()
{
    const int firstTreeColumn = 16;
    const int spacing = 18;

    for (int baseColumn = firstTreeColumn; baseColumn < kWorldColumns - 8; baseColumn += spacing) {
        if (!m_dirtBlocks.contains(tileKey(baseColumn, kSurfaceRow))) {
            continue;
        }

        const int trunkHeight = 4 + ((baseColumn / spacing) % 2);
        for (int offset = 1; offset <= trunkHeight; ++offset) {
            const int row = kSurfaceRow - offset;
            if (row < 0) break;
            m_treeBlocks.insert(tileKey(baseColumn, row), TreeBlock(TreeBlock::Kind::Trunk));
        }

        const int canopyTopRow = kSurfaceRow - trunkHeight - 2;
        for (int row = canopyTopRow; row <= canopyTopRow + 3; ++row) {
            for (int column = baseColumn - 2; column <= baseColumn + 2; ++column) {
                if (column < 0 || column >= kWorldColumns || row < 0) continue;

                const bool corner = (qAbs(column - baseColumn) == 2) && (row == canopyTopRow || row == canopyTopRow + 3);
                if (corner) continue;

                m_treeBlocks.insert(tileKey(column, row), TreeBlock(TreeBlock::Kind::Leaves));
            }
        }
    }
}
void GameWidget::generateWaterPools()
{
    const int poolCount = 9;
    auto solidForWaterBasin = [this](int column, int row) {
        if (column < 0 || column >= kWorldColumns || row < 0 || row >= kWorldRows) {
            return false;
        }
        const int key = tileKey(column, row);
        return m_dirtBlocks.contains(key) || m_oreBlocks.contains(key);
    };

    for (int i = 0; i < poolCount; ++i) {
        const int centerColumn = randomBetween(6, kWorldColumns - 6);
        const int centerRow = randomBetween(kSurfaceRow + 10, kWorldRows - 4);
        const int radiusX = randomBetween(2, 4);
        const int radiusY = randomBetween(1, 2);
        const int netherTopRow = kSurfaceRow + kNetherTopOffsetRows;
        const int netherBottomRow = kSurfaceRow + kNetherBottomOffsetRows;

        if (centerRow + radiusY >= netherTopRow && centerRow - radiusY <= netherBottomRow) {
            continue;
        }

        bool enclosedBasin = true;
        for (int column = centerColumn - radiusX; column <= centerColumn + radiusX && enclosedBasin; ++column) {
            enclosedBasin = solidForWaterBasin(column, centerRow + radiusY + 1);
        }
        for (int row = centerRow - radiusY; row <= centerRow + radiusY && enclosedBasin; ++row) {
            enclosedBasin = solidForWaterBasin(centerColumn - radiusX - 1, row)
                             && solidForWaterBasin(centerColumn + radiusX + 1, row);
        }

        if (!enclosedBasin) {
            continue;
        }

        for (int row = centerRow - radiusY; row <= centerRow + radiusY; ++row) {
            for (int column = centerColumn - radiusX; column <= centerColumn + radiusX; ++column) {
                if (column < 0 || column >= kWorldColumns || row < kSurfaceRow + 4 || row >= kWorldRows) {
                    continue;
                }

                const double normalizedX = double(column - centerColumn) / qMax(1, radiusX);
                const double normalizedY = double(row - centerRow) / qMax(1, radiusY);
                const double shape = normalizedX * normalizedX + normalizedY * normalizedY;
                if (shape > 1.2) {
                    continue;
                }

                const int key = tileKey(column, row);
                if (m_treeBlocks.contains(key) || m_oreBlocks.contains(key) || m_lavaTiles.contains(key)) {
                    continue;
                }

                if (shape > 0.95 && randomChance(35)) {
                    continue;
                }

                m_dirtBlocks.remove(key);
                m_waterTiles.insert(key);
            }
        }
    }
}

void GameWidget::generateCavernLights()
{
    for (const QPoint &center : std::as_const(m_largeCavernCenters)) {
        const int leftTorchColumn = qMax(1, center.x() - 3);
        const int rightTorchColumn = qMin(kWorldColumns - 2, center.x() + 3);
        const int torchRow = qMax(kSurfaceRow + 3, center.y() - 1);

        const int leftKey = tileKey(leftTorchColumn, torchRow);
        if (!m_dirtBlocks.contains(leftKey) && !m_oreBlocks.contains(leftKey)
            && !m_waterTiles.contains(leftKey) && !m_lavaTiles.contains(leftKey)) {
            m_torchTiles.insert(leftKey);
        }

        const int rightKey = tileKey(rightTorchColumn, torchRow);
        if (!m_dirtBlocks.contains(rightKey) && !m_oreBlocks.contains(rightKey)
            && !m_waterTiles.contains(rightKey) && !m_lavaTiles.contains(rightKey)) {
            m_torchTiles.insert(rightKey);
        }
    }
}

void GameWidget::generateNetherTransitionWalls()
{
    const QPoint rampStart(10, kSurfaceRow + 10);
    const QPoint rampEnd(90, kSurfaceRow + 50);
    const int tunnelRadius = 8;



    const int transitionMinX = 40;
    const int transitionMaxX = 110;
    const int transitionRange = transitionMaxX - transitionMinX;


    const int steps = qMax(qAbs(rampEnd.x() - rampStart.x()), qAbs(rampEnd.y() - rampStart.y())) * 3 + 1;
    for (int step = 0; step <= steps; ++step)
    {
        const double t = steps == 0 ? 0.0 : double(step) / double(steps);
        const double curveLift = qSin(t * kPi) * 1.5;
        const int centerColumn = qRound(rampStart.x() + (rampEnd.x() - rampStart.x()) * t);
        const int centerRow    = qRound(rampStart.y() + (rampEnd.y() - rampStart.y()) * t - curveLift);


        const int surfRow = surfaceRowAt(centerColumn);


        const double netherProb = [&]() -> double {
            if (centerColumn <= transitionMinX) return 0.0;
            if (centerColumn >= transitionMaxX) return 1.0;
            double p = double(centerColumn - transitionMinX) / double(transitionRange);
            return (1.0 - qCos(p * kPi)) * 0.5;
        }();


        const int upperWallStart = centerRow - tunnelRadius - 1;
        const int upperWallEnd   = upperWallStart - 2;
        const int lowerWallStart = centerRow + tunnelRadius + 1;
        const int lowerWallEnd   = lowerWallStart + 2;


        for (int row = upperWallStart; row >= upperWallEnd && row >= surfRow + 2; --row) {
            for (int col = centerColumn - tunnelRadius - 2; col <= centerColumn + tunnelRadius + 2; ++col) {
                if (col < 0 || col >= kWorldColumns || row < 0 || row >= kWorldRows)
                    continue;
                int key = tileKey(col, row);
                auto it = m_dirtBlocks.find(key);
                if (it != m_dirtBlocks.end()) {
                    DirtBlock::Kind oldKind = it->kind();
                    if (oldKind != DirtBlock::Kind::Stone &&
                        oldKind != DirtBlock::Kind::Dirt &&
                        oldKind != DirtBlock::Kind::Grassdirt) {
                        continue;
                    }
                }

                m_dirtBlocks.remove(key);
                m_oreBlocks.remove(key);
                m_treeBlocks.remove(key);
                m_waterTiles.remove(key);

                DirtBlock::Kind kind = DirtBlock::Kind::Stone;
                if (m_worldRng.generateDouble() < netherProb) {
                    double r = m_worldRng.generateDouble(); // 0~1
                    if (r < 0.70)       kind = DirtBlock::Kind::Netherrack;
                    else if (r < 0.90)  kind = DirtBlock::Kind::Magma;
                    else                kind = DirtBlock::Kind::Quartz;
                }
                m_dirtBlocks.insert(key, DirtBlock(kind));
                if (kind == DirtBlock::Kind::Magma)
                    m_magmaTiles.insert(key);
            }
        }

        for (int row = lowerWallStart; row <= lowerWallEnd && row < kWorldRows - kBedrockThickness; ++row) {
            for (int col = centerColumn - tunnelRadius - 2; col <= centerColumn + tunnelRadius + 2; ++col) {
                if (col < 0 || col >= kWorldColumns || row < 0 || row >= kWorldRows)
                    continue;
                int key = tileKey(col, row);
                auto it = m_dirtBlocks.find(key);
                if (it != m_dirtBlocks.end()) {
                    DirtBlock::Kind oldKind = it->kind();
                    if (oldKind != DirtBlock::Kind::Stone &&
                        oldKind != DirtBlock::Kind::Dirt &&
                        oldKind != DirtBlock::Kind::Grassdirt) {
                        continue;
                    }
                }

                m_dirtBlocks.remove(key);
                m_oreBlocks.remove(key);
                m_treeBlocks.remove(key);
                m_waterTiles.remove(key);
                DirtBlock::Kind kind = DirtBlock::Kind::Stone;
                if (m_worldRng.generateDouble() < netherProb) {
                    double r = m_worldRng.generateDouble();
                    if (r < 0.70)       kind = DirtBlock::Kind::Netherrack;
                    else if (r < 0.90)  kind = DirtBlock::Kind::Magma;
                    else                kind = DirtBlock::Kind::Quartz;
                }
                m_dirtBlocks.insert(key, DirtBlock(kind));
                if (kind == DirtBlock::Kind::Magma)
                    m_magmaTiles.insert(key);
            }
        }
    }


}

void GameWidget::tick()
{
    const double deltaSeconds = qMin(0.033, m_frameClock.restart() / 1000.0);
    if (m_endState != GameEndState::None) {
        update();
        return;
    }
    if (!m_gameWon) {
        m_totalRunTime += deltaSeconds;
    }
    m_shootCooldown = qMax(0.0, m_shootCooldown - deltaSeconds);
    m_player.hurtTimer = qMax(0.0, m_player.hurtTimer - deltaSeconds);
    m_player.knockbackTimer = qMax(0.0, m_player.knockbackTimer - deltaSeconds);
    m_player.crouchTimer = qMax(0.0, m_player.crouchTimer - deltaSeconds);
    const double previousEatTimer = m_player.eatTimer;
    m_player.eatTimer = qMax(0.0, m_player.eatTimer - deltaSeconds);
    if (m_pendingGoldenAppleUse && previousEatTimer > 0.0 && m_player.eatTimer <= 0.0) {
        m_pendingGoldenAppleUse = false;
        m_goldenHealth = m_goldenHealthMax;
        m_goldenEffectTimeRemaining = kGoldenAppleEffectSeconds;
        m_goldenRegenTickAccumulator = 0.0;
        m_goldenRegenActive = true;
    }
    m_miningFinishTimer = qMax(0.0, m_miningFinishTimer - deltaSeconds);
    m_fearDebuffTimer = qMax(0.0, m_fearDebuffTimer - deltaSeconds);
    m_fearFogBoost = qMax(0.0, m_fearFogBoost - deltaSeconds * 0.035);
    m_phaseFlashTimer = qMax(0.0, m_phaseFlashTimer - deltaSeconds);
    m_playerOutlineFlashTimer = qMax(0.0, m_playerOutlineFlashTimer - deltaSeconds);
    m_fogClearTimer = qMax(0.0, m_fogClearTimer - deltaSeconds);
    if (m_deepLayerSpawnTimer > 0.0 && m_currentDepthLayer == DepthLayer::DeepCavern && !m_deepLayerBossSpawned) {
        m_deepLayerSpawnTimer = qMax(0.0, m_deepLayerSpawnTimer - deltaSeconds);
    }
    if (m_victoryBannerTimer > 0.0) {
        m_victoryBannerTimer = qMax(0.0, m_victoryBannerTimer - deltaSeconds);
    }
    if (m_victorySummaryTimer > 0.0) {
        m_victorySummaryTimer = qMax(0.0, m_victorySummaryTimer - deltaSeconds);
    }
    if (m_gameWon && m_deepLayerBossDefeated && m_fogClearTimer <= 0.0 && m_endState == GameEndState::None) {
        m_endState = GameEndState::Succeeded;
        stopMonsterSounds();
        stopMining();
        m_pressedKeys.clear();
    }
    if (m_fearDebuffTimer <= 0.0) {
        m_fearDebuffStacks = 0;
    }
    if (isPlayerInLava()) {
        m_player.burnTimeRemaining = qMax(m_player.burnTimeRemaining, kBlazeBurnDurationSeconds);
        m_player.burnTickTimer = qMin(m_player.burnTickTimer, kBlazeBurnTickSeconds);
        m_player.slowTimer = 0.8;
        m_player.slowMultiplier = 0.5;
    }
    if (m_player.burnTimeRemaining > 0.0) {
        m_player.burnTickTimer += deltaSeconds;
        while (m_player.burnTickTimer >= kBlazeBurnTickSeconds) {
            m_player.burnTickTimer -= kBlazeBurnTickSeconds;
            applyDamage(kHeartHitPoints, 0.0);
        }
        m_player.burnTimeRemaining = qMax(0.0, m_player.burnTimeRemaining - deltaSeconds);
    } else {
        m_player.burnTickTimer = 0.0;
    }


    m_player.slowTimer = qMax(0.0, m_player.slowTimer - deltaSeconds);
    if (m_player.slowTimer <= 0.0) {
        m_player.slowMultiplier = 1.0;
    }

    const bool playerWalking = m_player.onGround && qAbs(m_player.velocity.x()) > 1.0;
    m_stepSoundTimer = qMax(0.0, m_stepSoundTimer - deltaSeconds);
    m_zombieAmbientTimer = qMax(0.0, m_zombieAmbientTimer - deltaSeconds);
    m_piglinAmbientTimer = qMax(0.0, m_piglinAmbientTimer - deltaSeconds);
    if (playerWalking) {
        m_player.walkPhase += deltaSeconds * qAbs(m_player.velocity.x()) * 0.06;
        if (m_stepSoundTimer <= 0.0) {
            playSound(QStringLiteral("sounds/step.mp3"));
            m_stepSoundTimer = 0.34;
        }
    }
    if (m_player.facingLeft == m_lastPlayerFacingLeft) {
        m_playerFacingSameDirectionTimer += deltaSeconds;
    } else {
        m_playerFacingSameDirectionTimer = 0.0;
        m_lastPlayerFacingLeft = m_player.facingLeft;
    }

    m_swordAttackCooldown = qMax(0.0, m_swordAttackCooldown - deltaSeconds);
    if (m_swordSwingTimer > 0.0) {
        const double previousSwingTimer = m_swordSwingTimer;
        m_swordSwingTimer = qMax(0.0, m_swordSwingTimer - deltaSeconds);
        const double elapsedAfterUpdate = kSwordSwingDurationSeconds - m_swordSwingTimer;
        if (!m_swordAttackHitApplied && elapsedAfterUpdate >= kSwordPreSwingSeconds) {
            applySwordAttackDamage();
            m_swordAttackHitApplied = true;
        }
        if (previousSwingTimer > 0.0 && m_swordSwingTimer <= 0.0) {
            m_swordAttackType = SwordAttackType::None;
        }
    }




    if (!m_netherPortalLit && m_blazeDefeatedCount >= 5 && m_zombiePigmanDefeatedCount >= 15) {
        m_netherPortalLit = true;
        for (int dx = 0; dx < 5; ++dx) {
            int col = m_netherPortalOrigin.x() + dx;
            int row = m_netherPortalOrigin.y();
            int key = tileKey(col, row);
            m_netherPortalTiles.insert(key);

            m_dirtBlocks.insert(key, DirtBlock(DirtBlock::Kind::Stone));
        }
        m_currentLayerHint = QStringLiteral("The Nether Portal is open");
        m_layerHintTimer = 2.5;
    }


    updatePortalTravel(deltaSeconds);
    updateGoldenAppleEffect(deltaSeconds);
    updateMining(deltaSeconds);
    updatePurpleBeam(deltaSeconds);
    updateLootFx(deltaSeconds);

    if (m_player.crouching && m_player.crouchTimer <= 0.0 && !m_inPurpleBeam) {
        const qreal previousBottom = m_player.rect.bottom();
        QRectF standingRect = m_player.rect;
        standingRect.setHeight(kStandingPlayerHeight);
        standingRect.moveBottom(previousBottom);
        bool blocked = false;
        const int minColumn = qFloor(standingRect.left() / kTileSize) - 1;
        const int maxColumn = qFloor(standingRect.right() / kTileSize) + 1;
        const int minRow = qFloor(standingRect.top() / kTileSize) - 1;
        const int maxRow = qFloor(standingRect.bottom() / kTileSize) + 1;
        for (int row = minRow; row <= maxRow && !blocked; ++row) {
            for (int column = minColumn; column <= maxColumn; ++column) {
                if (!isSolidTile(column, row)) {
                    continue;
                }
                if (standingRect.intersects(QRectF(tileRect(column, row)))) {
                    blocked = true;
                    break;
                }
            }
        }
        if (!blocked) {
            m_player.rect = standingRect;
            m_player.crouching = false;
        }
    }

    if (!m_controlsLocked) {
        updateHorizontalMovement(deltaSeconds);
        if (!m_inPurpleBeam) {
            updateVerticalMovement(deltaSeconds);
        }
    }
    if (!m_gameWon) {
        updateEnemies(deltaSeconds);
    }
    updateProjectiles(deltaSeconds);
    clampPlayerToWorld();
    if (!m_controlsLocked) {
        checkPortalEntry();
    }
    updateCamera();
    updateDepthLayer(deltaSeconds);
    update();
}

void GameWidget::updateHorizontalMovement(double deltaSeconds)
{
    const QRectF previousRect = m_player.rect;
    const bool inFluid = isPlayerInFluid();

    double direction = 0.0;
    if (m_pressedKeys.contains(Qt::Key_A)) {
        direction -= 1.0;
        m_player.facingLeft = true;
    }
    if (m_pressedKeys.contains(Qt::Key_D)) {
        direction += 1.0;
        m_player.facingLeft = false;
    }

    const double moveSpeed = kMoveSpeed * movementSpeedMultiplier();
    if (m_player.knockbackTimer > 0.0) {
        m_player.velocity.setX(m_player.knockbackDirection * kHitKnockbackSpeed * kPlayerHitKnockbackScale);
    } else {
        m_player.velocity.setX(direction * (inFluid ? moveSpeed * kWaterMoveSpeedFactor : moveSpeed) * m_player.slowMultiplier);
    }
    m_player.rect.translate(m_player.velocity.x() * deltaSeconds, 0.0);
    resolveHorizontalCollisions(previousRect);
}

void GameWidget::updateVerticalMovement(double deltaSeconds)
{
    const bool wasOnGround = m_player.onGround;
    const QRectF previousRect = m_player.rect;
    const bool inFluid = isPlayerInFluid();
    if (m_inPurpleBeam) {
        m_player.velocity.setY(m_pressedKeys.contains(Qt::Key_Shift) ? kPurpleBeamFastFallSpeed : kPurpleBeamFallSpeed);
        m_player.rect.translate(0.0, m_player.velocity.y() * deltaSeconds);
        m_player.onGround = false;
        m_airbornePeakY = m_player.rect.top();
        return;
    }
    const double gravity = inFluid ? kGravity * kWaterGravityFactor : kGravity;
    const double maxFallSpeed = inFluid ? kWaterMaxFallSpeed : kMaxFallSpeed;

    m_player.velocity.setY(qMin(m_player.velocity.y() + gravity * deltaSeconds, maxFallSpeed));
    if (inFluid && !m_pressedKeys.contains(Qt::Key_W)) {
        m_player.velocity.setY(qMin(m_player.velocity.y(), 120.0));
    }
    m_player.rect.translate(0.0, m_player.velocity.y() * deltaSeconds);
    m_player.onGround = false;
    resolveVerticalCollisions(previousRect);

    if (isPlayerInFluid()) {
        m_airbornePeakY = m_player.rect.top();
        return;
    }

    if (m_player.onGround) {
        if (!wasOnGround) {
            const double fallDistance = qMax(0.0, m_player.rect.top() - m_airbornePeakY);
            const int fallenTiles = qFloor(fallDistance / kTileSize);
            if (fallenTiles >= 5) {
                const int damage = kHeartHitPoints * 2 + ((fallenTiles - 5) / 2) * (kHeartHitPoints / 2);
                playSound(QStringLiteral("sounds/jumphurt.mp3"));
                applyDamage(damage, 0.0, false, false);
            }
        }

        m_airbornePeakY = m_player.rect.top();
        return;
    }

    if (wasOnGround) {
        m_airbornePeakY = previousRect.top();
    }
    m_airbornePeakY = qMin(m_airbornePeakY, m_player.rect.top());
}

void GameWidget::resolveHorizontalCollisions(const QRectF &previousRect)
{
    const int minColumn = qFloor(m_player.rect.left() / kTileSize) - 1;
    const int maxColumn = qFloor(m_player.rect.right() / kTileSize) + 1;
    const int minRow = qFloor(m_player.rect.top() / kTileSize) - 1;
    const int maxRow = qFloor(m_player.rect.bottom() / kTileSize) + 1;

    for (int row = minRow; row <= maxRow; ++row) {
        for (int column = minColumn; column <= maxColumn; ++column) {
            if (!isSolidTile(column, row)) {
                continue;
            }

            const QRectF tileBounds = tileRect(column, row);
            if (!m_player.rect.intersects(tileBounds)) {
                continue;
            }

            if (previousRect.right() <= tileBounds.left()) {
                m_player.rect.moveRight(tileBounds.left());
            } else if (previousRect.left() >= tileBounds.right()) {
                m_player.rect.moveLeft(tileBounds.right());
            }
        }
    }
}

void GameWidget::resolveVerticalCollisions(const QRectF &previousRect)
{
    const int minColumn = qFloor(m_player.rect.left() / kTileSize) - 1;
    const int maxColumn = qFloor(m_player.rect.right() / kTileSize) + 1;
    const int minRow = qFloor(m_player.rect.top() / kTileSize) - 1;
    const int maxRow = qFloor(m_player.rect.bottom() / kTileSize) + 1;

    for (int row = minRow; row <= maxRow; ++row) {
        for (int column = minColumn; column <= maxColumn; ++column) {
            if (!isSolidTile(column, row)) {
                continue;
            }

            const QRectF tileBounds = tileRect(column, row);
            if (!m_player.rect.intersects(tileBounds)) {
                continue;
            }

            if (previousRect.bottom() <= tileBounds.top()) {
                m_player.rect.moveBottom(tileBounds.top());
                m_player.velocity.setY(0.0);
                m_player.onGround = true;
            } else if (previousRect.top() >= tileBounds.bottom()) {
                m_player.rect.moveTop(tileBounds.bottom());
                m_player.velocity.setY(0.0);
            }
        }
    }
}

int GameWidget::tileKey(int column, int row) const
{
    return row * kWorldColumns + column;
}

int GameWidget::tileKey(const QPoint &tile) const
{
    return tileKey(tile.x(), tile.y());
}

QPointF GameWidget::screenToWorld(const QPoint &screenPos) const
{
    return QPointF(screenPos) + QPointF(m_cameraX, m_cameraY);
}

QPointF GameWidget::playerWeaponAnchor() const
{
    const qreal weaponX = m_player.facingLeft ? m_player.rect.left() + 5.0 : m_player.rect.right() - 5.0;
    return QPointF(weaponX, m_player.rect.top() + 24.0);
}

int GameWidget::experienceToNextLevel() const
{
    if (m_playerLevel <= 10) {
        return m_playerLevel * 10;
    }

    return 100;
}

void GameWidget::addExperience(int amount)
{
    m_levelExperience += qMax(0, amount);

    while (m_levelExperience >= experienceToNextLevel()) {
        m_levelExperience -= experienceToNextLevel();
        ++m_playerLevel;
    }
}

double GameWidget::movementSpeedMultiplier() const
{
    const double fearMultiplier = qMax(0.4, 1.0 - 0.2 * m_fearDebuffStacks);
    if (m_playerLevel <= 10) {
        return (1.0 + 0.02 * (m_playerLevel - 1)) * fearMultiplier;
    }

    return 1.18 * fearMultiplier;
}

double GameWidget::miningSpeedMultiplier() const
{
    if (m_playerLevel <= 10) {
        return 1.0 + 0.05 * (m_playerLevel - 1);
    }

    return 1.45;
}

GameWidget::MiningTargetType GameWidget::miningTargetTypeAt(const QPoint &tile) const
{
    if (tile.y() >= kWorldRows - kBedrockThickness) {
        return MiningTargetType::None;
    }

    const int key = tileKey(tile);
    if (m_oreBlocks.contains(key)) {
        return MiningTargetType::Ore;
    }

    const auto dirtIt = m_dirtBlocks.constFind(key);
    if (dirtIt != m_dirtBlocks.constEnd()) {
        if (dirtIt->kind() == DirtBlock::Kind::Dirt || dirtIt->kind() == DirtBlock::Kind::Grassdirt) {
            return MiningTargetType::Dirt;
        } else {
            return MiningTargetType::Stone;
        }
    }


    if (m_treeBlocks.contains(key)) {
        return MiningTargetType::Tree;
    }

    return MiningTargetType::None;
}
double GameWidget::miningDurationForTile(const QPoint &tile, MiningTargetType type) const
{
    switch (type) {
    case MiningTargetType::Dirt:
        return 0.0;
    case MiningTargetType::Tree:
        return 1.5 / miningSpeedMultiplier();
    case MiningTargetType::Stone:
        return 2.0 / miningSpeedMultiplier();
    case MiningTargetType::Ore: {
        const auto oreIt = m_oreBlocks.constFind(tileKey(tile));
        if (oreIt == m_oreBlocks.constEnd()) {
            return 0.0;
        }

        switch (oreIt->kind()) {
        case OreBlock::Kind::Gold:
        case OreBlock::Kind::Green:
            return 2.5 / miningSpeedMultiplier();
        case OreBlock::Kind::Red:
        case OreBlock::Kind::Blue:
            return 3.0 / miningSpeedMultiplier();
        }
    }
    case MiningTargetType::None:
        break;
    }

    return 0.0;
}

void GameWidget::startMining(const QPoint &tile)
{
    if (!isTileReachable(tile)) {
        stopMining();
        return;
    }

    const MiningTargetType type = miningTargetTypeAt(tile);
    if (type == MiningTargetType::None) {
        stopMining();
        return;
    }

    if (m_selectedHotbarItem != HotbarItem::Pickaxe) {
        stopMining();
        return;
    }

    m_miningState.type = type;
    m_miningState.tile = tile;
    m_miningState.elapsedSeconds = 0.0;
    m_miningState.requiredSeconds = miningDurationForTile(tile, type);
    m_miningState.active = true;

    if (m_miningState.requiredSeconds <= 0.0) {
        completeMiningTarget();
        stopMining();
    }
}

void GameWidget::stopMining()
{
    m_miningState = MiningState();
}

bool GameWidget::completeMiningTarget()
{
    const int key = tileKey(m_miningState.tile);

    auto oreIt = m_oreBlocks.find(key);
    if (oreIt != m_oreBlocks.end()) {
        int experienceReward = 10;
        switch (oreIt->kind()) {
        case OreBlock::Kind::Gold:
        case OreBlock::Kind::Green:
            experienceReward = 10;
            break;
        case OreBlock::Kind::Red:
        case OreBlock::Kind::Blue:
            experienceReward = 20;
            break;
        }

        addExperience(experienceReward);
        m_oreBlocks.remove(key);
        m_lavaTiles.remove(key);
        return true;
    }

    auto dirtIt = m_dirtBlocks.find(key);
    if (dirtIt != m_dirtBlocks.end()) {
        m_dirtBlocks.remove(key);
        m_magmaTiles.remove(key);
        m_lavaTiles.remove(key);
        return true;
    }

    auto treeIt = m_treeBlocks.find(key);
    if (treeIt != m_treeBlocks.end()) {
        m_treeBlocks.remove(key);
        m_lavaTiles.remove(key);
        return true;
    }

    return false;
}

void GameWidget::updateMining(double deltaSeconds)
{
    if (!m_miningState.active) {
        return;
    }

    if (!m_leftMousePressed) {
        stopMining();
        return;
    }

    if (!isTileReachable(m_miningState.tile) || miningTargetTypeAt(m_miningState.tile) != m_miningState.type) {
        stopMining();
        return;
    }

    m_miningState.elapsedSeconds += deltaSeconds;
    if (m_miningState.elapsedSeconds >= m_miningState.requiredSeconds) {
        m_miningFinishTimer = kMiningFinishSwingSeconds;
        completeMiningTarget();
        stopMining();
    }
}

bool GameWidget::placeDirtBlock(const QPoint &screenPos)
{
    const QPoint tile = screenToTile(screenPos);
    if (!isTileReachable(tile)) {
        return false;
    }

    if (tile.x() < 0 || tile.x() >= kWorldColumns || tile.y() < 0 || tile.y() >= kWorldRows) {
        return false;
    }

    if (tile.y() >= kWorldRows - kBedrockThickness) {
        return false;
    }

    const int key = tileKey(tile);
    if (m_dirtBlocks.contains(key) || m_oreBlocks.contains(key) || m_treeBlocks.contains(key) || m_torchTiles.contains(key)) {
        return false;
    }

    const QRectF blockRect = tileRect(tile.x(), tile.y());
    if (blockRect.intersects(m_player.rect.adjusted(2.0, 2.0, -2.0, -2.0))) {
        return false;
    }

    m_waterTiles.remove(key);
    m_lavaTiles.remove(key);
    m_dirtBlocks.insert(key, DirtBlock(DirtBlock::Kind::Dirt));
    return true;
}

void GameWidget::useGoldenApple()
{
    if (m_player.eatTimer > 0.0 || m_pendingGoldenAppleUse) {
        return;
    }
    m_player.eatTimer = kGoldenAppleEatSeconds;
    m_pendingGoldenAppleUse = true;
}

void GameWidget::updateGoldenAppleEffect(double deltaSeconds)
{
    if (m_goldenEffectTimeRemaining <= 0.0) {
        return;
    }

    m_goldenEffectTimeRemaining = qMax(0.0, m_goldenEffectTimeRemaining - deltaSeconds);

    if (m_goldenRegenActive) {
        m_goldenRegenTickAccumulator += deltaSeconds;
        while (m_goldenRegenTickAccumulator >= kGoldenAppleRegenIntervalSeconds) {
            m_goldenRegenTickAccumulator -= kGoldenAppleRegenIntervalSeconds;
            m_playerHealth = qMin(m_playerMaxHealth, m_playerHealth + kGoldenAppleRegenAmount);
        }
    }

    if (m_goldenEffectTimeRemaining <= 0.0) {
        m_goldenHealth = 0;
        m_goldenRegenActive = false;
        m_goldenRegenTickAccumulator = 0.0;
    }
}

void GameWidget::applyDamage(int damage, qreal sourceDirection, bool isBlaze, bool playHurtSound)
{
    if (m_layerGraceTimer > 0.0 || m_inPurpleBeam) {
        return;
    }

    int remainingDamage = qMax(0, damage);
    if (remainingDamage == 0) {
        return;
    }

    if (m_goldenEffectTimeRemaining > 0.0 && m_goldenHealth > 0) {
        const int absorbedDamage = qMin(m_goldenHealth, remainingDamage);
        m_goldenHealth -= absorbedDamage;
        remainingDamage -= absorbedDamage;

        if (m_goldenHealth <= 0) {
            m_goldenHealth = 0;
            m_goldenRegenActive = false;
        }
    }

    if (remainingDamage > 0) {
        m_playerHealth = qMax(0, m_playerHealth - remainingDamage);
    }

    m_player.hurtTimer = kHurtEffectSeconds;
    m_player.knockbackTimer = kHurtEffectSeconds;
    m_player.knockbackDirection = sourceDirection > 0.0 ? 1.0 : (sourceDirection < 0.0 ? -1.0 : (m_player.facingLeft ? 1.0 : -1.0));
    m_player.velocity.setX(m_player.knockbackDirection * kHitKnockbackSpeed * kPlayerHitKnockbackScale * 0.5);
    m_player.velocity.setY(-kHitJumpSpeed * 0.5);
    m_player.onGround = false;

    if (playHurtSound) {
        playSound(QStringLiteral("sounds/hurt.mp3"));
    }
    if (isBlaze) {
        m_player.slowTimer = 0.8;
        m_player.slowMultiplier = 0.5;
    }

    if (m_playerHealth <= 0 && m_endState == GameEndState::None) {
        m_endState = GameEndState::Failed;
        m_controlsLocked = true;
        m_pressedKeys.clear();
        stopMining();
        stopMonsterSounds();
    }
}

void GameWidget::applyEnemyDamage(Enemy &enemy, int damage, qreal sourceDirection)
{
    if (damage <= 0) {
        return;
    }

    enemy.health -= damage;
    const bool surfaceMobSoundsEnabled = qFloor(m_player.rect.center().y() / kTileSize) < 50;
    if (surfaceMobSoundsEnabled && enemy.kind == EnemyKind::Zombie) {
        playRandomSurfaceMobSound(QStringList() << QStringLiteral("sounds/zombiehurt1.mp3") << QStringLiteral("sounds/zombiehurt2.mp3"));
    } else if (surfaceMobSoundsEnabled && enemy.kind == EnemyKind::Skeleton) {
        playSurfaceMobSound(QStringLiteral("sounds/kulouhurt.mp3"));
    } else if (enemy.kind == EnemyKind::ZombifiedPiglin) {
        playSound(QStringLiteral("sounds/hurt.mp3"), true);
    }
    enemy.hurtTimer = (enemy.kind == EnemyKind::Blaze || enemy.kind == EnemyKind::ZombifiedPiglin)
                          ? kBlazeHurtFlashSeconds
                          : kHurtEffectSeconds;
    enemy.knockbackTimer = kHurtEffectSeconds;
    enemy.knockbackDirection = sourceDirection > 0.0 ? 1.0 : (sourceDirection < 0.0 ? -1.0 : (enemy.facingLeft ? 1.0 : -1.0));
    enemy.velocity.setX(enemy.knockbackDirection * kHitKnockbackSpeed);
    enemy.velocity.setY(-kHitJumpSpeed);
    enemy.onGround = false;

    if (enemy.kind == EnemyKind::FogStalker && enemy.health > 0 && !enemy.isPhantomClone) {
        teleportFogStalkerToRandomSafePosition(enemy, 8.0, 12.0, true);
        m_phaseFlashTimer = kFogStalkerPhaseFlashSeconds;
        playSound(QStringLiteral("sounds/fogflash.mp3"));
    }
}

void GameWidget::spawnBlazeRodLoot(const QPointF &worldPos)
{
    LootFx loot;
    loot.screenPos = worldPos - QPointF(m_cameraX, m_cameraY);
    loot.velocity = QPointF(0.0, -55.0);
    loot.scale = 1.0;
    loot.alpha = 1.0;
    loot.active = true;
    m_lootFx.append(loot);
}

void GameWidget::updateLootFx(double deltaSeconds)
{
    const QPointF target = blazeCounterRect().center();
    for (int i = m_lootFx.size() - 1; i >= 0; --i) {
        LootFx &loot = m_lootFx[i];
        const QPointF toTarget = target - loot.screenPos;
        loot.velocity = loot.velocity * 0.84 + toTarget * 3.8 * deltaSeconds;
        loot.screenPos += loot.velocity * deltaSeconds;
        loot.scale = qMax<qreal>(0.28, loot.scale - deltaSeconds * 0.55);
        loot.alpha = qMax<qreal>(0.18, loot.alpha - deltaSeconds * 0.42);

        if (QLineF(loot.screenPos, target).length() <= 16.0) {
            m_lootFx.remove(i);
        }
    }
}

void GameWidget::fireProjectile(const QPoint &screenPos)
{
    if (m_selectedHotbarItem != HotbarItem::Bow) {
        return;
    }

    if (m_shootCooldown > 0.0 || m_projectiles.size() >= kMaxProjectiles) {
        return;
    }

    const QPointF targetWorld = screenToWorld(screenPos);
    const QPointF origin = playerWeaponAnchor();
    QPointF direction = targetWorld - origin;
    const qreal length = std::hypot(direction.x(), direction.y());
    if (length < 1.0) {
        direction = QPointF(m_player.facingLeft ? -1.0 : 1.0, 0.0);
    } else {
        direction /= length;
    }

    m_player.facingLeft = direction.x() < 0.0;

    Projectile projectile;
    projectile.position = origin + direction * 20.0;
    projectile.velocity = direction * kProjectileSpeed;
    projectile.damage = kPlayerArrowDamage;
    projectile.hostile = false;
    projectile.color = QColor(255, 78, 78);
    projectile.highlight = QColor(255, 190, 190);
    m_projectiles.append(projectile);
    playSound(QStringLiteral("sounds/bowhit1.mp3"));
    m_shootCooldown = kShootCooldownSeconds;
}

void GameWidget::performSwordAttack()
{
    if (m_selectedHotbarItem != HotbarItem::Sword) {
        return;
    }

    if (m_swordSwingTimer > 0.0 || m_swordAttackCooldown > 0.0) {
        return;
    }

    const QPointF playerCenter = m_player.rect.center();
    const QPointF cursorDelta = m_cursorWorldPos - playerCenter;
    m_swordAttackType = cursorDelta.y() > kTileSize * 0.45
                            ? SwordAttackType::Downslash
                            : SwordAttackType::Sweep;
    m_swordAttackFacingLeft = cursorDelta.x() < 0.0;
    m_player.facingLeft = m_swordAttackFacingLeft;
    m_swordSwingTimer = kSwordSwingDurationSeconds;
    m_swordAttackCooldown = kSwordAttackCooldownSeconds;
    m_swordAttackHitApplied = false;
}

void GameWidget::applySwordAttackDamage()
{
    if (m_swordAttackType == SwordAttackType::None) {
        return;
    }

    const qreal facingSign = m_swordAttackFacingLeft ? -1.0 : 1.0;
    QRectF attackRect;
    int attackDamage = kSwordDamage;

    if (m_swordAttackType == SwordAttackType::Sweep) {
        const qreal attackX = m_swordAttackFacingLeft
                                  ? m_player.rect.left() - 3.0 * kTileSize
                                  : m_player.rect.right();
        attackRect = QRectF(attackX,
                            m_player.rect.center().y() - 1.5 * kTileSize,
                            3.0 * kTileSize,
                            3.0 * kTileSize);
    } else {
        attackDamage = qRound(kSwordDamage * 1.5);
        const qreal centerX = m_player.rect.center().x() + facingSign * kTileSize;
        attackRect = QRectF(centerX - 1.5 * kTileSize,
                            m_player.rect.top(),
                            3.0 * kTileSize,
                            4.0 * kTileSize);
    }

    for (int projectileIndex = m_projectiles.size() - 1; projectileIndex >= 0; --projectileIndex) {
        const Projectile projectile = m_projectiles.at(projectileIndex);
        if (!projectile.hostile) {
            continue;
        }

        const QRectF projectileRect(projectile.position.x() - projectile.radius,
                                    projectile.position.y() - projectile.radius,
                                    projectile.radius * 2.0,
                                    projectile.radius * 2.0);
        if (attackRect.intersects(projectileRect)) {
            const QPointF deflectCenter = projectile.position;
            for (int sparkIndex = 0; sparkIndex < 6 && m_projectiles.size() < kMaxProjectiles; ++sparkIndex) {
                const qreal angle = (kPi * 2.0 * sparkIndex) / 6.0;
                Projectile spark;
                spark.position = deflectCenter;
                spark.velocity = QPointF(qCos(angle) * 120.0, qSin(angle) * 120.0 - 30.0);
                spark.radius = 3.0;
                spark.damage = 0;
                spark.hostile = false;
                spark.decorative = true;
                spark.gravity = 260.0;
                spark.lifetime = 0.22;
                spark.color = projectile.appliesBurn ? QColor(255, 176, 62, 180) : QColor(230, 236, 242, 180);
                spark.highlight = projectile.appliesBurn ? QColor(255, 236, 150, 210) : QColor(255, 255, 255, 210);
                m_projectiles.append(spark);
            }
            m_projectiles.remove(projectileIndex);
        }
    }

    bool triggerFogVictory = false;
    for (int enemyIndex = 0; enemyIndex < m_enemies.size(); ++enemyIndex) {
        Enemy &enemy = m_enemies[enemyIndex];
        if (!attackRect.intersects(enemy.rect)) {
            continue;
        }

        enemy.facingLeft = !m_swordAttackFacingLeft;
        applyEnemyDamage(enemy, attackDamage, facingSign);
        if (enemy.kind == EnemyKind::ZombifiedPiglin) {
            enrageNearbyPiglins(enemy.rect.center());
        }
        if (enemy.health <= 0) {
            enemy.countedDeath = true;
            if (enemy.kind == EnemyKind::FogStalker && !enemy.isPhantomClone) {
                triggerFogVictory = true;
            }
            if (enemy.kind == EnemyKind::Blaze) {
                ++m_blazeDefeatedCount;
                spawnBlazeRodLoot(enemy.rect.center());
                playSound(QStringLiteral("sounds/blazedeath.mp3"), true);
                for (int particleIndex = 0; particleIndex < 18; ++particleIndex) {
                    const qreal angle = particleIndex * ((kPi * 2.0) / 18.0);
                    const qreal speed = 140.0 + (particleIndex % 3) * 55.0;
                    Projectile burst;
                    burst.position = enemy.rect.center();
                    burst.velocity = QPointF(qCos(angle) * speed, qSin(angle) * speed - 120.0);
                    burst.radius = 5.0 + (particleIndex % 2);
                    burst.damage = 0;
                    burst.hostile = false;
                    burst.decorative = true;
                    burst.gravity = 420.0;
                    burst.lifetime = 0.9;
                    burst.color = QColor(255, 168, 52, 180);
                    burst.highlight = QColor(255, 232, 160, 220);
                    m_projectiles.append(burst);
                }
            } else if (enemy.kind == EnemyKind::ZombifiedPiglin) {
                ++m_zombiePigmanDefeatedCount;
            } else if (enemy.kind == EnemyKind::FogStalker) {
                // Fog stalker clones fade out without counting as normal kills.
            } else {
                ++m_defeatedEnemyCount;
            }
            if (!(enemy.kind == EnemyKind::FogStalker && enemy.isPhantomClone)) {
                addExperience(enemy.kind == EnemyKind::Blaze ? 20 : (enemy.kind == EnemyKind::Skeleton ? 12 : 8));
            }
        }
    }

    if (triggerFogVictory) {
        m_gameWon = true;
        m_controlsLocked = true;
        m_pressedKeys.clear();
        stopMining();
        m_deepLayerBossDefeated = true;
        m_currentLayerHint = QStringLiteral("Cleared The Fog");
        m_layerHintTimer = 5.0;
        m_fogClearTimer = kFogStalkerFogClearSeconds;
        m_victoryBannerTimer = 0.0;
        m_victorySummaryTimer = 0.0;
        for (int enemyIndex = m_enemies.size() - 1; enemyIndex >= 0; --enemyIndex) {
            if (m_enemies[enemyIndex].kind != EnemyKind::FogStalker || m_enemies[enemyIndex].isPhantomClone) {
                m_enemies.remove(enemyIndex);
            }
        }
    }
}

void GameWidget::updateCamera()
{
    const double targetX = m_player.rect.center().x() - width() / 2.0;
    const double targetY = m_player.rect.center().y() - height() / 2.0;
    const double maxCameraX = kWorldColumns * kTileSize - width();
    const double maxCameraY = kWorldRows * kTileSize - height();

    m_cameraX = qBound(0.0, targetX, qMax(0.0, maxCameraX));
    m_cameraY = qBound(0.0, targetY, qMax(0.0, maxCameraY));
}

void GameWidget::updateProjectiles(double deltaSeconds)
{
    bool triggerFogVictory = false;
    for (int i = m_projectiles.size() - 1; i >= 0; --i) {
        Projectile &projectile = m_projectiles[i];
        if (projectile.lifetime >= 0.0) {
            projectile.lifetime -= deltaSeconds;
            if (projectile.lifetime <= 0.0) {
                m_projectiles.remove(i);
                continue;
            }
        }

        if (!qFuzzyIsNull(projectile.gravity)) {
            projectile.velocity.setY(projectile.velocity.y() + projectile.gravity * deltaSeconds);
        }
        projectile.position += projectile.velocity * deltaSeconds;

        const int column = qFloor(projectile.position.x() / kTileSize);
        const int row = qFloor(projectile.position.y() / kTileSize);
        const bool outOfBounds = projectile.position.x() < 0.0
                                 || projectile.position.x() >= kWorldColumns * kTileSize
                                 || projectile.position.y() < 0.0
                                 || projectile.position.y() >= kWorldRows * kTileSize;

        if (outOfBounds || isSolidTile(column, row)) {
            m_projectiles.remove(i);
            continue;
        }

        if (projectile.decorative) {
            continue;
        }

        const QRectF projectileRect(projectile.position.x() - projectile.radius,
                                    projectile.position.y() - projectile.radius,
                                    projectile.radius * 2.0,
                                    projectile.radius * 2.0);

        if (projectile.hostile) {
            if (projectileRect.intersects(m_player.rect)) {
                applyDamage(projectile.damage, projectile.velocity.x(), projectile.appliesBurn); // 浼犲叆 true 琛ㄧず鐑堢劙浜?
                if (projectile.appliesBurn) {
                    m_player.burnTimeRemaining = qMax(m_player.burnTimeRemaining, projectile.burnDuration);
                    m_player.burnTickTimer = 0.0;
                }
                m_projectiles.remove(i);
            }
            continue;
        }

        bool hitEnemy = false;
        for (int enemyIndex = 0; enemyIndex < m_enemies.size(); ++enemyIndex) {
            Enemy &enemy = m_enemies[enemyIndex];
            if (!projectileRect.intersects(enemy.rect)) {
                continue;
            }

            const int appliedDamage = projectile.damage;
            applyEnemyDamage(enemy, appliedDamage, projectile.velocity.x());
            enemy.facingLeft = projectile.velocity.x() > 0.0;
            if (enemy.kind == EnemyKind::ZombifiedPiglin) {
                enrageNearbyPiglins(enemy.rect.center());
            }
            if (enemy.kind == EnemyKind::FogStalker) {
                enemy.hurtTimer = kFogStalkerStunSeconds;
                enemy.fogStalkerStunTimer = kFogStalkerStunSeconds;
            }
            if (enemy.health <= 0) {
                enemy.countedDeath = true;
                if (enemy.kind == EnemyKind::FogStalker && !enemy.isPhantomClone) {
                    triggerFogVictory = true;
                }
                if (enemy.kind == EnemyKind::Blaze) {
                    ++m_blazeDefeatedCount;
                    spawnBlazeRodLoot(enemy.rect.center());
                    for (int particleIndex = 0; particleIndex < 18; ++particleIndex) {
                        const qreal angle = particleIndex * ((kPi * 2.0) / 18.0);
                        const qreal speed = 140.0 + (particleIndex % 3) * 55.0;
                        Projectile burst;
                        burst.position = enemy.rect.center();
                        burst.velocity = QPointF(qCos(angle) * speed, qSin(angle) * speed - 120.0);
                        burst.radius = 5.0 + (particleIndex % 2);
                        burst.damage = 0;
                        burst.hostile = false;
                        burst.decorative = true;
                        burst.gravity = 420.0;
                        burst.lifetime = 0.9;
                        burst.color = QColor(255, 168, 52, 180);
                        burst.highlight = QColor(255, 232, 160, 220);
                        m_projectiles.append(burst);
                    }
                } else if (enemy.kind == EnemyKind::ZombifiedPiglin) {
                    ++m_zombiePigmanDefeatedCount;
                } else if (enemy.kind == EnemyKind::FogStalker) {
                    // Fog stalker clones fade out without counting as normal kills.
                } else {
                    ++m_defeatedEnemyCount;
                }
                if (!(enemy.kind == EnemyKind::FogStalker && enemy.isPhantomClone)) {
                    addExperience(enemy.kind == EnemyKind::Blaze ? 20 : (enemy.kind == EnemyKind::Skeleton ? 12 : 8));
                }
            }
            m_projectiles.remove(i);
            hitEnemy = true;
            break;
        }

        if (hitEnemy) {
            continue;
        }
    }

    if (triggerFogVictory) {
        m_gameWon = true;
        m_controlsLocked = true;
        m_pressedKeys.clear();
        stopMining();
        m_deepLayerBossDefeated = true;
        m_currentLayerHint = QStringLiteral("Cleared The Fog");
        m_layerHintTimer = 5.0;
        m_fogClearTimer = kFogStalkerFogClearSeconds;
        m_victoryBannerTimer = 0.0;
        m_victorySummaryTimer = 0.0;
        for (int enemyIndex = m_enemies.size() - 1; enemyIndex >= 0; --enemyIndex) {
            if (m_enemies[enemyIndex].kind != EnemyKind::FogStalker || m_enemies[enemyIndex].isPhantomClone) {
                m_enemies.remove(enemyIndex);
            }
        }
    }
}







/////////////////





void GameWidget::updateEnemies(double deltaSeconds)
{
    generateSurfaceMobs();
    generateNetherBlazes();
    generateZombifiedPiglins();
    generateFogStalker();

    const qreal enemyMoveSpeed = kMoveSpeed * movementSpeedMultiplier() * 0.6;
    const qreal enemyGravity = kGravity;
    const auto intersectsSolid = [this](const QRectF &rect) {
        const int minColumn = qFloor(rect.left() / kTileSize) - 1;
        const int maxColumn = qFloor((rect.right() - 1.0) / kTileSize) + 1;
        const int minRow = qFloor(rect.top() / kTileSize) - 1;
        const int maxRow = qFloor((rect.bottom() - 1.0) / kTileSize) + 1;

        for (int row = minRow; row <= maxRow; ++row) {
            for (int column = minColumn; column <= maxColumn; ++column) {
                if (!isSolidTile(column, row)) {
                    continue;
                }

                if (rect.intersects(QRectF(tileRect(column, row)))) {
                    return true;
                }
            }
        }

        return false;
    };
    const auto stepJumpSpeedFor = [](EnemyKind kind) {
        switch (kind) {
        case EnemyKind::ZombifiedPiglin:
            return kPiglinJumpSpeed;
        case EnemyKind::FogStalker:
            return 0.0;
        default:
            return kEnemyJumpSpeed;
        }
    };
    const auto nudgeOutOfSolid = [this, &intersectsSolid](QRectF &rect) {
        if (!intersectsSolid(rect)) {
            return;
        }

        const QVector<QPointF> offsets = {
            QPointF(0.0, -kTileSize),
            QPointF(-kTileSize, 0.0),
            QPointF(kTileSize, 0.0),
            QPointF(0.0, kTileSize),
            QPointF(-kTileSize, -kTileSize),
            QPointF(kTileSize, -kTileSize),
            QPointF(-kTileSize * 2.0, 0.0),
            QPointF(kTileSize * 2.0, 0.0),
            QPointF(0.0, -kTileSize * 2.0)
        };

        const QPointF baseCenter = rect.center();
        for (const QPointF &offset : offsets) {
            QRectF probe = rect;
            probe.moveCenter(baseCenter + offset);
            if (!intersectsSolid(probe)) {
                rect = probe;
                return;
            }
        }
    };
    const auto relocateFogStalkerNearPlayer = [this, &intersectsSolid](Enemy &enemy) {
        const int deepMinRow = kSurfaceRow + 100;
        const int deepMaxRow = kWorldRows - kBedrockThickness - 1;
        for (int offset = 6; offset <= int(kFogStalkerRecoveryTeleportTiles); ++offset) {
            for (int directionSign : { -1, 1 }) {
                QRectF candidate = enemy.rect;
                candidate.moveCenter(QPointF(m_player.rect.center().x() + directionSign * offset * kTileSize,
                                             m_player.rect.center().y()));
                const int minColumn = qFloor(candidate.left() / kTileSize) - 1;
                const int maxColumn = qFloor((candidate.right() - 1.0) / kTileSize) + 1;
                const int minRow = qFloor(candidate.top() / kTileSize) - 1;
                const int maxRow = qFloor((candidate.bottom() - 1.0) / kTileSize) + 1;
                bool blocked = candidate.top() < deepMinRow * kTileSize
                               || candidate.bottom() > (deepMaxRow + 1) * kTileSize;
                for (int row = minRow; row <= maxRow && !blocked; ++row) {
                    for (int column = minColumn; column <= maxColumn; ++column) {
                        if (column < 0 || column >= kWorldColumns || row < deepMinRow || row > deepMaxRow) {
                            blocked = true;
                            break;
                        }
                        if (isSolidTile(column, row) && candidate.intersects(QRectF(tileRect(column, row)))) {
                            blocked = true;
                            break;
                        }
                    }
                }
                if (!blocked && !intersectsSolid(candidate)) {
                    enemy.rect = candidate;
                    enemy.fogStalkerTeleportQueued = false;
                    enemy.fogStalkerTeleportFlashTimer = 0.0;
                    enemy.fogStalkerAfterimageTimer = kFogStalkerAfterimageSeconds;
                    return;
                }
            }
        }
    };

    for (int i = m_enemies.size() - 1; i >= 0; --i) {
        Enemy &enemy = m_enemies[i];
        enemy.attackCooldown = qMax(0.0, enemy.attackCooldown - deltaSeconds);
        enemy.hurtTimer = qMax(0.0, enemy.hurtTimer - deltaSeconds);
        enemy.knockbackTimer = qMax(0.0, enemy.knockbackTimer - deltaSeconds);
        enemy.angerTimeRemaining = qMax(0.0, enemy.angerTimeRemaining - deltaSeconds);
        enemy.attackWindupTimer = qMax(0.0, enemy.attackWindupTimer - deltaSeconds);
        enemy.walkPhase += deltaSeconds * qAbs(enemy.velocity.x()) * 0.06;
        enemy.blazeFloatPhase += deltaSeconds * 6.283185307179586;
        enemy.turnCooldown = qMax(0.0, enemy.turnCooldown - deltaSeconds);
        enemy.randomTurnTimer = qMax(0.0, enemy.randomTurnTimer - deltaSeconds);

        if (enemy.health > 0 && rectIntersectsLava(enemy.rect)) {
            enemy.lavaDamageTimer += deltaSeconds;
            while (enemy.lavaDamageTimer >= kBlazeBurnTickSeconds && enemy.health > 0) {
                enemy.lavaDamageTimer -= kBlazeBurnTickSeconds;
                applyEnemyDamage(enemy, kHeartHitPoints, 0.0);
            }
        } else {
            enemy.lavaDamageTimer = 0.0;
        }

        if (enemy.health <= 0) {
            if (!enemy.countedDeath) {
                if (enemy.kind == EnemyKind::Blaze) {
                    ++m_blazeDefeatedCount;
                    spawnBlazeRodLoot(enemy.rect.center());
                    addExperience(20);
                } else if (enemy.kind == EnemyKind::ZombifiedPiglin) {
                    ++m_zombiePigmanDefeatedCount;
                    addExperience(8);
                } else if (enemy.kind == EnemyKind::Skeleton) {
                    ++m_defeatedEnemyCount;
                    addExperience(12);
                } else if (enemy.kind == EnemyKind::Zombie) {
                    ++m_defeatedEnemyCount;
                    addExperience(8);
                }
                enemy.countedDeath = true;
            }
            if (enemy.kind == EnemyKind::FogStalker && !enemy.isPhantomClone) {
                enemy.deathTimer += deltaSeconds;
                enemy.visibility = qMax(0.0, enemy.visibility - deltaSeconds * 1.8);
                if (enemy.deathTimer < kFogStalkerDeathFadeSeconds) {
                    continue;
                }
            } else if (enemy.kind == EnemyKind::FogStalker && enemy.isPhantomClone) {
                enemy.deathTimer += deltaSeconds;
                enemy.visibility = qMax(0.0, enemy.visibility - deltaSeconds * 2.2);
                if (enemy.deathTimer < kFogStalkerDeathFadeSeconds) {
                    continue;
                }
            }
            if (enemy.kind == EnemyKind::Zombie || enemy.kind == EnemyKind::Skeleton || enemy.kind == EnemyKind::ZombifiedPiglin) {
                enemy.deathTimer += deltaSeconds;
                const double deathAnimDuration = enemy.kind == EnemyKind::ZombifiedPiglin
                                                     ? kEnemyDeathAnimSeconds
                                                     : kEnemyDeathAnimSeconds;
                if (enemy.deathTimer < deathAnimDuration) {
                    continue;
                }
            }
            m_enemies.remove(i);
            continue;
        }

        const QPointF toPlayer = m_player.rect.center() - enemy.rect.center();
        const qreal distanceTiles = std::hypot(toPlayer.x(), toPlayer.y()) / kTileSize;
        qreal desiredVelocityX = 0.0;
        bool facingLeft = enemy.facingLeft;

        if (enemy.kind == EnemyKind::FogStalker) {
            const qreal healthRatio = qreal(enemy.health) / qreal(m_fogStalkerMaxHealth);
            const int phase = enemy.isPhantomClone ? 3 : (healthRatio > 0.7 ? 1 : (healthRatio > 0.3 ? 2 : 3));
            const qreal moveSpeed = phase == 1 ? kFogStalkerBaseSpeed : (phase == 2 ? kFogStalkerPhase2Speed : kFogStalkerPhase3Speed);

            if (!enemy.isPhantomClone && enemy.fogStalkerPhaseState != phase) {
                enemy.fogStalkerPhaseState = phase;
                m_phaseFlashTimer = kFogStalkerPhaseFlashSeconds;
                m_currentLayerHint = phase == 2 ? QStringLiteral("Phase 2: Shadow Blink")
                                                : QStringLiteral("Phase 3: Frenzy In Fog");
                m_layerHintTimer = 2.0;
            }

            enemy.teleportTimer = qMax(0.0, enemy.teleportTimer - deltaSeconds);
            if (!enemy.isPhantomClone && !hasFogStalkerClone()) {
                enemy.phantomTimer = qMax(0.0, enemy.phantomTimer - deltaSeconds);
            }
            enemy.fogStalkerStunTimer = qMax(0.0, enemy.fogStalkerStunTimer - deltaSeconds);
            enemy.fogStalkerTeleportFlashTimer = qMax(0.0, enemy.fogStalkerTeleportFlashTimer - deltaSeconds);
            enemy.fogStalkerAfterimageTimer = qMax(0.0, enemy.fogStalkerAfterimageTimer - deltaSeconds);
            enemy.fogStalkerLightningTimer = qMax(0.0, enemy.fogStalkerLightningTimer - deltaSeconds);
            if (enemy.isPhantomClone) {
                enemy.fogStalkerCloneLifeTimer = qMax(0.0, enemy.fogStalkerCloneLifeTimer - deltaSeconds);
                if (enemy.fogStalkerCloneLifeTimer <= 0.0) {
                    enemy.health = 0;
                    enemy.deathTimer = 0.0;
                    continue;
                }
            }

            // 条件A：角色朝向（面朝左侧且Boss在左 或 面朝右侧且Boss在右）
            const bool bossOnFacingSide = (m_player.facingLeft && enemy.rect.center().x() < m_player.rect.center().x())
                                          || (!m_player.facingLeft && enemy.rect.center().x() > m_player.rect.center().x());

            // 条件B：鼠标瞄准方向与Boss方向的夹角是否小于30度（点积判断）
            QPointF cursorDir = m_cursorWorldPos - m_player.rect.center();
            QPointF bossDir = enemy.rect.center() - m_player.rect.center();
            qreal cursorLength = std::hypot(cursorDir.x(), cursorDir.y());
            qreal bossLength = std::hypot(bossDir.x(), bossDir.y());
            bool cursorFacingBoss = false;
            if (cursorLength > 1.0 && bossLength > 1.0)
            {
                cursorDir /= cursorLength;
                bossDir /= bossLength;
                cursorFacingBoss = QPointF::dotProduct(cursorDir, bossDir) >= qCos(qDegreesToRadians(30.0));
            }

            bool torchFacingBoss = false;// 条件C：Boss是否在火把的2格范围内（火把可以驱散隐身）
            const qreal torchSightRange = 2.0 * kTileSize * (m_fearDebuffStacks > 0 ? 0.7 : 1.0);
            for (int torchKey : std::as_const(m_torchTiles)) {
                const int torchColumn = torchKey % kWorldColumns;
                const int torchRow = torchKey / kWorldColumns;
                const QPointF torchCenter((torchColumn + 0.5) * kTileSize, (torchRow + 0.5) * kTileSize);
                if (QLineF(torchCenter, enemy.rect.center()).length() <= torchSightRange) {
                    torchFacingBoss = true;// 计算火把与Boss的距离
                    break;
                }
            }

            const bool playerFacingBoss = bossOnFacingSide || cursorFacingBoss || torchFacingBoss;// 任意条件满足即视为“玩家正在看Boss”


            enemy.targetVisibility = playerFacingBoss ? 0.05 : (enemy.isPhantomClone ? 0.52 : 0.9);// 根据视线设置目标可见度：看着时只有0.05（几乎隐身），否则本体0.9、分身0.52




            if (enemy.fogStalkerTeleportFlashTimer > 0.0) {
                const int blinkBand = qFloor((kFogStalkerTeleportFlashSeconds - enemy.fogStalkerTeleportFlashTimer) / (kFogStalkerTeleportFlashSeconds / 3.0));
                enemy.targetVisibility = (blinkBand % 2 == 0) ? 0.95 : 0.08;
            }
            // 将剩余闪灭时间分成3段，根据当前段数的奇偶决定目标可见度
            // 偶数段 → 几乎完全可见 (0.95)，奇数段 → 几乎不可见 (0.08)

            //  平滑过渡当前可见度
            if (enemy.visibility < enemy.targetVisibility)
            {

                enemy.visibility = qMin(enemy.targetVisibility, enemy.visibility + deltaSeconds * 3.4);// 当前透明度不够 , 向目标透明度增加
            }
            else {
                enemy.visibility = qMax(enemy.targetVisibility, enemy.visibility - deltaSeconds * 3.4);// 当前透明度太高,向目标透明度减少
            }

            // 定时随机传送
            if (!enemy.isPhantomClone && enemy.teleportTimer <= 0.0)
            {
                enemy.teleportTimer = kFogStalkerRandomTeleportSeconds;// 重置传送计时器
                teleportFogStalkerToRandomSafePosition(enemy, 8.0, 12.0, true);// 在距离玩家 8~12 格范围内寻找安全位置并瞬移
            }
  //  生成分身
            if (!enemy.isPhantomClone && enemy.phantomTimer <= 0.0) {
                if (!hasFogStalkerClone()) {
                    spawnFogStalkerClone(enemy);
                    enemy.phantomTimer = kFogStalkerCloneIntervalSeconds;
                }
            }

            // 雷电打击
            if (!enemy.isPhantomClone && enemy.fogStalkerLightningTimer <= 0.0) {
                enemy.fogStalkerLightningTimer = kFogStalkerLightningStrikeSeconds;
                m_phaseFlashTimer = kFogStalkerPhaseFlashSeconds;
                m_playerOutlineFlashTimer = 0.35;
                playSound(QStringLiteral("sounds/fogflash.mp3"));
                applyDamage(kFogStalkerLightningDamage, 0.0);
            }

            QPointF desiredMove(0.0, 0.0);
            if (enemy.fogStalkerStunTimer > 0.0) {
                enemy.targetVisibility = 0.9;
                enemy.visibility = 0.9;
            } else if (enemy.fogStalkerTeleportQueued) {
                if (enemy.fogStalkerTeleportFlashTimer <= 0.0) {
                    QRectF teleported = enemy.rect;
                    teleported.moveCenter(enemy.fogStalkerTeleportTarget);
                    nudgeOutOfSolid(teleported);
                    if (!intersectsSolid(teleported)) {
                        enemy.rect = teleported;
                        enemy.visibility = 0.08;
                        enemy.fogStalkerAfterimageTimer = kFogStalkerAfterimageSeconds;
                    }
                    enemy.fogStalkerTeleportQueued = false;
                    enemy.teleportTimer = kFogStalkerRandomTeleportSeconds;
                }
            } else if (!enemy.isPhantomClone && m_playerFacingSameDirectionTimer >= kFogStalkerSameFacingPunishSeconds) {
                const qreal behindSign = m_player.facingLeft ? 1.0 : -1.0;
                QRectF teleported = enemy.rect;
                teleported.moveCenter(QPointF(m_player.rect.center().x() + behindSign * 2.2 * kTileSize,
                                              m_player.rect.center().y() - 0.4 * kTileSize));
                nudgeOutOfSolid(teleported);
                if (!intersectsSolid(teleported)) {
                    enemy.rect = teleported;
                    enemy.visibility = 0.9;
                    enemy.fogStalkerAfterimageTimer = kFogStalkerAfterimageSeconds;
                    enemy.fogStalkerTeleportFlashTimer = kFogStalkerTeleportFlashSeconds;
                    m_phaseFlashTimer = kFogStalkerPhaseFlashSeconds;
                    applyDamage(kFogStalkerFearDamage, behindSign);
                    enemy.attackCooldown = 1.0;
                }
                m_playerFacingSameDirectionTimer = 0.0;
            } else if (playerFacingBoss) {
                if (distanceTiles < 5.0) {
                    QPointF retreat = enemy.rect.center() - m_player.rect.center();
                    const qreal retreatLength = std::hypot(retreat.x(), retreat.y());
                    if (retreatLength > 1.0) {
                        retreat /= retreatLength;
                        desiredMove = retreat * moveSpeed * 0.55;
                    }
                }
            } else {
                QPointF chase = m_player.rect.center() - enemy.rect.center();
                const qreal chaseLength = std::hypot(chase.x(), chase.y());
                if (chaseLength > 1.0) {
                    chase /= chaseLength;
                    desiredMove = chase * moveSpeed;
                }

                if (distanceTiles <= kFogStalkerFearRangeTiles && enemy.attackCooldown <= 0.0) {
                    const bool cloneAttack = enemy.isPhantomClone;
                    applyDamage(cloneAttack ? kHeartHitPoints : kFogStalkerFearDamage, toPlayer.x());
                    enemy.attackCooldown = enemy.isPhantomClone ? 1.0 : (phase == 3 ? kFogStalkerFearTickSeconds : 1.0);
                    if (!cloneAttack) {
                        m_fearDebuffStacks = qMin(kFogStalkerFearMaxStacks, m_fearDebuffStacks + 1);
                        m_fearDebuffTimer = kFogStalkerFearDebuffSeconds;
                        m_fearFogBoost = qMin(0.95, m_fearFogBoost + 0.05);
                        for (int cloneIndex = 0; cloneIndex < m_enemies.size(); ++cloneIndex) {
                            Enemy &clone = m_enemies[cloneIndex];
                            if (clone.kind == EnemyKind::FogStalker && clone.isPhantomClone) {
                                clone.health = 0;
                            }
                        }
                    }
                }
            }


            enemy.facingLeft = toPlayer.x() < 0.0;
            if (enemy.isPhantomClone) {
                QPointF dive = m_player.rect.center() - enemy.rect.center();
                const qreal diveLength = std::hypot(dive.x(), dive.y());
                if (diveLength > 1.0) {
                    dive /= diveLength;
                    desiredMove = dive * kFogStalkerCloneDiveSpeed;
                }
            }
            QRectF movedRect = enemy.rect.translated(desiredMove.x() * deltaSeconds, desiredMove.y() * deltaSeconds);
            if (!intersectsSolid(movedRect)) {
                enemy.rect = movedRect;
            }
            nudgeOutOfSolid(enemy.rect);
            const qreal deepTop = (kSurfaceRow + 100) * kTileSize;
            const qreal deepBottom = (kWorldRows - kBedrockThickness) * kTileSize;
            if (enemy.rect.top() < deepTop) {
                enemy.rect.moveTop(deepTop);
            } else if (enemy.rect.bottom() > deepBottom) {
                enemy.rect.moveBottom(deepBottom);
            }
            if (intersectsSolid(enemy.rect)) {
                relocateFogStalkerNearPlayer(enemy);
            }
            continue;
        }

        if (enemy.kind == EnemyKind::Blaze) {
            const bool playerDeep = (playerTileRow() >= 80 && playerTileRow() < kSurfaceRow + 100);
            const bool hasAggro = playerDeep && (distanceTiles <= kBlazeAggroRangeTiles || enemy.attackCooldown > 0.0 || enemy.blazeChargeTimer > 0.0);
            if (hasAggro && distanceTiles <= kBlazeLoseAggroRangeTiles) {
                if (distanceTiles < kBlazeMinSafeRangeTiles) {
                    desiredVelocityX = toPlayer.x() < 0.0 ? kBlazeMoveSpeed : -kBlazeMoveSpeed;
                } else if (distanceTiles > kBlazeOptimalRangeTiles) {
                    desiredVelocityX = toPlayer.x() < 0.0 ? -kBlazeMoveSpeed : kBlazeMoveSpeed;
                } else {
                    desiredVelocityX = 0.0;
                }
                facingLeft = toPlayer.x() < 0.0;

                const qreal targetY = m_player.rect.center().y() - kTileSize * 1.5;
                const qreal verticalDelta = targetY - enemy.rect.center().y();
                const qreal verticalStep = qBound(-kBlazeMoveSpeed, verticalDelta, kBlazeMoveSpeed);
                enemy.velocity.setY(verticalStep);

                if (enemy.attackCooldown <= 0.0) {
                    enemy.blazeChargeTimer += deltaSeconds;
                    if (enemy.blazeChargeTimer >= kBlazeChargeSeconds && m_projectiles.size() < kMaxProjectiles) {
                        QPointF direction = m_player.rect.center() - enemy.rect.center();
                        const qreal length = std::hypot(direction.x(), direction.y());
                        if (length >= 1.0) {
                            direction /= length;
                        } else {
                            direction = QPointF(facingLeft ? -1.0 : 1.0, 0.0);
                        }

                        Projectile fireball;
                        fireball.position = enemy.rect.center() + direction * 20.0;
                        fireball.velocity = direction * (kProjectileSpeed * 0.75);
                        fireball.damage = kBlazeFireballDamage;
                        fireball.hostile = true;
                        fireball.appliesBurn = true;
                        fireball.burnDuration = kBlazeBurnDurationSeconds;
                        fireball.burnTickInterval = kBlazeBurnTickSeconds;
                        fireball.color = QColor(255, 168, 52);
                        fireball.highlight = QColor(255, 233, 153);
                        m_projectiles.append(fireball);
                        enemy.attackCooldown = kBlazeCooldownSeconds;
                        enemy.blazeChargeTimer = 0.0;
                    }
                }
            } else {
                desiredVelocityX = qSin(enemy.blazeFloatPhase * 0.4) * kBlazeMoveSpeed * 0.35;
                enemy.velocity.setY(qSin(enemy.blazeFloatPhase) * 16.0);
                enemy.blazeChargeTimer = 0.0;
            }


            const int checkCol = qFloor(enemy.rect.center().x() / kTileSize);
            int ceilingRow = -1;
            for (int r = qFloor(enemy.rect.top() / kTileSize) - 1; r >= 0; --r) {
                if (isSolidTile(checkCol, r)) {
                    ceilingRow = r;
                    break;
                }
            }
            if (ceilingRow >= 0) {
                qreal ceilingBottom = (ceilingRow + 1) * kTileSize;
                if (enemy.rect.top() - ceilingBottom < 5 * kTileSize) {
                    enemy.velocity.setY(qMax(enemy.velocity.y(), 80.0)); // 寮哄埗鍚戜笅
                }
            }


            enemy.facingLeft = facingLeft;
            if (enemy.knockbackTimer > 0.0) {
                enemy.velocity.setX(enemy.knockbackDirection * kHitKnockbackSpeed);
            } else {
                enemy.velocity.setX(desiredVelocityX);
            }

            enemy.rect.translate(enemy.velocity.x() * deltaSeconds, enemy.velocity.y() * deltaSeconds);

            const int blazeColumn = qBound(0, qFloor(enemy.rect.center().x() / kTileSize), kWorldColumns - 1);
            int stoneRowBelow = kWorldRows - kBedrockThickness;
            for (int row = qFloor(enemy.rect.bottom() / kTileSize); row < kWorldRows - kBedrockThickness; ++row) {
                if (isSolidTile(blazeColumn, row)) {
                    stoneRowBelow = row;
                    break;
                }
            }
            const qreal maxBottom = stoneRowBelow * kTileSize - kBlazeMinStoneClearanceTiles * kTileSize;
            if (enemy.rect.bottom() > maxBottom) {
                enemy.rect.moveBottom(maxBottom);
            }

            const qreal blazeBottomLimit = (kSurfaceRow + 100) * kTileSize - 2.0;
            if (enemy.rect.bottom() > blazeBottomLimit) {
                enemy.rect.moveBottom(blazeBottomLimit);
                enemy.velocity.setY(qMin<qreal>(enemy.velocity.y(), 0.0));
            }

            const qreal worldWidth = kWorldColumns * kTileSize;
            const qreal worldHeight = (kWorldRows - kBedrockThickness) * kTileSize;
            if (enemy.rect.left() < 0.0) {
                enemy.rect.moveLeft(0.0);
                enemy.facingLeft = false;
            } else if (enemy.rect.right() > worldWidth) {
                enemy.rect.moveRight(worldWidth);
                enemy.facingLeft = true;
            }
            if (enemy.rect.top() < 0.0) {
                enemy.rect.moveTop(0.0);
            } else if (enemy.rect.bottom() > worldHeight) {
                enemy.rect.moveBottom(worldHeight);
            }


            for (int j = i + 1; j < m_enemies.size(); ++j) {
                Enemy &other = m_enemies[j];
                if (other.kind != EnemyKind::Blaze || other.health <= 0) continue;
                QPointF delta = enemy.rect.center() - other.rect.center();
                qreal dist = std::hypot(delta.x(), delta.y());
                qreal minDist = kTileSize * 1.8;
                if (dist < minDist && dist > 0.01) {
                    QPointF push = delta / dist;
                    qreal force = (minDist - dist) * 0.5;
                    enemy.rect.translate(push * force);
                    other.rect.translate(-push * force);
                }
            }


            enemy.airbornePeakY = enemy.rect.top();

            nudgeOutOfSolid(enemy.rect);

            continue;
        }
        if (enemy.kind == EnemyKind::ZombifiedPiglin) {
            const bool angry = enemy.angerTimeRemaining > 0.0;
            if (enemy.health > 0 && m_piglinAmbientTimer <= 0.0
                    && playerTileRow() >= 80 && playerTileRow() <= 100) {
                    playSound(QStringLiteral("sounds/zpig1.mp3"), true);
                    m_piglinAmbientTimer = 4.5;
            }
            const qreal patrolMinX = enemy.patrolOriginX - kPiglinPatrolRadiusTiles * kTileSize;
            const qreal patrolMaxX = enemy.patrolOriginX + kPiglinPatrolRadiusTiles * kTileSize;

            if (angry && distanceTiles <= kPiglinChaseRangeTiles) {
                if (distanceTiles > 3.0) {
                    desiredVelocityX = toPlayer.x() < 0.0 ? -kPiglinMoveSpeed : kPiglinMoveSpeed;
                    facingLeft = desiredVelocityX < 0.0;
                } else {
                    desiredVelocityX = 0.0;
                    facingLeft = toPlayer.x() < 0.0;
                    if (enemy.attackCooldown <= 0.0 && enemy.attackWindupTimer <= 0.0) {
                        enemy.attackWindupTimer = kPiglinAttackWindupSeconds;
                        enemy.attackCooldown = kPiglinAttackCooldownSeconds;
                    }
                }
            } else {
                if (enemy.angerTimeRemaining <= 0.0) {
                    if (enemy.randomTurnTimer <= 0.0) {
                        enemy.randomTurnTimer = randomBetween(3, 6);   // 3~6绉掗殢鏈?
                        enemy.patrolDirection = randomChance(50) ? -1.0 : 1.0;
                    }
                    const qreal centerX = enemy.rect.center().x();
                    if (centerX <= patrolMinX) {
                        enemy.patrolDirection = 1.0;
                    } else if (centerX >= patrolMaxX) {
                        enemy.patrolDirection = -1.0;
                    }
                    desiredVelocityX = enemy.patrolDirection * kPiglinMoveSpeed * 0.5;
                    facingLeft = desiredVelocityX < 0.0;
                }
            }

            if (enemy.attackWindupTimer > 0.0 && enemy.attackWindupTimer <= kPiglinAttackWindupSeconds * 0.35
                && distanceTiles <= 3.0) {
                applyDamage(kPiglinDamage, toPlayer.x());
                enemy.attackWindupTimer = 0.0;
            }
        }

        if (enemy.kind == EnemyKind::Zombie) {
            if (enemy.health > 0 && m_zombieAmbientTimer <= 0.0 && qFloor(m_player.rect.center().y() / kTileSize) < 50) {
                playRandomSurfaceMobSound(QStringList() << QStringLiteral("sounds/zombie1.mp3") << QStringLiteral("sounds/zombie2.mp3"));
                m_zombieAmbientTimer = 3.8;
            }
            if (distanceTiles <= kZombieAggroRangeTiles) {
                if (distanceTiles > 0.35) {
                    desiredVelocityX = toPlayer.x() < 0.0 ? -enemyMoveSpeed : enemyMoveSpeed;
                    facingLeft = desiredVelocityX < 0.0;
                }

                if (distanceTiles <= 3.0 && enemy.attackCooldown <= 0.0 && enemy.attackWindupTimer <= 0.0) {
                    enemy.attackWindupTimer = kZombieAttackSwingSeconds;
                    applyDamage(kEnemyDamage, toPlayer.x());
                    enemy.attackCooldown = 1.5;
                }
            } else {
                if (enemy.randomTurnTimer <= 0.0) {
                    enemy.randomTurnTimer = randomBetween(2, 4);   // 2~4绉掗殢鏈?
                    enemy.facingLeft = randomChance(50);
                }
                desiredVelocityX = enemy.facingLeft ? -enemyMoveSpeed * 0.45 : enemyMoveSpeed * 0.45;
                facingLeft = enemy.facingLeft;
            }
        }
        else if (enemy.kind == EnemyKind::Skeleton) {
            if (distanceTiles <= kSkeletonAggroRangeTiles) {
                if (distanceTiles < 8.0) {
                    desiredVelocityX = toPlayer.x() < 0.0 ? enemyMoveSpeed : -enemyMoveSpeed;
                    facingLeft = desiredVelocityX < 0.0;
                } else if (distanceTiles > 15.0) {
                    desiredVelocityX = toPlayer.x() < 0.0 ? -enemyMoveSpeed : enemyMoveSpeed;
                    facingLeft = desiredVelocityX < 0.0;
                } else {
                    facingLeft = toPlayer.x() < 0.0;
                    if (enemy.attackCooldown <= 0.0 && enemy.attackWindupTimer <= 0.0) {
                        enemy.attackWindupTimer = kSkeletonBowChargeSeconds;
                        enemy.attackCooldown = kSkeletonBowChargeSeconds + 1.0;
                    }
                    if (enemy.attackWindupTimer > 0.0
                        && enemy.attackWindupTimer <= 0.04
                        && m_projectiles.size() < kMaxProjectiles) {
                        QPointF direction = m_player.rect.center() - enemy.rect.center();
                        const qreal length = std::hypot(direction.x(), direction.y());
                        if (length >= 1.0) {
                            direction /= length;
                        } else {
                            direction = QPointF(-1.0, 0.0);
                        }

                        Projectile arrow;
                        arrow.position = enemy.rect.center() + direction * 18.0;
                        arrow.velocity = direction * kProjectileSpeed;
                        arrow.damage = kEnemyDamage;
                        arrow.hostile = true;
                        arrow.color = QColor(214, 222, 230);
                        arrow.highlight = QColor(250, 252, 255);
                        m_projectiles.append(arrow);
                        enemy.attackWindupTimer = 0.0;
                    }
                }
            } else {
                if (enemy.randomTurnTimer <= 0.0) {
                    enemy.randomTurnTimer = randomBetween(2, 5);
                    enemy.facingLeft = randomChance(50);
                }
                desiredVelocityX = enemy.facingLeft ? -enemyMoveSpeed * 0.4 : enemyMoveSpeed * 0.4;
                facingLeft = enemy.facingLeft;
            }
        }

        enemy.facingLeft = facingLeft;
        if (enemy.knockbackTimer > 0.0) {
            enemy.velocity.setX(enemy.knockbackDirection * kHitKnockbackSpeed);
        } else {
            enemy.velocity.setX(desiredVelocityX);
        }

        bool jumpedOntoStep = false;
        if (enemy.knockbackTimer <= 0.0 && enemy.onGround && !qFuzzyIsNull(enemy.velocity.x())) {
            const int directionSign = enemy.velocity.x() > 0.0 ? 1 : -1;
            const int footRow = qFloor((enemy.rect.bottom() - 1.0) / kTileSize);
            const int frontColumn = qFloor((directionSign > 0 ? enemy.rect.right() : enemy.rect.left() - 1.0) / kTileSize);
            const bool oneBlockStepAhead = isSolidTile(frontColumn, footRow)
                                           && !isSolidTile(frontColumn, footRow - 1);
            const bool jumpSpaceClear = !isSolidTile(frontColumn, footRow - 2)
                                        && !isSolidTile(frontColumn, footRow - 3);
            if (oneBlockStepAhead && jumpSpaceClear) {
                enemy.velocity.setY(-stepJumpSpeedFor(enemy.kind));
                qreal moveSpeed = (enemy.kind == EnemyKind::ZombifiedPiglin) ? kPiglinMoveSpeed : enemyMoveSpeed;
                enemy.velocity.setX(directionSign * moveSpeed * 0.7);
                enemy.onGround = false;
                enemy.rect.translate(directionSign * 8.0, -8.0);
                jumpedOntoStep = true;
            }
        }

        QRectF movedRect = enemy.rect;
        movedRect.translate(enemy.velocity.x() * deltaSeconds, 0.0);
        if (!intersectsSolid(movedRect)) {
            enemy.rect = movedRect;
        } else {
            enemy.velocity.setX(0.0);
            if (enemy.kind == EnemyKind::ZombifiedPiglin && enemy.angerTimeRemaining > 0.0
                && enemy.knockbackTimer <= 0.0 && enemy.onGround) {
                const int directionSign = enemy.facingLeft ? -1 : 1;
                const int footRow = qFloor((enemy.rect.bottom() - 1.0) / kTileSize);
                const int frontColumn = qFloor((directionSign > 0 ? enemy.rect.right() : enemy.rect.left() - 1.0) / kTileSize);
                const bool oneBlockStepAhead = isSolidTile(frontColumn, footRow)
                                               && !isSolidTile(frontColumn, footRow - 1);
                const bool jumpSpaceClear = !isSolidTile(frontColumn, footRow - 2)
                                            && !isSolidTile(frontColumn, footRow - 3);
                if (oneBlockStepAhead && jumpSpaceClear) {
                    enemy.velocity.setY(-stepJumpSpeedFor(enemy.kind));
                    enemy.onGround = false;
                    jumpedOntoStep = true;
                }
            }
            if (enemy.knockbackTimer <= 0.0 && !jumpedOntoStep
                && !(enemy.kind == EnemyKind::ZombifiedPiglin && enemy.angerTimeRemaining > 0.0)
                && enemy.turnCooldown <= 0.0) {
                enemy.facingLeft = randomChance(50);
                enemy.turnCooldown = 0.4;
            }
        }

        const bool wasOnGround = enemy.onGround;
        enemy.velocity.setY(qMin(enemy.velocity.y() + enemyGravity * deltaSeconds, kMaxFallSpeed));
        movedRect = enemy.rect;
        movedRect.translate(0.0, enemy.velocity.y() * deltaSeconds);
        enemy.onGround = false;

        if (!intersectsSolid(movedRect)) {
            enemy.rect = movedRect;
        } else {
            const qreal step = enemy.velocity.y() > 0.0 ? 1.0 : -1.0;
            const int maxSteps = qMax(1, qCeil(qAbs(enemy.velocity.y() * deltaSeconds)));
            for (int stepIndex = 0; stepIndex < maxSteps; ++stepIndex) {
                QRectF probe = enemy.rect;
                probe.translate(0.0, step);
                if (intersectsSolid(probe)) {
                    break;
                }
                enemy.rect = probe;
            }

            if (enemy.velocity.y() > 0.0) {
                enemy.onGround = true;
            }
            enemy.velocity.setY(0.0);
        }

        if (enemy.onGround) {
            if (!wasOnGround) {
                const double fallDistance = qMax(0.0, enemy.rect.top() - enemy.airbornePeakY);
                const int fallenTiles = qFloor(fallDistance / kTileSize);
                if (enemy.kind == EnemyKind::ZombifiedPiglin && fallenTiles >= 5) {
                    const int damage = kHeartHitPoints * 2 + ((fallenTiles - 5) / 2) * (kHeartHitPoints / 2);
                    applyEnemyDamage(enemy, damage, 0.0);
                }
            }
            enemy.airbornePeakY = enemy.rect.top();
        } else {
            if (wasOnGround) {
                enemy.airbornePeakY = enemy.rect.top();
            }
            enemy.airbornePeakY = qMin(enemy.airbornePeakY, enemy.rect.top());
        }

        const qreal worldWidth = kWorldColumns * kTileSize;
        const qreal worldHeight = (kWorldRows - kBedrockThickness) * kTileSize;
        if (enemy.rect.left() < 0.0) {
            enemy.rect.moveLeft(0.0);
        } else if (enemy.rect.right() > worldWidth) {
            enemy.rect.moveRight(worldWidth);
        }
        if (enemy.rect.bottom() > worldHeight) {
            enemy.rect.moveBottom(worldHeight);
            enemy.velocity.setY(0.0);
            enemy.onGround = true;
        }
    }
}


void GameWidget::clampPlayerToWorld()
{
    const double worldWidth = kWorldColumns * kTileSize;
    const double worldHeight = (kWorldRows - kBedrockThickness) * kTileSize;

    if (m_player.rect.left() < 0.0) {
        m_player.rect.moveLeft(0.0);
        m_player.velocity.setX(0.0);
    }

    if (m_player.rect.right() > worldWidth) {
        m_player.rect.moveRight(worldWidth);
        m_player.velocity.setX(0.0);
    }

    if (m_player.rect.top() < 0.0) {
        m_player.rect.moveTop(0.0);
        m_player.velocity.setY(0.0);
    }

    if (m_player.rect.bottom() > worldHeight) {
        m_player.rect.moveBottom(worldHeight);
        m_player.velocity.setY(0.0);
        m_player.onGround = true;
        m_airbornePeakY = m_player.rect.top();
    }
}
/////
void GameWidget::drawBackground(QPainter &painter)
{
    const qreal horizonY = height() * 2.0 / 3.0;
    const qreal parallaxX = 0.15;
    qreal totalOffset = -m_cameraX * parallaxX;


    QLinearGradient bgGradient(0, 0, 0, height());
    bgGradient.setColorAt(0.0, QColor(20, 22, 35));
    bgGradient.setColorAt(horizonY/height(), QColor(15, 16, 26));
    bgGradient.setColorAt(1.0, QColor(8, 9, 18));
    painter.fillRect(rect(), bgGradient);

    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    qreal currentX = totalOffset;


    while (currentX < width())
    {
        bool advanced = false;
        for (const QPixmap &bg : std::as_const(m_bgList))
        {
            if (bg.isNull()) continue;
            qreal scale = horizonY / bg.height();
            int drawW = bg.width() * scale;
            int drawH = horizonY;
            if (drawW <= 0 || drawH <= 0) continue;

            painter.drawPixmap(QRect(currentX, 0, drawW, drawH), bg);
            currentX += drawW;
            advanced = true;

            if (currentX >= width()) break;
        }
        if (!advanced) break;
    }


    QLinearGradient vignette(rect().center(), rect().topLeft());
    vignette.setColorAt(0, QColor(0,0,0,0));
    vignette.setColorAt(1, QColor(0,0,0,45));
    painter.fillRect(rect(), vignette);
}
/////

void GameWidget::drawProjectiles(QPainter &painter)
{
    painter.setPen(Qt::NoPen);
    for (const Projectile &projectile : std::as_const(m_projectiles)) {
        painter.setBrush(projectile.color);
        painter.drawEllipse(projectile.position, projectile.radius, projectile.radius);
        painter.setBrush(projectile.highlight);
        painter.drawEllipse(projectile.position + QPointF(-1.5, -1.5), projectile.radius * 0.35, projectile.radius * 0.35);
    }
}
void GameWidget::drawLootFx(QPainter &painter)
{
    painter.save();
    painter.setPen(Qt::NoPen);
    for (const LootFx &loot : std::as_const(m_lootFx)) {
        painter.save();
        painter.translate(loot.screenPos);
        painter.scale(loot.scale, loot.scale);
        painter.setOpacity(loot.alpha);
        painter.setBrush(QColor(246, 207, 72));
        painter.drawRect(QRectF(-8.0, -3.0, 16.0, 6.0));
        painter.setBrush(QColor(255, 240, 160));
        painter.drawRect(QRectF(-5.0, -1.5, 10.0, 3.0));
        painter.restore();
    }
    painter.restore();
}

void GameWidget::drawEnemies(QPainter &painter)
{
    painter.setPen(Qt::NoPen);
    for (const Enemy &enemy : std::as_const(m_enemies)) {
        if (enemy.kind == EnemyKind::Blaze) {
            const QPointF playerDelta = m_player.rect.center() - enemy.rect.center();
            const qreal playerDistanceTiles = std::hypot(playerDelta.x(), playerDelta.y()) / kTileSize;
            const bool blazeAggro = playerDistanceTiles <= kBlazeAggroRangeTiles || enemy.attackCooldown > 0.0 || enemy.blazeChargeTimer > 0.0;
            const qreal hoverOffset = blazeAggro ? 0.0 : qSin(enemy.blazeFloatPhase) * 4.0;
            const qreal rodSpin = enemy.blazeFloatPhase * (enemy.blazeChargeTimer > 0.0 ? 2.4 : 1.2);
            const qreal chargeRatio = qBound(0.0, enemy.blazeChargeTimer / kBlazeChargeSeconds, 1.0);
            const QPointF center(enemy.rect.center().x(), enemy.rect.center().y() + hoverOffset);

            painter.save();
            painter.translate(center);

            for (int particleIndex = 0; particleIndex < 5; ++particleIndex) {
                const qreal particlePhase = enemy.blazeFloatPhase + particleIndex * 0.9;
                const QPointF particlePos(qCos(particlePhase) * 10.0,
                                          qSin(particlePhase * 1.3) * 11.0 - 12.0);
                const qreal radius = 4.0 + (particleIndex % 2);
                painter.setBrush(QColor(255, 170, 52, 70));
                painter.drawEllipse(particlePos, radius, radius + 2.0);
                painter.setBrush(QColor(255, 230, 146, 90));
                painter.drawEllipse(particlePos + QPointF(-1.0, -1.0), radius * 0.45, radius * 0.45);
            }

            for (int rodIndex = 0; rodIndex < 8; ++rodIndex) {
                const qreal angle = rodSpin + rodIndex * (kPi / 4.0);
                const qreal orbitRadius = 17.0 + (rodIndex % 2) * 5.0;
                const QPointF rodCenter(qCos(angle) * orbitRadius,
                                        qSin(angle) * (orbitRadius * 0.8) - 6.0 + qSin(enemy.blazeFloatPhase + rodIndex) * 3.0);
                const QColor rodOuter = QColor(255, 208 + qRound(chargeRatio * 30.0), 64, 210);
                const QColor rodInner = QColor(255, 245, 170 + qRound(chargeRatio * 60.0), 235);
                painter.setBrush(rodOuter);
                painter.drawRoundedRect(QRectF(rodCenter.x() - 3.0, rodCenter.y() - 13.0, 6.0, 26.0), 2.0, 2.0);
                painter.setBrush(rodInner);
                painter.drawRoundedRect(QRectF(rodCenter.x() - 1.5, rodCenter.y() - 9.0, 3.0, 18.0), 1.5, 1.5);
            }

            painter.setBrush(QColor(30, 26, 26));
            painter.drawRect(QRectF(-8.0, -22.0, 16.0, 34.0));
            painter.setBrush(QColor(52, 46, 46));
            painter.drawRect(QRectF(-10.0, -8.0, 20.0, 28.0));
            painter.setBrush(QColor(255, 132 + qRound(chargeRatio * 60.0), 40, 180));
            painter.drawRect(QRectF(-6.0, -14.0, 12.0, 5.0));

            if (enemy.hurtTimer > 0.0) {
                painter.setBrush(QColor(255, 130, 130, 180));
                painter.drawRect(QRectF(-10.0, -22.0, 20.0, 42.0));
            }

            painter.setPen(QPen(QColor(255, 214, 96), 2));
            painter.drawPoint(QPointF(enemy.facingLeft ? -3.5 : 3.5, -7.0));
            painter.drawPoint(QPointF(enemy.facingLeft ? -0.5 : 6.5, -7.0));
            painter.restore();
            continue;
        }

        if (enemy.kind == EnemyKind::ZombifiedPiglin) {
            const bool angry = enemy.angerTimeRemaining > 0.0;
            const bool deathFalling = enemy.health <= 0;
            const qreal deathRatio = qBound(0.0, enemy.deathTimer / kEnemyDeathAnimSeconds, 1.0);
            const qreal hoverOffset = qSin(enemy.blazeFloatPhase * 0.5) * 1.5;
            const bool walking = qAbs(enemy.velocity.x()) > 1.0;
            const qreal legSwing = walking ? qSin(enemy.walkPhase) * 30.0 : 0.0;
            const qreal attackRatio = enemy.attackWindupTimer > 0.0
                                          ? qBound(0.0, 1.0 - enemy.attackWindupTimer / kPiglinAttackWindupSeconds, 1.0)
                                          : 0.0;
            const qreal swordArmAngle = enemy.attackWindupTimer > 0.0 ? -18.0 + 86.0 * attackRatio : 12.0;

            painter.save();
            if (deathFalling) {
                painter.translate(enemy.rect.center());
                painter.rotate(enemy.knockbackDirection * 90.0 * deathRatio);
                painter.translate(-enemy.rect.center());
            }

            painter.translate(enemy.rect.center().x(), enemy.rect.bottom());
            if (!enemy.facingLeft) {
                painter.scale(-1.0, 1.0);
            }
            painter.translate(-9.5, -62.0 + hoverOffset);

            painter.setPen(Qt::NoPen);
            if (angry) {
                painter.setBrush(QColor(255, 80, 80, 35));
                painter.drawEllipse(QRectF(-8.0, -8.0, 35.0, 78.0));
            }

            const QColor skinColor(234, 148, 148);
            const QColor earColor(255, 180, 180);
            const QColor tunicColor(212, 174, 55);
            const QColor pantsColor(180, 140, 40);
            const QColor bootColor(110, 75, 43);
            const QColor noseColor(255, 160, 160);

            const auto drawPiglinLeg = [&painter, &pantsColor, &bootColor](const QPointF &pivot, qreal angle) {
                painter.save();
                painter.translate(pivot);
                painter.rotate(angle);
                painter.setBrush(pantsColor);
                painter.drawRect(QRectF(-4.0, 0.0, 8.0, 20.0));
                painter.setBrush(bootColor);
                painter.drawRect(QRectF(-4.0, 14.0, 8.0, 6.0));
                painter.restore();
            };
            drawPiglinLeg(QPointF(5.0, 42.0), legSwing);
            drawPiglinLeg(QPointF(13.0, 42.0), -legSwing);
            painter.setBrush(tunicColor);
            painter.drawRect(QRectF(0.0, 14.0, 20.0, 28.0));

            painter.save();
            painter.translate(1.0, 22.0);
            painter.rotate(10.0);
            painter.setBrush(skinColor);
            painter.drawRect(QRectF(-11.0, -3.5, 28.0, 7.0));
            painter.restore();
            painter.save();
            painter.translate(3.0, 29.0);
            painter.rotate(swordArmAngle);
            painter.setBrush(skinColor);
            painter.drawRect(QRectF(-11.0, -3.5, 28.0, 7.0));
            painter.setBrush(QColor(212, 178, 64));
            painter.drawRect(QRectF(17.0, -2.0, 16.0, 4.0));
            painter.setBrush(QColor(236, 214, 120));
            painter.drawRect(QRectF(29.0, -4.0, 6.0, 8.0));
            painter.restore();

            painter.setBrush(skinColor);
            painter.drawRect(QRectF(-2.0, 0.0, 24.0, 24.0));
            painter.setBrush(earColor);
            painter.drawRect(QRectF(4.0, -4.0, 6.0, 8.0));
            painter.setBrush(noseColor);
            painter.drawRect(QRectF(-7.0, 13.0, 7.0, 6.0));
            painter.setBrush(QColor(24, 16, 16));
            painter.drawRect(QRectF(-5.0, 15.0, 1.0, 1.0));
            painter.setBrush(QColor(255, 0, 0));
            painter.drawRect(QRectF(3.0, 8.0, 3.0, 4.0));

            if (enemy.hurtTimer > 0.0 || deathFalling) {
                painter.setBrush(QColor(255, 140, 140, 150));
                painter.drawRect(QRectF(-4.0, -4.0, 40.0, 66.0));
            }
            painter.restore();
            continue;
        }

        if (enemy.kind == EnemyKind::FogStalker) {
            const qreal phaseHoverOffset = qSin(enemy.blazeFloatPhase * (2.0 / 3.0)) * 5.0;
            const qreal bodyAlpha = enemy.isPhantomClone ? enemy.visibility * 0.38 : enemy.visibility;
            const qreal eyeAlpha = enemy.isPhantomClone ? 0.65 : 1.0;
            const int phase = enemy.fogStalkerPhaseState;
            const qreal deathFade = enemy.health <= 0 ? qMax(0.0, 1.0 - enemy.deathTimer / kFogStalkerDeathFadeSeconds) : 1.0;
            const QColor bodyColor = enemy.fogStalkerStunTimer > 0.0
                                         ? QColor(245, 245, 255, qRound(220.0 * bodyAlpha * deathFade))
                                         : QColor(24, 24, 28, qRound(185.0 * bodyAlpha * deathFade));
            const QColor cloakColor = enemy.fogStalkerStunTimer > 0.0
                                          ? QColor(255, 255, 255, qRound(110.0 * bodyAlpha * deathFade))
                                          : QColor(36, 36, 44, qRound(92.0 * bodyAlpha * deathFade));
            const QColor eyeColor = enemy.health <= 0 ? QColor(255, 255, 255, 0)
                                                      : (enemy.fogStalkerStunTimer > 0.0
                                                             ? QColor(255, 255, 255, 255)
                                                             : (phase >= 3
                                                                    ? QColor(255, 72, 72, qRound(255.0 * eyeAlpha))
                                                                    : (phase == 2
                                                                           ? QColor(248, 236, 236, qRound(255.0 * eyeAlpha))
                                                                           : QColor(242, 246, 255, qRound(255.0 * eyeAlpha)))));
            const QColor eyeGlowColor = enemy.health <= 0 ? QColor(255, 255, 255, 0)
                                                          : (phase >= 3 ? QColor(255, 40, 40, 72)
                                                                        : (phase == 2 ? QColor(255, 92, 92, 42) : QColor(255, 255, 255, 24)));
            QRectF bodyRect = enemy.rect.translated(0.0, phaseHoverOffset);

            painter.save();
            if (enemy.fogStalkerAfterimageTimer > 0.0 || phase >= 3) {
                const qreal trailAlpha = enemy.fogStalkerAfterimageTimer > 0.0
                                             ? enemy.fogStalkerAfterimageTimer / kFogStalkerAfterimageSeconds
                                             : 0.22;
                painter.setBrush(QColor(22, 22, 26, qRound(90.0 * trailAlpha)));
                painter.drawRoundedRect(bodyRect.adjusted(-12.0, -4.0, -6.0, 8.0), 6.0, 6.0);
                painter.drawRoundedRect(bodyRect.adjusted(6.0, -2.0, 12.0, 10.0), 6.0, 6.0);
            }
            if (phase >= 2) {
                for (int mistIndex = 0; mistIndex < (phase >= 3 ? 5 : 3); ++mistIndex) {
                    const qreal mistPhase = enemy.blazeFloatPhase + mistIndex * 0.9;
                    const QPointF mistCenter(bodyRect.center().x() + qCos(mistPhase) * (10.0 + mistIndex * 2.0),
                                             bodyRect.center().y() + qSin(mistPhase * 1.4) * 18.0 - 8.0);
                    painter.setBrush(QColor(18, 18, 24, phase >= 3 ? 40 : 28));
                    painter.drawEllipse(mistCenter, 10.0 + mistIndex * 2.0, 14.0 + mistIndex * 1.5);
                }
            }
            painter.setBrush(cloakColor);
            painter.drawRoundedRect(bodyRect.adjusted(-4.0, -6.0, 4.0, 6.0), 6.0, 6.0);
            painter.setBrush(bodyColor);
            painter.drawRoundedRect(bodyRect, 5.0, 5.0);
            painter.setBrush(QColor(0, 0, 0, qRound(72.0 * bodyAlpha)));
            painter.drawRoundedRect(bodyRect.adjusted(4.0, bodyRect.height() * 0.52, -4.0, 0.0), 4.0, 4.0);
            painter.setPen(Qt::NoPen);
            painter.setBrush(eyeGlowColor);
            const qreal eyeY = bodyRect.top() + bodyRect.height() * 0.26;
            const qreal eyeOffset = bodyRect.width() * 0.2;
            const qreal faceCenterX = bodyRect.center().x() + (enemy.facingLeft ? -3.0 : 3.0);
            painter.drawEllipse(QPointF(faceCenterX - eyeOffset, eyeY), phase >= 3 ? 6.0 : 4.0, phase >= 3 ? 5.0 : 3.0);
            painter.drawEllipse(QPointF(faceCenterX + eyeOffset, eyeY), phase >= 3 ? 6.0 : 4.0, phase >= 3 ? 5.0 : 3.0);
            painter.setPen(QPen(eyeColor, enemy.isPhantomClone ? 2 : 3));
            painter.drawPoint(QPointF(faceCenterX - eyeOffset, eyeY));
            painter.drawPoint(QPointF(faceCenterX + eyeOffset, eyeY));
            painter.restore();
            continue;
        }

        if (enemy.kind == EnemyKind::Zombie) {
            const bool hurtTint = enemy.hurtTimer > 0.0;
            const bool deathFalling = enemy.health <= 0;
            const qreal deathRatio = qBound(0.0, enemy.deathTimer / kEnemyDeathAnimSeconds, 1.0);
            const qreal hoverOffset = qSin(enemy.blazeFloatPhase * 0.5) * 1.4;
            const bool walking = qAbs(enemy.velocity.x()) > 1.0;
            const qreal legSwing = walking ? qSin(enemy.walkPhase) * 30.0 : 0.0;
            const qreal attackRatio = enemy.attackWindupTimer > 0.0
                                          ? qBound(0.0, 1.0 - enemy.attackWindupTimer / kZombieAttackSwingSeconds, 1.0)
                                          : 0.0;
            const qreal armAngle = enemy.attackWindupTimer > 0.0 ? 8.0 + 76.0 * attackRatio : 10.0;

            painter.save();
            if (deathFalling) {
                painter.translate(enemy.rect.center());
                painter.rotate(enemy.knockbackDirection * 90.0 * deathRatio);
                painter.translate(-enemy.rect.center());
            }
            painter.translate(enemy.rect.center().x(), enemy.rect.bottom());
            if (!enemy.facingLeft) {
                painter.scale(-1.0, 1.0);
            }
            painter.translate(-9.5, -62.0 + hoverOffset);

            painter.setPen(Qt::NoPen);
            const auto drawZombieLeg = [&painter](const QPointF &pivot, qreal angle) {
                painter.save();
                painter.translate(pivot);
                painter.rotate(angle);
                painter.setBrush(QColor(52, 59, 84));
                painter.drawRect(QRectF(-4.0, 0.0, 8.0, 20.0));
                painter.setBrush(QColor(110, 75, 43));
                painter.drawRect(QRectF(-4.0, 14.0, 8.0, 6.0));
                painter.restore();
            };
            drawZombieLeg(QPointF(5.0, 42.0), legSwing);
            drawZombieLeg(QPointF(13.0, 42.0), -legSwing);
            painter.setBrush(QColor(61, 69, 84));
            painter.drawRect(QRectF(0.0, 14.0, 20.0, 28.0));

            painter.save();
            painter.translate(4.0, 22.0);
            painter.rotate(armAngle);
            painter.setBrush(QColor(88, 122, 88));
            painter.drawRect(QRectF(-14.0, -3.5, 32.0, 7.0));
            painter.restore();
            painter.save();
            painter.translate(4.0, 30.0);
            painter.rotate(armAngle);
            painter.setBrush(QColor(88, 122, 88));
            painter.drawRect(QRectF(-13.0, -3.5, 31.0, 7.0));
            painter.restore();

            painter.setBrush(QColor(88, 122, 88));
            painter.drawRect(QRectF(-2.0, 0.0, 24.0, 24.0));
            painter.setBrush(QColor(20, 20, 20));
            painter.drawRect(QRectF(3.0, 8.0, 3.0, 4.0));

            if (hurtTint || deathFalling) {
                painter.setBrush(QColor(255, 190, 190, 150));
                painter.drawRect(QRectF(-2.0, 0.0, 24.0, 24.0));
                painter.drawRect(QRectF(0.0, 14.0, 20.0, 28.0));
                painter.drawRect(QRectF(0.0, 42.0, 18.0, 20.0));
            }
            painter.restore();
            continue;
        }

        if (enemy.kind == EnemyKind::Skeleton) {
            const bool hurtTint = enemy.hurtTimer > 0.0;
            const bool deathFalling = enemy.health <= 0;
            const qreal deathRatio = qBound(0.0, enemy.deathTimer / kEnemyDeathAnimSeconds, 1.0);
            const qreal hoverOffset = qSin(enemy.blazeFloatPhase * 0.5) * 1.3;
            const bool walking = qAbs(enemy.velocity.x()) > 1.0;
            const qreal legSwing = walking ? qSin(enemy.walkPhase) * 30.0 : 0.0;
            const qreal bowCharge = enemy.attackWindupTimer > 0.0
                                        ? qBound(0.0, 1.0 - enemy.attackWindupTimer / kSkeletonBowChargeSeconds, 1.0)
                                        : 0.0;
            const qreal bowBob = walking ? qSin(enemy.walkPhase) * 2.0 : 0.0;

            painter.save();
            if (deathFalling) {
                painter.translate(enemy.rect.center());
                painter.rotate(enemy.knockbackDirection * 90.0 * deathRatio);
                painter.translate(-enemy.rect.center());
            }
            painter.translate(enemy.rect.center().x(), enemy.rect.bottom());
            if (!enemy.facingLeft) {
                painter.scale(-1.0, 1.0);
            }
            painter.translate(-9.5, -64.0 + hoverOffset);

            painter.setPen(Qt::NoPen);
            const auto drawSkeletonLeg = [&painter](const QPointF &pivot, qreal angle) {
                painter.save();
                painter.translate(pivot);
                painter.rotate(angle);
                painter.setBrush(QColor(238, 238, 238));
                painter.drawRect(QRectF(-2.0, 0.0, 4.0, 20.0));
                painter.restore();
            };
            drawSkeletonLeg(QPointF(5.0, 44.0), legSwing);
            drawSkeletonLeg(QPointF(13.0, 44.0), -legSwing);
            painter.drawRect(QRectF(2.0, 14.0, 16.0, 40.0));
            painter.setPen(QPen(QColor(221, 221, 221), 1));
            painter.drawLine(QPointF(4.0, 30.0), QPointF(16.0, 30.0));
            painter.drawLine(QPointF(4.0, 35.0), QPointF(16.0, 35.0));
            painter.setPen(Qt::NoPen);

            painter.save();
            painter.translate(1.0, 22.0 + bowBob * 0.5);
            painter.rotate(4.0);
            painter.setBrush(QColor(238, 238, 238));
            painter.drawRect(QRectF(-11.0, -2.0, 24.0, 4.0));
            painter.restore();
            painter.save();
            painter.translate(2.0 - bowCharge * 6.0, 28.0 + bowBob);
            painter.rotate(-8.0 - bowCharge * 18.0);
            painter.setBrush(QColor(238, 238, 238));
            painter.drawRect(QRectF(-10.0, -2.0, 23.0, 4.0));
            painter.setPen(QPen(QColor(92, 70, 44), 3));
            painter.drawArc(QRectF(-24.0, -14.0, 18.0, 34.0), 90 * 16, 180 * 16);
            painter.setPen(QPen(QColor(196, 196, 196), 1));
            painter.drawLine(QPointF(-15.0 + bowCharge * 6.0, -13.0), QPointF(-15.0 + bowCharge * 6.0, 19.0));
            painter.drawLine(QPointF(-10.0 + bowCharge * 3.0, -4.0), QPointF(-15.0 + bowCharge * 6.0, 2.0));
            if (enemy.attackWindupTimer > 0.0) {
                painter.setPen(QPen(QColor(214, 222, 230), 2));
                painter.drawLine(QPointF(-2.0, 0.0), QPointF(12.0, 0.0));
            }
            painter.setPen(Qt::NoPen);
            painter.restore();

            painter.setBrush(QColor(238, 238, 238));
            painter.drawRect(QRectF(-2.0, 0.0, 24.0, 24.0));
            painter.drawRect(QRectF(8.0, 22.0, 8.0, 2.0));
            painter.setBrush(QColor(0, 0, 0));
            painter.drawRect(QRectF(3.0, 8.0, 4.0, 4.0));

            if (hurtTint || deathFalling) {
                painter.setBrush(QColor(235, 235, 235, 135));
                painter.drawRect(QRectF(-2.0, 0.0, 24.0, 24.0));
                painter.drawRect(QRectF(2.0, 14.0, 16.0, 40.0));
                painter.drawRect(QRectF(3.0, 44.0, 12.0, 20.0));
            }
            painter.setPen(Qt::NoPen);
            painter.setBrush(Qt::NoBrush);
            painter.restore();
            continue;
        }


    }
}

void GameWidget::drawTorch(QPainter &painter, const QRect &rect)
{
    painter.fillRect(QRect(rect.center().x() - 2, rect.top() + 10, 4, rect.height() - 10), QColor(117, 78, 46));
    painter.fillRect(QRect(rect.center().x() - 3, rect.top() + 5, 6, 6), QColor(255, 176, 52));
    painter.fillRect(QRect(rect.center().x() - 2, rect.top() + 3, 4, 4), QColor(255, 231, 140));
}

void GameWidget::drawMiningCracks(QPainter &painter, const QRect &rect, double progress, const QColor &color) const
{
    const int stage = qBound(0, int(progress * 5.0), 4);
    if (stage <= 0) {
        return;
    }

    painter.save();
    painter.setPen(QPen(color, 2));
    painter.drawLine(rect.topLeft() + QPoint(8, 6), rect.center() + QPoint(0, 1));
    if (stage >= 2) {
        painter.drawLine(rect.topRight() + QPoint(-8, 7), rect.center() + QPoint(-2, 0));
    }
    if (stage >= 3) {
        painter.drawLine(rect.center() + QPoint(2, -1), rect.bottomLeft() + QPoint(10, -8));
    }
    if (stage >= 4) {
        painter.drawLine(rect.center() + QPoint(3, 2), rect.bottomRight() + QPoint(-9, -7));
        painter.drawLine(rect.left() + 12, rect.top() + 15, rect.right() - 11, rect.top() + 22);
    }
    painter.restore();
}
void GameWidget::drawPlayer(QPainter &painter)
{
    const bool hurtTint = m_player.hurtTimer > 0.0;
    const QRectF shadowRect(m_player.rect.left() + 6.0, m_player.rect.bottom() - 4.0, m_player.rect.width() - 12.0, 8.0);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 70));
    painter.drawEllipse(shadowRect);

    const bool walking = m_player.onGround && qAbs(m_player.velocity.x()) > 1.0;
    const bool airborne = !m_player.onGround && !isPlayerInFluid();
    const bool mining = m_miningState.active && m_selectedHotbarItem == HotbarItem::Pickaxe;
    const bool miningFinish = m_miningFinishTimer > 0.0 && m_selectedHotbarItem == HotbarItem::Pickaxe;
    const bool eating = m_player.eatTimer > 0.0 || m_pendingGoldenAppleUse;
    const qreal walkSwing = walking ? qSin(m_player.walkPhase) * 34.0 : 0.0;
    const qreal legSwing = walking ? qSin(m_player.walkPhase) * 30.0 : 0.0;
    const qreal jumpLift = airborne ? -24.0 : 0.0;
    const qreal mineWave = qSin(m_totalRunTime * 22.0) * 13.0;
    const qreal finishT = qBound(0.0, 1.0 - m_miningFinishTimer / kMiningFinishSwingSeconds, 1.0);
    const qreal eatT = eating ? qBound(0.0, 1.0 - m_player.eatTimer / kGoldenAppleEatSeconds, 1.0) : 0.0;
    const qreal chew = eating ? qSin(eatT * kPi * 8.0) : 0.0;

    qreal frontArmAngle = walkSwing + jumpLift;
    qreal backArmAngle = -walkSwing + jumpLift * 0.55;
    if (mining) {
        frontArmAngle = -72.0 + mineWave;
        backArmAngle = -58.0 + mineWave * 0.65;
    } else if (miningFinish) {
        frontArmAngle = -118.0 + 148.0 * finishT;
        backArmAngle = -96.0 + 120.0 * finishT;
    } else if (eating) {
        frontArmAngle = -112.0 + chew * 7.0;
        backArmAngle = -68.0 + chew * 5.0;
    }
    const qreal headDrop = eating ? 2.0 + qMax<qreal>(0.0, chew) * 1.4 : 0.0;

    painter.save();
    painter.translate(m_player.rect.center().x(), m_player.rect.bottom());
    if (m_player.facingLeft) {
        painter.scale(-1.0, 1.0);
    }
    painter.translate(-12.0, -52.0);
    painter.setPen(Qt::NoPen);

    const auto drawLimb = [&painter](const QPointF &pivot, qreal angle, const QSizeF &size, const QColor &color) {
        painter.save();
        painter.translate(pivot);
        painter.rotate(angle);
        painter.setBrush(color);
        painter.drawRect(QRectF(-size.width() / 2.0, 0.0, size.width(), size.height()));
        painter.restore();
    };

    drawLimb(QPointF(8.0, 34.0), legSwing, QSizeF(7.0, 18.0), QColor(43, 63, 94));
    drawLimb(QPointF(16.0, 34.0), -legSwing, QSizeF(7.0, 18.0), QColor(35, 55, 84));
    drawLimb(QPointF(18.0, 18.0), backArmAngle, QSizeF(7.0, 24.0), QColor(237, 211, 170));

    painter.setBrush(QColor(49, 98, 189));
    painter.drawRect(QRectF(5.0, 16.0, 14.0, 28.0));
    painter.setBrush(QColor(237, 211, 170));
    painter.drawRect(QRectF(4.0, 0.0 + headDrop, 18.0, 18.0));
    painter.setBrush(QColor(85, 51, 29));
    painter.drawRect(QRectF(3.0, -4.0 + headDrop, 19.0, 8.0));
    painter.setPen(QPen(Qt::black, 2));
    painter.drawPoint(QPointF(17.0, 8.0 + headDrop));
    painter.setPen(Qt::NoPen);

    drawLimb(QPointF(6.0, 18.0), frontArmAngle, QSizeF(7.0, 24.0), QColor(237, 211, 170));
    QPointF weaponAnchor;
    qreal weaponAngle = 0.0;
    {
        QTransform armTransform;
        armTransform.translate(6.0, 18.0);
        armTransform.rotate(frontArmAngle);
        weaponAnchor = armTransform.map(QPointF(0.0, 22.0));
        weaponAngle = frontArmAngle + 82.0;
    }
    if (m_selectedHotbarItem == HotbarItem::Bow) {
        const QPointF weaponWorldAnchor = m_player.rect.center()
                                          + QPointF(m_player.facingLeft ? -weaponAnchor.x() + 12.0 : weaponAnchor.x() - 12.0,
                                                    weaponAnchor.y() - 52.0);
        QPointF aimVector = m_cursorWorldPos - weaponWorldAnchor;
        if (m_player.facingLeft) {
            aimVector.setX(-aimVector.x());
        }
        if (std::hypot(aimVector.x(), aimVector.y()) > 1.0) {
            weaponAngle = qRadiansToDegrees(std::atan2(aimVector.y(), aimVector.x()));
        } else {
            weaponAngle = 0.0;
        }
    }
    drawWeapon(painter, weaponAnchor, weaponAngle, false);

    if (hurtTint) {
        painter.setBrush(QColor(255, 190, 190, 135));
        painter.drawRect(QRectF(3.0, -4.0, 20.0, 56.0));
    }
    painter.restore();
}

void GameWidget::drawWeapon(QPainter &painter, const QPointF &anchor, qreal angleDegrees, bool backArm)
{
    Q_UNUSED(backArm);

    if (m_selectedHotbarItem == HotbarItem::GoldenApple) {
        painter.save();
        painter.translate(anchor);
        painter.rotate(angleDegrees);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(48, 23, 80));
        painter.drawRect(QRectF(-10.0, -10.0, 20.0, 20.0));
        painter.setBrush(QColor(246, 207, 72));
        painter.drawRect(QRectF(-6.0, -6.0, 14.0, 14.0));
        painter.setBrush(QColor(255, 248, 168));
        painter.drawRect(QRectF(-3.0, -4.0, 6.0, 6.0));
        painter.setBrush(QColor(84, 34, 120));
        painter.drawRect(QRectF(-1.0, -13.0, 3.0, 5.0));
        painter.drawRect(QRectF(2.0, -10.0, 3.0, 3.0));
        painter.restore();
        return;
    }

    qreal finalAngleDegrees = angleDegrees;
    if (m_selectedHotbarItem == HotbarItem::Sword) {
        finalAngleDegrees = -45.0;
    }

    painter.save();
    painter.translate(anchor);
    painter.rotate(finalAngleDegrees);

    painter.setPen(Qt::NoPen);
    if (m_selectedHotbarItem == HotbarItem::Bow) {
        painter.fillRect(QRectF(-16.0, -3.0, 12.0, 6.0), QColor(115, 83, 29));
        painter.fillRect(QRectF(-4.0, -10.0, 8.0, 20.0), QColor(164, 123, 49));
        painter.fillRect(QRectF(4.0, -14.0, 12.0, 6.0), QColor(130, 94, 31));
        painter.fillRect(QRectF(4.0, 8.0, 12.0, 6.0), QColor(130, 94, 31));
        painter.fillRect(QRectF(16.0, -18.0, 12.0, 8.0), QColor(147, 108, 38));
        painter.fillRect(QRectF(16.0, 10.0, 12.0, 8.0), QColor(147, 108, 38));
        painter.fillRect(QRectF(28.0, -22.0, 11.0, 9.0), QColor(104, 73, 25));
        painter.fillRect(QRectF(28.0, 13.0, 11.0, 9.0), QColor(104, 73, 25));
        painter.fillRect(QRectF(36.0, -24.0, 8.0, 48.0), QColor(79, 57, 20));
        painter.fillRect(QRectF(40.0, -18.0, 6.0, 36.0), QColor(182, 139, 61));

        painter.setPen(QPen(QColor(210, 214, 220), 2));
        painter.drawLine(QPointF(34.0, -21.0), QPointF(9.0, -4.0));
        painter.drawLine(QPointF(34.0, 21.0), QPointF(9.0, 4.0));
    } else if (m_selectedHotbarItem == HotbarItem::Sword) {
        qreal swordRotation = -8.0;
        QPointF swordOffset(0.0, 0.0);
        qreal slashProgress = 0.0;
        if (m_swordSwingTimer > 0.0 && m_swordAttackType != SwordAttackType::None) {
            const double elapsed = kSwordSwingDurationSeconds - m_swordSwingTimer;
            if (m_swordAttackType == SwordAttackType::Sweep) {
                if (elapsed < kSwordPreSwingSeconds) {
                    const double t = elapsed / kSwordPreSwingSeconds;
                    swordRotation = 18.0 + (60.0 - 18.0) * t;
                    swordOffset = QPointF(-6.0, -3.0);
                } else if (elapsed < kSwordPreSwingSeconds + kSwordSwingPhaseSeconds) {
                    const double t = (elapsed - kSwordPreSwingSeconds) / kSwordSwingPhaseSeconds;
                    swordRotation = 60.0 + (-60.0 - 60.0) * t;
                    swordOffset = QPointF(3.0 + 6.0 * t, -4.0 + 2.0 * t);
                    slashProgress = t;
                } else {
                    const double t = (elapsed - kSwordPreSwingSeconds - kSwordSwingPhaseSeconds) / kSwordPostSwingSeconds;
                    swordRotation = -60.0 + (10.0 + 60.0) * t;
                    swordOffset = QPointF(9.0 - 4.0 * t, -2.0 + 4.0 * t);
                    slashProgress = 1.0;
                }
            } else {
                if (elapsed < kSwordPreSwingSeconds) {
                    const double t = elapsed / kSwordPreSwingSeconds;
                    swordRotation = -105.0 + (-150.0 + 105.0) * t;
                    swordOffset = QPointF(-4.0, -10.0 - 4.0 * t);
                } else if (elapsed < kSwordPreSwingSeconds + kSwordSwingPhaseSeconds) {
                    const double t = (elapsed - kSwordPreSwingSeconds) / kSwordSwingPhaseSeconds;
                    swordRotation = -150.0 + (12.0 + 150.0) * t;
                    swordOffset = QPointF(-4.0 + 10.0 * t, -14.0 + 18.0 * t);
                    slashProgress = t;
                } else {
                    const double t = (elapsed - kSwordPreSwingSeconds - kSwordSwingPhaseSeconds) / kSwordPostSwingSeconds;
                    swordRotation = 12.0 + (-8.0 - 12.0) * t;
                    swordOffset = QPointF(6.0 - 6.0 * t, 4.0 - 4.0 * t);
                    slashProgress = 1.0;
                }
            }
        }
        painter.translate(swordOffset);
        painter.rotate(swordRotation);
        painter.fillRect(QRectF(-10.0, -4.0, 16.0, 8.0), QColor(96, 64, 31));
        painter.fillRect(QRectF(6.0, -2.0, 8.0, 4.0), QColor(201, 169, 76));
        painter.fillRect(QRectF(14.0, -4.0, 10.0, 8.0), QColor(179, 182, 189));
        painter.fillRect(QRectF(24.0, -3.0, 18.0, 6.0), QColor(215, 218, 225));
        painter.fillRect(QRectF(42.0, -2.0, 10.0, 4.0), QColor(240, 242, 246));
        painter.setPen(QPen(QColor(255, 223, 102, 220), 2));
        painter.drawLine(QPointF(22.0, -2.0), QPointF(52.0, -2.0));
        painter.drawLine(QPointF(22.0, 2.0), QPointF(52.0, 2.0));
        if (m_swordSwingTimer > 0.0 && slashProgress > 0.0) {
            painter.save();
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(255, 255, 255, 80));
            if (m_swordAttackType == SwordAttackType::Sweep) {
                const qreal widthScale = 44.0 + slashProgress * 24.0;
                painter.drawRoundedRect(QRectF(26.0, -11.0, widthScale, 22.0), 9.0, 9.0);
                painter.setBrush(QColor(255, 250, 225, 115));
                painter.drawRoundedRect(QRectF(34.0, -6.0, 18.0 + slashProgress * 16.0, 12.0), 6.0, 6.0);
            } else {
                const qreal heightScale = 30.0 + slashProgress * 34.0;
                painter.drawRoundedRect(QRectF(28.0, -8.0, 18.0, heightScale), 8.0, 8.0);
                painter.setBrush(QColor(255, 250, 225, 115));
                painter.drawRoundedRect(QRectF(31.0, 2.0, 12.0, 14.0 + slashProgress * 16.0), 5.0, 5.0);
            }
            painter.restore();
        }
    } else if (m_selectedHotbarItem == HotbarItem::Pickaxe) {
        painter.fillRect(QRectF(-14.0, -3.0, 28.0, 6.0), QColor(126, 88, 42));
        painter.fillRect(QRectF(10.0, -14.0, 8.0, 28.0), QColor(126, 88, 42));
        painter.fillRect(QRectF(16.0, -17.0, 18.0, 8.0), QColor(164, 168, 176));
        painter.fillRect(QRectF(8.0, -24.0, 18.0, 8.0), QColor(188, 192, 198));
    } else if (m_selectedHotbarItem == HotbarItem::Dirt) {
        painter.fillRect(QRectF(-6.0, -6.0, 20.0, 20.0), QColor(120, 80, 54));
        painter.fillRect(QRectF(-6.0, -6.0, 20.0, 5.0), QColor(70, 170, 84));
    }

    painter.restore();
}

void GameWidget::drawHud(QPainter &painter)
{
    const auto drawHeart = [&painter](const QPoint &topLeft, int fillState, bool golden) {
        static const QVector<QPoint> heartPixels = {
            {1, 0}, {2, 0}, {5, 0}, {6, 0},
            {0, 1}, {1, 1}, {2, 1}, {3, 1}, {4, 1}, {5, 1}, {6, 1}, {7, 1},
            {0, 2}, {1, 2}, {2, 2}, {3, 2}, {4, 2}, {5, 2}, {6, 2}, {7, 2},
            {0, 3}, {1, 3}, {2, 3}, {3, 3}, {4, 3}, {5, 3}, {6, 3}, {7, 3},
            {1, 4}, {2, 4}, {3, 4}, {4, 4}, {5, 4}, {6, 4},
            {2, 5}, {3, 5}, {4, 5}, {5, 5},
            {3, 6}, {4, 6}
        };

        const QColor borderColor = (fillState == 0)
                                       ? QColor(92, 92, 92)
                                       : (golden ? QColor(99, 72, 15) : QColor(63, 22, 22));
        const QColor emptyColor(42, 42, 42);
        const QColor fillColor = golden ? QColor(232, 184, 44) : QColor(214, 52, 52);
        const QColor highlightColor = golden ? QColor(255, 223, 120) : QColor(255, 115, 115);
        const int pixelSize = 3;

        for (const QPoint &pixel : heartPixels) {
            bool isBorder = false;
            const QPoint neighbors[] = {
                QPoint(pixel.x() - 1, pixel.y()),
                QPoint(pixel.x() + 1, pixel.y()),
                QPoint(pixel.x(), pixel.y() - 1),
                QPoint(pixel.x(), pixel.y() + 1)
            };

            for (const QPoint &neighbor : neighbors) {
                if (!heartPixels.contains(neighbor)) {
                    isBorder = true;
                    break;
                }
            }

            QColor color = borderColor;
            if (!isBorder) {
                color = emptyColor;
                if (fillState == 2) {
                    color = pixel.y() <= 1 ? highlightColor : fillColor;
                } else if (fillState == 1 && pixel.x() <= 3) {
                    color = pixel.y() <= 1 ? highlightColor : fillColor;
                }
            }

            painter.fillRect(QRect(topLeft.x() + pixel.x() * pixelSize,
                                   topLeft.y() + pixel.y() * pixelSize,
                                   pixelSize,
                                   pixelSize),
                             color);
        }
    };

    const int filledHearts = m_playerHealth / kHeartHitPoints;
    const bool hasHalfHeart = (m_playerHealth % kHeartHitPoints) != 0;
    const QPoint heartStart(18, 18);
    const int heartSpacing = 28;

    for (int index = 0; index < kHeartCount; ++index) {
        int fillState = 0;
        if (index < filledHearts) {
            fillState = 2;
        } else if (index == filledHearts && hasHalfHeart) {
            fillState = 1;
        }

        drawHeart(QPoint(heartStart.x() + index * heartSpacing, heartStart.y()), fillState, false);
    }

    const int filledGoldenHearts = m_goldenHealth / kHeartHitPoints;
    const bool hasGoldenHalfHeart = (m_goldenHealth % kHeartHitPoints) != 0;
    const QPoint goldenHeartStart(18, 46);

    for (int index = 0; index < kGoldenHeartCount; ++index) {
        int fillState = 0;
        if (index < filledGoldenHearts) {
            fillState = 2;
        } else if (index == filledGoldenHearts && hasGoldenHalfHeart) {
            fillState = 1;
        }

        drawHeart(QPoint(goldenHeartStart.x() + index * heartSpacing, goldenHeartStart.y()), fillState, true);
    }

    const QRect firstSlotRect = hotbarSlotRect(0);
    const QRect lastSlotRect = hotbarSlotRect(kHotbarSlotCount - 1);
    const QRect centerSlotRect = hotbarSlotRect(kHotbarSlotCount / 2);
    const int experienceBarWidth = lastSlotRect.right() - firstSlotRect.left() + 1;
    const int experienceBarHeight = 14;
    const QRect experienceRect(firstSlotRect.left(),
                               centerSlotRect.top() - experienceBarHeight - 8,
                               experienceBarWidth,
                               experienceBarHeight);
    const QRect fillRect = experienceRect.adjusted(2, 2, -2, -2);
    const int nextLevelExperience = experienceToNextLevel();
    const double experienceRatio = nextLevelExperience > 0
                                       ? qBound(0.0, double(m_levelExperience) / double(nextLevelExperience), 1.0)
                                       : 0.0;
    const int filledWidth = qRound(fillRect.width() * experienceRatio);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(52, 52, 56));
    painter.drawRect(experienceRect);
    painter.setBrush(QColor(102, 74, 41));
    painter.drawRect(fillRect);
    if (filledWidth > 0) {
        painter.setBrush(QColor(146, 224, 54));
        painter.drawRect(QRect(fillRect.left(), fillRect.top(), filledWidth, fillRect.height()));
        painter.setBrush(QColor(194, 244, 92));
        painter.drawRect(QRect(fillRect.left(), fillRect.top(), filledWidth, qMax(2, fillRect.height() / 3)));
    }
    if (m_playerLevel >= 10) {
        painter.setPen(QPen(QColor(52, 52, 56), 1));
        for (int i = 1; i < 10; ++i) {
            const int x = fillRect.left() + (fillRect.width() * i) / 10;
            painter.drawLine(x, fillRect.top(), x, fillRect.bottom());
        }
        painter.setPen(Qt::NoPen);
    }

    QFont levelFont = painter.font();
    levelFont.setPointSize(16);
    levelFont.setBold(true);
    levelFont.setWeight(QFont::Black);
    levelFont.setStretch(QFont::Expanded);
    painter.setFont(levelFont);
    const QString levelText = QString::number(m_playerLevel);
    const QRect levelRect(experienceRect.left() - 24,
                          experienceRect.center().y() - 13,
                          experienceRect.width() + 48,
                          26);
    painter.setPen(QColor(0, 0, 0));
    for (const QPoint &offset : { QPoint(-1, 0), QPoint(1, 0), QPoint(0, -1), QPoint(0, 1) }) {
        painter.drawText(levelRect.translated(offset), Qt::AlignCenter, levelText);
    }
    painter.setPen(Qt::white);
    painter.drawText(levelRect, Qt::AlignCenter, levelText);

    QFont hudFont = painter.font();
    hudFont.setPointSize(10);
    hudFont.setBold(true);
    painter.setFont(hudFont);
    const QFont labelFont(hudFont.family(), 10, QFont::Bold);
    painter.setFont(labelFont);

    for (int index = 0; index < kHotbarSlotCount; ++index) {
        const HotbarItem item = static_cast<HotbarItem>(index);
        const QRect slotRect = hotbarSlotRect(index);
        const bool selected = item == m_selectedHotbarItem;

        painter.setPen(selected ? QColor(255, 227, 133) : QColor(40, 28, 20));
        painter.setBrush(selected ? QColor(112, 82, 52, 240) : QColor(68, 48, 34, 220));
        painter.drawRoundedRect(slotRect, 8, 8);

        const QRect iconRect = slotRect.adjusted(12, 8, -12, -24);
        painter.setPen(Qt::NoPen);

        switch (item) {
        case HotbarItem::Dirt:
            painter.setBrush(QColor(120, 80, 54));
            painter.drawRect(iconRect.adjusted(12, 2, -12, -6));
            painter.setBrush(QColor(70, 170, 84));
            painter.drawRect(QRect(iconRect.left() + 12, iconRect.top() + 2, iconRect.width() - 24, 8));
            break;
        case HotbarItem::Bow:
            painter.setBrush(QColor(150, 109, 43));
            painter.drawRect(QRect(iconRect.left() + 8, iconRect.top() + 4, 12, iconRect.height() - 8));
            painter.drawRect(QRect(iconRect.left() + 24, iconRect.top() + 1, 10, iconRect.height() - 2));
            painter.setPen(QPen(QColor(215, 220, 224), 2));
            painter.drawLine(iconRect.left() + 30, iconRect.top() + 4, iconRect.left() + 18, iconRect.center().y());
            painter.drawLine(iconRect.left() + 30, iconRect.bottom() - 4, iconRect.left() + 18, iconRect.center().y());
            break;
        case HotbarItem::Sword:
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(196, 199, 206));
            painter.drawRect(QRect(iconRect.left() + 18, iconRect.top() + 8, 24, 6));
            painter.drawRect(QRect(iconRect.left() + 36, iconRect.top() + 5, 10, 12));
            painter.setBrush(QColor(117, 81, 38));
            painter.drawRect(QRect(iconRect.left() + 8, iconRect.top() + 9, 12, 4));
            break;
        case HotbarItem::Pickaxe:
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(126, 88, 42));
            painter.drawRect(QRect(iconRect.left() + 10, iconRect.top() + 16, 30, 5));
            painter.drawRect(QRect(iconRect.left() + 28, iconRect.top() + 6, 5, 24));
            painter.setBrush(QColor(182, 186, 193));
            painter.drawRect(QRect(iconRect.left() + 24, iconRect.top(), 20, 7));
            painter.drawRect(QRect(iconRect.left() + 14, iconRect.top() + 6, 18, 7));
            break;
        case HotbarItem::GoldenApple:
            painter.setBrush(QColor(48, 23, 80));
            painter.drawRect(QRect(iconRect.left() + 18, iconRect.top() + 6, 20, 20));
            painter.setBrush(QColor(246, 207, 72));
            painter.drawRect(QRect(iconRect.left() + 21, iconRect.top() + 9, 14, 14));
            painter.setBrush(QColor(255, 248, 168));
            painter.drawRect(QRect(iconRect.left() + 24, iconRect.top() + 11, 6, 6));
            painter.setBrush(QColor(84, 34, 120));
            painter.drawRect(QRect(iconRect.left() + 28, iconRect.top() + 2, 3, 5));
            painter.drawRect(QRect(iconRect.left() + 31, iconRect.top() + 5, 3, 3));
            break;
        }

        painter.setPen(Qt::white);
        painter.drawText(slotRect.adjusted(0, 0, 0, -4),
                         Qt::AlignHCenter | Qt::AlignBottom,
                         hotbarLabel(item));
        painter.drawText(QRect(slotRect.left() + 6, slotRect.top() + 4, 18, 18),
                         Qt::AlignCenter,
                         QString::number(index + 1));
    }

    const QRect layerPanel(width() - 250, 18, 232, 74);
    painter.setPen(QColor(28, 20, 16, 220));
    painter.setBrush(QColor(45, 33, 26, 185));
    painter.drawRoundedRect(layerPanel, 10, 10);

    QFont layerTitleFont = painter.font();
    layerTitleFont.setPointSize(12);
    layerTitleFont.setBold(true);
    painter.setFont(layerTitleFont);
    painter.setPen(Qt::white);
    painter.drawText(layerPanel.adjusted(14, 8, -14, -36), Qt::AlignLeft | Qt::AlignVCenter,
                     depthLayerTitle(m_currentDepthLayer));

    QFont layerInfoFont = painter.font();
    layerInfoFont.setPointSize(9);
    layerInfoFont.setBold(false);
    painter.setFont(layerInfoFont);
    const int playerCol = qFloor(m_player.rect.center().x() / kTileSize);
    const int playerRow = playerTileRow();
    const QString infoText = QStringLiteral("X: %1  Y: %2  |  Fog: %3  |  Mobs: %4")
                                 .arg(playerCol)
                                 .arg(playerRow)
                                 .arg(m_screenFog.alpha())
                                 .arg(m_currentMonsterProfile);
    painter.drawText(layerPanel.adjusted(14, 34, -14, -10), Qt::AlignLeft | Qt::AlignVCenter, infoText);

    const QRect killPanel(width() - 250, 100, 232, 52);
    painter.setPen(QColor(28, 20, 16, 220));
    painter.setBrush(QColor(45, 33, 26, 185));
    painter.drawRoundedRect(killPanel, 10, 10);
    if (m_currentDepthLayer == DepthLayer::ShallowCavern) {
        painter.setFont(layerTitleFont);
        painter.setPen(Qt::white);
        painter.drawText(killPanel.adjusted(14, 4, -14, -28), Qt::AlignLeft | Qt::AlignVCenter,
                         QStringLiteral("Nether Tasks"));
        painter.setFont(layerInfoFont);
        painter.drawText(QRect(killPanel.left() + 14, killPanel.top() + 22, killPanel.width() - 28, 14),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         QStringLiteral("Blaze: %1/5").arg(m_blazeDefeatedCount));
        painter.drawText(QRect(killPanel.left() + 14, killPanel.top() + 36, killPanel.width() - 28, 14),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         QStringLiteral("Pigman: %1/15").arg(m_zombiePigmanDefeatedCount));
    } else {
        painter.setFont(layerTitleFont);
        painter.setPen(Qt::white);
        painter.drawText(killPanel.adjusted(14, 6, -14, -24), Qt::AlignLeft | Qt::AlignVCenter,
                         QStringLiteral("Defeated"));
        painter.setFont(layerInfoFont);
        painter.drawText(killPanel.adjusted(14, 22, -14, -6), Qt::AlignLeft | Qt::AlignVCenter,
                         QStringLiteral("%1/%2").arg(m_defeatedEnemyCount).arg(kDefeatTargetCount));
    }
    if (m_layerHintTimer > 0.0 && !m_currentLayerHint.isEmpty()) {
        const QRect hintRect(width() / 2 - 210, 26, 420, 56);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(12, 12, 18, 210));
        painter.drawRoundedRect(hintRect, 12, 12);
        QFont hintFont = painter.font();
        hintFont.setPointSize(14);
        hintFont.setBold(true);
        painter.setFont(hintFont);
        painter.setPen(QColor(255, 243, 196));
        painter.drawText(hintRect, Qt::AlignCenter, m_currentLayerHint);
    }

    const Enemy *fogStalker = nullptr;
    for (const Enemy &enemy : std::as_const(m_enemies)) {
        if (enemy.kind == EnemyKind::FogStalker && !enemy.isPhantomClone) {
            fogStalker = &enemy;
            break;
        }
    }

    if (m_currentDepthLayer == DepthLayer::DeepCavern && !m_deepLayerBossDefeated) {
        const QRect bossPanel(18, 86, 300, 42);
        const QRect bossFill = bossPanel.adjusted(86, 12, -12, -12);
        const qreal bossRatio = fogStalker != nullptr
                                    ? qBound(0.0, qreal(fogStalker->health) / qreal(m_fogStalkerMaxHealth), 1.0)
                                    : 0.0;
        painter.setPen(QColor(18, 18, 22, 220));
        painter.setBrush(QColor(20, 20, 28, 190));
        painter.drawRoundedRect(bossPanel, 10, 10);
        painter.setBrush(QColor(54, 22, 22));
        painter.drawRoundedRect(bossFill, 6, 6);
        if (fogStalker != nullptr) {
            painter.setBrush(QColor(204, 48, 48));
            painter.drawRoundedRect(QRectF(bossFill.left(), bossFill.top(), bossFill.width() * bossRatio, bossFill.height()), 6, 6);
        }
        painter.setPen(Qt::white);
        painter.drawText(QRect(bossPanel.left() + 14, bossPanel.top() + 9, 86, 24),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         QStringLiteral("mist hunter"));
        painter.drawText(QRect(bossFill.left(), bossPanel.top() + 8, bossFill.width(), 24),
                         Qt::AlignCenter,
                         fogStalker != nullptr
                             ? QStringLiteral("mist hunter %1%").arg(qRound(bossRatio * 100.0))
                             : QStringLiteral("mist hunter --"));
    }

    if (m_victoryBannerTimer > 0.0 || m_victorySummaryTimer > 0.0) {
        const QRect bannerRect(width() / 2 - 280, height() / 2 - 90, 560, 180);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(10, 10, 16, 220));
        painter.drawRoundedRect(bannerRect, 16, 16);
        QFont titleFont = painter.font();
        titleFont.setPointSize(22);
        titleFont.setBold(true);
        painter.setFont(titleFont);
        painter.setPen(QColor(255, 221, 112));
        painter.drawText(bannerRect.adjusted(20, 20, -20, -92), Qt::AlignCenter,
                         QStringLiteral("Victory"));

        QFont subtitleFont = painter.font();
        subtitleFont.setPointSize(11);
        subtitleFont.setBold(false);
        painter.setFont(subtitleFont);
        painter.setPen(QColor(235, 235, 235));
        painter.drawText(bannerRect.adjusted(20, 78, -20, -54), Qt::AlignCenter,
                         QStringLiteral("You defeated mist hunter and cleared the deep fog"));

        if (m_victorySummaryTimer <= 0.0) {
            const QRect summaryRect = bannerRect.adjusted(54, 112, -54, -20);
            painter.setPen(QColor(40, 40, 48));
            painter.setBrush(QColor(24, 24, 32, 210));
            painter.drawRoundedRect(summaryRect, 12, 12);
            painter.setFont(subtitleFont);
            painter.setPen(Qt::white);
            const int totalKills = m_defeatedEnemyCount + m_blazeDefeatedCount + m_zombiePigmanDefeatedCount;
            const int totalMinutes = qFloor(m_totalRunTime / 60.0);
            const int totalSeconds = qFloor(m_totalRunTime) % 60;
            painter.drawText(summaryRect.adjusted(16, 12, -16, -30), Qt::AlignLeft | Qt::AlignTop,
                             QStringLiteral("Total kills: %1").arg(totalKills));
            painter.drawText(summaryRect.adjusted(16, 34, -16, -8), Qt::AlignLeft | Qt::AlignTop,
                             QStringLiteral("Clear time: %1:%2")
                                 .arg(totalMinutes, 2, 10, QChar('0'))
                                 .arg(totalSeconds, 2, 10, QChar('0')));
        }
    }
}

QRect GameWidget::endPrimaryButtonRect() const
{
    const int buttonWidth = qMin(320, width() / 3);
    const int buttonHeight = 68;
    const int centerX = width() / 2;
    const int buttonY = height() / 2 + 92;
    if (m_endState == GameEndState::Failed) {
        return QRect(centerX - buttonWidth - 18, buttonY, buttonWidth, buttonHeight);
    }
    return QRect(centerX - buttonWidth / 2, buttonY + 12, buttonWidth, buttonHeight);
}

QRect GameWidget::endSecondaryButtonRect() const
{
    if (m_endState != GameEndState::Failed) {
        return QRect();
    }
    const int buttonWidth = qMin(320, width() / 3);
    const int buttonHeight = 68;
    const int centerX = width() / 2;
    const int buttonY = height() / 2 + 92;
    return QRect(centerX + 18, buttonY, buttonWidth, buttonHeight);
}

void GameWidget::restartGame()
{
    for (QMediaPlayer *player : std::as_const(m_soundPlayers)) {
        if (player != nullptr) {
            player->stop();
            player->deleteLater();
        }
    }
    m_soundPlayers.clear();
    m_monsterSoundPlayers.clear();

    m_projectiles.clear();
    m_enemies.clear();
    m_lootFx.clear();
    m_pressedKeys.clear();
    m_discoveredLayerIds.clear();
    m_portals.clear();
    m_purpleTiles.clear();
    m_purpleBeamTiles.clear();
    m_netherPortalTiles.clear();
    m_netherPortalBeamTiles.clear();
    m_blackPortalTiles.clear();
    m_blackPortalBeamTiles.clear();
    m_magmaTiles.clear();

    m_player = Player{};
    m_miningState = MiningState{};
    m_cursorWorldPos = QPointF();
    m_selectedHotbarItem = HotbarItem::Bow;
    m_playerHealth = 100;
    m_playerMaxHealth = 100;
    m_playerLevel = 1;
    m_levelExperience = 0;
    m_defeatedEnemyCount = 0;
    m_blazeDefeatedCount = 0;
    m_zombiePigmanDefeatedCount = 0;
    m_goldenHealth = 0;
    m_fogStalkerMaxHealth = 300;
    m_swordSwingTimer = 0.0;
    m_swordAttackCooldown = 0.0;
    m_shootCooldown = 0.0;
    m_miningFinishTimer = 0.0;
    m_cameraX = 0.0;
    m_cameraY = 0.0;
    m_airbornePeakY = 0.0;
    m_totalRunTime = 0.0;
    m_goldenEffectTimeRemaining = 0.0;
    m_goldenRegenTickAccumulator = 0.0;
    m_layerHintTimer = 0.0;
    m_portalCooldown = 0.0;
    m_layerGraceTimer = 0.0;
    m_fearDebuffTimer = 0.0;
    m_fearFogBoost = 0.0;
    m_phaseFlashTimer = 0.0;
    m_playerOutlineFlashTimer = 0.0;
    m_fogClearTimer = 0.0;
    m_deepLayerSpawnTimer = -1.0;
    m_victoryBannerTimer = 0.0;
    m_victorySummaryTimer = 0.0;
    m_fearDebuffStacks = 0;
    m_playerFacingSameDirectionTimer = 0.0;
    m_stepSoundTimer = 0.0;
    m_zombieAmbientTimer = 0.0;
    m_piglinAmbientTimer = 0.0;
    m_lastPlayerFacingLeft = false;
    m_goldenRegenActive = false;
    m_leftMousePressed = false;
    m_swordAttackHitApplied = false;
    m_swordAttackFacingLeft = false;
    m_pendingGoldenAppleUse = false;
    m_deepLayerBossSpawned = false;
    m_deepLayerBossDefeated = false;
    m_deepLayerEntryRegistered = false;
    m_controlsLocked = false;
    m_screenFog = QColor(235, 245, 255, 18);
    m_globalBrightness = 1.0;
    m_currentDepthLayer = DepthLayer::Surface;
    m_currentLayerHint.clear();
    m_currentMonsterProfile = QStringLiteral("Normal mobs");
    m_netherPortalActive = false;
    m_netherPortalOrigin = QPoint();
    m_netherPortalLit = false;
    m_blackPortalOrigin = QPoint();
    m_blackPortalActive = true;
    m_swordAttackType = SwordAttackType::None;
    m_inPurpleBeam = false;
    m_gameWon = false;
    m_endState = GameEndState::None;
    m_deepLayerSpawnAnchor = QPointF();

    initializeWorld();
    generateOreVeins();
    generateWaterPools();
    generateCavernLights();
    generateTrees();
    generateNetherBlazes();
    generateZombifiedPiglins();

    for (int col = 44; col <= 56; ++col) {
        for (int row = 43; row <= 50; ++row) {
            m_waterTiles.remove(tileKey(col, row));
        }
    }

    m_player.rect.moveTo(8.0 * kTileSize, (kSurfaceRow - 2) * kTileSize - m_player.rect.height());
    m_cursorWorldPos = m_player.rect.center() + QPointF(80.0, -18.0);
    m_airbornePeakY = m_player.rect.top();
    startLayerGracePeriod();
    applyPurpleLandingZone(QPoint(qFloor(m_player.rect.right() / kTileSize),
                                  qFloor(m_player.rect.bottom() / kTileSize) - 1),
                           false);
    updateDepthLayer(0.0);
    m_frameClock.restart();
    setFocus();
    update();
}

void GameWidget::drawEndScreen(QPainter &painter)
{
    if (m_endState == GameEndState::None) {
        return;
    }

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, false);

    const QPixmap &background = m_endState == GameEndState::Failed ? m_failScreenBg : m_successScreenBg;
    if (!background.isNull()) {
        painter.drawPixmap(rect(), background);
    } else {
        painter.fillRect(rect(), QColor(18, 18, 18));
    }

    painter.fillRect(rect(), QColor(0, 0, 0, 90));

    QFont titleFont(QStringLiteral("Minecraft AE"));
    titleFont.setBold(true);
    titleFont.setPixelSize(qMax(38, width() / 18));
    painter.setFont(titleFont);
    painter.setPen(Qt::white);
    const QString failTitle = utf8Hex("E68C91E68898E5A4B1E8B4A5");
    const QString successTitle = utf8Hex("E68C91E68898E68890E58A9F");
    painter.drawText(QRect(0, height() / 5, width(), 90), Qt::AlignCenter,
                     m_endState == GameEndState::Failed ? failTitle : successTitle);

    const auto drawPixelButton = [&painter](const QRect &buttonRect, const QString &text) {
        painter.fillRect(buttonRect.adjusted(4, 4, 4, 4), QColor(0, 0, 0, 120));
        painter.fillRect(buttonRect, QColor(6, 6, 6, 235));
        QPen border(QColor(90, 255, 245));
        border.setWidth(4);
        painter.setPen(border);
        painter.drawRect(buttonRect.adjusted(2, 2, -2, -2));
        QFont buttonFont(QStringLiteral("Minecraft AE"));
        buttonFont.setBold(true);
        buttonFont.setPixelSize(26);
        painter.setFont(buttonFont);
        painter.setPen(Qt::white);
        painter.drawText(buttonRect, Qt::AlignCenter, text);
    };

    const QString restartText = utf8Hex("E9878DE696B0E5BC80E5A78B");
    const QString quitText = utf8Hex("E98080E587BAE6B8B8E6888F");
    drawPixelButton(endPrimaryButtonRect(), m_endState == GameEndState::Failed ? restartText : quitText);
    if (m_endState == GameEndState::Failed) {
        drawPixelButton(endSecondaryButtonRect(), quitText);
    }

    painter.restore();
}
QRect GameWidget::hotbarSlotRect(int index) const
{
    const int slotWidth = 88;
    const int slotHeight = 64;
    const int gap = 10;
    const int totalWidth = kHotbarSlotCount * slotWidth + (kHotbarSlotCount - 1) * gap;
    const int startX = (width() - totalWidth) / 2;
    const int startY = height() - slotHeight - 18;
    return QRect(startX + index * (slotWidth + gap), startY, slotWidth, slotHeight);
}

QRect GameWidget::blazeCounterRect() const
{
    return QRect(width() - 236, 114, 92, 18);
}

QString GameWidget::hotbarLabel(HotbarItem item) const
{
    switch (item) {
    case HotbarItem::Dirt:
        return QStringLiteral("Dirt");
    case HotbarItem::Bow:
        return QStringLiteral("Bow");
    case HotbarItem::Sword:
        return QStringLiteral("Sword");
    case HotbarItem::Pickaxe:
        return QStringLiteral("Pickaxe");
    case HotbarItem::GoldenApple:
        return QStringLiteral("Apple");
    }

    return QString();
}

bool GameWidget::isSolidTile(int column, int row) const
{
    if (column < 0 || column >= kWorldColumns || row < 0 || row >= kWorldRows) {
        return false;
    }
    if (m_blackPortalTiles.contains(tileKey(column, row)) || m_blackPortalBeamTiles.contains(tileKey(column, row)))
        return false;

    if (m_defeatedEnemyCount >= kDefeatTargetCount && m_purpleTiles.contains(tileKey(column, row))) {
        return false;
    }

    if (row >= kWorldRows - kBedrockThickness) {
        return true;
    }

    return m_dirtBlocks.contains(tileKey(column, row))
           || m_oreBlocks.contains(tileKey(column, row));
}

bool GameWidget::isWaterTile(int column, int row) const
{
    if (column < 0 || column >= kWorldColumns || row < 0 || row >= kWorldRows) {
        return false;
    }

    return m_waterTiles.contains(tileKey(column, row));
}

bool GameWidget::isLavaTile(int column, int row) const
{
    if (column < 0 || column >= kWorldColumns || row < 0 || row >= kWorldRows) {
        return false;
    }

    return m_lavaTiles.contains(tileKey(column, row));
}

bool GameWidget::isPlayerInWater() const
{
    const int minColumn = qFloor(m_player.rect.left() / kTileSize);
    const int maxColumn = qFloor((m_player.rect.right() - 1.0) / kTileSize);
    const int minRow = qFloor((m_player.rect.top() + m_player.rect.height() * 0.35) / kTileSize);
    const int maxRow = qFloor((m_player.rect.bottom() - 1.0) / kTileSize);

    for (int row = minRow; row <= maxRow; ++row) {
        for (int column = minColumn; column <= maxColumn; ++column) {
            if (isWaterTile(column, row)) {
                return true;
            }
        }
    }

    return false;
}

bool GameWidget::isPlayerInLava() const
{
    const int minColumn = qFloor(m_player.rect.left() / kTileSize);
    const int maxColumn = qFloor((m_player.rect.right() - 1.0) / kTileSize);
    const int minRow = qFloor((m_player.rect.top() + m_player.rect.height() * 0.35) / kTileSize);
    const int maxRow = qFloor((m_player.rect.bottom() - 1.0) / kTileSize);

    for (int row = minRow; row <= maxRow; ++row) {
        for (int column = minColumn; column <= maxColumn; ++column) {
            if (isLavaTile(column, row)) {
                return true;
            }
        }
    }

    return false;
}

bool GameWidget::isPlayerInFluid() const
{
    return isPlayerInWater() || isPlayerInLava();
}

bool GameWidget::rectIntersectsLava(const QRectF &rect) const
{
    const int minColumn = qFloor(rect.left() / kTileSize);
    const int maxColumn = qFloor((rect.right() - 1.0) / kTileSize);
    const int minRow = qFloor((rect.top() + rect.height() * 0.2) / kTileSize);
    const int maxRow = qFloor((rect.bottom() - 1.0) / kTileSize);

    for (int row = minRow; row <= maxRow; ++row) {
        for (int column = minColumn; column <= maxColumn; ++column) {
            if (isLavaTile(column, row)) {
                return true;
            }
        }
    }

    return false;
}

QRect GameWidget::tileRect(int column, int row) const
{
    return QRect(column * kTileSize, row * kTileSize, kTileSize, kTileSize);
}

QPoint GameWidget::screenToTile(const QPoint &screenPos) const
{
    const QPointF worldPos = QPointF(screenPos) + QPointF(m_cameraX, m_cameraY);
    return QPoint(qFloor(worldPos.x() / kTileSize), qFloor(worldPos.y() / kTileSize));
}

bool GameWidget::isTileReachable(const QPoint &tile) const
{
    const QPointF tileCenter(tile.x() + 0.5, tile.y() + 0.5);
    const QPointF playerTileCenter(m_player.rect.center().x() / kTileSize,
                                   m_player.rect.center().y() / kTileSize);
    const QPointF delta = tileCenter - playerTileCenter;
    return std::hypot(delta.x(), delta.y()) <= kDigRangeTiles;
}

bool GameWidget::isOccupiedTile(int column, int row) const
{
    if (column < 0 || column >= kWorldColumns || row < 0 || row >= kWorldRows)
        return false;
    int key = tileKey(column, row);
    return m_dirtBlocks.contains(key)
           || m_oreBlocks.contains(key)
           || m_treeBlocks.contains(key);
}

void GameWidget::drawBlockEdges(QPainter &painter, const QRect &rect, int column, int row) const
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, false);
    QPen brightPen(QColor(255, 255, 255, 35), 1);
    QPen darkPen(QColor(0, 0, 0, 70), 1);


    if (!isOccupiedTile(column, row - 1)) {
        painter.setPen(brightPen);
        painter.drawLine(rect.topLeft(), rect.topRight());
    }

    if (!isOccupiedTile(column - 1, row)) {
        painter.setPen(brightPen);
        painter.drawLine(rect.topLeft(), rect.bottomLeft());
    }

    if (!isOccupiedTile(column, row + 1)) {
        painter.setPen(darkPen);
        painter.drawLine(rect.bottomLeft(), rect.bottomRight());
    }

    if (!isOccupiedTile(column + 1, row)) {
        painter.setPen(darkPen);
        painter.drawLine(rect.topRight(), rect.bottomRight());
    }
    painter.restore();
}
