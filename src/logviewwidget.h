/*
    Copyright (C) 2009  Matteo Agostinelli <agostinelli@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef LOGVIEWWIDGET_H
#define LOGVIEWWIDGET_H

#include <QDockWidget>
#include <KTextEdit>

class LogViewWidget : public QDockWidget
{
    Q_OBJECT
public:
    LogViewWidget ( const QString & title, QWidget* parent = 0, Qt::WindowFlags flags = 0 );
    
public slots:
    void displayError(const QString& app, const QString& msg);
    void displayMessage(const QString& app, const QString& msg);
    void clear();
    
private:
    KTextEdit* m_textEdit;
};

#endif // LOGVIEWWIDGET_H