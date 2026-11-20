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
class QToolBar;
class QSpinBox;
class QDoubleSpinBox;
class QLabel;
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

/// Ribbon of the editor and of the viewer: the commands of the application are
/// arranged into tabs and groups with labels and descriptions, as in the office
/// applications (File, Home, Insert, Edit, Comments, References, Page, Tools,
/// View, Help). The ribbon uses the actions of the application and of the
/// plugins, it doesn't implement the commands itself, except the controls of
/// the font and of the paragraph, which drive the page content editor. Commands,
/// which the application doesn't have (the viewer can't edit), are left out,
/// together with the groups and the pages, which remain empty.
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

    /// Places the ribbon into the main window: the ribbon replaces the menu bar
    /// and the toolbars (the menus stay available for their shortcuts).
    /// \param classicToolBar Main toolbar of the window, the ribbon is placed before it
    void installIntoMainWindow(QToolBar* classicToolBar);

    /// Places the page number and the zoom into the status bar, as in the office applications
    void createStatusBarControls(QSpinBox* pageNumberSpinBox, QLabel* pageNumberLabel, QDoubleSpinBox* zoomSpinBox);

private:
    QAction* getAction(int action) const;
    QList<QAction*> getGroupActions(int actionGroup) const;
    QIcon getIcon(const QString& fileName) const;
    QToolButton* createStatusBarButton(QAction* action);
    void hideClassicToolBars();
    void updateDrawnIcons();
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

    QToolButton* createTextStyleButton(const QString& title, const QString& description, bool checkable);

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
    PDFRibbonPage* m_homePage;
    QToolBar* m_ribbonToolBar;

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
