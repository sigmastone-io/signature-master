#pragma once

#include <QIcon>
#include <QMenu>
#include <QObject>
#include <QSystemTrayIcon>

class DeskStatus;
class Settings;

// The tray icon and its menu. The icon tells the desk's state at a glance; the menu opens the
// window, pauses listening, opens the log and quits. It never offers to sign or to switch
// automatic signing on.
class TrayController : public QObject {
    Q_OBJECT

public:
    enum class State { Idle, Pending, Signing, AutoSign, Paused, Attention };

    TrayController(Settings* settings, DeskStatus* desk, QObject* parent = nullptr);

Q_SIGNALS:
    void openRequested();
    void quitRequested();

private:
    void refresh();
    State currentState() const;
    QIcon renderIcon(State state, int count) const;
    QString tooltipFor(State state) const;

    Settings* m_settings;
    DeskStatus* m_desk;
    QSystemTrayIcon m_tray;
    QMenu m_menu;
    QAction* m_pendingLine = nullptr;
    QAction* m_pauseAction = nullptr;
    QByteArray m_shieldSvg;
};
