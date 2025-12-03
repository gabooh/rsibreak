/*
    SPDX-FileCopyrightText: 2005 Bram Schoenmakers <bramschoenmakers@kde.nl>
    SPDX-FileCopyrightText: 2005-2007 Tom Albers <toma@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "passivepopup.h"

#include <QGuiApplication>
#include <QMouseEvent>
#include <QScreen>

PassivePopup::PassivePopup(QWidget *parent)
    : KPassivePopup(parent)
{
}

void PassivePopup::show()
{
    // Show at bottom-center, 10% from bottom edge (same as BreakControl)
    const QRect screenRect = QGuiApplication::primaryScreen()->availableGeometry();
    const int posX = screenRect.left() + (screenRect.width() - sizeHint().width()) / 2;
    const int posY = screenRect.bottom() - sizeHint().height() - screenRect.height() / 10;
    KPassivePopup::show(QPoint(posX, posY));
}

void PassivePopup::mouseReleaseEvent(QMouseEvent *event)
{
    event->accept();
    /* eat this! */
}
