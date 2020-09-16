/*
 * Libinput Touch Translator
 *
 * Copyright (C) 2020, KylinSoft Co., Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Authors: Yue Lan <lanyue@kylinos.cn>
 *
 */

#include "touchpad-gesture-manager.h"

#include "settings-manager.h"
#include "uinput-helper.h"

#include <KWindowSystem>

#include <QDebug>

static TouchpadGestureManager *instance = nullptr;

TouchpadGestureManager *TouchpadGestureManager::getManager()
{
    if (!instance)
        instance = new TouchpadGestureManager;
    return instance;
}

void TouchpadGestureManager::processEvent(libinput_event *event)
{
    // Fixme:
    auto type = libinput_event_get_type(event);
    libinput_event_gesture *t = libinput_event_get_gesture_event(event);

    switch (type) {
    case LIBINPUT_EVENT_GESTURE_SWIPE_BEGIN: {
        reset();
        m_lastFinger = libinput_event_gesture_get_finger_count(t);
        break;
    }
    case LIBINPUT_EVENT_GESTURE_SWIPE_UPDATE: {
        double dx = libinput_event_gesture_get_dx(t);
        double dy = libinput_event_gesture_get_dy(t);
        //double dx_unaccel = libinput_event_gesture_get_dx_unaccelerated(t);
        //double dy_unaccel = libinput_event_gesture_get_dy_unaccelerated(t);

        // total offset
        m_totalDxmm += dx;
        m_totalDymm += dy;

        m_lastDxmm += dx;
        m_lastDymm += dy;
        if (qAbs(m_lastDxmm) > 20 || qAbs(m_lastDymm) > 20) {
            // update
            if (qAbs(m_lastDxmm) > qAbs(m_lastDymm)) {
                if (m_lastDxmm > 0) {
                    // right
                    emit eventTriggered(Swipe, m_lastFinger, Update, Right);
                } else {
                    // left
                    emit eventTriggered(Swipe, m_lastFinger, Update, Left);
                }
            } else {
                if (m_lastDymm > 0) {
                    // down
                    emit eventTriggered(Swipe, m_lastFinger, Update, Down);
                } else {
                    // up
                    emit eventTriggered(Swipe, m_lastFinger, Update, Up);
                }
            }

            m_lastDxmm = 0;
            m_lastDymm = 0;
        }
        break;
    }
    case LIBINPUT_EVENT_GESTURE_SWIPE_END: {
        m_isCancelled = libinput_event_gesture_get_cancelled(t);
        m_lastFinger = libinput_event_gesture_get_finger_count(t);
        if (!m_isCancelled) {
            if (qAbs(m_totalDxmm) > qAbs(m_totalDymm)) {
                if (m_totalDxmm > 20) {
                    // right
                    emit eventTriggered(Swipe, m_lastFinger, Finished, Right);
                } else if (m_totalDxmm < -20) {
                    // left
                    emit eventTriggered(Swipe, m_lastFinger, Finished, Left);
                } else {
                    //none
                    emit eventTriggered(Swipe, m_lastFinger, Finished, None);
                }
            } else {
                if (m_totalDymm > 20) {
                    // down
                    emit eventTriggered(Swipe, m_lastFinger, Finished, Down);
                } else if (m_totalDymm < -20) {
                    // up
                    emit eventTriggered(Swipe, m_lastFinger, Finished, Up);
                } else {
                    // none
                    emit eventTriggered(Swipe, m_lastFinger, Finished, None);
                }
            }
        } else {
            // cancelled
            emit eventTriggered(Swipe, m_lastFinger, Cancelled, None);
        }
        reset();
        break;
    }
    case LIBINPUT_EVENT_GESTURE_PINCH_BEGIN: {
        reset();
        m_lastFinger = libinput_event_gesture_get_finger_count(t);
        m_lastScale = -1;
        break;
    }
    case LIBINPUT_EVENT_GESTURE_PINCH_UPDATE: {
        m_totalScale = libinput_event_gesture_get_scale(t);
        m_totalAngle += libinput_event_gesture_get_angle_delta(t); // useless now
        qDebug()<<m_totalScale;

        if (m_lastScale < 0) {
            m_lastScale = m_totalScale;
            break;
        }

        if (qMax(m_totalScale/m_lastScale, m_lastScale/m_totalScale) > 1.5) {
            if (m_totalScale > m_lastScale) {
                // zoom in
                emit eventTriggered(Pinch, m_lastFinger, Update, ZoomIn);
            } else {
                // zoom out
                emit eventTriggered(Pinch, m_lastFinger, Update, ZoomOut);
            }
            m_lastScale = m_totalScale;
        }

        break;
    }
    case LIBINPUT_EVENT_GESTURE_PINCH_END: {
        m_isCancelled = libinput_event_gesture_get_cancelled(t);
        m_lastFinger = libinput_event_gesture_get_finger_count(t);

        // some zoom out gesture is easy to be recognized as cancelled,
        // so take them into judgement.
        if (!m_isCancelled || m_totalScale != 0) {
            if (m_totalScale > 1) {
                emit eventTriggered(Pinch, m_lastFinger, Finished, ZoomIn);
            } else {
                emit eventTriggered(Pinch, m_lastFinger, Finished, ZoomOut);
            }
        } else {
            emit eventTriggered(Pinch, m_lastFinger, Cancelled, None);
        }
        reset();
        break;
    }
    default:
        break;
    }

}

void TouchpadGestureManager::reset()
{
    m_lastFinger = 0;
    m_isCancelled = 0;

    m_totalDxmm = 0;
    m_totalDymm = 0;

    m_lastDxmm = 0;
    m_lastDymm = 0;

    m_totalScale = 0;
    m_totalAngle = 0;

    m_lastScale = -1;
    m_lastAngle = 0;
}

void TouchpadGestureManager::onEventTriggerd(TouchpadGestureManager::GestureType type, int fingerCount, TouchpadGestureManager::State state, TouchpadGestureManager::Direction direction)
{
    qDebug()<<type<<fingerCount<<state<<direction;
    qDebug()<<m_totalDxmm<<m_totalDymm<<m_totalAngle<<m_totalScale;

    auto shortcut = SettingsManager::getManager()->gesShortCut(fingerCount, type, state, direction);
    if (type == Pinch && state == Finished) {
        if (direction == ZoomIn) {
            if (shortcut.toString() == "Meta+PgUp") {
                // check if maximized
                auto activeWid = KWindowSystem::activeWindow();
                auto windowInfo = KWindowInfo(activeWid, NET::Property::WMAllProperties, NET::Property2::WM2AllProperties);
                if (windowInfo.hasState(NET::State::Max)) {
                    return;
                }
            }
        }

        if (direction == ZoomOut) {
            if (shortcut.toString() == "Meta+PgUp") {
                // check if maximized
                auto activeWid = KWindowSystem::activeWindow();
                auto windowInfo = KWindowInfo(activeWid, NET::Property::WMAllProperties, NET::Property2::WM2AllProperties);
                if (!windowInfo.hasState(NET::State::Max)) {
                    return;
                }
            }
        }
    }
    UInputHelper::getInstance()->executeShortCut(shortcut);
}

TouchpadGestureManager::TouchpadGestureManager(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<GestureType>("GestureType");
    qRegisterMetaType<State>("State");
    qRegisterMetaType<Direction>("Direction");
    connect(this, &TouchpadGestureManager::eventTriggered, this, &TouchpadGestureManager::onEventTriggerd);
}
