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

#ifndef PDFEDITORRIBBON_H
#define PDFEDITORRIBBON_H

#include "pdfviewerglobal.h"

#include <QObject>
#include <QColor>
#include <QPointer>

class QMainWindow;
class QComboBox;
class QFontComboBox;
class QToolButton;
class QAction;
class QIcon;

namespace pdf
{
class PDFPageContentEditorWidget;
}

namespace pdfviewer
{
class PDFRibbon;
class PDFRibbonPage;
class PDFActionManager;
class PDFProgramController;

/// Ribbon of the editor: the commands of the application are arranged into
/// tabs and groups with labels and descriptions, as in the office
/// applications (File, Home, Insert, Edit, Comments, References, Page, Tools,
/// View, Help). The ribbon uses the actions of the application and of the
/// plugins, it doesn't implement the commands itself, except the controls of
/// the font and of the paragraph, which drive the page content editor.
class PDFEditorRibbon : public QObject
{
    Q_OBJECT

public:
    explicit PDFEditorRibbon(QMainWindow* mainWindow,
                             PDFActionManager* actionManager,
                             PDFProgramController* programController,
                             QAction* sidebarAction,
                             QAction* advancedFindAction,
                             QAction* outlineNewItemAction,
                             QWidget* searchWidget);

    PDFRibbon* getRibbon() const { return m_ribbon; }

private:
    QAction* getAction(int action) const;
    QAction* findAction(const QString& objectName) const;
    QList<QAction*> getActions(const QStringList& objectNames) const;

    void createFilePage();
    void createHomePage();
    void createInsertPage();
    void createEditPage();
    void createCommentsPage();
    void createReferencesPage();
    void createPagePage();
    void createToolsPage();
    void createViewPage();
    void createHelpPage();

    void createFontGroup(PDFRibbonPage* page);
    void createParagraphGroup(PDFRibbonPage* page);

    QToolButton* createTextStyleButton(const QIcon& icon, const QString& title, const QString& description, bool checkable);

    /// Returns the page content editor. If the editor is not active and
    /// \p activate is true, the editor is activated first.
    pdf::PDFPageContentEditorWidget* getEditorWidget(bool activate);

    void connectEditorWidget();
    void loadTextStyle();
    void updateTextStyleAvailability();

    void onFontFamilyChanged();
    void onFontSizeChanged();
    void onFontStyleChanged();
    void onTextColorClicked();
    void onAlignmentClicked(Qt::Alignment horizontalAlignment);

    static QIcon createLetterIcon(QChar letter, int fontWeight, bool italic, bool underline, QColor underlineColor);
    static QIcon createAlignmentIcon(Qt::Alignment alignment);

    QMainWindow* m_mainWindow;
    PDFActionManager* m_actionManager;
    PDFProgramController* m_programController;
    QAction* m_sidebarAction;
    QAction* m_advancedFindAction;
    QAction* m_outlineNewItemAction;
    PDFRibbon* m_ribbon;

    QAction* m_editorActivateAction;
    QPointer<pdf::PDFPageContentEditorWidget> m_connectedEditorWidget;
    QFontComboBox* m_fontComboBox;
    QComboBox* m_fontSizeComboBox;
    QToolButton* m_boldButton;
    QToolButton* m_italicButton;
    QToolButton* m_underlineButton;
    QToolButton* m_textColorButton;
    QToolButton* m_alignLeftButton;
    QToolButton* m_alignCenterButton;
    QToolButton* m_alignRightButton;
    QColor m_textColor;
    bool m_isLoadingTextStyle;
};

}   // namespace pdfviewer

#endif // PDFEDITORRIBBON_H
