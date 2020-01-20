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
    QCoreApplication a(argc, argv);

    auto conn = new ConnectionThread;
    auto thread = new QThread;
    conn->moveToThread(thread);
    thread->start();

    QObject::connect(conn, &ConnectionThread::connected, [conn]{
        qDebug() << "connect successfully to wayland at socket:" << conn->socketName();
    });
    QObject::connect(conn, &ConnectionThread::connected, [conn]{
        qDebug() << "connect failed to wayland at socket:" << conn->socketName();
    });

    return a.exec();
}
