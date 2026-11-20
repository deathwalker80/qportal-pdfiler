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

#ifndef PDFPAGEMASTER_DOCUMENTCONVERTER_H
#define PDFPAGEMASTER_DOCUMENTCONVERTER_H

#include <QString>
#include <QStringList>
#include <QCoreApplication>

namespace pdfpagemaster
{

/// Converts documents of other formats (text processor, spreadsheet, presentation,
/// plain text, web page) to PDF, so they can be inserted into the workspace as any
/// other PDF document. Plain text, Markdown and web pages are converted by the
/// application itself. Office documents are converted by the office suite installed
/// on the computer (Microsoft Office on Windows, otherwise LibreOffice), nothing is
/// sent out of the computer.
class DocumentConverter
{
    Q_DECLARE_TR_FUNCTIONS(pdfpagemaster::DocumentConverter)

public:
    DocumentConverter() = delete;

    /// Returns true, if the file with the suffix (lower case, without the dot) can be converted to PDF
    static bool isConvertible(const QString& suffix);

    /// Returns the suffixes of all convertible files (lower case, without the dot)
    static QStringList getConvertibleSuffixes();

    /// Converts the file to PDF. Returns the file name of the created PDF document
    /// (in the temporary directory of the application, with the base name of the
    /// source file), or an empty string, if the conversion fails.
    /// \param fileName Source file
    /// \param[out] errorMessage Error message, if the conversion fails
    static QString convertToPdf(const QString& fileName, QString* errorMessage);

private:
    enum class Kind
    {
        None,
        Text,           ///< Converted by the application
        TextDocument,   ///< Text processor
        Spreadsheet,
        Presentation,
        Drawing         ///< Converted by LibreOffice only
    };

    static Kind getKind(const QString& suffix);
    static QString getOutputFileName(const QString& fileName);
    static bool convertText(const QString& fileName, const QString& outputFileName, QString* errorMessage);
    static bool convertByMicrosoftOffice(Kind kind, const QString& fileName, const QString& outputFileName, QString* errorMessage);
    static bool convertByLibreOffice(const QString& fileName, const QString& outputFileName, QString* errorMessage);
    static QString findLibreOffice();
};

}   // namespace pdfpagemaster

#endif // PDFPAGEMASTER_DOCUMENTCONVERTER_H
