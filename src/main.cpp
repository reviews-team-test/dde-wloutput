/*
 ** Copyright (C) 2011 ~ 2019 Deepin Technology Co., Ltd.
 **
 ** Author:     lq <longqi_cm@deepin.com>
 **
 ** Maintainer: lq <longqi_cm@deepin.com>
 **
 ** This program is free software: you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation, either version 3 of the License, or
 ** any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include <string.h>
#include <stdlib.h>

#include <iostream>

#include <QCoreApplication>
#include <QThread>
#include <QDebug>

#include <outputdevice.h>
#include <registry.h>
#include <connection_thread.h>
#include <outputconfiguration.h>
#include <outputmanagement.h>

using namespace KWayland::Client;

enum modeFlag {
    None,
    Current = 1 << 0,
    Preferred = 1 << 1
};

struct command_set {
    bool enabled;
    char *uuid;
    int x, y;
    int width, height;
    int refresh, transform;
};

struct command_argument {
        bool cmd_get;
        struct command_set cmd_set;
};

static struct command_argument *cmd_args = nullptr;
static bool beConnect = false;


static OutputManagement *manager = nullptr;
static OutputConfiguration *conf = nullptr;

#define parse_int_arg(manager,item,str) do { \
    manager->cmd_set.item = (int32_t)QString(str).toInt(); \
    } while(0);

void parse_arguments(char **argv)
{
    cmd_args = (struct command_argument*)calloc(1, sizeof(struct command_argument));
    if (QString(argv[1]).compare("get") == 0) {
        cmd_args->cmd_get = true;
        return;
    }

    qDebug()<<"11111"<<cmd_args->cmd_set.uuid<<endl;
    cmd_args->cmd_set.uuid = (char*)calloc(strlen(argv[2]), sizeof(char)+1);
    strcpy(cmd_args->cmd_set.uuid, argv[2]);

    if (QString(argv[3]).toInt() == 1) {
        cmd_args->cmd_set.enabled = true;
    } else {
        cmd_args->cmd_set.enabled = false;
    }

    parse_int_arg(cmd_args, x, argv[4]);
    parse_int_arg(cmd_args, y, argv[5]);
    parse_int_arg(cmd_args, width, argv[6]);
    parse_int_arg(cmd_args, height, argv[7]);
    parse_int_arg(cmd_args, refresh, argv[8]);
    parse_int_arg(cmd_args, transform, argv[9]);
}

void free_command_argument(struct command_argument *manager)
{
    if (!manager) {
        return;
    }
    if (manager->cmd_set.uuid) {
        free(manager->cmd_set.uuid);
    }
    free(manager);
}

void dump_outputs(Registry *reg) {
    QObject::connect(reg, &Registry::outputDeviceAnnounced, [reg](quint32 name, quint32 version){
        auto dev = reg->createOutputDevice(name, version);
        if (!dev) {
            qDebug() << "get dev error!";
            return;
        }
        QObject::connect(dev, &OutputDevice::changed, [dev]{
            std::cout << dev->model().split(" ").first().toStdString();
            std::cout<<" "<<((dev->enabled() == OutputDevice::Enablement::Enabled)?"enabled":"disabled");
            std::cout<<" "<<dev->pixelSize().width()<<"x"<<dev->pixelSize().height();
            std::cout<<"+"<<dev->globalPosition().x()<<"+"<<dev->globalPosition().y()<<" "<<(dev->refreshRate()/1000.0);
            std::cout<<" "<<int(dev->transform())<<" "<<dev->scaleF()<<" "<<dev->physicalSize().width()<<"x"<<dev->physicalSize().height();
            std::cout<<" "<<dev->uuid().toStdString()<<" "<<dev->manufacturer().toStdString()<<std::endl;
            //            qDebug()<< dev->model().split(" ").first()<<((dev->enabled() == OutputDevice::Enablement::Enabled)?"enabled":"disabled")
            //                   <<dev->pixelSize().width()<<"x"<<dev->pixelSize().height()
            //                  <<"+"<<dev->globalPosition().x()<<"+"<<dev->globalPosition().y()<<(dev->refreshRate()/1000.0)
            //                 <<int(dev->transform())<<dev->scaleF()<<dev->physicalSize().width()<<"x"<<dev->physicalSize().height()
            //                <<dev->uuid()<<dev->manufacturer();

            auto modes = dev->modes();
            for(auto m : modes) {
                std::cout<<"\t" << m.size.width()<<"x"<<m.size.height() << "\t" << m.refreshRate/1000.0;
                std::cout<<((m.flags & modeFlag::Current)?"\tcurrent":"");
                std::cout<<((m.flags & modeFlag::Preferred)?"\tpreferred":"")<<std::endl;
                //                qDebug()<<"\t" << m.size.width()<<"x"<<m.size.height() << "\t" << m.refreshRate/1000.0
                //                       <<((m.flags & modeFlag::Current)?"\tcurrent":"")
                //                      <<((m.flags & modeFlag::Preferred)?"\tpreferred":"");
            }
            std::cout<<std::endl;
            beConnect = true;
        });
        beConnect = true;
    });
}

void set_output(Registry *reg)
{
    QObject::connect(reg, &Registry::outputManagementAnnounced, [reg](quint32 name, quint32 version) {
        qDebug() << "output management announced with name :" << name << " & version :" << version;
        qDebug() << "reg pt :" << reg << "\treg is valid :" << reg->isValid();
        manager = reg->createOutputManagement(name, version);
        if (!manager || !manager->isValid()) {
            qDebug() << "create manager is nullptr or not valid!";
            return;
        }
        conf = manager->createConfiguration();
        if (!conf || !conf->isValid()) {
            qDebug() << "create output configure is null or is not vaild";
            return;
        }
        QObject::connect(reg, &Registry::outputDeviceAnnounced, [reg](quint32 name, quint32 version) {
            beConnect = true;

            auto dev = reg->createOutputDevice(name, version);
            if (!dev || !dev->isValid()) {
                qDebug() << "get dev is null or not valid!";
                return;
            }

            QObject::connect(dev, &OutputDevice::changed, [dev] {
                beConnect = true;

                QString tmode(dev->model());
                QString uuid = tmode.left(tmode.indexOf(' '));
                if (cmd_args->cmd_set.uuid != uuid) {
                    qDebug() << "skip output:" << uuid;
                    return;
                }
                qDebug() << "start set output " << uuid;
                for (auto m : dev->modes()) {
                    if (m.size.width() == cmd_args->cmd_set.width
                            && m.size.height() == cmd_args->cmd_set.height
                            && m.refreshRate == cmd_args->cmd_set.refresh) {
                        qDebug() << "set output mode :" << cmd_args->cmd_set.width << "x" << cmd_args->cmd_set.height
                                 << "and refreshRate :" << cmd_args->cmd_set.refresh;
                        conf->setMode(dev, m.id);
                        break;
                    }
                }
                conf->setPosition(dev, QPoint(cmd_args->cmd_set.x, cmd_args->cmd_set.y));
                conf->setEnabled(dev, OutputDevice::Enablement(cmd_args->cmd_set.enabled));
                qDebug() << "set output transform to " << cmd_args->cmd_set.transform;
                conf->setTransform(dev, OutputDevice::Transform(cmd_args->cmd_set.transform));
                conf->apply();
                if (dev)
                    dev->deleteLater();
            });
        });
        QObject::connect(conf, &OutputConfiguration::applied, []() {
            qDebug() << "Configuration applied!";
            conf->deleteLater();
            beConnect = true;
        });
        QObject::connect(conf, &OutputConfiguration::failed, []() {
            qDebug() << "Configuration failed!";
            conf->deleteLater();
            beConnect = true;
        });
        beConnect = true;
    });
}

// command:
//          get
//          set output eanble x y width height refresh transform
int main(int argc, char *argv[])
{
    if (argc < 2 ||
            (QString(argv[1]).compare("set") == 0 && argc != 10) ||
            (QString(argv[1]).compare("get") == 0 && argc != 2)) {
        qDebug()<<"Usage: "<<argv[0]<<"<get>/<set <uuid> <enable> <x> <y> <width> <height> <refresh> <transform>>"<<endl;
        return -1;
    }

    parse_arguments(argv);

    QCoreApplication app(argc, argv);

    auto conn = new ConnectionThread;
    auto thread = new QThread;
    conn->moveToThread(thread);
    thread->start();

    conn->initConnection();

    Registry *reg = nullptr;
    QObject::connect(conn, &ConnectionThread::connected, [ & ] {

        reg = new Registry;
        reg->create(conn);
        reg->setup();

        if (cmd_args->cmd_get) {
            dump_outputs(reg);
        } else {
            set_output(reg);
        }

        QObject::connect(reg, &Registry::outputDeviceRemoved, [](quint32 name) {
            qDebug() << "output device removed with name: " << name;
            beConnect = true;
        });

        do {
            beConnect = false;
            conn->roundtrip();
        } while (beConnect);

        if (conn) {
            conn->deleteLater();
            thread->quit();
            thread->wait();
        }
        if (reg)
            reg->deleteLater();
        if (conf)
            conf->deleteLater();


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
    QObject::connect(conn, &ConnectionThread::connectionDied, [ & ] {
        qDebug() << "connect failed to wayland at socket:" << conn->socketName();
        free_command_argument(cmd_args);

        if (reg)
            reg->deleteLater();
        if (conf)
            conf->deleteLater();
        if (conf)
            conf->deleteLater();

        exit(-1);
    });

    return app.exec();
}
