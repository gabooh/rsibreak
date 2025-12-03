/*
    SPDX-FileCopyrightText: 2024 RSIBreak contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "platformhelper.h"

#include <KWindowSystem>

#include <QWidget>

#include <KScreen/Config>
#include <KScreen/GetConfigOperation>
#include <KScreen/Output>

// X11-specific includes
#include <KX11Extras>
#include <NETWM>

// KWayland for plasma-shell protocol
#include <KWayland/Client/connection_thread.h>
#include <KWayland/Client/plasmashell.h>
#include <KWayland/Client/registry.h>
#include <KWayland/Client/surface.h>
#include <LayerShellQt/Window>

static QString s_primaryOutputName;
static KWayland::Client::PlasmaShell *s_plasmaShell = nullptr;
static KWayland::Client::Registry *s_registry = nullptr;

namespace PlatformHelper
{

bool isX11()
{
    return KWindowSystem::isPlatformX11();
}

bool isWayland()
{
    return KWindowSystem::isPlatformWayland();
}

void configureAsOverlay(QWidget *widget)
{
    if (!widget) {
        return;
    }

    if (isWayland()) {
        widget->winId();
        QWindow *window = widget->windowHandle();
        if (!window) {
            return;
        }

        auto *layerWindow = LayerShellQt::Window::get(window);
        if (!layerWindow) {
            return;
        }

        // Overlay layer - above everything including fullscreen windows
        layerWindow->setLayer(LayerShellQt::Window::LayerOverlay);

        // Anchor to all edges for fullscreen coverage
        layerWindow->setAnchors(LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorTop | LayerShellQt::Window::AnchorBottom
                                                              | LayerShellQt::Window::AnchorLeft | LayerShellQt::Window::AnchorRight));

        // -1 means don't reserve space (overlap everything)
        layerWindow->setExclusiveZone(-1);

        // No keyboard focus - BreakControl handles interaction
        layerWindow->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityNone);
    } else if (isX11()) {
        widget->winId();
        KX11Extras::setOnAllDesktops(widget->winId(), true);
        KX11Extras::setState(widget->winId(), NET::KeepAbove | NET::FullScreen);
    }
}

void configureAsBreakControl(QWidget *widget)
{
    if (!widget) {
        return;
    }

    QScreen *screen = getKDEPrimaryScreen();

    // Position widget on target screen before native window creation
    QRect screenGeometry = screen->geometry();
    widget->move(screenGeometry.topLeft());

    if (isWayland()) {
        // Reset layer-shell surface by destroying existing window
        if (widget->windowHandle()) {
            widget->windowHandle()->destroy();
        }

        widget->winId();
        QWindow *window = widget->windowHandle();
        if (!window) {
            return;
        }

        window->setScreen(screen);

        auto *layerWindow = LayerShellQt::Window::get(window);
        if (!layerWindow) {
            return;
        }

        // Tell layer-shell to use the QWindow's screen
        layerWindow->setScreenConfiguration(LayerShellQt::Window::ScreenFromQWindow);

        layerWindow->setLayer(LayerShellQt::Window::LayerOverlay);

        // Anchor to bottom - window uses natural size and is centered
        layerWindow->setAnchors(LayerShellQt::Window::AnchorBottom);

        // Margin from bottom: 10% of screen height
        int bottomMargin = screen->geometry().height() / 10;
        layerWindow->setMargins(QMargins(0, 0, 0, bottomMargin));

        // -1 means don't reserve space (overlap everything)
        layerWindow->setExclusiveZone(-1);

        layerWindow->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityExclusive);
    } else if (isX11()) {
        widget->winId();

        // Calculate position: centered horizontally, 10% from bottom
        QSize widgetSize = widget->sizeHint();
        int x = screenGeometry.x() + (screenGeometry.width() - widgetSize.width()) / 2;
        int y = screenGeometry.y() + screenGeometry.height() - widgetSize.height() - screenGeometry.height() / 10;
        widget->move(x, y);

        KX11Extras::forceActiveWindow(widget->winId());
        KX11Extras::setOnAllDesktops(widget->winId(), true);
        KX11Extras::setState(widget->winId(), NET::KeepAbove);
    }
}

void activateWindow(QWidget *widget)
{
    if (!widget) {
        return;
    }

    widget->show();
    widget->raise();

    if (isX11()) {
        KX11Extras::forceActiveWindow(widget->winId());
    } else {
        // On Wayland, windows cannot steal focus from other applications.
        // The best we can do is show, raise, and request activation.
        widget->activateWindow();
    }
}

void configureStayOnTop(QWidget *widget)
{
    if (!widget) {
        return;
    }

    if (isWayland()) {
        widget->winId();
        QWindow *window = widget->windowHandle();
        if (!window) {
            return;
        }

        auto *layerWindow = LayerShellQt::Window::get(window);
        if (!layerWindow) {
            return;
        }

        // LayerTop = above normal windows but below overlay
        layerWindow->setLayer(LayerShellQt::Window::LayerTop);
        layerWindow->setExclusiveZone(0); // Don't reserve space
        layerWindow->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityOnDemand);
    } else if (isX11()) {
        widget->winId();
        KX11Extras::setState(widget->winId(), NET::KeepAbove);
    }
}

void configureAsNotification(QWidget *widget)
{
    if (!widget) {
        return;
    }

    if (isWayland()) {
        widget->winId();
        QWindow *window = widget->windowHandle();
        if (!window) {
            qDebug() << "No window handle for notification widget";
            return;
        }

        if (!s_plasmaShell) {
            qDebug() << "PlasmaShell not available, notification may not stay on top";
            return;
        }

        // Get the wl_surface for this window
        auto *surface = KWayland::Client::Surface::fromWindow(window);
        if (!surface) {
            qDebug() << "Could not get Wayland surface for window";
            return;
        }

        // Create PlasmaShellSurface with Notification role
        auto *plasmaSurface = s_plasmaShell->createSurface(surface, widget);
        if (plasmaSurface) {
            plasmaSurface->setRole(KWayland::Client::PlasmaShellSurface::Role::Notification);
            qDebug() << "Configured window as Notification via plasma-shell";
        }
    } else if (isX11()) {
        widget->winId();
        KX11Extras::setState(widget->winId(), NET::KeepAbove);
    }
}

void initKScreenIntegration()
{
    auto *op = new KScreen::GetConfigOperation();
    QObject::connect(op, &KScreen::GetConfigOperation::finished, [op]() {
        if (op->hasError()) {
            qDebug() << "KScreen config error:" << op->errorString();
            op->deleteLater();
            return;
        }

        KScreen::ConfigPtr config = op->config();
        if (config) {
            KScreen::OutputPtr primary = config->primaryOutput();
            if (primary) {
                s_primaryOutputName = primary->name();
            }
        }
        op->deleteLater();
    });

    // Initialize PlasmaShell on Wayland
    if (isWayland()) {
        auto *connection = KWayland::Client::ConnectionThread::fromApplication();
        if (!connection) {
            qDebug() << "No Wayland connection available";
            return;
        }

        s_registry = new KWayland::Client::Registry();
        QObject::connect(s_registry, &KWayland::Client::Registry::plasmaShellAnnounced, [](quint32 name, quint32 version) {
            s_plasmaShell = s_registry->createPlasmaShell(name, version);
            qDebug() << "PlasmaShell interface initialized";
        });

        s_registry->create(connection);
        s_registry->setup();
    }
}

QScreen *getKDEPrimaryScreen()
{
    if (!s_primaryOutputName.isEmpty()) {
        for (QScreen *screen : QGuiApplication::screens()) {
            if (screen->name() == s_primaryOutputName) {
                return screen;
            }
        }
    }
    return QGuiApplication::primaryScreen();
}

} // namespace PlatformHelper
