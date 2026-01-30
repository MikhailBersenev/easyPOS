#ifndef WINDOWCREATOR_H
#define WINDOWCREATOR_H

#pragma once

#include <QObject>
#include <QWidget>
#include <type_traits>
#include <memory>
#include "../easyposcore.h"

class WindowCreator : public QObject
{
    Q_OBJECT
public:
    explicit WindowCreator(QObject *parent = nullptr) {

    }
    
    // Создание окна без EasyPOSCore
    template <typename T>
    static T* Create(QWidget *parent = nullptr, bool show = true)
    {
        static_assert(std::is_base_of<QWidget, T>::value,
                      "T must inherit QWidget");
        T *windowPtr = new T(parent);
        if(show)
            windowPtr->show();
        return windowPtr;
    }
    
    // Создание окна с EasyPOSCore
    template <typename T>
    static T* Create(QWidget *parent, std::shared_ptr<EasyPOSCore> easyPOSCore, bool show = true)
    {
        static_assert(std::is_base_of<QWidget, T>::value,
                      "T must inherit QWidget");
        T *windowPtr = new T(parent, easyPOSCore);
        if(show)
            windowPtr->show();
        return windowPtr;
    }

signals:
};

// Специализация для MainWindow - получает сессию из EasyPOSCore
#include "../mainwindow.h"
template <>
inline MainWindow* WindowCreator::Create<MainWindow>(QWidget *parent, std::shared_ptr<EasyPOSCore> easyPOSCore, bool show)
{
    if (!easyPOSCore || !easyPOSCore->hasActiveSession()) {
        return nullptr;
    }
    MainWindow *windowPtr = new MainWindow(parent, easyPOSCore, easyPOSCore->getCurrentSession());
    if(show)
        windowPtr->show();
    return windowPtr;
}

#endif // WINDOWCREATOR_H
