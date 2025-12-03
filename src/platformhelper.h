/*
    SPDX-FileCopyrightText: 2024 RSIBreak contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef RSIBREAK_PLATFORMHELPER_H
#define RSIBREAK_PLATFORMHELPER_H

class QScreen;
class QWidget;

namespace PlatformHelper
{

/**
 * Check if running on X11.
 * @return true if the platform is X11
 */
bool isX11();

/**
 * Check if running on Wayland.
 * @return true if the platform is Wayland
 */
bool isWayland();

/**
 * Configure a window as a fullscreen overlay.
 * On Wayland: Uses layer-shell protocol to appear above all windows.
 * On X11: Uses KX11Extras to set KeepAbove and FullScreen states.
 * @param widget The widget to configure as an overlay
 */
void configureAsOverlay(QWidget *widget);

/**
 * Configure a window as a break control widget.
 * On Wayland: Uses layer-shell positioned at top of screen.
 * On X11: Uses KX11Extras for window management.
 * @param widget The widget to configure
 */
void configureAsBreakControl(QWidget *widget);

/**
 * Activate a window (bring to front).
 * On X11: Forces window activation.
 * On Wayland: Best-effort activation (cannot steal focus).
 * @param widget The widget to activate
 */
void activateWindow(QWidget *widget);

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

} // namespace PlatformHelper

#endif // RSIBREAK_PLATFORMHELPER_H
