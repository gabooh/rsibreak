/*
    SPDX-FileCopyrightText: 2024 RSIBreak contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef RSIBREAK_WAYLANDHELPER_H
#define RSIBREAK_WAYLANDHELPER_H

class QScreen;
class QWidget;

namespace WaylandHelper
{

/**
 * Configure a window as a fullscreen overlay using layer-shell.
 * The window will appear above all other windows including fullscreen apps.
 * @param widget The widget to configure as an overlay
 */
void configureAsOverlay( QWidget *widget );

/**
 * Configure a window as a break control widget.
 * Similar to overlay but positioned at top of screen.
 * @param widget The widget to configure
 */
void configureAsBreakControl( QWidget *widget );

/**
 * Activate a window (bring to front).
 * Note: On Wayland, windows cannot steal focus. This is best-effort.
 * @param widget The widget to activate
 */
void activateWindow( QWidget *widget );

/**
 * Initialize KScreen integration for primary screen detection.
 * Should be called once at application startup.
 */
void initKScreenIntegration();

/**
 * Get the KDE primary screen (priority 1).
 * Falls back to Qt's primaryScreen() if KScreen is unavailable.
 * @return The primary screen
 */
QScreen *getKDEPrimaryScreen();

} // namespace WaylandHelper

#endif // RSIBREAK_WAYLANDHELPER_H
