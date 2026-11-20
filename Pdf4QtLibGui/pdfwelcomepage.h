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

#ifndef PDFWELCOMEPAGE_H
#define PDFWELCOMEPAGE_H

#include "pdfviewerglobal.h"

#include <QWidget>

class QAction;
class QIcon;
class QLabel;
class QMainWindow;
class QVBoxLayout;
class QPushButton;

namespace pdfviewer
{
class PDFActionManager;
class PDFProgramController;

/// Start page of the editor, displayed while no document is opened (as the start
/// page of the office applications): new document, open document, tools working
/// with files (create PDF from files, merge, split), recent documents. The page
/// also provides the actions of these commands for the ribbon.
class PDFWelcomePage : public QWidget
{
    Q_OBJECT

public:
    /// \param isEditor True for the editor, false for the viewer (which cannot create or change documents)
    /// \param parent Widget covered by the page (the widget of the document)
    explicit PDFWelcomePage(QMainWindow* mainWindow,
                            PDFActionManager* actionManager,
                            PDFProgramController* programController,
                            bool isEditor,
                            QWidget* parent);

    QAction* getNewDocumentAction() const { return m_newDocumentAction; }
    QAction* getCreatePdfAction() const { return m_createPdfAction; }
    QAction* getMergePdfAction() const { return m_mergePdfAction; }
    QAction* getSplitPdfAction() const { return m_splitPdfAction; }

protected:
    virtual bool eventFilter(QObject* watched, QEvent* event) override;
    virtual void showEvent(QShowEvent* event) override;
    virtual void changeEvent(QEvent* event) override;

private:
    QIcon getIcon(const QString& fileName) const;
    QPushButton* createTile(QAction* action, const QIcon& icon, const QString& title, const QString& description);
    void updateRecentFiles();
    void updateStyleSheet();

    void onNewDocument();
    void onCreatePdf();
    void onMergeOrSplitPdf();

    /// Starts the tool for merging, splitting and creating of PDF documents
    /// \param arguments Arguments of the tool
    void startPdfTool(const QStringList& arguments);

    QMainWindow* m_mainWindow;
    PDFActionManager* m_actionManager;
    PDFProgramController* m_programController;
    QAction* m_newDocumentAction;
    QAction* m_createPdfAction;
    QAction* m_mergePdfAction;
    QAction* m_splitPdfAction;
    QVBoxLayout* m_recentFilesLayout;
    bool m_isUpdatingStyleSheet = false;
};

}   // namespace pdfviewer

#endif // PDFWELCOMEPAGE_H
