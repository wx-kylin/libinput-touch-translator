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

#include "touch-screen-four-finger-swipe-gesture.h"

#include <QDebug>

TouchScreenFourFingerSwipeGesture::TouchScreenFourFingerSwipeGesture(QObject *parent) : TouchScreenGestureInterface(parent)
{

}

TouchScreenGestureInterface::State TouchScreenFourFingerSwipeGesture::handleInputEvent(libinput_event *event)
{
    switch (libinput_event_get_type(event)) {
    case LIBINPUT_EVENT_TOUCH_DOWN: {
        m_currentFingerCount++;
        if (m_isCancelled)
            return Ignore;
        auto touch_event = libinput_event_get_touch_event(event);

        int current_finger_count = m_currentFingerCount;
        //qDebug()<<"current finger count:"<<current_finger_count;
        int current_slot = libinput_event_touch_get_slot(touch_event);

        double mmx = libinput_event_touch_get_x(touch_event);
        double mmy = libinput_event_touch_get_y(touch_event);

        if (current_finger_count <= 4) {
            m_startPoints[current_slot] = QPointF(mmx, mmy);
        }

        if (current_finger_count == 4) {
            // start the gesture
            m_isStarted = true;
            for (int i = 0; i < 4; i++) {
                m_lastPoints[i] = m_startPoints[i];
                m_currentPoints[i] = m_startPoints[i];
            }
            emit gestureBegin(getGestureIndex());
            return Maybe;
        }

        if (current_finger_count > 4) {
            m_isCancelled = true;
            emit gestureCancelled(getGestureIndex());
            return Cancelled;
        }
        break;
    }
    case LIBINPUT_EVENT_TOUCH_MOTION: {
        if (m_isCancelled)
            return Ignore;

        // update position
        auto touch_event = libinput_event_get_touch_event(event);

        int current_slot = libinput_event_touch_get_slot(touch_event);

        double mmx = libinput_event_touch_get_x(touch_event);
        double mmy = libinput_event_touch_get_y(touch_event);

        m_currentPoints[current_slot] = QPointF(mmx, mmy);

        if (!m_isStarted) {
            m_startPoints[current_slot] = m_currentPoints[current_slot];
        }
        break;
    }
    case LIBINPUT_EVENT_TOUCH_UP: {
        m_currentFingerCount--;
        //m_isCancelled = true;
        int current_finger_count = m_currentFingerCount;

        if (current_finger_count <= 0) {
            if (!m_isCancelled && m_isStarted && m_lastDirection != None) {
                emit gestureFinished(getGestureIndex());
                //qDebug()<<"total direction:"<<totalDirection();
                return Finished;
            } else {
                // we have post cancel event yet.
                reset();
                return Ignore;
            }
        }

        break;
    }
    case LIBINPUT_EVENT_TOUCH_FRAME: {
        if (m_isCancelled || !m_isStarted)
            return Ignore;

        if (m_currentFingerCount != 4)
            return Ignore;

        // update gesture

        // count offset
        auto last_center_points = (m_lastPoints[0] + m_lastPoints[1] + m_lastPoints[2] + m_lastPoints[3])/4;
        auto current_center_points = (m_currentPoints[0] + m_currentPoints[1] + m_currentPoints[2] + m_lastPoints[3])/4;
        auto delta = current_center_points - last_center_points;
        auto offset = delta.manhattanLength();
        if (offset < 25) {
            return Ignore;
        }

        for (int i = 0; i < 4; i++) {
            m_lastPoints[i] = m_currentPoints[i];
        }

        if (qAbs(delta.x()) > qAbs(delta.y())) {
            m_lastDirection = delta.x() > 0? Right: Left;
        } else {
            m_lastDirection = delta.y() > 0? Down: Up;
        }

        emit gestureUpdate(getGestureIndex());

        return Update;

        break;
    }
    case LIBINPUT_EVENT_TOUCH_CANCEL: {
        m_isCancelled = true;
        emit gestureCancelled(getGestureIndex());
        return Cancelled;
        break;
    }
    default:
        break;
    }

    return Ignore;
}

void TouchScreenFourFingerSwipeGesture::reset()
{
    m_isCancelled = false;
    m_isStarted = false;
    m_lastDirection = None;

    for (int i = 0; i < 4; i++) {
        m_startPoints[i] = QPointF();
        m_lastPoints[i] = QPointF();
        m_currentPoints[i] = QPointF();
    }
}

TouchScreenGestureInterface::Direction TouchScreenFourFingerSwipeGesture::totalDirection()
{
    // count total offset
    auto start_center_points = (m_startPoints[0] + m_startPoints[1] + m_startPoints[2] + m_startPoints[3])/4;
    auto current_center_points = (m_currentPoints[0] + m_currentPoints[1] + m_currentPoints[2] + m_currentPoints[3])/4;
    auto delta = current_center_points - start_center_points;
    auto offset = delta.manhattanLength();
    if (offset < 25) {
        return None;
    }

    if (qAbs(delta.x()) > qAbs(delta.y())) {
        return delta.x() > 0? Right: Left;
    } else {
        return delta.y() > 0? Down: Up;
    }
}

TouchScreenGestureInterface::Direction TouchScreenFourFingerSwipeGesture::lastDirection()
{
    return m_lastDirection;
}

void TouchScreenFourFingerSwipeGesture::cancel()
{
    m_isCancelled = true;
    emit gestureCancelled(getGestureIndex());
}
