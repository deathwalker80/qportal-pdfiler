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

#include "pdfeditorribbon.h"
#include "pdfribbon.h"
#include "pdfprogramcontroller.h"
#include "pdfrecentfilemanager.h"
#include "pdfwidgetutils.h"
#include "pdfpagecontenteditorwidget.h"
#include "pdfutils.h"

#include <QMenu>
#include <QLabel>
#include <QTimer>
#include <QPainter>
#include <QPixmap>
#include <QComboBox>
#include <QMainWindow>
#include <QToolButton>
#include <QApplication>
#include <QColorDialog>
#include <QFontComboBox>
#include <QIntValidator>
#include <QActionGroup>

#include "pdfdbgheap.h"

namespace pdfviewer
{

PDFEditorRibbon::PDFEditorRibbon(QMainWindow* mainWindow,
                                 PDFActionManager* actionManager,
                                 PDFProgramController* programController,
                                 QAction* sidebarAction,
                                 QAction* advancedFindAction,
                                 QAction* outlineNewItemAction,
                                 QWidget* searchWidget) :
    QObject(mainWindow),
    m_mainWindow(mainWindow),
    m_actionManager(actionManager),
    m_programController(programController),
    m_sidebarAction(sidebarAction),
    m_advancedFindAction(advancedFindAction),
    m_outlineNewItemAction(outlineNewItemAction),
    m_ribbon(new PDFRibbon(mainWindow)),
    m_editorActivateAction(nullptr),
    m_fontComboBox(nullptr),
    m_fontSizeComboBox(nullptr),
    m_boldButton(nullptr),
    m_italicButton(nullptr),
    m_underlineButton(nullptr),
    m_textColorButton(nullptr),
    m_alignLeftButton(nullptr),
    m_alignCenterButton(nullptr),
    m_alignRightButton(nullptr),
    m_textColor(Qt::black),
    m_isLoadingTextStyle(false)
{
    m_editorActivateAction = findAction("editortool_activateAction");

    m_ribbon->addQuickAccessAction(getAction(PDFActionManager::Save), tr("Save"), tr("Saves the document."));
    m_ribbon->addQuickAccessAction(getAction(PDFActionManager::Undo), tr("Undo"), tr("Undoes the last change of the document."));
    m_ribbon->addQuickAccessAction(getAction(PDFActionManager::Redo), tr("Redo"), tr("Repeats the change, which was undone."));

    if (searchWidget)
    {
        m_ribbon->setCornerWidget(searchWidget);
    }

    createFilePage();
    createHomePage();
    createInsertPage();
    createEditPage();
    createCommentsPage();
    createReferencesPage();
    createPagePage();
    createToolsPage();
    createViewPage();
    createHelpPage();

    m_ribbon->finish();

    // Home page is opened at the start, as in the office applications
    m_ribbon->setCurrentPage(1);

    if (m_editorActivateAction)
    {
        connect(m_editorActivateAction, &QAction::changed, this, &PDFEditorRibbon::updateTextStyleAvailability);
        connect(m_editorActivateAction, &QAction::toggled, this, [this]()
        {
            // The tool box of the editor is created when the editor is activated
            QTimer::singleShot(0, this, &PDFEditorRibbon::connectEditorWidget);
        });
    }

    updateTextStyleAvailability();
}

QAction* PDFEditorRibbon::getAction(int action) const
{
    return m_actionManager->getAction(PDFActionManager::Action(action));
}

QAction* PDFEditorRibbon::findAction(const QString& objectName) const
{
    for (QAction* action : m_actionManager->getActions())
    {
        if (action && action->objectName() == objectName)
        {
            return action;
        }
    }

    return nullptr;
}

QList<QAction*> PDFEditorRibbon::getActions(const QStringList& objectNames) const
{
    QList<QAction*> actions;
    for (const QString& objectName : objectNames)
    {
        if (QAction* action = findAction(objectName))
        {
            actions << action;
        }
    }
    return actions;
}

void PDFEditorRibbon::createFilePage()
{
    PDFRibbonPage* page = m_ribbon->addPage(tr("File"));

    PDFRibbonGroup* documentGroup = page->addGroup(tr("Document"));
    documentGroup->addLargeAction(getAction(PDFActionManager::Open), tr("Open"), tr("Opens a PDF document from the computer or from a network folder."));

    QList<QAction*> recentFileActions;
    for (QAction* action : m_programController->getRecentFileManager()->getActions())
    {
        recentFileActions << action;
    }
    recentFileActions << getAction(PDFActionManager::ClearRecentFileHistory);
    documentGroup->addLargeMenu(QIcon(":/resources/book.svg"), tr("Recent"), tr("Opens one of the documents, which were opened recently."), recentFileActions);

    documentGroup->addLargeAction(getAction(PDFActionManager::Save), tr("Save"), tr("Saves the changes into the document."));
    documentGroup->addSmallAction(getAction(PDFActionManager::SaveAs), tr("Save As"), tr("Saves the document into a new file, the original file stays unchanged."));
    documentGroup->addSmallAction(getAction(PDFActionManager::Close), tr("Close"), tr("Closes the document."));
    documentGroup->addSmallAction(getAction(PDFActionManager::AutomaticDocumentRefresh), tr("Automatic Refresh"), tr("Reloads the document automatically, when the file is changed by another program."));

    PDFRibbonGroup* shareGroup = page->addGroup(tr("Print and Share"));
    shareGroup->addLargeAction(getAction(PDFActionManager::Print), tr("Print"), tr("Prints the document on a printer."));
    shareGroup->addLargeAction(getAction(PDFActionManager::SendByMail), tr("Send by E-Mail"), tr("Creates a new e-mail message with the document as an attachment."));
    shareGroup->addLargeAction(getAction(PDFActionManager::RenderToImages), tr("Export as Images"), tr("Saves the pages of the document as image files (PNG, JPEG, TIFF and others)."), QIcon(":/resources/extract-image.svg"));

    PDFRibbonGroup* infoGroup = page->addGroup(tr("Information"));
    infoGroup->addLargeAction(getAction(PDFActionManager::Properties), tr("Properties"), tr("Shows the properties of the document: title, author, page size, fonts, security."));
    infoGroup->addLargeAction(getAction(PDFActionManager::Encryption), tr("Protect Document"), tr("Protects the document by a password and sets the permissions (printing, copying, changes)."));

    PDFRibbonGroup* applicationGroup = page->addGroup(tr("Application"));
    applicationGroup->addLargeAction(getAction(PDFActionManager::Options), tr("Options"), tr("Settings of the application: display, rendering, security, shortcuts, plugins."));
    applicationGroup->addLargeAction(getAction(PDFActionManager::Quit), tr("Exit"), tr("Closes the application."));
}

void PDFEditorRibbon::createHomePage()
{
    PDFRibbonPage* page = m_ribbon->addPage(tr("Home"));

    PDFRibbonGroup* clipboardGroup = page->addGroup(tr("Clipboard"));
    clipboardGroup->addLargeAction(getAction(PDFActionManager::CopyText), tr("Copy"), tr("Copies the selected text to the clipboard. Select the text first by the Select Text tool."));
    clipboardGroup->addSmallAction(getAction(PDFActionManager::SelectTextAll), tr("Select All"), tr("Selects all the text of the document."));
    clipboardGroup->addSmallAction(getAction(PDFActionManager::DeselectText), tr("Deselect"), tr("Cancels the selection of the text."));

    PDFRibbonGroup* undoGroup = page->addGroup(tr("Undo"));
    undoGroup->addSmallAction(getAction(PDFActionManager::Undo), tr("Undo"), tr("Undoes the last change of the document."));
    undoGroup->addSmallAction(getAction(PDFActionManager::Redo), tr("Redo"), tr("Repeats the change, which was undone."));

    createFontGroup(page);
    createParagraphGroup(page);

    PDFRibbonGroup* selectionGroup = page->addGroup(tr("Selection"));
    selectionGroup->addLargeAction(getAction(PDFActionManager::ToolSelectText), tr("Select Text"), tr("Selects the text of the page by the mouse, the selected text can be copied."));
    selectionGroup->addSmallAction(getAction(PDFActionManager::ToolSelectTable), tr("Select Table"), tr("Selects a table on the page and copies its cells (for example into a spreadsheet)."));
    selectionGroup->addSmallAction(getAction(PDFActionManager::ToolScreenshot), tr("Snapshot"), tr("Copies a rectangular area of the page as an image into the clipboard."));
    selectionGroup->addSmallAction(getAction(PDFActionManager::ToolExtractImage), tr("Extract Image"), tr("Saves an image of the page into a file or copies it into the clipboard."));

    PDFRibbonGroup* highlightGroup = page->addGroup(tr("Highlight"));
    highlightGroup->addLargeAction(getAction(PDFActionManager::CreateTextHighlight), tr("Highlight"), tr("Highlights the text, which is selected by the mouse, as by a highlighter."));
    highlightGroup->addSmallAction(getAction(PDFActionManager::CreateTextUnderline), tr("Underline"), tr("Underlines the text, which is selected by the mouse."));
    highlightGroup->addSmallAction(getAction(PDFActionManager::CreateTextStrikeout), tr("Strikeout"), tr("Strikes out the text, which is selected by the mouse."));
    highlightGroup->addSmallAction(getAction(PDFActionManager::CreateTextSquiggly), tr("Squiggly"), tr("Underlines the text, which is selected by the mouse, by a wavy line."));

    PDFRibbonGroup* editGroup = page->addGroup(tr("Edit"));
    editGroup->addLargeAction(m_editorActivateAction, tr("Edit Text and Images"), tr("Turns on the editing of the content of the pages: texts, images and graphics can be selected, moved, changed and deleted. Double-click an item to change it."));

    PDFRibbonGroup* findGroup = page->addGroup(tr("Find"));
    findGroup->addLargeAction(getAction(PDFActionManager::Find), tr("Find"), tr("Finds a word or a phrase in the document."));
    findGroup->addSmallAction(getAction(PDFActionManager::FindPrevious), tr("Previous"), tr("Goes to the previous occurrence of the searched text."));
    findGroup->addSmallAction(getAction(PDFActionManager::FindNext), tr("Next"), tr("Goes to the next occurrence of the searched text."));
    findGroup->addSmallAction(m_advancedFindAction, tr("Advanced Find"), tr("Finds the text with more options (whole words, case sensitivity, regular expressions) and lists all occurrences."));
}

void PDFEditorRibbon::createInsertPage()
{
    PDFRibbonPage* page = m_ribbon->addPage(tr("Insert"));

    PDFRibbonGroup* pagesGroup = page->addGroup(tr("Pages"));
    pagesGroup->addLargeAction(getAction(PDFActionManager::InsertPageNumbers), tr("Page Numbers"), tr("Inserts the numbers of the pages (header or footer) into the document."));

    PDFRibbonGroup* textGroup = page->addGroup(tr("Text"));
    textGroup->setPrerequisite(m_editorActivateAction);
    textGroup->addLargeAction(findAction("editortool_createTextAction"), tr("Text Box"), tr("Inserts a text into the page. Draw a rectangle by the mouse and write the text. The text becomes a part of the page."));
    textGroup->setPrerequisite(nullptr);
    textGroup->addSmallAction(getAction(PDFActionManager::CreateInlineText), tr("Text Comment"), tr("Inserts a text as a comment, which is displayed over the page and can be removed later."));

    PDFRibbonGroup* illustrationsGroup = page->addGroup(tr("Illustrations"));
    illustrationsGroup->setPrerequisite(m_editorActivateAction);
    illustrationsGroup->addLargeAction(findAction("editortool_createSvgImageAction"), tr("Image"), tr("Inserts an image into the page. Draw a rectangle by the mouse and select the image file."));
    illustrationsGroup->addLargeMenu(QIcon(":/pdfplugins/editorplugin/create-rectangle.svg"), tr("Shapes"), tr("Inserts a shape into the page: rectangle, line, point or freehand drawing."),
                                     getActions({ "editortool_createRectangleAction", "editortool_createRoundedRectangleAction", "editortool_createHorizontalLineAction",
                                                  "editortool_createVerticalLineAction", "editortool_createLineAction", "editortool_createDotAction",
                                                  "editortool_createFreehandCurveAction" }));
    illustrationsGroup->addSmallAction(findAction("editortool_createAcceptMarkAction"), tr("Check Mark"), tr("Inserts a check mark into the page (for example into a check box of a form)."));
    illustrationsGroup->addSmallAction(findAction("editortool_createRejectMarkAction"), tr("Cross Mark"), tr("Inserts a cross mark into the page."));
    illustrationsGroup->addSmallAction(findAction("editortool_createMultipleElementsAction"), tr("Insert Repeatedly"), tr("When it is turned on, the selected tool stays active and more items can be inserted one after another."));

    PDFRibbonGroup* linksGroup = page->addGroup(tr("Links"));
    linksGroup->addLargeAction(getAction(PDFActionManager::CreateHyperlink), tr("Link"), tr("Inserts a link to a web page or to a file. Draw a rectangle over the text or the image, which will be the link."));
    linksGroup->addLargeMenu(QIcon(":/resources/hyperlink.svg"), tr("Link in Document"), tr("Inserts a link to another place of this document."),
                             { getAction(PDFActionManager::CreateHyperlinkToThisPDFFit), getAction(PDFActionManager::CreateHyperlinkToThisPDFFitH),
                               getAction(PDFActionManager::CreateHyperlinkToThisPDFFitV), getAction(PDFActionManager::CreateHyperlinkToThisPDFFitR),
                               getAction(PDFActionManager::CreateHyperlinkToThisPDFFitB), getAction(PDFActionManager::CreateHyperlinkToThisPDFFitBH),
                               getAction(PDFActionManager::CreateHyperlinkToThisPDFFitBV), getAction(PDFActionManager::CreateHyperlinkToThisPDFXYZ),
                               getAction(PDFActionManager::CreateHyperlinkToThisPDFXYZInheritZoom) });
    linksGroup->addSmallAction(m_outlineNewItemAction, tr("Outline Item"), tr("Inserts a new item of the outline (table of contents in the sidebar), which points to the current place of the document."));

    PDFRibbonGroup* commentsGroup = page->addGroup(tr("Comments"));
    commentsGroup->addLargeMenu(QIcon(":/resources/annot-sticky-note.svg"), tr("Note"), tr("Inserts a note (sticky note) into the page. Click on the place of the note."),
                                m_actionManager->getActionGroup(PDFActionManager::CreateStickyNoteGroup)->actions());
    commentsGroup->addLargeMenu(QIcon(":/resources/bubble.svg"), tr("Stamp"), tr("Inserts a stamp (Approved, Draft, Confidential, ...) into the page."),
                                m_actionManager->getActionGroup(PDFActionManager::CreateStampGroup)->actions());
}

void PDFEditorRibbon::createEditPage()
{
    PDFRibbonPage* page = m_ribbon->addPage(tr("Edit"));

    PDFRibbonGroup* editGroup = page->addGroup(tr("Edit"));
    editGroup->addLargeAction(m_editorActivateAction, tr("Edit Text and Images"), tr("Turns on the editing of the content of the pages: texts, images and graphics can be selected, moved, changed and deleted. Double-click an item to change it."));
    editGroup->setPrerequisite(m_editorActivateAction);
    editGroup->addSmallAction(findAction("editortool_undoAction"), tr("Undo Edit"), tr("Undoes the last change made by the editing of the content."));
    editGroup->addSmallAction(findAction("editortool_redoAction"), tr("Redo Edit"), tr("Repeats the change of the content, which was undone."));
    editGroup->addSmallAction(findAction("editortool_clearAction"), tr("Remove New Items"), tr("Removes all items inserted by the editing, which were not saved yet."));

    PDFRibbonGroup* insertGroup = page->addGroup(tr("Insert"));
    insertGroup->setPrerequisite(m_editorActivateAction);
    insertGroup->addLargeAction(findAction("editortool_createTextAction"), tr("Text Box"), tr("Inserts a text into the page. Draw a rectangle by the mouse and write the text. The text becomes a part of the page."));
    insertGroup->addLargeAction(findAction("editortool_createSvgImageAction"), tr("Image"), tr("Inserts an image into the page. Draw a rectangle by the mouse and select the image file."));
    insertGroup->addLargeMenu(QIcon(":/pdfplugins/editorplugin/create-rectangle.svg"), tr("Shapes"), tr("Inserts a shape into the page: rectangle, line, point or freehand drawing."),
                              getActions({ "editortool_createRectangleAction", "editortool_createRoundedRectangleAction", "editortool_createHorizontalLineAction",
                                           "editortool_createVerticalLineAction", "editortool_createLineAction", "editortool_createDotAction",
                                           "editortool_createFreehandCurveAction" }));

    PDFRibbonGroup* pageGroup = page->addGroup(tr("Page"));
    pageGroup->addLargeAction(getAction(PDFActionManager::PageGeometry), tr("Page Size"), tr("Changes the size, the margins and the cropping of the pages."));
    pageGroup->addSmallAction(getAction(PDFActionManager::RotateLeft), tr("Rotate Left"), tr("Rotates the view of the pages by 90 degrees to the left."));
    pageGroup->addSmallAction(getAction(PDFActionManager::RotateRight), tr("Rotate Right"), tr("Rotates the view of the pages by 90 degrees to the right."));
}

void PDFEditorRibbon::createCommentsPage()
{
    PDFRibbonPage* page = m_ribbon->addPage(tr("Comments"));

    PDFRibbonGroup* notesGroup = page->addGroup(tr("Notes"));
    notesGroup->addLargeMenu(QIcon(":/resources/annot-sticky-note.svg"), tr("New Note"), tr("Inserts a note (sticky note) into the page. Click on the place of the note."),
                             m_actionManager->getActionGroup(PDFActionManager::CreateStickyNoteGroup)->actions());
    notesGroup->addLargeAction(getAction(PDFActionManager::CreateInlineText), tr("Text Comment"), tr("Inserts a text as a comment, which is displayed over the page and can be removed later."));
    notesGroup->addLargeMenu(QIcon(":/resources/bubble.svg"), tr("Stamp"), tr("Inserts a stamp (Approved, Draft, Confidential, ...) into the page."),
                             m_actionManager->getActionGroup(PDFActionManager::CreateStampGroup)->actions());

    PDFRibbonGroup* markupGroup = page->addGroup(tr("Text Markup"));
    markupGroup->addLargeAction(getAction(PDFActionManager::CreateTextHighlight), tr("Highlight"), tr("Highlights the text, which is selected by the mouse, as by a highlighter."));
    markupGroup->addSmallAction(getAction(PDFActionManager::CreateTextUnderline), tr("Underline"), tr("Underlines the text, which is selected by the mouse."));
    markupGroup->addSmallAction(getAction(PDFActionManager::CreateTextStrikeout), tr("Strikeout"), tr("Strikes out the text, which is selected by the mouse."));
    markupGroup->addSmallAction(getAction(PDFActionManager::CreateTextSquiggly), tr("Squiggly"), tr("Underlines the text, which is selected by the mouse, by a wavy line."));

    PDFRibbonGroup* drawingGroup = page->addGroup(tr("Drawing"));
    drawingGroup->addSmallAction(getAction(PDFActionManager::CreateStraightLine), tr("Line"), tr("Draws a line as a comment."));
    drawingGroup->addSmallAction(getAction(PDFActionManager::CreatePolyline), tr("Polyline"), tr("Draws a broken line as a comment. Finish it by a double-click."));
    drawingGroup->addSmallAction(getAction(PDFActionManager::CreateFreehandCurve), tr("Freehand"), tr("Draws by the mouse as by a pen, as a comment."));
    drawingGroup->addSmallAction(getAction(PDFActionManager::CreateRectangle), tr("Rectangle"), tr("Draws a rectangle as a comment."));
    drawingGroup->addSmallAction(getAction(PDFActionManager::CreateEllipse), tr("Ellipse"), tr("Draws an ellipse as a comment."));
    drawingGroup->addSmallAction(getAction(PDFActionManager::CreatePolygon), tr("Polygon"), tr("Draws a polygon as a comment. Finish it by a double-click."));

    PDFRibbonGroup* deleteGroup = page->addGroup(tr("Delete"));
    deleteGroup->addLargeAction(getAction(PDFActionManager::DeleteAnnotation), tr("Delete Comments"), tr("Deletes the selected comments, notes, highlights and stamps."), QIcon(":/resources/close.svg"));

    PDFRibbonGroup* measureGroup = page->addGroup(tr("Measure"));
    measureGroup->addLargeMenu(QIcon(":/pdfplugins/dimensiontool/linear.svg"), tr("Measure"), tr("Measures distances, perimeters, areas and angles on the page."),
                               getActions({ "dimensiontool_LinearHorizontalAction", "dimensiontool_LinearVerticalAction", "dimensiontool_LinearAction",
                                            "dimensiontool_PerimeterAction", "dimensiontool_RectanglePerimeterAction", "dimensiontool_AreaAction",
                                            "dimensiontool_RectangleAreaAction", "dimensiontool_AngleAction", "dimensiontool_CalibrateAction" }));
    measureGroup->addSmallAction(findAction("dimensiontool_ShowDimensionsAction"), tr("Show Measurements"), tr("Shows or hides the measurements."));
    measureGroup->addSmallAction(findAction("dimensiontool_ClearDimensionsAction"), tr("Clear Measurements"), tr("Removes all measurements."));
    measureGroup->addSmallAction(findAction("dimensiontool_ExportAction"), tr("Export Measurements"), tr("Saves the measurements into a file."));

    PDFRibbonGroup* signGroup = page->addGroup(tr("Sign"));
    signGroup->addLargeAction(findAction("signaturetool_activateAction"), tr("Prepare Signature"), tr("Turns on the creation of the signature: write a text or draw the signature by the mouse."));
    signGroup->setPrerequisite(findAction("signaturetool_activateAction"));
    signGroup->addSmallAction(findAction("signaturetool_signElectronicallyAction"), tr("Sign Electronically"), tr("Inserts the prepared signature into the document."));
    signGroup->addSmallAction(findAction("signaturetool_signDigitallyAction"), tr("Sign with Certificate"), tr("Signs the document digitally by a certificate (smart card or certificate file)."));
    signGroup->setPrerequisite(nullptr);
    signGroup->addSmallAction(getAction(PDFActionManager::CertificateManager), tr("Certificates"), tr("Manages the certificates used for the digital signatures."));
}

void PDFEditorRibbon::createReferencesPage()
{
    PDFRibbonPage* page = m_ribbon->addPage(tr("References"));

    PDFRibbonGroup* outlineGroup = page->addGroup(tr("Table of Contents"));
    outlineGroup->addLargeAction(m_outlineNewItemAction, tr("Add Item"), tr("Inserts a new item of the outline (table of contents in the sidebar), which points to the current place of the document."));
    outlineGroup->addLargeAction(m_sidebarAction, tr("Show Sidebar"), tr("Shows the sidebar with the outline, thumbnails of the pages, bookmarks, attachments and notes."), QIcon(":/resources/sidebar-visibility.svg"));

    PDFRibbonGroup* bookmarksGroup = page->addGroup(tr("Bookmarks"));
    bookmarksGroup->addLargeAction(getAction(PDFActionManager::BookmarkPage), tr("Bookmark Page"), tr("Adds the current page to the bookmarks, to return to it quickly."));
    bookmarksGroup->addSmallAction(getAction(PDFActionManager::BookmarkGoToPrevious), tr("Previous Bookmark"), tr("Goes to the previous bookmarked page."));
    bookmarksGroup->addSmallAction(getAction(PDFActionManager::BookmarkGoToNext), tr("Next Bookmark"), tr("Goes to the next bookmarked page."));
    bookmarksGroup->addSmallAction(getAction(PDFActionManager::BookmarkGenerateAutomatically), tr("Generate Bookmarks"), tr("Creates the bookmarks automatically from the outline of the document."));
    bookmarksGroup->addSmallAction(getAction(PDFActionManager::BookmarkExport), tr("Export Bookmarks"), tr("Saves the bookmarks into a file."));
    bookmarksGroup->addSmallAction(getAction(PDFActionManager::BookmarkImport), tr("Import Bookmarks"), tr("Loads the bookmarks from a file."));

    PDFRibbonGroup* linksGroup = page->addGroup(tr("Links"));
    linksGroup->addLargeAction(getAction(PDFActionManager::CreateHyperlink), tr("Link"), tr("Inserts a link to a web page or to a file. Draw a rectangle over the text or the image, which will be the link."));
    linksGroup->addLargeMenu(QIcon(":/resources/hyperlink.svg"), tr("Link in Document"), tr("Inserts a link to another place of this document."),
                             { getAction(PDFActionManager::CreateHyperlinkToThisPDFFit), getAction(PDFActionManager::CreateHyperlinkToThisPDFFitH),
                               getAction(PDFActionManager::CreateHyperlinkToThisPDFFitV), getAction(PDFActionManager::CreateHyperlinkToThisPDFFitR),
                               getAction(PDFActionManager::CreateHyperlinkToThisPDFFitB), getAction(PDFActionManager::CreateHyperlinkToThisPDFFitBH),
                               getAction(PDFActionManager::CreateHyperlinkToThisPDFFitBV), getAction(PDFActionManager::CreateHyperlinkToThisPDFXYZ),
                               getAction(PDFActionManager::CreateHyperlinkToThisPDFXYZInheritZoom) });
    linksGroup->addSmallAction(getAction(PDFActionManager::RemoveExternalLinks), tr("Remove External Links"), tr("Removes the links to web pages and to other files from the document."));
}

void PDFEditorRibbon::createPagePage()
{
    PDFRibbonPage* page = m_ribbon->addPage(tr("Page"));

    PDFRibbonGroup* rotateGroup = page->addGroup(tr("Rotate"));
    rotateGroup->addLargeAction(getAction(PDFActionManager::RotateLeft), tr("Rotate Left"), tr("Rotates the view of the pages by 90 degrees to the left."));
    rotateGroup->addLargeAction(getAction(PDFActionManager::RotateRight), tr("Rotate Right"), tr("Rotates the view of the pages by 90 degrees to the right."));

    PDFRibbonGroup* setupGroup = page->addGroup(tr("Page Setup"));
    setupGroup->addLargeAction(getAction(PDFActionManager::PageGeometry), tr("Page Size"), tr("Changes the size, the margins and the cropping of the pages."));
    setupGroup->addLargeAction(getAction(PDFActionManager::InsertPageNumbers), tr("Page Numbers"), tr("Inserts the numbers of the pages (header or footer) into the document."));

    PDFRibbonGroup* scanGroup = page->addGroup(tr("Scanned Pages"));
    scanGroup->addLargeAction(findAction("scannerplugin_ScanPages"), tr("Scan"), tr("Acquires the pages from a scanner into the document."));
    scanGroup->addLargeAction(getAction(PDFActionManager::PrepareScannedPages), tr("Straighten and Crop"), tr("Straightens the skewed scanned pages, crops their margins and splits the double pages."), QIcon(":/resources/page.svg"));

    PDFRibbonGroup* navigationGroup = page->addGroup(tr("Go To"));
    navigationGroup->addSmallAction(getAction(PDFActionManager::GoToDocumentStart), tr("First Page"), tr("Goes to the first page of the document."));
    navigationGroup->addSmallAction(getAction(PDFActionManager::GoToPreviousPage), tr("Previous Page"), tr("Goes to the previous page."));
    navigationGroup->addSmallAction(getAction(PDFActionManager::GoToNextPage), tr("Next Page"), tr("Goes to the next page."));
    navigationGroup->addSmallAction(getAction(PDFActionManager::GoToDocumentEnd), tr("Last Page"), tr("Goes to the last page of the document."));
}

void PDFEditorRibbon::createToolsPage()
{
    PDFRibbonPage* page = m_ribbon->addPage(tr("Tools"));

    PDFRibbonGroup* ocrGroup = page->addGroup(tr("Text Recognition"));
    ocrGroup->addLargeAction(getAction(PDFActionManager::RecognizeText), tr("Recognize Text (OCR)"), tr("Recognizes the text of the scanned pages, the text can be then searched, selected and copied."));
    ocrGroup->addSmallAction(getAction(PDFActionManager::BatchRecognizeText), tr("Recognize More Files"), tr("Recognizes the text of more documents one after another."));
    ocrGroup->addSmallAction(getAction(PDFActionManager::ManageOCRLanguages), tr("OCR Languages"), tr("Shows the languages available for the recognition of the text and adds new ones."));

    PDFRibbonGroup* optimizeGroup = page->addGroup(tr("Reduce Size"));
    optimizeGroup->addLargeAction(getAction(PDFActionManager::Optimize), tr("Optimize"), tr("Reduces the size of the document file, the content stays the same."));
    optimizeGroup->addSmallAction(getAction(PDFActionManager::OptimizeImages), tr("Optimize Images"), tr("Reduces the size of the images of the document (resolution and compression)."));
    optimizeGroup->addSmallAction(getAction(PDFActionManager::CreateBitonalDocument), tr("Black and White"), tr("Converts the pages into black and white, the file becomes much smaller (suitable for scanned documents)."));

    PDFRibbonGroup* protectGroup = page->addGroup(tr("Protection"));
    protectGroup->addLargeAction(getAction(PDFActionManager::Encryption), tr("Protect Document"), tr("Protects the document by a password and sets the permissions (printing, copying, changes)."));
    protectGroup->addLargeMenu(QIcon(":/pdfplugins/redactplugin/redact-rectangle.svg"), tr("Redact"), tr("Marks the parts of the pages, which will be removed permanently (blacked out), then creates the redacted document."),
                               getActions({ "redactplugin_RedactRectangle", "redactplugin_RedactText", "redactplugin_RedactTextSelection",
                                            "redactplugin_RedactPage", "redactplugin_CreateRedactedDocument" }));
    protectGroup->addSmallAction(getAction(PDFActionManager::Sanitize), tr("Remove Hidden Data"), tr("Removes the hidden information from the document: metadata, scripts, attachments, comments."));
    protectGroup->addSmallAction(getAction(PDFActionManager::RemoveExternalLinks), tr("Remove External Links"), tr("Removes the links to web pages and to other files from the document."));

    PDFRibbonGroup* readGroup = page->addGroup(tr("Read Aloud"));
    readGroup->addLargeMenu(QIcon(":/resources/speech.svg"), tr("Audio Book"), tr("Creates an audio book from the text of the document."),
                            getActions({ "actionAudioBook_CreateTextStream", "actionAudioBook_SynchronizeFromTableToGraphics", "actionAudioBook_SynchronizeFromGraphicsToTable",
                                         "actionAudioBook_ActivateSelection", "actionAudioBook_DeactivateSelection", "actionAudioBook_SelectByRectangle",
                                         "actionAudioBook_SelectByContainedText", "actionAudioBook_SelectByRegularExpression", "actionAudioBook_SelectByPageList",
                                         "actionAudioBook_RestoreOriginalText", "actionAudioBook_MoveSelectionUp", "actionAudioBook_MoveSelectionDown",
                                         "actionAudioBook_CreateAudioBook", "actionAudioBook_Clear" }));

    PDFRibbonGroup* advancedGroup = page->addGroup(tr("Advanced"));
    advancedGroup->addSmallAction(findAction("actionObjectInspector_ObjectInspector"), tr("Object Inspector"), tr("Shows the internal objects of the document (for experts)."));
    advancedGroup->addSmallAction(findAction("actionObjectInspector_ObjectStatistics"), tr("Object Statistics"), tr("Shows the statistics of the internal objects of the document (for experts)."));
    advancedGroup->addSmallAction(getAction(PDFActionManager::ShowRenderingErrors), tr("Rendering Errors"), tr("Lists the errors found while the pages were displayed."));
    advancedGroup->addSmallAction(findAction("actionOutputPreview_OutputPreview"), tr("Output Preview"), tr("Shows the separations of the colors for the printing (for the printing house)."));
    advancedGroup->addSmallAction(findAction("actionOutputPreview_InkCoverage"), tr("Ink Coverage"), tr("Computes the coverage of the inks of the pages (for the printing house)."));
    advancedGroup->addSmallMenu(QIcon(":/pdfplugins/softproofing/soft-proofing.svg"), tr("Soft Proofing"), tr("Simulates the colors of the print on the screen."),
                                getActions({ "actionSoftProofing_EnableSoftProofing", "actionSoftProofing_EnableGamutChecking", "actionSoftProofing_ShowSettings" }));

    PDFRibbonGroup* settingsGroup = page->addGroup(tr("Settings"));
    settingsGroup->addLargeAction(getAction(PDFActionManager::Options), tr("Options"), tr("Settings of the application: display, rendering, security, shortcuts, plugins."));
    settingsGroup->addSmallAction(getAction(PDFActionManager::CertificateManager), tr("Certificates"), tr("Manages the certificates used for the digital signatures."));
    settingsGroup->addSmallAction(getAction(PDFActionManager::ResetToFactorySettings), tr("Reset Settings"), tr("Restores the original settings of the application."));
}

void PDFEditorRibbon::createViewPage()
{
    PDFRibbonPage* page = m_ribbon->addPage(tr("View"));

    PDFRibbonGroup* zoomGroup = page->addGroup(tr("Zoom"));
    zoomGroup->addLargeAction(getAction(PDFActionManager::FitPage), tr("Whole Page"), tr("Zooms the view, so the whole page is visible."));
    zoomGroup->addLargeAction(getAction(PDFActionManager::FitWidth), tr("Page Width"), tr("Zooms the view, so the page fills the width of the window."));
    zoomGroup->addSmallAction(getAction(PDFActionManager::ZoomIn), tr("Zoom In"), tr("Enlarges the view of the page."));
    zoomGroup->addSmallAction(getAction(PDFActionManager::ZoomOut), tr("Zoom Out"), tr("Reduces the view of the page."));
    zoomGroup->addSmallAction(getAction(PDFActionManager::FitHeight), tr("Page Height"), tr("Zooms the view, so the page fills the height of the window."));
    zoomGroup->addSmallAction(getAction(PDFActionManager::ToolMagnifier), tr("Magnifier"), tr("Shows a magnified part of the page under the mouse."));

    PDFRibbonGroup* layoutGroup = page->addGroup(tr("Page Layout"));
    layoutGroup->addSmallAction(getAction(PDFActionManager::PageLayoutSinglePage), tr("Single Page"), tr("Shows one page at a time."));
    layoutGroup->addSmallAction(getAction(PDFActionManager::PageLayoutContinuous), tr("Continuous"), tr("Shows the pages one below another, scrolling continuously."));
    layoutGroup->addSmallAction(getAction(PDFActionManager::PageLayoutTwoPages), tr("Two Pages"), tr("Shows two pages side by side."));
    layoutGroup->addSmallAction(getAction(PDFActionManager::PageLayoutTwoColumns), tr("Two Columns"), tr("Shows the pages in two columns, scrolling continuously."));
    layoutGroup->addSmallAction(getAction(PDFActionManager::PageLayoutFirstPageOnRightSide), tr("First Page on Right"), tr("In the two page layouts, the first page is shown on the right side (as a book)."));

    PDFRibbonGroup* colorsGroup = page->addGroup(tr("Colors"));
    colorsGroup->addSmallAction(getAction(PDFActionManager::RenderOptionInvertColors), tr("Inverted"), tr("Shows the pages with inverted colors (light text on a dark background)."));
    colorsGroup->addSmallAction(getAction(PDFActionManager::RenderOptionGrayscale), tr("Grayscale"), tr("Shows the pages in shades of gray."));
    colorsGroup->addSmallAction(getAction(PDFActionManager::RenderOptionHighContrast), tr("High Contrast"), tr("Shows the pages with a high contrast, for a better readability."));
    colorsGroup->addSmallAction(getAction(PDFActionManager::RenderOptionBitonal), tr("Monochromatic"), tr("Shows the pages in black and white only."));
    colorsGroup->addSmallAction(getAction(PDFActionManager::RenderOptionCustomColors), tr("Custom Colors"), tr("Shows the pages with the colors set in the options."));

    PDFRibbonGroup* showGroup = page->addGroup(tr("Show"));
    showGroup->addLargeAction(m_sidebarAction, tr("Sidebar"), tr("Shows the sidebar with the outline, thumbnails of the pages, bookmarks, attachments and notes."), QIcon(":/resources/sidebar-visibility.svg"));
    showGroup->addLargeAction(getAction(PDFActionManager::FullscreenMode), tr("Full Screen"), tr("Shows the document on the whole screen. Press Esc to return."), QIcon(":/resources/zoom-fit.svg"));
    showGroup->addSmallAction(getAction(PDFActionManager::RenderOptionDisplayAnnotations), tr("Comments"), tr("Shows or hides the comments, notes and highlights of the pages."));
    showGroup->addSmallMenu(QIcon(":/resources/rendering.svg"), tr("Display Quality"), tr("Settings of the quality of the display of the pages (smoothing of the text and of the images)."),
                            { getAction(PDFActionManager::RenderOptionAntialiasing), getAction(PDFActionManager::RenderOptionTextAntialiasing),
                              getAction(PDFActionManager::RenderOptionSmoothPictures), getAction(PDFActionManager::RenderOptionIgnoreOptionalContentSettings),
                              getAction(PDFActionManager::RenderOptionDisplayRenderTimes) });
}

void PDFEditorRibbon::createHelpPage()
{
    PDFRibbonPage* page = m_ribbon->addPage(tr("Help"));

    PDFRibbonGroup* helpGroup = page->addGroup(tr("Help"));
    helpGroup->addLargeAction(getAction(PDFActionManager::About), tr("About"), tr("Shows the version of the application, the authors and the licenses."));
    helpGroup->addLargeAction(getAction(PDFActionManager::GetSource), tr("Source Code"), tr("Opens the web page with the source code of the application."));
}

void PDFEditorRibbon::createFontGroup(PDFRibbonPage* page)
{
    // The font group is placed on two pages (Home and Edit), but the controls
    // exist only once - they are created for the first page only.
    if (m_fontComboBox)
    {
        return;
    }

    PDFRibbonGroup* fontGroup = page->addGroup(tr("Font"));

    m_fontComboBox = new QFontComboBox(fontGroup);
    m_fontComboBox->setFocusPolicy(Qt::ClickFocus);
    m_fontComboBox->setFixedWidth(pdf::PDFWidgetUtils::scaleDPI_x(m_fontComboBox, 160));
    m_fontComboBox->setToolTip(PDFRibbonGroup::createToolTip(tr("Font"), tr("Font of the text. It is used for the new text boxes and for the selected text boxes."), QKeySequence()));

    m_fontSizeComboBox = new QComboBox(fontGroup);
    m_fontSizeComboBox->setEditable(true);
    m_fontSizeComboBox->setFocusPolicy(Qt::ClickFocus);
    m_fontSizeComboBox->setFixedWidth(pdf::PDFWidgetUtils::scaleDPI_x(m_fontSizeComboBox, 56));
    m_fontSizeComboBox->setValidator(new QIntValidator(1, 999, m_fontSizeComboBox));
    for (int size : { 8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 26, 28, 36, 48, 72 })
    {
        m_fontSizeComboBox->addItem(QString::number(size));
    }
    m_fontSizeComboBox->setCurrentText("12");
    m_fontSizeComboBox->setToolTip(PDFRibbonGroup::createToolTip(tr("Font Size"), tr("Size of the text in points."), QKeySequence()));

    // Letters of the bold, italic and underline buttons are translated (for example
    // "G", "C", "S" in Italian), so the icons are created from the translations.
    const QColor letterColor = QApplication::palette().color(QPalette::WindowText);
    m_boldButton = createTextStyleButton(createLetterIcon(tr("B", "Letter on the Bold button").at(0), QFont::Bold, false, false, letterColor), tr("Bold"), tr("Writes the text in bold."), true);
    m_italicButton = createTextStyleButton(createLetterIcon(tr("I", "Letter on the Italic button").at(0), QFont::Normal, true, false, letterColor), tr("Italic"), tr("Writes the text in italic."), true);
    m_underlineButton = createTextStyleButton(createLetterIcon(tr("U", "Letter on the Underline button").at(0), QFont::Normal, false, true, letterColor), tr("Underline"), tr("Underlines the text."), true);
    m_textColorButton = createTextStyleButton(createLetterIcon(QChar('A'), QFont::Normal, false, true, m_textColor), tr("Font Color"), tr("Changes the color of the text."), false);

    fontGroup->addWidgetRow({ m_fontComboBox, m_fontSizeComboBox });
    fontGroup->addWidgetRow({ m_boldButton, m_italicButton, m_underlineButton, m_textColorButton });

    QLabel* hintLabel = new QLabel(tr("For text boxes"), fontGroup);
    hintLabel->setObjectName("ribbonHint");
    fontGroup->addWidgetRow({ hintLabel });

    connect(m_fontComboBox, &QFontComboBox::currentFontChanged, this, &PDFEditorRibbon::onFontFamilyChanged);
    connect(m_fontSizeComboBox, &QComboBox::currentTextChanged, this, &PDFEditorRibbon::onFontSizeChanged);
    connect(m_boldButton, &QToolButton::clicked, this, &PDFEditorRibbon::onFontStyleChanged);
    connect(m_italicButton, &QToolButton::clicked, this, &PDFEditorRibbon::onFontStyleChanged);
    connect(m_underlineButton, &QToolButton::clicked, this, &PDFEditorRibbon::onFontStyleChanged);
    connect(m_textColorButton, &QToolButton::clicked, this, &PDFEditorRibbon::onTextColorClicked);
}

void PDFEditorRibbon::createParagraphGroup(PDFRibbonPage* page)
{
    if (m_alignLeftButton)
    {
        return;
    }

    PDFRibbonGroup* paragraphGroup = page->addGroup(tr("Paragraph"));

    m_alignLeftButton = createTextStyleButton(createAlignmentIcon(Qt::AlignLeft), tr("Align Left"), tr("Aligns the text to the left edge of the text box."), true);
    m_alignCenterButton = createTextStyleButton(createAlignmentIcon(Qt::AlignHCenter), tr("Center"), tr("Centers the text in the text box."), true);
    m_alignRightButton = createTextStyleButton(createAlignmentIcon(Qt::AlignRight), tr("Align Right"), tr("Aligns the text to the right edge of the text box."), true);

    paragraphGroup->addWidgetRow({ m_alignLeftButton, m_alignCenterButton, m_alignRightButton });

    connect(m_alignLeftButton, &QToolButton::clicked, this, [this]() { onAlignmentClicked(Qt::AlignLeft); });
    connect(m_alignCenterButton, &QToolButton::clicked, this, [this]() { onAlignmentClicked(Qt::AlignHCenter); });
    connect(m_alignRightButton, &QToolButton::clicked, this, [this]() { onAlignmentClicked(Qt::AlignRight); });
}

QToolButton* PDFEditorRibbon::createTextStyleButton(const QIcon& icon, const QString& title, const QString& description, bool checkable)
{
    QToolButton* button = new QToolButton(m_ribbon);
    button->setAutoRaise(true);
    button->setFocusPolicy(Qt::NoFocus);
    button->setIcon(icon);
    button->setIconSize(pdf::PDFWidgetUtils::scaleDPI(button, QSize(16, 16)));
    button->setCheckable(checkable);
    button->setToolTip(PDFRibbonGroup::createToolTip(title, description, QKeySequence()));
    button->setProperty("ribbonSmall", true);
    return button;
}

pdf::PDFPageContentEditorWidget* PDFEditorRibbon::getEditorWidget(bool activate)
{
    if (!m_editorActivateAction)
    {
        return nullptr;
    }

    if (activate && !m_editorActivateAction->isChecked() && m_editorActivateAction->isEnabled())
    {
        m_editorActivateAction->trigger();
    }

    if (!m_editorActivateAction->isChecked())
    {
        return nullptr;
    }

    connectEditorWidget();
    return m_connectedEditorWidget;
}

void PDFEditorRibbon::connectEditorWidget()
{
    pdf::PDFPageContentEditorWidget* editorWidget = m_mainWindow->findChild<pdf::PDFPageContentEditorWidget*>();
    if (!editorWidget || editorWidget == m_connectedEditorWidget)
    {
        return;
    }

    m_connectedEditorWidget = editorWidget;

    // The style of the editor follows the selected item, the ribbon follows the editor
    connect(editorWidget, &pdf::PDFPageContentEditorWidget::fontChanged, this, &PDFEditorRibbon::loadTextStyle);
    connect(editorWidget, &pdf::PDFPageContentEditorWidget::alignmentChanged, this, &PDFEditorRibbon::loadTextStyle);
    connect(editorWidget, &pdf::PDFPageContentEditorWidget::penChanged, this, &PDFEditorRibbon::loadTextStyle);
    loadTextStyle();
}

void PDFEditorRibbon::loadTextStyle()
{
    if (!m_connectedEditorWidget || !m_fontComboBox || !m_alignLeftButton)
    {
        return;
    }

    pdf::PDFTemporaryValueChange guard(&m_isLoadingTextStyle, true);

    const QFont font = m_connectedEditorWidget->getFont();
    m_fontComboBox->setCurrentFont(font);
    if (font.pointSizeF() > 0.0)
    {
        m_fontSizeComboBox->setCurrentText(QString::number(qRound(font.pointSizeF())));
    }
    m_boldButton->setChecked(font.bold());
    m_italicButton->setChecked(font.italic());
    m_underlineButton->setChecked(font.underline());

    m_textColor = m_connectedEditorWidget->getTextColor();
    m_textColorButton->setIcon(createLetterIcon(QChar('A'), QFont::Normal, false, true, m_textColor));

    const Qt::Alignment horizontalAlignment = m_connectedEditorWidget->getAlignment() & Qt::AlignHorizontal_Mask;
    m_alignLeftButton->setChecked(horizontalAlignment == Qt::AlignLeft);
    m_alignCenterButton->setChecked(horizontalAlignment == Qt::AlignHCenter);
    m_alignRightButton->setChecked(horizontalAlignment == Qt::AlignRight);
}

void PDFEditorRibbon::updateTextStyleAvailability()
{
    const bool enabled = m_editorActivateAction && m_editorActivateAction->isEnabled();
    for (QWidget* widget : std::initializer_list<QWidget*>{ m_fontComboBox, m_fontSizeComboBox, m_boldButton, m_italicButton, m_underlineButton,
                                                            m_textColorButton, m_alignLeftButton, m_alignCenterButton, m_alignRightButton })
    {
        if (widget)
        {
            widget->setEnabled(enabled);
        }
    }
}

void PDFEditorRibbon::onFontFamilyChanged()
{
    onFontStyleChanged();
}

void PDFEditorRibbon::onFontSizeChanged()
{
    bool ok = false;
    const int size = m_fontSizeComboBox->currentText().toInt(&ok);
    if (ok && size > 0)
    {
        onFontStyleChanged();
    }
}

void PDFEditorRibbon::onFontStyleChanged()
{
    if (m_isLoadingTextStyle)
    {
        return;
    }

    pdf::PDFPageContentEditorWidget* editorWidget = getEditorWidget(true);
    if (!editorWidget)
    {
        return;
    }

    QFont font = editorWidget->getFont();
    font.setFamily(m_fontComboBox->currentFont().family());

    bool ok = false;
    const int size = m_fontSizeComboBox->currentText().toInt(&ok);
    if (ok && size > 0)
    {
        font.setPointSize(size);
    }

    font.setBold(m_boldButton->isChecked());
    font.setItalic(m_italicButton->isChecked());
    font.setUnderline(m_underlineButton->isChecked());
    editorWidget->changeFont(font);
}

void PDFEditorRibbon::onTextColorClicked()
{
    pdf::PDFPageContentEditorWidget* editorWidget = getEditorWidget(true);
    if (!editorWidget)
    {
        return;
    }

    QColor color = QColorDialog::getColor(editorWidget->getTextColor(), m_mainWindow, tr("Font Color"));
    if (color.isValid())
    {
        editorWidget->changeTextColor(color);
        loadTextStyle();
    }
}

void PDFEditorRibbon::onAlignmentClicked(Qt::Alignment horizontalAlignment)
{
    pdf::PDFPageContentEditorWidget* editorWidget = getEditorWidget(true);
    if (!editorWidget)
    {
        loadTextStyle();
        return;
    }

    // Vertical alignment of the text in the text box is kept
    Qt::Alignment verticalAlignment = editorWidget->getAlignment() & Qt::AlignVertical_Mask;
    if (verticalAlignment == Qt::Alignment())
    {
        verticalAlignment = Qt::AlignVCenter;
    }

    editorWidget->changeAlignment(horizontalAlignment | verticalAlignment);
    loadTextStyle();
}

QIcon PDFEditorRibbon::createLetterIcon(QChar letter, int fontWeight, bool italic, bool underline, QColor underlineColor)
{
    const int size = 64;
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    QFont font = QApplication::font();
    font.setPixelSize(underline ? 44 : 52);
    font.setWeight(QFont::Weight(fontWeight));
    font.setItalic(italic);
    painter.setFont(font);
    painter.setPen(QApplication::palette().color(QPalette::WindowText));

    QRect textRect(0, 0, size, underline ? size - 14 : size);
    painter.drawText(textRect, Qt::AlignCenter, QString(letter));

    if (underline)
    {
        painter.fillRect(QRect(8, size - 12, size - 16, 9), underlineColor);
    }

    painter.end();
    return QIcon(pixmap);
}

QIcon PDFEditorRibbon::createAlignmentIcon(Qt::Alignment alignment)
{
    const int size = 64;
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    const QColor color = QApplication::palette().color(QPalette::WindowText);
    const int lineHeight = 6;
    const int lineWidths[] = { 56, 36, 56, 28, 48 };

    for (int i = 0; i < 5; ++i)
    {
        const int width = lineWidths[i];
        int x = 4;
        if (alignment == Qt::AlignHCenter)
        {
            x = (size - width) / 2;
        }
        else if (alignment == Qt::AlignRight)
        {
            x = size - 4 - width;
        }

        painter.fillRect(QRect(x, 6 + i * 12, width, lineHeight), color);
    }

    painter.end();
    return QIcon(pixmap);
}

}   // namespace pdfviewer
