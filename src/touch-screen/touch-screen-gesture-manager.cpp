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

#include "touch-screen-gesture-manager.h"

#include "touch-screen-gesture-interface.h"

#include "settings-manager.h"
#include "uinput-helper.h"

#include "touch-screen-two-finger-swipe-gesture.h"

#include <KWindowSystem>

#include <QDebug>

static TouchScreenGestureManager *instance = nullptr;

TouchScreenGestureManager::TouchScreenGestureManager(QObject *parent) : QObject(parent)
{

}

TouchScreenGestureManager *TouchScreenGestureManager::getManager()
{
    if (!instance)
        instance = new TouchScreenGestureManager;
    return instance;
}

int TouchScreenGestureManager::registerGesuture(TouchScreenGestureInterface *gesture)
{
    m_gestures<<gesture;
    connect(gesture, &TouchScreenGestureInterface::gestureBegin, this, &TouchScreenGestureManager::onGestureBegin);
    connect(gesture, &TouchScreenGestureInterface::gestureUpdate, this, &TouchScreenGestureManager::onGestureUpdated);
    connect(gesture, &TouchScreenGestureInterface::gestureCancelled, this, &TouchScreenGestureManager::onGestureCancelled);
    connect(gesture, &TouchScreenGestureInterface::gestureFinished, this, &TouchScreenGestureManager::onGestureFinished);
    return m_gestures.indexOf(gesture);
}

int TouchScreenGestureManager::queryGestureIndex(TouchScreenGestureInterface *gesture)
{
    return m_gestures.indexOf(gesture);
}

void TouchScreenGestureManager::processEvent(libinput_event *event)
{
    for (auto gesture : m_gestures) {
        auto state = gesture->handleInputEvent(event);
        //qDebug()<<gesture->finger()<<state;
    }
}

void TouchScreenGestureManager::forceReset()
{
    for (auto gesture : m_gestures) {
        gesture->reset();
    }
}

void TouchScreenGestureManager::onGestureBegin(int index)
{

}

void TouchScreenGestureManager::onGestureUpdated(int index)
{
    auto gesture = m_gestures.at(index);
    qDebug()<<gesture->finger()<<"finger"<<gesture->type()<<"updated, current direction:"<<gesture->lastDirection();


    // cancel swipe gesture if any zoom gesture triggered.
    if (gesture->type() == TouchScreenGestureInterface::Zoom) {
        for (auto gesture : m_gestures) {
            if (gesture->type() == TouchScreenGestureInterface::Swipe) {
                gesture->cancel();
            }
        }
        if (gesture->finger() == 2) {
            UInputHelper::getInstance()->executeShortCut(gesture->lastDirection() == TouchScreenGestureInterface::ZoomIn? QKeySequence("Ctrl++"): QKeySequence("Ctrl+-"));
        }
    } else {
        if (gesture->finger() == 2) {
            if (gesture->type() == TouchScreenGestureInterface::Swipe) {
                auto twoFingerSwipe = static_cast<TouchScreenTwoFingerSwipeGesture *>(gesture);
                auto offset = twoFingerSwipe->getLastOffset();
                UInputHelper::getInstance()->wheel(offset/10);
            }
        }
    }

    if (gesture->type() == TouchScreenGestureInterface::Swipe || gesture->type() == TouchScreenGestureInterface::Zoom) {
        // cancel drag and tap gesture
        for (auto gesture : m_gestures) {
            if (gesture->type() == TouchScreenGestureInterface::DragAndTap) {
                gesture->cancel();
            }
        }
    }

    if (gesture->type() == TouchScreenGestureInterface::DragAndTap) {
        UInputHelper::getInstance()->clickMouseRightButton();
    }
}

void TouchScreenGestureManager::onGestureCancelled(int index)
{

}

void TouchScreenGestureManager::onGestureFinished(int index)
{
    auto gesture = m_gestures.at(index);
    qDebug()<<gesture->finger()<<"finger"<<gesture->type()<<"finished, total direction:"<<gesture->totalDirection();

    if (gesture->type() == TouchScreenGestureInterface::Tap) {
        if (gesture->finger() == 2)
            UInputHelper::getInstance()->clickMouseRightButton();
    } else {
        auto settingsManager = SettingsManager::getManager();
        auto shortCut = settingsManager->getShortCut(gesture, TouchScreenGestureInterface::Finished, gesture->totalDirection());
        qDebug()<<shortCut;
        if (gesture->type() == TouchScreenGestureInterface::Zoom) {
            if (gesture->totalDirection() == TouchScreenGestureInterface::ZoomIn) {
                if (shortCut.toString() == "Meta+PgUp") {
                    // check if maximized
                    auto activeWid = KWindowSystem::activeWindow();
                    auto windowInfo = KWindowInfo(activeWid, NET::Property::WMAllProperties, NET::Property2::WM2AllProperties);
                    if (windowInfo.hasState(NET::State::Max)) {
                        goto end;
                    }
                }
            }

            if (gesture->totalDirection() == TouchScreenGestureInterface::ZoomOut) {
                if (shortCut.toString() == "Meta+PgUp") {
                    // check if maximized
                    auto activeWid = KWindowSystem::activeWindow();
                    auto windowInfo = KWindowInfo(activeWid, NET::Property::WMAllProperties, NET::Property2::WM2AllProperties);
                    if (!windowInfo.hasState(NET::State::Max)) {
                        goto end;
                    }
                }
            }
        }

        UInputHelper::getInstance()->executeShortCut(shortCut);
    }

end:
    // reset all gesture
    for (auto gesture : m_gestures) {
        gesture->reset();
    }
}
