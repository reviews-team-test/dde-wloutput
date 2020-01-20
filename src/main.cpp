/*
 *  * Copyright (C) 2011 ~ 2019 Deepin Technology Co., Ltd.
 *   *
 *    * Author:     lq <longqi_cm@deepin.com>
 *     *
 *      * Maintainer: lq <longqi_cm@deepin.com>
 *       *
 *        * This program is free software: you can redistribute it and/or modify
 *         * it under the terms of the GNU General Public License as published by
 *          * the Free Software Foundation, either version 3 of the License, or
 *           * any later version.
 *            *
 *             * This program is distributed in the hope that it will be useful,
 *              * but WITHOUT ANY WARRANTY; without even the implied warranty of
 *               * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *                * GNU General Public License for more details.
 *                 *
 *                  * You should have received a copy of the GNU General Public License
 *                   * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *                    */

#include <QCoreApplication>
#include <QThread>
#include <QDebug>

#include <outputdevice.h>
#include <registry.h>
#include <connection_thread.h>

using namespace KWayland::Client;

int main(int argc, char *argv[])
{
    qDebug() << "start Connection";
    
    auto conn = new ConnectionThread;
    auto thread = new QThread;
    conn->moveToThread(thread);
    thread->start();
    
    conn->initConnection();
    bool beConnect = true;

    Registry *reg = nullptr;
    QObject::connect(conn, &ConnectionThread::connected, [conn, &reg]{
        qDebug() << "connect successfully to wayland at socket:" << conn->socketName();

        reg = new Registry;
        reg->create(conn);
        reg->setup();

        QObject::connect(reg, &Registry::outputDeviceAnnounced, [](quint32 name, quint32 version){
            qDebug() << "output device announced with name: " << name << " and version :" << version;
        });
        QObject::connect(reg, &Registry::outputDeviceRemoved, [](quint32 name){
            qDebug() << "output device removed with name: " << name;
        });
    });
    QObject::connect(conn, &ConnectionThread::failed, [conn]{
        qDebug() << "connect failed to wayland at socket:" << conn->socketName();
    });
    QObject::connect(conn, &ConnectionThread::failed, [conn]{
        qDebug() << "connect failed to wayland at socket:" << conn->socketName();
    });
    QObject::connect(conn, &ConnectionThread::connectionDied, [conn, &beConnect, reg]{
        qDebug() << "connect failed to wayland at socket:" << conn->socketName();
        reg->deleteLater();
        beConnect = false;
    });
    QObject::connect(conn, &ConnectionThread::eventsRead, [conn] {

    });

    conn->roundtrip();
    while(beConnect) {
        conn->roundtrip();
    }
    
    return 1;
}
