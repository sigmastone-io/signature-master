#include "TrayController.h"

#include "DeskStatus.h"
#include "RequestStore.h"
#include "Settings.h"

#include <QAction>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QPainter>
#include <QPixmap>
#include <QSettings>
#include <QStyleHints>
#include <QSvgRenderer>
#include <QTimer>

TrayController::TrayController(Settings* settings,
                               DeskStatus* desk,
                               RequestStore* store,
                               QObject* parent)
    : QObject(parent), m_settings(settings), m_desk(desk), m_store(store) {
    QFile svg(QStringLiteral(":/qt/qml/SignatureMaster/icons/shield.svg"));
    if (svg.open(QIODevice::ReadOnly)) {
        m_shieldSvg = svg.readAll();
    }

    QAction* open = m_menu.addAction(QStringLiteral("Open Signature Master"));
    connect(open, &QAction::triggered, this, &TrayController::openRequested);
    m_menu.setDefaultAction(open);

    m_pendingLine = m_menu.addAction(QString());
    m_pendingLine->setEnabled(false);

    m_menu.addSeparator();
    m_pauseAction = m_menu.addAction(QStringLiteral("Pause listening"));
    m_pauseAction->setCheckable(true);
    connect(m_pauseAction, &QAction::toggled, m_settings, &Settings::setPauseListening);

    QAction* log = m_menu.addAction(QStringLiteral("Open the log"));
    connect(log, &QAction::triggered, m_desk, &DeskStatus::openLogFolder);

    m_menu.addSeparator();
    QAction* quit = m_menu.addAction(QStringLiteral("Quit"));
    connect(quit, &QAction::triggered, this, &TrayController::quitRequested);

    m_tray.setContextMenu(&m_menu);
    connect(&m_tray,
            &QSystemTrayIcon::activated,
            this,
            [this](QSystemTrayIcon::ActivationReason reason) {
                if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
                    Q_EMIT openRequested();
                }
            });
    connect(&m_tray, &QSystemTrayIcon::messageClicked, this, &TrayController::openRequested);

    connect(m_desk, &DeskStatus::changed, this, &TrayController::refresh);
    connect(m_settings, &Settings::autoSignChanged, this, &TrayController::refresh);
    connect(m_settings, &Settings::pauseListeningChanged, this, &TrayController::refresh);
    connect(m_store, &RequestStore::entered, this, &TrayController::notify);
    connect(QGuiApplication::styleHints(),
            &QStyleHints::colorSchemeChanged,
            this,
            &TrayController::refresh);

    refresh();
    m_tray.show();
    // Explorer writes the icon's registry entry a moment after it appears; try twice.
    QTimer::singleShot(1500, this, &TrayController::promoteInTaskbar);
    QTimer::singleShot(8000, this, &TrayController::promoteInTaskbar);
}

void TrayController::promoteInTaskbar() {
#ifdef Q_OS_WIN
    const QString self = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    QSettings tray(QStringLiteral("HKEY_CURRENT_USER\\Control Panel\\NotifyIconSettings"),
                   QSettings::NativeFormat);
    const QStringList groups = tray.childGroups();
    for (const QString& group : groups) {
        tray.beginGroup(group);
        const QString executable = tray.value(QStringLiteral("ExecutablePath")).toString();
        if (executable.compare(self, Qt::CaseInsensitive) == 0 &&
            !tray.contains(QStringLiteral("IsPromoted"))) {
            tray.setValue(QStringLiteral("IsPromoted"), 1);
        }
        tray.endGroup();
    }
    tray.sync();
#endif
}

void TrayController::refresh() {
    const State state = currentState();
    const int pending = m_desk->pendingCount();
    m_tray.setIcon(renderIcon(state, pending));
    m_tray.setToolTip(tooltipFor(state));

    m_pendingLine->setText(pending == 0 ? QStringLiteral("No request waiting")
                           : pending == 1
                               ? QStringLiteral("1 request awaiting approval")
                               : QStringLiteral("%1 requests awaiting approval").arg(pending));

    const QSignalBlocker blocker(m_pauseAction);
    m_pauseAction->setChecked(m_settings->pauseListening());
    m_pauseAction->setText(m_settings->pauseListening() ? QStringLiteral("Resume listening")
                                                        : QStringLiteral("Pause listening"));
}

void TrayController::notify(const QString& id, RequestState state) {
    const std::optional<SigningRequest> request = m_store->find(id);
    if (!request.has_value()) {
        return;
    }
    const QString files = request->files.size() == 1
                              ? QStringLiteral("1 file")
                              : QStringLiteral("%1 files").arg(request->files.size());
    const QString what = QStringLiteral("%1 %2, %3, by %4.")
                             .arg(request->name(), request->refShort(), files, request->actor);
    switch (state) {
    case RequestState::Pending:
        m_tray.showMessage(QStringLiteral("Signature requested"),
                           what + (request->anyCheckUnverified()
                                       ? QStringLiteral(" The caller is not verified.")
                                       : QStringLiteral(" All checks passed.")),
                           QSystemTrayIcon::Information,
                           15000);
        break;
    case RequestState::Refused:
        m_tray.showMessage(QStringLiteral("Signature refused"),
                           what + QStringLiteral(" ") + request->reason,
                           QSystemTrayIcon::Warning,
                           15000);
        break;
    case RequestState::Signed:
        if (request->approvedBy == QStringLiteral("automatic")) {
            m_tray.showMessage(
                QStringLiteral("Signed automatically"), what, QSystemTrayIcon::Information, 10000);
        }
        break;
    case RequestState::Failed:
        m_tray.showMessage(QStringLiteral("Signing failed"),
                           what + QStringLiteral(" ") + request->reason,
                           QSystemTrayIcon::Critical,
                           15000);
        break;
    default:
        break;
    }
}

TrayController::State TrayController::currentState() const {
    if (!m_desk->certificateFound() || m_desk->certificateExpired() ||
        (!m_settings->pauseListening() && !m_desk->listening())) {
        return State::Attention;
    }
    if (m_settings->pauseListening()) {
        return State::Paused;
    }
    for (const SigningRequest& request : m_store->requests()) {
        if (request.state == RequestState::Signing) {
            return State::Signing;
        }
    }
    if (m_desk->pendingCount() > 0) {
        return State::Pending;
    }
    if (m_settings->autoSign()) {
        return State::AutoSign;
    }
    return State::Idle;
}

QString TrayController::tooltipFor(State state) const {
    switch (state) {
    case State::Attention:
        if (!m_desk->certificateFound()) {
            return QStringLiteral("Signature Master: no certificate chosen");
        }
        if (m_desk->certificateExpired()) {
            return QStringLiteral("Signature Master: the certificate has expired");
        }
        return QStringLiteral("Signature Master: not listening. %1").arg(m_desk->listenError());
    case State::Paused:
        return QStringLiteral("Signature Master: listening paused");
    case State::Pending:
        return QStringLiteral("Signature Master: %1 awaiting approval")
            .arg(m_desk->pendingCount() == 1
                     ? QStringLiteral("1 request")
                     : QStringLiteral("%1 requests").arg(m_desk->pendingCount()));
    case State::Signing:
        return QStringLiteral("Signature Master: signing");
    case State::AutoSign:
        return QStringLiteral("Signature Master: automatic signing on, %1").arg(m_desk->endpoint());
    case State::Idle:
        break;
    }
    return QStringLiteral("Signature Master: listening on %1").arg(m_desk->endpoint());
}

QIcon TrayController::renderIcon(State state, int count) const {
    const bool darkTaskbar = QGuiApplication::styleHints()->colorScheme() != Qt::ColorScheme::Light;
    QByteArray shieldColor = darkTaskbar ? "#fafafa" : "#18181b";
    if (state == State::Paused) {
        shieldColor = "#71717a";
    }
    QByteArray svg = m_shieldSvg;
    svg.replace("#000000", shieldColor);

    QColor dot;
    switch (state) {
    case State::Pending:
        dot = QColor(QStringLiteral("#d97706"));
        break;
    case State::Signing:
        dot = QColor(QStringLiteral("#6366f1"));
        break;
    case State::AutoSign:
        dot = QColor(QStringLiteral("#f97316"));
        break;
    case State::Attention:
        dot = QColor(QStringLiteral("#ef4444"));
        break;
    case State::Paused:
    case State::Idle:
        break;
    }

    QIcon icon;
    for (const int size : {16, 20, 24, 32, 48}) {
        QPixmap pixmap(size, size);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        QSvgRenderer renderer(svg);
        renderer.render(&painter, QRectF(0, 0, size, size));

        if (state == State::Paused) {
            QPen pen(QColor(QStringLiteral("#71717a")));
            pen.setWidthF(size / 10.0);
            pen.setCapStyle(Qt::RoundCap);
            painter.setPen(pen);
            painter.drawLine(QPointF(size * 0.2, size * 0.2), QPointF(size * 0.8, size * 0.8));
        } else if (dot.isValid()) {
            const qreal radius = size * (state == State::Pending ? 0.26 : 0.2);
            const QPointF centre(size - radius, radius);
            painter.setPen(Qt::NoPen);
            painter.setBrush(dot);
            painter.drawEllipse(centre, radius, radius);
            if (state == State::Pending && count > 0 && size >= 24) {
                QFont font = painter.font();
                font.setPixelSize(static_cast<int>(radius * 1.5));
                font.setBold(true);
                painter.setFont(font);
                painter.setPen(Qt::white);
                painter.drawText(
                    QRectF(centre.x() - radius, centre.y() - radius, radius * 2, radius * 2),
                    Qt::AlignCenter,
                    count > 9 ? QStringLiteral("9+") : QString::number(count));
            }
        }
        icon.addPixmap(pixmap);
    }
    return icon;
}
