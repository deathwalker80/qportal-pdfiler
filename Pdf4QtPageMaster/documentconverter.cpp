// MIT License
//
// Copyright (c) 2018-2026 Jakub Melka and Contributors
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "documentconverter.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QPageLayout>
#include <QPageSize>
#include <QPdfWriter>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QUrl>
#include <QMarginsF>

#include <map>
#include <memory>

namespace pdfpagemaster
{

bool DocumentConverter::isConvertible(const QString& suffix)
{
    return getKind(suffix) != Kind::None;
}

QStringList DocumentConverter::getConvertibleSuffixes()
{
    return { "doc", "docx", "docm", "dot", "dotx", "rtf", "odt", "ott", "wps",
             "xls", "xlsx", "xlsm", "xlsb", "xlt", "xltx", "ods", "csv",
             "ppt", "pptx", "pptm", "pps", "ppsx", "pot", "potx", "odp",
             "odg", "vsd", "vsdx", "pub",
             "txt", "log", "md", "htm", "html" };
}

DocumentConverter::Kind DocumentConverter::getKind(const QString& suffix)
{
    static const std::map<QString, Kind> kinds =
    {
        { "txt", Kind::Text }, { "log", Kind::Text }, { "md", Kind::Text }, { "htm", Kind::Text }, { "html", Kind::Text },
        { "doc", Kind::TextDocument }, { "docx", Kind::TextDocument }, { "docm", Kind::TextDocument }, { "dot", Kind::TextDocument },
        { "dotx", Kind::TextDocument }, { "rtf", Kind::TextDocument }, { "odt", Kind::TextDocument }, { "ott", Kind::TextDocument },
        { "wps", Kind::TextDocument },
        { "xls", Kind::Spreadsheet }, { "xlsx", Kind::Spreadsheet }, { "xlsm", Kind::Spreadsheet }, { "xlsb", Kind::Spreadsheet },
        { "xlt", Kind::Spreadsheet }, { "xltx", Kind::Spreadsheet }, { "ods", Kind::Spreadsheet }, { "csv", Kind::Spreadsheet },
        { "ppt", Kind::Presentation }, { "pptx", Kind::Presentation }, { "pptm", Kind::Presentation }, { "pps", Kind::Presentation },
        { "ppsx", Kind::Presentation }, { "pot", Kind::Presentation }, { "potx", Kind::Presentation }, { "odp", Kind::Presentation },
        { "odg", Kind::Drawing }, { "vsd", Kind::Drawing }, { "vsdx", Kind::Drawing }, { "pub", Kind::Drawing }
    };

    auto it = kinds.find(suffix.toLower());
    return it != kinds.cend() ? it->second : Kind::None;
}

QString DocumentConverter::getOutputFileName(const QString& fileName)
{
    // Converted documents are deleted, when the application is closed. Each
    // document has its own directory, so the name of the document is kept.
    static std::unique_ptr<QTemporaryDir> temporaryDirectory;
    static int counter = 0;

    if (!temporaryDirectory)
    {
        temporaryDirectory = std::make_unique<QTemporaryDir>();
    }

    if (!temporaryDirectory->isValid())
    {
        return QString();
    }

    const QString directoryName = QString::number(++counter);
    QDir directory(temporaryDirectory->path());
    if (!directory.mkpath(directoryName))
    {
        return QString();
    }

    return directory.filePath(QString("%1/%2.pdf").arg(directoryName, QFileInfo(fileName).completeBaseName()));
}

QString DocumentConverter::convertToPdf(const QString& fileName, QString* errorMessage)
{
    const Kind kind = getKind(QFileInfo(fileName).suffix());
    const QString outputFileName = getOutputFileName(fileName);

    if (kind == Kind::None || outputFileName.isEmpty())
    {
        *errorMessage = tr("File '%1' cannot be converted to PDF.").arg(QFileInfo(fileName).fileName());
        return QString();
    }

    bool ok = false;
    if (kind == Kind::Text)
    {
        ok = convertText(fileName, outputFileName, errorMessage);
    }
    else
    {
        QString microsoftOfficeError;
        QString libreOfficeError;

#ifdef Q_OS_WIN
        if (kind != Kind::Drawing)
        {
            ok = convertByMicrosoftOffice(kind, fileName, outputFileName, &microsoftOfficeError);
        }
#endif

        if (!ok)
        {
            ok = convertByLibreOffice(fileName, outputFileName, &libreOfficeError);
        }

        if (!ok)
        {
            *errorMessage = tr("File '%1' cannot be converted to PDF. The conversion needs Microsoft Office or LibreOffice installed on this computer.").arg(QFileInfo(fileName).fileName());

            for (const QString& detail : { microsoftOfficeError, libreOfficeError })
            {
                if (!detail.isEmpty())
                {
                    *errorMessage += QString("\n\n%1").arg(detail);
                }
            }
        }
    }

    if (ok && !(QFileInfo::exists(outputFileName) && QFileInfo(outputFileName).size() > 0))
    {
        ok = false;
        *errorMessage = tr("File '%1' was not converted to PDF (no output was created).").arg(QFileInfo(fileName).fileName());
    }

    return ok ? outputFileName : QString();
}

bool DocumentConverter::convertText(const QString& fileName, const QString& outputFileName, QString* errorMessage)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly))
    {
        *errorMessage = tr("File '%1' cannot be read. %2").arg(QFileInfo(fileName).fileName(), file.errorString());
        return false;
    }

    const QString text = QString::fromUtf8(file.readAll());
    file.close();

    QPdfWriter writer(outputFileName);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setPageMargins(QMarginsF(20.0, 20.0, 20.0, 20.0), QPageLayout::Millimeter);
    writer.setResolution(300);
    writer.setTitle(QFileInfo(fileName).completeBaseName());

    QTextDocument document;
    const QString suffix = QFileInfo(fileName).suffix().toLower();
    if (suffix == "htm" || suffix == "html")
    {
        // Images and styles of the web page are loaded from its directory only
        document.setBaseUrl(QUrl::fromLocalFile(QFileInfo(fileName).absolutePath() + "/"));
        document.setHtml(text);
    }
    else if (suffix == "md")
    {
        document.setMarkdown(text);
    }
    else
    {
        QFont font("Courier New");
        font.setStyleHint(QFont::Monospace);
        font.setPointSize(10);
        document.setDefaultFont(font);
        document.setPlainText(text);
    }

    document.print(&writer);
    return true;
}

bool DocumentConverter::convertByMicrosoftOffice(Kind kind, const QString& fileName, const QString& outputFileName, QString* errorMessage)
{
    // The office application is driven by its automation interface. The script is passed
    // as a command (not as a file), the file names are passed by the environment, so no
    // quoting of the file names is needed.
    QString script;
    switch (kind)
    {
        case Kind::TextDocument:
            script = "$a = New-Object -ComObject Word.Application; "
                     "try { $a.Visible = $false; $a.DisplayAlerts = 0; "
                     "$d = $a.Documents.Open($env:PDFILER_INPUT, $false, $true); "
                     "$d.ExportAsFixedFormat($env:PDFILER_OUTPUT, 17); $d.Close($false) } "
                     "finally { $a.Quit() }";
            break;

        case Kind::Spreadsheet:
            script = "$a = New-Object -ComObject Excel.Application; "
                     "try { $a.Visible = $false; $a.DisplayAlerts = $false; "
                     "$w = $a.Workbooks.Open($env:PDFILER_INPUT, 0, $true); "
                     "$w.ExportAsFixedFormat(0, $env:PDFILER_OUTPUT); $w.Close($false) } "
                     "finally { $a.Quit() }";
            break;

        case Kind::Presentation:
            script = "$a = New-Object -ComObject PowerPoint.Application; "
                     "try { $p = $a.Presentations.Open($env:PDFILER_INPUT, -1, 0, 0); "
                     "$p.SaveAs($env:PDFILER_OUTPUT, 32); $p.Close() } "
                     "finally { $a.Quit() }";
            break;

        default:
            return false;
    }

    script = "$ErrorActionPreference = 'Stop'; " + script;

    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert("PDFILER_INPUT", QDir::toNativeSeparators(fileName));
    environment.insert("PDFILER_OUTPUT", QDir::toNativeSeparators(outputFileName));

    QProcess process;
    process.setProcessEnvironment(environment);
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start("powershell.exe", { "-NoProfile", "-NonInteractive", "-Command", script });

    if (!process.waitForStarted(10000))
    {
        *errorMessage = tr("Microsoft Office: %1").arg(process.errorString());
        return false;
    }

    if (!process.waitForFinished(180000))
    {
        process.kill();
        process.waitForFinished(5000);
        *errorMessage = tr("Microsoft Office: the conversion took too long and was stopped.");
        return false;
    }

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
    {
        *errorMessage = tr("Microsoft Office: %1").arg(QString::fromLocal8Bit(process.readAll()).trimmed());
        return false;
    }

    return true;
}

QString DocumentConverter::findLibreOffice()
{
    for (const QString& program : { QString("soffice"), QString("libreoffice") })
    {
        const QString path = QStandardPaths::findExecutable(program);
        if (!path.isEmpty())
        {
            return path;
        }
    }

#ifdef Q_OS_WIN
    for (const char* variable : { "ProgramFiles", "ProgramFiles(x86)", "ProgramW6432" })
    {
        const QString programFiles = qEnvironmentVariable(variable);
        if (!programFiles.isEmpty())
        {
            const QString path = QDir(programFiles).filePath("LibreOffice/program/soffice.exe");
            if (QFileInfo::exists(path))
            {
                return path;
            }
        }
    }
#endif

    return QString();
}

bool DocumentConverter::convertByLibreOffice(const QString& fileName, const QString& outputFileName, QString* errorMessage)
{
    const QString program = findLibreOffice();
    if (program.isEmpty())
    {
        *errorMessage = tr("LibreOffice: not installed.");
        return false;
    }

    // LibreOffice writes the file <base name of the source>.pdf into the output directory.
    // It runs with its own profile, so it is not blocked by a running instance of the user.
    const QString outputDirectory = QFileInfo(outputFileName).absolutePath();
    QTemporaryDir profileDirectory;

    QStringList arguments = { "--headless", "--norestore", "--nolockcheck", "--convert-to", "pdf", "--outdir", outputDirectory };
    if (profileDirectory.isValid())
    {
        arguments.prepend(QString("-env:UserInstallation=%1").arg(QString::fromUtf8(QUrl::fromLocalFile(profileDirectory.path()).toEncoded())));
    }
    arguments << fileName;

    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(program, arguments);

    if (!process.waitForStarted(10000))
    {
        *errorMessage = tr("LibreOffice: %1").arg(process.errorString());
        return false;
    }

    if (!process.waitForFinished(180000))
    {
        process.kill();
        process.waitForFinished(5000);
        *errorMessage = tr("LibreOffice: the conversion took too long and was stopped.");
        return false;
    }

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0 || !QFileInfo::exists(outputFileName))
    {
        *errorMessage = tr("LibreOffice: %1").arg(QString::fromLocal8Bit(process.readAll()).trimmed());
        return false;
    }

    return true;
}

}   // namespace pdfpagemaster
