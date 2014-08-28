/********************************************************************
 KWin - the KDE window manager
 This file is part of the KDE project.

Copyright (C) 2014 Martin Gräßlin <mgraesslin@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
#include "display.h"
#include "compositor_interface.h"
#include "output_interface.h"

#include <QCoreApplication>
#include <QDebug>
#include <QAbstractEventDispatcher>
#include <QSocketNotifier>

#include <wayland-server.h>

namespace KWin
{
namespace WaylandServer
{

Display::Display(QObject *parent)
    : QObject(parent)
    , m_display(nullptr)
    , m_loop(nullptr)
    , m_socketName(QStringLiteral("wayland-0"))
    , m_running(false)
{
    connect(QCoreApplication::eventDispatcher(), &QAbstractEventDispatcher::aboutToBlock, this, &Display::flush);
}

Display::~Display()
{
    terminate();
}

void Display::flush()
{
    if (!m_display || !m_loop) {
        return;
    }
    if (wl_event_loop_dispatch(m_loop, 0) != 0) {
        qWarning() << "Error on dispatching Wayland event loop";
    }
    wl_display_flush_clients(m_display);
}

void Display::setSocketName(const QString &name)
{
    if (m_socketName == name) {
        return;
    }
    m_socketName = name;
    emit socketNameChanged(m_socketName);
}

QString Display::socketName() const
{
    return m_socketName;
}

void Display::start()
{
    Q_ASSERT(!m_running);
    Q_ASSERT(!m_display);
    m_display = wl_display_create();
    if (wl_display_add_socket(m_display, qPrintable(m_socketName)) != 0) {
        return;
    }

    m_loop = wl_display_get_event_loop(m_display);
    int fd = wl_event_loop_get_fd(m_loop);
    if (fd == -1) {
        qWarning() << "Did not get the file descriptor for the event loop";
        return;
    }
    QSocketNotifier *m_notifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, &Display::flush);
    setRunning(true);
}

void Display::terminate()
{
    if (!m_running) {
        return;
    }
    emit aboutToTerminate();
    wl_display_terminate(m_display);
    wl_display_destroy(m_display);
    m_display = nullptr;
    m_loop = nullptr;
    setRunning(false);
}

void Display::setRunning(bool running)
{
    if (m_running == running) {
        return;
    }
    m_running = running;
    emit runningChanged(m_running);
}

OutputInterface *Display::createOutput(QObject *parent)
{
    OutputInterface *output = new OutputInterface(this, parent);
    connect(output, &QObject::destroyed, this, [this,output] { m_outputs.removeAll(output); });
    connect(this, &Display::aboutToTerminate, output, [this,output] { removeOutput(output); });
    m_outputs << output;
    return output;
}

CompositorInterface *Display::createCompositor(QObject *parent)
{
    CompositorInterface *compositor = new CompositorInterface(this, parent);
    connect(this, &Display::aboutToTerminate, compositor, [this,compositor] { delete compositor; });
    return compositor;
}

void Display::removeOutput(OutputInterface *output)
{
    m_outputs.removeAll(output);
    delete output;
}

}
}