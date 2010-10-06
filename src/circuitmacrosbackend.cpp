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

#include "circuitmacrosbackend.h"
#include "command.h"
#include "documenttemplate.h"

#include <KStandardDirs>
#include <KTemporaryFile>
#include <QDir>
#include <QDebug>
#include <QTextStream>
#include <QProcess>

CircuitMacrosGenerator::CircuitMacrosGenerator(const QString& origDir, QObject* parent): GraphicsGenerator(origDir, parent)
{
    createTempSource(".m4");
}

bool CircuitMacrosGenerator::convert(GraphicsGenerator::Format in, GraphicsGenerator::Format out)
{
    // Check if the conversion can be handled by the super-class
    bool done = GraphicsGenerator::convert(in, out);
    if (done) {
        return true;
    }
    
    qDebug() << "Inside the CM backend...";
    
    if (in == Source) {
        m_tempFile->open();
        QTextStream tmpStream(m_tempFile);
        tmpStream << m_source;
        m_tempFile->close();    
        
        QStringList env = QProcess::systemEnvironment();
        env << QString("M4PATH=%1:%2").arg(KStandardDirs::locate("data", "cirkuit/circuit_macros/")).arg(m_origDir->absolutePath());
                
        QStringList m4args;
        m4args << KStandardDirs::locate("data", "cirkuit/circuit_macros/libcct.m4")
               << KStandardDirs::locate("data", "cirkuit/circuit_macros/pstricks.m4")
               << m_tempFileInfo->fileName();
        
        Command* m4command = new Command("m4", "", m4args);
        m4command->setEnvironment(env);
        m4command->setWorkingDirectory(m_workingDir->absolutePath());
        execute(m4command);
        
        QString m4out = m4command->stdout();
        
        QStringList picargs;
        picargs << "-p";
            
        Command* picCommand = new Command("dpic", m4out, picargs);
        execute(picCommand);
            
        QString picout = picCommand->stdout();
       
        DocumentTemplate latexTemplate(KStandardDirs::locate("data", "cirkuit/templates/cm_latex.ckt"));
        QString latexDoc = latexTemplate.insert(picout);
        
        QStringList environment = QProcess::systemEnvironment();
        // the following enviroment variable is needed to find boxdims.sty in the circuit maaros distribution
        QString dirString = QString("TEXINPUTS=.:%1:%2:").arg(KStandardDirs::locate("data", "cirkuit/circuit_macros/")).arg(m_origDir->absolutePath());
        environment << dirString;
        
        QStringList latexArgs;
        latexArgs << QString("-jobname=%1").arg(m_tempFileInfo->baseName());
        
        Command* latexCmd = new Command("latex", latexDoc, latexArgs);
        latexCmd->setWorkingDirectory(m_workingDir->absolutePath());
        latexCmd->setEnvironment(environment);
        execute(latexCmd);
    
        // Now that a DVI has been generated, convert it to the
        // desired output format
        GraphicsGenerator::convert(Dvi, out);
    }
    
    return false;
}
