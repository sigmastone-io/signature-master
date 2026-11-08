#pragma once

#include "SigningRequest.h"

#include <QIcon>
#include <QMenu>
#include <QObject>
#include <QSystemTrayIcon>

class DeskStatus;
class RequestStore;
class Settings;

// The tray icon and its menu. The icon tells the desk's state at a glance; the menu opens the
// window, pauses listening, opens the log and quits. It never offers to sign or to switch
// automatic signing on. New and refused requests raise a notification; clicking it opens the
// window.
class TrayController : public QObject {
    Q_OBJECT

public:
    enum class State { Idle, Pending, Signing, AutoSign, Paused, Attention };

    TrayController(Settings* settings,
                   DeskStatus* desk,
                   RequestStore* store,
                   QObject* parent = nullptr);

Q_SIGNALS:
    void openRequested();
    void quitRequested();

private:
    void refresh();
    void notify(const QString& id, RequestState state);
    // Windows 11 puts a new tray icon in the hidden area, which some taskbars cannot even open.
    // A desk nobody can see is no desk, so the icon promotes itself the first time Windows
    // registers it; a person who later hides it again is left alone.
    void promoteInTaskbar();
    State currentState() const;
    QIcon renderIcon(State state, int count) const;
    QString tooltipFor(State state) const;

    Settings* m_settings;
    DeskStatus* m_desk;
    RequestStore* m_store;
    QSystemTrayIcon m_tray;
    QMenu m_menu;
    QAction* m_pendingLine = nullptr;
    QAction* m_pauseAction = nullptr;
    QByteArray m_shieldSvg;
};
