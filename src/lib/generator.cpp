/***************************************************************************
*   Copyright (C) 2011 by Matteo Agostinelli                              *
*   agostinelli@gmail.com                                                 *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
***************************************************************************/

#include "generator.h"
#include "backend.h"
#include "format.h"
#include "document.h"
#include "command.h"

#include <QFileInfo>
#include <QDir>

#include <KStandardDirs>
#include <KTemporaryFile>
#include <KIO/NetAccess>

using namespace Cirkuit;

class Cirkuit::GeneratorPrivate {
public:
    GeneratorPrivate() {
        backend = 0;
        tempFile = 0;
        tempFileInfo = 0;
        resolution = 300;
    }
    Backend* backend;
    KTemporaryFile* tempFile;
    QFileInfo* tempFileInfo;
    Document* document;
    int resolution;
};

Cirkuit::Generator::Generator(Cirkuit::Backend* backend, QObject* parent): QObject(parent), d(new GeneratorPrivate)
{
    d->backend = backend;
    createTempFiles();
}

Cirkuit::Generator::~Generator()
{
    delete d;
}

KUrl Cirkuit::Generator::workingDir()
{
    return KUrl::fromPath(KStandardDirs::locateLocal("tmp", "cirkuit/build/", true));
}

void Cirkuit::Generator::createTempFiles(const QString& suffix)
{
    delete d->tempFile;
    delete d->tempFileInfo;
    
    d->tempFile = new KTemporaryFile;
    d->tempFile->setPrefix(workingDir().path(KUrl::AddTrailingSlash));
    d->tempFile->setSuffix(suffix);
    d->tempFile->open();
    
    d->tempFileInfo = new QFileInfo(d->tempFile->fileName());
}

bool Cirkuit::Generator::formatExists(const Cirkuit::Format& format) const
{
    if (d->tempFileInfo == 0) {
        return false;
    }
    
    KUrl formatUrl = workingDir();
    formatUrl.addPath(d->tempFileInfo->baseName() + format.extension());
    return KIO::NetAccess::exists(formatUrl, KIO::NetAccess::SourceSide, 0);
}

QString Cirkuit::Generator::formatPath(const Cirkuit::Format& format) const
{
    KUrl url = workingDir();
    QString filename;
    if (format.type() == Format::Png || format.type() == Format::Jpeg || format.type() == Format::Ppm) {
        filename = QString("%1-1%2").arg(d->tempFileInfo->baseName()).arg(format.extension());
    } else {
        filename = QString("%1%2").arg(d->tempFileInfo->baseName()).arg(format.extension());
    }
    url.addPath(filename);
    return url.path();
}

void Generator::setDocument(Document* doc)
{
    d->document = doc;
}

void Generator::setResolution(int resolution)
{
    d->resolution = resolution;
}

Document* Generator::document() const
{
    return d->document;
}

int Generator::resolution() const
{
    return d->resolution;
}

bool Generator::execute(Cirkuit::Command* c)
{
    c->setWorkingDirectory(workingDir().path());
    connect(c, SIGNAL(newStandardError(QString,QString)), this, SIGNAL(error(QString,QString)));
    kDebug() << "Executing " << c->name() << " with arguments " << c->args();
    
    if (!c->execute()) {
        kDebug() << c->name() << " failed";
        emit fail();
        return false;
    }
    
    kDebug() << c->name() << " executed correctly";
    emit output(c->name(), c->stdOutput());
    return true;
}

bool Cirkuit::Generator::convert(const Cirkuit::Format& in, const Cirkuit::Format& out)
{
    kDebug() << "Inside the converter..." << "in: " << in.type() << " " << in.extension() << ", out: " << out.type() << " " << out.extension();
    
    // this class doesn't know how to convert from source
    if (in == Format::Source || out == Format::Source || out == Format::Dvi) {
        kDebug() << "Cannot convert from or to source/DVI";
        return false;
    }
    
    // Check that input and output formats are different
    if (in == out) {
        return true;
    }
    
    if (out == Format::QtImage) {
        return convert(in, Format::Pdf);
    }
    
    if (in == Format::Dvi) {
        if (out == Format::Postscript) {
            QStringList args;
            args << formatPath(in) << "-q" << QString("-o %1").arg(formatPath(Format::Postscript));
            return execute(new Command("dvips", "", args, this));
        } else if (out == Format::Eps) {
            QStringList args;
            args << "-E" << formatPath(in) << "-q" << "-o" << formatPath(Format::Eps);
            return execute(new Command("dvips", "", args, this));
        } else {
            bool b = true;
            if (!convert(in,Format::Eps)) b = false;
            if (!convert(Format::Eps,out)) b = false;
            return b;
        }
    }
    
    if (in == Format::Postscript) {
        if (out == Format::Eps) {
            QStringList args;
            args << formatPath(in) << formatPath(Format::Eps);
            return execute(new Command("ps2epsi", "", args, this));
        } else if (out == Format::Pdf) {
            QStringList args;
            args << formatPath(in) << formatPath(Format::Pdf);
            return execute(new Command("ps2pdf", "", args, this));
        } else if (out == Format::Png) {
            bool b = true;
            if (!convert(in,Format::Eps)) b = false;
            if (!convert(Format::Eps,Format::Png)) b = false;
            return b;
        } else {
            return false;
        }     
    }
    
    if (in == Format::Eps) {
        if (out == Format::Postscript) {
            QStringList args;
            args << formatPath(in) << formatPath(Format::Postscript);
            return execute(new Command("ps2ps", "", args, this));
        } else if (out == Format::Pdf) {
            QStringList args;
            args << formatPath(in);// << QString("--outfile=%1").arg(formatPath(Pdf));
            return execute(new Command("epstopdf", "", args, this));
        } else {
            bool b = true;
            if (!convert(in,Format::Pdf)) b = false;
            if (!convert(Format::Pdf,out)) b = false;
            return b;
        }   
    }
    
    if (in == Format::Pdf) {
        if (out == Format::Svg) {
            QStringList args;
            args << formatPath(in) << formatPath(Format::Svg);
            return execute(new Command("pdf2svg", "", args, this));
        } else if (out == Format::Png) {
            QStringList args;
            args << "-png" << "-r" << QString::number(d->resolution) << formatPath(in) << d->tempFileInfo->baseName();
            return execute(new Command("pdftoppm", "", args, this));
        } else if (out == Format::Jpeg) {
            QStringList args;
            args << "-jpeg" << "-r" << QString::number(d->resolution) << formatPath(in) << d->tempFileInfo->baseName();
            return execute(new Command("pdftoppm", "", args, this));
        } else if (out == Format::Gif) {
            bool b = true;
            if (!convert(in,Format::Ppm)) b = false;
            if (!convert(Format::Ppm,out)) b = false;
            return b;
        } else if (out == Format::Ppm) {
            QStringList args;
            args << "-r" << QString::number(d->resolution) << formatPath(in) << d->tempFileInfo->baseName();
            return execute(new Command("pdftoppm", "", args, this));
        } else if (out == Format::Eps) {
            QStringList args;
            args << "-eps" << formatPath(in) << formatPath(out);
            return execute(new Command("pdftops", "", args, this));
        } else if (out == Format::Postscript) {
            QStringList args;
            args << formatPath(in) << formatPath(out);
            return execute(new Command("pdftops", "", args, this));
        }
    }

    if (in == Format::Ppm) {
        if (out == Format::Gif) {
            QStringList args;
            args << formatPath(Format::Ppm) << formatPath(Format::Gif);
            return execute(new Command("convert", "", args, this));
        }
    }

    return false;
}

bool Cirkuit::Generator::generate(Document* doc, const Cirkuit::Format& format)
{
    setDocument(doc);
    return convert(Format::Source, format);
}

Cirkuit::Backend* Cirkuit::Generator::backend() const
{
    return d->backend;
}

KTemporaryFile* Cirkuit::Generator::tempFile() const
{
    return d->tempFile;
}

QFileInfo* Cirkuit::Generator::tempFileInfo() const
{
    return d->tempFileInfo;
}
