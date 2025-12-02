/*
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "rsiidletime.h"

// -------------------- RSIIdleTimeImpl --------------------

RSIIdleTimeImpl::RSIIdleTimeImpl(QObject *parent)
    : RSIIdleTime(parent)
{
    connect(KIdleTime::instance(), qOverload<int, int>(&KIdleTime::timeoutReached), this, &RSIIdleTimeImpl::onTimeoutReached);
    connect(KIdleTime::instance(), &KIdleTime::resumingFromIdle, this, &RSIIdleTime::resumingFromIdle);
}

int RSIIdleTimeImpl::addIdleTimeout(int msec)
{
    int id = KIdleTime::instance()->addIdleTimeout(msec);
    m_timeouts.insert(id, msec);
    return id;
}

void RSIIdleTimeImpl::removeAllIdleTimeouts()
{
    KIdleTime::instance()->removeAllIdleTimeouts();
    m_timeouts.clear();
}

void RSIIdleTimeImpl::catchNextResumeEvent()
{
    KIdleTime::instance()->catchNextResumeEvent();
}

void RSIIdleTimeImpl::onTimeoutReached(int identifier, int msec)
{
    Q_UNUSED(identifier)
    emit idleTimeoutReached(msec);

    // Re-register for resume event to track when user becomes active
    KIdleTime::instance()->catchNextResumeEvent();
}

// -------------------- RSIIdleTimeFake --------------------

int RSIIdleTimeFake::addIdleTimeout(int msec)
{
    int id = m_nextId++;
    m_timeouts.insert(id, msec);
    return id;
}

void RSIIdleTimeFake::removeAllIdleTimeouts()
{
    m_timeouts.clear();
}

void RSIIdleTimeFake::catchNextResumeEvent()
{
    // No-op for fake implementation
}

void RSIIdleTimeFake::simulateIdleTimeout(int msec)
{
    emit idleTimeoutReached(msec);
}

void RSIIdleTimeFake::simulateResumeFromIdle()
{
    emit resumingFromIdle();
}
