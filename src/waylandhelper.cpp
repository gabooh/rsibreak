/*
    SPDX-FileCopyrightText: 2024 RSIBreak contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "waylandhelper.h"

#include <LayerShellQt/Shell>
#include <LayerShellQt/Window>

#include <KScreen/Config>
#include <KScreen/GetConfigOperation>
#include <KScreen/Output>

#include <QDebug>
#include <QGuiApplication>
#include <QScreen>
#include <QWidget>
#include <QWindow>

static QString s_primaryOutputName;

namespace WaylandHelper
{

void configureAsOverlay( QWidget *widget )
{
    if ( !widget ) {
        return;
    }

    widget->winId();
    QWindow *window = widget->windowHandle();
    if ( !window ) {
        return;
    }

    auto *layerWindow = LayerShellQt::Window::get( window );
    if ( !layerWindow ) {
        return;
    }

    // Overlay layer - above everything including fullscreen windows
    layerWindow->setLayer( LayerShellQt::Window::LayerOverlay );

    // Anchor to all edges for fullscreen coverage
    layerWindow->setAnchors( LayerShellQt::Window::Anchors( LayerShellQt::Window::AnchorTop | LayerShellQt::Window::AnchorBottom
                                                            | LayerShellQt::Window::AnchorLeft | LayerShellQt::Window::AnchorRight ) );

    // -1 means don't reserve space (overlap everything)
    layerWindow->setExclusiveZone( -1 );

    // No keyboard focus - BreakControl handles interaction
    layerWindow->setKeyboardInteractivity( LayerShellQt::Window::KeyboardInteractivityNone );
}

void configureAsBreakControl( QWidget *widget )
{
    if ( !widget ) {
        return;
    }

    QScreen *screen = getKDEPrimaryScreen();

    // Position widget on target screen before native window creation
    QRect screenGeometry = screen->geometry();
    widget->move( screenGeometry.topLeft() );

    // Reset layer-shell surface by destroying existing window
    if ( widget->windowHandle() ) {
        widget->windowHandle()->destroy();
    }

    widget->winId();
    QWindow *window = widget->windowHandle();
    if ( !window ) {
        return;
    }

    window->setScreen( screen );

    auto *layerWindow = LayerShellQt::Window::get( window );
    if ( !layerWindow ) {
        return;
    }

    // Tell layer-shell to use the QWindow's screen
    layerWindow->setScreenConfiguration( LayerShellQt::Window::ScreenFromQWindow );

    layerWindow->setLayer( LayerShellQt::Window::LayerOverlay );

    // Anchor to top - window uses natural size and is centered
    layerWindow->setAnchors( LayerShellQt::Window::AnchorTop );

    // Margin from top: 10% of screen height
    int topMargin = screen->geometry().height() / 10;
    layerWindow->setMargins( QMargins( 0, topMargin, 0, 0 ) );

    // -1 means don't reserve space (overlap everything)
    layerWindow->setExclusiveZone( -1 );

    layerWindow->setKeyboardInteractivity( LayerShellQt::Window::KeyboardInteractivityExclusive );
}

void activateWindow( QWidget *widget )
{
    if ( !widget ) {
        return;
    }

    // On Wayland, windows cannot steal focus from other applications.
    // The best we can do is show, raise, and request activation.
    widget->show();
    widget->raise();
    widget->activateWindow();
}

void initKScreenIntegration()
{
    auto *op = new KScreen::GetConfigOperation();
    QObject::connect( op, &KScreen::GetConfigOperation::finished, [op]() {
        if ( op->hasError() ) {
            qDebug() << "KScreen config error:" << op->errorString();
            op->deleteLater();
            return;
        }

        KScreen::ConfigPtr config = op->config();
        if ( config ) {
            KScreen::OutputPtr primary = config->primaryOutput();
            if ( primary ) {
                s_primaryOutputName = primary->name();
            }
        }
        op->deleteLater();
    } );
}

QScreen *getKDEPrimaryScreen()
{
    if ( !s_primaryOutputName.isEmpty() ) {
        for ( QScreen *screen : QGuiApplication::screens() ) {
            if ( screen->name() == s_primaryOutputName ) {
                return screen;
            }
        }
    }
    return QGuiApplication::primaryScreen();
}

} // namespace WaylandHelper
