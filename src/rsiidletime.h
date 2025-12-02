/*
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef RSIBREAK_RSIIDLETIME_H
#define RSIBREAK_RSIIDLETIME_H

#include <KIdleTime>

/**
 * Abstract interface for idle time detection.
 * Uses an event-based approach compatible with Wayland.
 */
class RSIIdleTime : public QObject
{
    Q_OBJECT

public:
    explicit RSIIdleTime(QObject *parent = nullptr)
        : QObject(parent)
    {
    }
    ~RSIIdleTime() override = default;

    /**
     * Register a timeout to be notified when idle for msec milliseconds.
     * @param msec Milliseconds of idle time to trigger notification
     * @return Token identifier for this timeout
     */
    virtual int addIdleTimeout(int msec) = 0;

    /**
     * Remove all registered idle timeouts.
     */
    virtual void removeAllIdleTimeouts() = 0;

    /**
     * Request notification when user becomes active again.
     */
    virtual void catchNextResumeEvent() = 0;

signals:
    /**
     * Emitted when a registered idle timeout is reached.
     * @param msec The timeout duration that was reached
     */
    void idleTimeoutReached(int msec);

    /**
     * Emitted when user activity resumes after being idle.
     */
    void resumingFromIdle();
};

/**
 * Real implementation using KIdleTime.
 */
class RSIIdleTimeImpl : public RSIIdleTime
{
    Q_OBJECT

public:
    explicit RSIIdleTimeImpl(QObject *parent = nullptr);
    ~RSIIdleTimeImpl() override = default;

    int addIdleTimeout(int msec) override;
    void removeAllIdleTimeouts() override;
    void catchNextResumeEvent() override;

private slots:
    void onTimeoutReached(int identifier, int msec);

private:
    QHash<int, int> m_timeouts; // id -> msec mapping
};

/**
 * Fake implementation for testing.
 */
class RSIIdleTimeFake : public RSIIdleTime
{
    Q_OBJECT

public:
    explicit RSIIdleTimeFake(QObject *parent = nullptr)
        : RSIIdleTime(parent)
    {
    }
    ~RSIIdleTimeFake() override = default;

    int addIdleTimeout(int msec) override;
    void removeAllIdleTimeouts() override;
    void catchNextResumeEvent() override;

    // Test helper methods
    void simulateIdleTimeout(int msec);
    void simulateResumeFromIdle();

private:
    int m_nextId = 1;
    QHash<int, int> m_timeouts;
};

#endif // RSIBREAK_RSIIDLETIME_H
