/*
    Copyright (C) 2009 Alexander Rieder <alexanderrieder@gmail.com>
    Copyright (C) 2011 Matteo Agostinelli <agostinelli@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "backend.h"
#include "document.h"

#include <QStringList>

#include <KUrl>
#include <KDebug>
#include <KServiceTypeTrader>
#include <KService>
#include <KPluginInfo>

class Cirkuit::BackendPrivate {
public:
    QString name;
    QString comment;
    QString icon;
    KUrl url;
};

using namespace Cirkuit;

Backend::Backend(QObject* parent, const QList<QVariant>& args): QObject(parent), d(new BackendPrivate)
{
    Q_UNUSED(args);
}

Backend::~Backend()
{
    delete d;
}


bool Backend::checkRequirements() const
{
    return false;
}

QString Backend::name() const
{
    return d->name;
}

QString Backend::comment() const
{
    return d->comment;
}

QString Backend::description() const
{
    return comment();
}

QString Backend::icon() const
{
    return d->icon;
}

KUrl Backend::url() const
{
    return d->url;
}

KUrl Backend::helpUrl() const
{
    return KUrl();
}

QWidget* Backend::settingsWidget(QWidget* parent) const
{
    Q_UNUSED(parent)
    return 0;
}

KConfigSkeleton* Backend::config() const
{
    return 0;
}

static QList<Backend*> backendCache;

QStringList Backend::listAvailableBackends()
{
    QList<Backend* > backends=availableBackends();
    QStringList l;
    foreach (Backend* b, backends) {
        if (b->checkRequirements()) {
            l << b->name();
        }
    }

    return l;
}

QList<Backend*> Backend::availableBackends()
{
    //if we already have all backends Cached, just return the cache.
    //otherwise create the available backends
    if (!backendCache.isEmpty()) {
        return backendCache;
    }

    KService::List services;
    KServiceTypeTrader* trader = KServiceTypeTrader::self();

    services = trader->query("Cirkuit/Backend");
    
    KService::List::const_iterator iter;
    for (iter = services.begin(); iter < services.end(); ++iter) {
        QString error;
        KService::Ptr service = *iter;
        
        KPluginFactory *factory = KPluginLoader(service->library()).factory();
        if (!factory) {
            kError(5001) << "error: " << error;
            continue;    
        }
        
        Backend* backend = factory->create<Backend>(0);
        if (!backend) {
            kDebug() << "error: " << error;
            continue;
        }        
   
        KPluginInfo info(service);
        backend->d->name = info.name();
        backend->d->comment = info.comment();
        backend->d->icon = info.icon();
        backend->d->url = info.website();
        backendCache << backend;
    }
    return backendCache;
}

Backend* Backend::getBackend(const QString& name)
{
    QList<Backend*> backends = availableBackends();
    foreach(Backend* b, backends) {
        if (QString::compare(b->name(), name, Qt::CaseInsensitive) == 0 || QString::compare(b->id(), name, Qt::CaseInsensitive) == 0) {
            return b;
        }
    }

    return 0;
}

Cirkuit::DocumentSettings* Cirkuit::Backend::documentSettings() const
{
    return 0;
}

Cirkuit::Backend* Cirkuit::Backend::autoChooseBackend(Document* doc)
{
    //TODO to be implemented
    Q_UNUSED(doc)
    return 0;
}
