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
#include <outputconfiguration.h>
#include <outputmanagement.h>

using namespace KWayland::Client;

bool beConnect = false;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    qDebug() << "start Connection";

    auto conn = new ConnectionThread;
    auto thread = new QThread;
    conn->moveToThread(thread);
    thread->start();

    conn->initConnection();

    Registry *reg = nullptr;
    OutputManagement *manager = nullptr;
    OutputDevice *dev = nullptr;
    OutputConfiguration *conf = nullptr;
    QObject::connect(conn, &ConnectionThread::connected, [ & ] {
        qDebug() << "connect successfully to wayland at socket:" << conn->socketName();

        reg = new Registry;
        reg->create(conn);
        reg->setup();

        QObject::connect(reg, &Registry::outputManagementAnnounced, [ & ](quint32 name, quint32 version) {
            Q_UNUSED(name)
            Q_UNUSED(version)

            qDebug() << "output management announced with name :" << name << " & version :" << version;
            manager = reg->createOutputManagement(name, version);
	    if (!manager || !manager->isValid()) {
	    	qDebug() << "manager is nullptr or not valid!";
		return;
	    }
            qDebug() << "create output configuretion";
            conf = manager->createConfiguration();
            qDebug() << "create output done";
            if (!conf || !conf->isValid()) {
                qDebug() << "output configure is null or is not vaild";
                return;
            }
            QObject::connect(conf, &OutputConfiguration::applied, [conf]() {
                qDebug() << "Configuration applied!";
                conf->deleteLater();
            });
            QObject::connect(conf, &OutputConfiguration::failed, [conf]() {
                qDebug() << "Configuration failed!";
                conf->deleteLater();
            });
            QObject::connect(reg, &Registry::outputDeviceAnnounced, [ & ](quint32 name, quint32 version) {
                qDebug() << "output device announced with name: " << name << " and version :" << version;
                beConnect = true;
                dev = reg->createOutputDevice(name, version);
                if (!dev) {
                    qDebug() << "get dev error!";
                    return;
                }
                QObject::connect(dev, &OutputDevice::changed, [ & ] {
                    beConnect = true;
                    auto modes = dev->modes();
                    qDebug() << "get dev modes size: " << modes.size();
                    for (auto m : modes)
                    {
                        qDebug() << "mode size:" << m.size << "rate:" << m.refreshRate;
                    }
                });
             
	        if (!manager || !manager->isValid()) {
	            qDebug() << "manager is nullptr or not valid!";
	            return;
	        }
	        if (!conf || !conf->isValid()) {
                    qDebug() << "output configure is null or is not valid";
                    return;
	        }
                qDebug() << "set output transform to Rotated90";
                conf->setTransform(dev, OutputDevice::Transform::Rotated90);
                qDebug() << "apply changed";
                conf->apply();
            });
        });

        QObject::connect(reg, &Registry::outputDeviceRemoved, [](quint32 name) {
            qDebug() << "output device removed with name: " << name;
            beConnect = true;
        });

        do {
            beConnect = false;
            conn->roundtrip();
        } while (beConnect);

        exit(0);
    });
    QObject::connect(conn, &ConnectionThread::failed, [conn] {
        qDebug() << "connect failed to wayland at socket:" << conn->socketName();
        beConnect = true;
    });
    QObject::connect(conn, &ConnectionThread::failed, [conn] {
        qDebug() << "connect failed to wayland at socket:" << conn->socketName();
        beConnect = true;
    });
    QObject::connect(conn, &ConnectionThread::connectionDied, [conn, reg] {
        qDebug() << "connect failed to wayland at socket:" << conn->socketName();
        reg->deleteLater();
        beConnect = false;
    });
    QObject::connect(conn, &ConnectionThread::eventsRead, [conn] {

    });

    return app.exec();
}
