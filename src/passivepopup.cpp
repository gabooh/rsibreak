/*
    SPDX-FileCopyrightText: 2005 Bram Schoenmakers <bramschoenmakers@kde.nl>
    SPDX-FileCopyrightText: 2005-2007 Tom Albers <toma@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "passivepopup.h"
#include "platformhelper.h"

#include <QGuiApplication>
#include <QMouseEvent>
#include <QScreen>

PassivePopup::PassivePopup(QWidget *parent)
    : KPassivePopup(parent)
{
}

void PassivePopup::show()
{
    // Calculate position: centered horizontally, 10% from bottom of primary screen
    const QRect screenRect = QGuiApplication::primaryScreen()->availableGeometry();
    const int posX = screenRect.left() + (screenRect.width() - sizeHint().width()) / 2;
    const int posY = screenRect.bottom() - sizeHint().height() - screenRect.height() / 10;

    move(posX, posY);
    QWidget::show();
    PlatformHelper::configureAsNotification(this);
}

void PassivePopup::setVisible(bool visible)
{
    // Bypass KPassivePopup::setVisible() which calls positionSelf() and overrides our manual positioning
    QFrame::setVisible(visible);
}

void PassivePopup::mouseReleaseEvent(QMouseEvent *event)
{
    event->accept();
    /* eat this! */
}
