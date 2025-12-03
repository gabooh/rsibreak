/*
    SPDX-FileCopyrightText: 2009 Tom Albers <toma@kde.org>
    SPDX-FileCopyrightText: 2010 Juan Luis Baptiste <juan.baptiste@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "breakbase.h"
#include "breakcontrol.h"
#include "platformhelper.h"

#include <QApplication>
#include <QKeyEvent>
#include <QObject>
#include <QPainter>
#include <QScreen>

BreakBase::BreakBase(QObject *parent)
    : QObject(parent)
    , m_grayEffectOnAllScreens(nullptr)
    , m_readOnly(false)
    , m_disableShortcut(false)
    , m_grayEffectOnAllScreensActivated(false)
{
    Qt::WindowFlags flags = Qt::Window | Qt::FramelessWindowHint;
    // X11: Bypass window manager to prevent stacking reordering when clicking on overlay
    if (PlatformHelper::isX11()) {
        flags |= Qt::X11BypassWindowManagerHint;
    }
    m_breakControl = new BreakControl(nullptr, flags);
    m_breakControl->hide();
    m_breakControl->installEventFilter(this);
    connect(m_breakControl, &BreakControl::skip, this, &BreakBase::skip);
    connect(m_breakControl, &BreakControl::lock, this, &BreakBase::lock);
    connect(m_breakControl, &BreakControl::postpone, this, &BreakBase::postpone);
}

BreakBase::~BreakBase()
{
    delete m_grayEffectOnAllScreens;
    delete m_breakControl;
}

void BreakBase::activate()
{
    if (m_grayEffectOnAllScreensActivated)
        m_grayEffectOnAllScreens->activate();

    PlatformHelper::configureAsBreakControl(m_breakControl);
    m_breakControl->show();
    m_breakControl->setFocus();
}

void BreakBase::deactivate()
{
    if (m_grayEffectOnAllScreensActivated)
        m_grayEffectOnAllScreens->deactivate();

    m_breakControl->hide();
}

bool BreakBase::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (!m_disableShortcut && keyEvent->key() == Qt::Key_Escape) {
            emit skip();
        }
        return true;
    } else if (m_readOnly && (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick)) {
        return true;
    } else {
        return QObject::eventFilter(obj, event);
    }
}

void BreakBase::setReadOnly(bool ro)
{
    m_readOnly = ro;
}

bool BreakBase::readOnly() const
{
    return m_readOnly;
}

void BreakBase::setLabel(const QString &text)
{
    m_breakControl->setText(text);
}

void BreakBase::showMinimize(bool show)
{
    m_breakControl->showMinimize(show);
}

void BreakBase::showLock(bool show)
{
    m_breakControl->showLock(show);
}

void BreakBase::showPostpone(bool show)
{
    m_breakControl->showPostpone(show);
}

void BreakBase::disableShortcut(bool disable)
{
    m_disableShortcut = disable;
}

void BreakBase::setGrayEffectOnAllScreens(bool on)
{
    m_grayEffectOnAllScreensActivated = on;
    delete m_grayEffectOnAllScreens;
    if (on) {
        m_grayEffectOnAllScreens = new GrayEffectOnAllScreens();
        m_grayEffectOnAllScreens->setLevel(70);
    }
}

void BreakBase::setGrayEffectLevel(int level)
{
    m_grayEffectOnAllScreens->setLevel(level);
}

void BreakBase::excludeGrayEffectOnScreen(QScreen *screen)
{
    m_grayEffectOnAllScreens->disable(screen);
}

// ------------------------ GrayEffectOnAllScreens -------------//

GrayEffectOnAllScreens::GrayEffectOnAllScreens()
{
    for (QScreen *screen : QGuiApplication::screens()) {
        auto *grayWidget = new GrayWidget(nullptr);
        m_widgets.insert(screen, grayWidget);

        const QRect rect = screen->geometry();
        grayWidget->move(rect.topLeft());
        grayWidget->setGeometry(rect);

        // X11: Make overlay transparent to mouse events so clicks reach BreakControl
        if (PlatformHelper::isX11()) {
            grayWidget->setAttribute(Qt::WA_TransparentForMouseEvents);
        }

        PlatformHelper::configureAsOverlay(grayWidget);
    }
}

GrayEffectOnAllScreens::~GrayEffectOnAllScreens()
{
    qDeleteAll(m_widgets);
}

void GrayEffectOnAllScreens::disable(QScreen *screen)
{
    if (!m_widgets.contains(screen))
        return;

    delete m_widgets.take(screen);
}

void GrayEffectOnAllScreens::activate()
{
    foreach (GrayWidget *widget, m_widgets) {
        widget->show();
        widget->update();
    }
}

void GrayEffectOnAllScreens::deactivate()
{
    foreach (GrayWidget *widget, m_widgets) {
        widget->hide();
    }
}

void GrayEffectOnAllScreens::setLevel(int val)
{
    foreach (GrayWidget *widget, m_widgets) {
        widget->setLevel(val);
    }
}

//-------------------- GrayWidget ----------------------------//

GrayWidget::GrayWidget(QWidget *parent)
    // Use Qt::Window instead of Qt::Popup to prevent auto-close behavior under Wayland
    : QWidget(parent, Qt::Window | Qt::FramelessWindowHint)
    , m_alpha(180)
{
    setAutoFillBackground(false);
    setAttribute(Qt::WA_TranslucentBackground);
}

bool GrayWidget::event(QEvent *event)
{
    if (event->type() == QEvent::Paint) {
        QPainter p(this);
        p.setCompositionMode(QPainter::CompositionMode_Source);
        p.fillRect(rect(), QColor(0, 0, 0, m_alpha));
    }
    return QWidget::event(event);
}

void GrayWidget::setLevel(int val)
{
    // val is 0-100 percentage, convert to 0-255 alpha
    if (val > 0) {
        m_alpha = static_cast<int>(val * 255.0 / 100.0);
    } else {
        m_alpha = 0;
    }
    update();
}
