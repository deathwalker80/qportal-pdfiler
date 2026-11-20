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

#include "pdfwelcomepage.h"
#include "pdfprogramcontroller.h"
#include "pdfrecentfilemanager.h"
#include "pdfwidgetutils.h"
#include "pdfdocumentbuilder.h"
#include "pdfdocumentwriter.h"
#include "pdfutils.h"

#include <QDir>
#include <QEvent>
#include <QLabel>
#include <QAction>
#include <QProcess>
#include <QFileInfo>
#include <QBoxLayout>
#include <QFileDialog>
#include <QGridLayout>
#include <QMainWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QApplication>
#include <QStandardPaths>

#include "pdfdbgheap.h"

namespace pdfviewer
{

PDFWelcomePage::PDFWelcomePage(QMainWindow* mainWindow,
                               PDFActionManager* actionManager,
                               PDFProgramController* programController,
                               bool isEditor,
                               QWidget* parent) :
    QWidget(parent),
    m_mainWindow(mainWindow),
    m_actionManager(actionManager),
    m_programController(programController),
    m_newDocumentAction(nullptr),
    m_createPdfAction(nullptr),
    m_mergePdfAction(nullptr),
    m_splitPdfAction(nullptr),
    m_recentFilesLayout(nullptr)
{
    setObjectName("welcomePage");
    setAutoFillBackground(true);

    m_newDocumentAction = new QAction(getIcon(":/resources/new-document.svg"), tr("New Document..."), this);
    m_newDocumentAction->setObjectName("actionNewDocument");
    m_newDocumentAction->setShortcut(QKeySequence::New);
    connect(m_newDocumentAction, &QAction::triggered, this, &PDFWelcomePage::onNewDocument);

    m_createPdfAction = new QAction(getIcon(":/resources/create-pdf.svg"), tr("Create PDF from Files..."), this);
    m_createPdfAction->setObjectName("actionPdfToolCreate");
    connect(m_createPdfAction, &QAction::triggered, this, &PDFWelcomePage::onCreatePdf);

    m_mergePdfAction = new QAction(getIcon(":/resources/merge-pdf.svg"), tr("Merge PDF Documents..."), this);
    m_mergePdfAction->setObjectName("actionPdfToolMerge");
    connect(m_mergePdfAction, &QAction::triggered, this, &PDFWelcomePage::onMergeOrSplitPdf);

    m_splitPdfAction = new QAction(getIcon(":/resources/split-pdf.svg"), tr("Split PDF Document..."), this);
    m_splitPdfAction->setObjectName("actionPdfToolSplit");
    connect(m_splitPdfAction, &QAction::triggered, this, &PDFWelcomePage::onMergeOrSplitPdf);

    // The viewer cannot create documents, so it doesn't offer the new document
    if (isEditor)
    {
        m_actionManager->addAdditionalAction(m_newDocumentAction);
    }

    for (QAction* action : { m_createPdfAction, m_mergePdfAction, m_splitPdfAction })
    {
        m_actionManager->addAdditionalAction(action);
    }

    QHBoxLayout* pageLayout = new QHBoxLayout(this);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->setSpacing(0);

    // Left panel: name of the application and the main commands
    QFrame* leftPanel = new QFrame(this);
    leftPanel->setObjectName("welcomeLeftPanel");
    leftPanel->setFixedWidth(pdf::PDFWidgetUtils::scaleDPI_x(this, 250));

    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(pdf::PDFWidgetUtils::scaleDPI_x(this, 24), pdf::PDFWidgetUtils::scaleDPI_y(this, 28),
                                   pdf::PDFWidgetUtils::scaleDPI_x(this, 24), pdf::PDFWidgetUtils::scaleDPI_y(this, 16));
    leftLayout->setSpacing(pdf::PDFWidgetUtils::scaleDPI_y(this, 8));

    QLabel* applicationLabel = new QLabel(QApplication::applicationDisplayName(), leftPanel);
    applicationLabel->setObjectName("welcomeApplicationName");
    applicationLabel->setWordWrap(true);
    leftLayout->addWidget(applicationLabel);

    QLabel* versionLabel = new QLabel(tr("Version %1").arg(QApplication::applicationVersion()), leftPanel);
    versionLabel->setObjectName("welcomeVersion");
    leftLayout->addWidget(versionLabel);
    leftLayout->addSpacing(pdf::PDFWidgetUtils::scaleDPI_y(this, 24));

    QAction* openAction = m_actionManager->getAction(PDFActionManager::Open);

    if (isEditor)
    {
        QPushButton* newButton = new QPushButton(tr("New Document"), leftPanel);
        newButton->setObjectName("welcomeMainButton");
        connect(newButton, &QPushButton::clicked, m_newDocumentAction, &QAction::trigger);
        leftLayout->addWidget(newButton);
    }

    QPushButton* openButton = new QPushButton(tr("Open Document"), leftPanel);
    openButton->setObjectName("welcomeMainButton");
    connect(openButton, &QPushButton::clicked, openAction, &QAction::trigger);
    leftLayout->addWidget(openButton);

    leftLayout->addStretch(1);

    QLabel* hintLabel = new QLabel(tr("The documents stay on this computer: nothing is sent to the internet."), leftPanel);
    hintLabel->setObjectName("welcomeHint");
    hintLabel->setWordWrap(true);
    leftLayout->addWidget(hintLabel);

    pageLayout->addWidget(leftPanel);

    // Right panel: tools and recent documents
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setObjectName("welcomeScrollArea");
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget* content = new QWidget(scrollArea);
    content->setObjectName("welcomeContent");
    QVBoxLayout* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(pdf::PDFWidgetUtils::scaleDPI_x(this, 36), pdf::PDFWidgetUtils::scaleDPI_y(this, 28),
                                      pdf::PDFWidgetUtils::scaleDPI_x(this, 36), pdf::PDFWidgetUtils::scaleDPI_y(this, 20));
    contentLayout->setSpacing(pdf::PDFWidgetUtils::scaleDPI_y(this, 12));

    QLabel* toolsLabel = new QLabel(tr("What do you want to do?"), content);
    toolsLabel->setObjectName("welcomeHeading");
    contentLayout->addWidget(toolsLabel);

    QGridLayout* tilesLayout = new QGridLayout();
    tilesLayout->setSpacing(pdf::PDFWidgetUtils::scaleDPI_x(this, 12));

    QList<QPushButton*> tiles;
    if (isEditor)
    {
        tiles << createTile(openAction, getIcon(":/resources/open.svg"), tr("Open a PDF"), tr("Read, comment, sign or change a document."));
        tiles << createTile(m_newDocumentAction, getIcon(":/resources/new-document.svg"), tr("New document"), tr("Create an empty A4 document and write into it."));
    }
    else
    {
        tiles << createTile(openAction, getIcon(":/resources/open.svg"), tr("Open a PDF"), tr("Read and print a document."));
    }

    tiles << createTile(m_createPdfAction, getIcon(":/resources/create-pdf.svg"), tr("Create a PDF from files"), tr("Word, Excel, PowerPoint, images, text: one PDF from many files."));
    tiles << createTile(m_mergePdfAction, getIcon(":/resources/merge-pdf.svg"), tr("Merge PDF documents"), tr("Join documents, sort, rotate and remove the pages."));
    tiles << createTile(m_splitPdfAction, getIcon(":/resources/split-pdf.svg"), tr("Split a PDF"), tr("Extract pages or divide a document into more files."));

    // Scanning is provided by a plugin (the icon of its action is already prepared for the theme)
    for (QAction* action : m_actionManager->getActions())
    {
        if (action && action->objectName() == "scannerplugin_ScanPages")
        {
            tiles << createTile(action, action->icon(), tr("Scan"), tr("Acquire pages from a scanner into a new PDF document."));
            break;
        }
    }

    if (QAction* batchOcrAction = m_actionManager->getAction(PDFActionManager::BatchRecognizeText))
    {
        tiles << createTile(batchOcrAction, getIcon(":/resources/ocr.svg"), tr("Recognize text (OCR)"), tr("Make scanned documents searchable, more files at once."));
    }

    for (int i = 0; i < tiles.size(); ++i)
    {
        tilesLayout->addWidget(tiles[i], i / 3, i % 3);
    }
    for (int column = 0; column < 3; ++column)
    {
        tilesLayout->setColumnStretch(column, 1);
    }
    contentLayout->addLayout(tilesLayout);
    contentLayout->addSpacing(pdf::PDFWidgetUtils::scaleDPI_y(this, 16));

    QLabel* recentLabel = new QLabel(tr("Recent documents"), content);
    recentLabel->setObjectName("welcomeHeading");
    contentLayout->addWidget(recentLabel);

    m_recentFilesLayout = new QVBoxLayout();
    m_recentFilesLayout->setSpacing(0);
    contentLayout->addLayout(m_recentFilesLayout);
    contentLayout->addStretch(1);

    scrollArea->setWidget(content);
    pageLayout->addWidget(scrollArea, 1);

    connect(m_programController->getRecentFileManager(), &PDFRecentFileManager::fileOpenRequest, this, &PDFWelcomePage::updateRecentFiles, Qt::QueuedConnection);

    updateStyleSheet();
    updateRecentFiles();

    if (parent)
    {
        parent->installEventFilter(this);
        setGeometry(parent->rect());
    }
}

bool PDFWelcomePage::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == parentWidget() && event->type() == QEvent::Resize)
    {
        setGeometry(parentWidget()->rect());
    }

    return QWidget::eventFilter(watched, event);
}

void PDFWelcomePage::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);

    if (parentWidget())
    {
        setGeometry(parentWidget()->rect());
    }

    raise();
    updateRecentFiles();
}

void PDFWelcomePage::changeEvent(QEvent* event)
{
    QWidget::changeEvent(event);

    const bool isPaletteChanged = event->type() == QEvent::PaletteChange ||
                                  event->type() == QEvent::ApplicationPaletteChange ||
                                  event->type() == QEvent::ThemeChange;

    if (isPaletteChanged && !m_isUpdatingStyleSheet)
    {
        updateStyleSheet();
    }
}

QIcon PDFWelcomePage::getIcon(const QString& fileName) const
{
    QIcon icon(fileName);
    if (pdf::PDFWidgetUtils::isDarkTheme())
    {
        icon = pdf::PDFWidgetUtils::convertIconForDarkTheme(icon, pdf::PDFWidgetUtils::scaleDPI(this, QSize(36, 36)), devicePixelRatioF());
    }
    return icon;
}

QPushButton* PDFWelcomePage::createTile(QAction* action, const QIcon& icon, const QString& title, const QString& description)
{
    QPushButton* tile = new QPushButton(this);
    tile->setObjectName("welcomeTile");
    tile->setCursor(Qt::PointingHandCursor);
    tile->setMinimumHeight(pdf::PDFWidgetUtils::scaleDPI_y(this, 86));
    tile->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QHBoxLayout* tileLayout = new QHBoxLayout(tile);
    tileLayout->setContentsMargins(pdf::PDFWidgetUtils::scaleDPI_x(this, 14), pdf::PDFWidgetUtils::scaleDPI_y(this, 10),
                                   pdf::PDFWidgetUtils::scaleDPI_x(this, 14), pdf::PDFWidgetUtils::scaleDPI_y(this, 10));
    tileLayout->setSpacing(pdf::PDFWidgetUtils::scaleDPI_x(this, 12));

    const QSize iconSize = pdf::PDFWidgetUtils::scaleDPI(this, QSize(36, 36));

    QLabel* iconLabel = new QLabel(tile);
    iconLabel->setPixmap(icon.pixmap(iconSize));
    iconLabel->setFixedSize(iconSize);
    iconLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    tileLayout->addWidget(iconLabel, 0, Qt::AlignVCenter);

    QVBoxLayout* textLayout = new QVBoxLayout();
    textLayout->setSpacing(pdf::PDFWidgetUtils::scaleDPI_y(this, 2));

    QLabel* titleLabel = new QLabel(title, tile);
    titleLabel->setObjectName("welcomeTileTitle");
    titleLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    textLayout->addWidget(titleLabel);

    QLabel* descriptionLabel = new QLabel(description, tile);
    descriptionLabel->setObjectName("welcomeTileDescription");
    descriptionLabel->setWordWrap(true);
    descriptionLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    textLayout->addWidget(descriptionLabel);

    tileLayout->addLayout(textLayout, 1);

    connect(tile, &QPushButton::clicked, action, &QAction::trigger);
    return tile;
}

void PDFWelcomePage::updateRecentFiles()
{
    while (QLayoutItem* item = m_recentFilesLayout->takeAt(0))
    {
        if (QWidget* widget = item->widget())
        {
            widget->deleteLater();
        }
        delete item;
    }

    const QStringList recentFiles = m_programController->getRecentFileManager()->getRecentFiles();
    int count = 0;

    for (const QString& fileName : recentFiles)
    {
        const QFileInfo fileInfo(fileName);
        if (fileName.isEmpty())
        {
            continue;
        }

        QPushButton* button = new QPushButton(this);
        button->setObjectName("welcomeRecentFile");
        button->setCursor(Qt::PointingHandCursor);
        button->setText(QString("%1\n%2").arg(fileInfo.fileName(), QDir::toNativeSeparators(fileInfo.absolutePath())));
        button->setToolTip(QDir::toNativeSeparators(fileName));
        connect(button, &QPushButton::clicked, this, [this, fileName]()
        {
            if (QFileInfo::exists(fileName))
            {
                m_programController->openDocument(fileName);
            }
            else
            {
                QMessageBox::warning(m_mainWindow, tr("Recent documents"), tr("File '%1' no longer exists.").arg(QDir::toNativeSeparators(fileName)));
            }
        });
        m_recentFilesLayout->addWidget(button);

        if (++count == 8)
        {
            break;
        }
    }

    if (count == 0)
    {
        QLabel* emptyLabel = new QLabel(tr("No recent documents. The documents you open will be listed here."), this);
        emptyLabel->setObjectName("welcomeTileDescription");
        m_recentFilesLayout->addWidget(emptyLabel);
    }
}

void PDFWelcomePage::updateStyleSheet()
{
    // Colors of the right panel are computed from the palette (light and dark theme),
    // the left panel has the institutional dark blue color in both themes.
    const QPalette palette = QApplication::palette(this);
    const QColor base = palette.color(QPalette::Base);
    const QColor text = palette.color(QPalette::Text);

    auto mix = [](const QColor& first, const QColor& second, qreal ratio)
    {
        return QColor::fromRgbF(first.redF() * (1.0 - ratio) + second.redF() * ratio,
                                first.greenF() * (1.0 - ratio) + second.greenF() * ratio,
                                first.blueF() * (1.0 - ratio) + second.blueF() * ratio).name();
    };

    const QString blue = "#0b2f5b";
    const QString styleSheet = QString(
                "QWidget#welcomePage, QWidget#welcomeContent, QScrollArea#welcomeScrollArea { background: %1; }"
                "QFrame#welcomeLeftPanel { background: %2; }"
                "QLabel#welcomeApplicationName { color: white; font-size: 17pt; font-weight: bold; }"
                "QLabel#welcomeVersion { color: #b9c9de; font-size: 9pt; }"
                "QLabel#welcomeHint { color: #b9c9de; font-size: 8pt; }"
                "QPushButton#welcomeMainButton { color: white; background: #1d4f8f; border: 1px solid #3a6fb3; border-radius: 4px; padding: 9px 12px; font-size: 10pt; text-align: left; }"
                "QPushButton#welcomeMainButton:hover { background: #2a62ab; }"
                "QLabel#welcomeHeading { color: %3; font-size: 14pt; }"
                "QPushButton#welcomeTile { background: %4; border: 1px solid %5; border-radius: 6px; text-align: left; }"
                "QPushButton#welcomeTile:hover { background: %6; border-color: #3a6fb3; }"
                "QLabel#welcomeTileTitle { color: %3; font-size: 10pt; font-weight: bold; background: transparent; }"
                "QLabel#welcomeTileDescription { color: %7; font-size: 9pt; background: transparent; }"
                "QPushButton#welcomeRecentFile { color: %3; background: transparent; border: none; border-bottom: 1px solid %5; padding: 7px 6px; text-align: left; font-size: 9pt; }"
                "QPushButton#welcomeRecentFile:hover { background: %6; }")
            .arg(base.name(), blue, text.name(), mix(base, text, 0.03), mix(base, text, 0.18), mix(base, text, 0.09), mix(base, text, 0.62));

    pdf::PDFTemporaryValueChange guard(&m_isUpdatingStyleSheet, true);
    setStyleSheet(styleSheet);
}

void PDFWelcomePage::onNewDocument()
{
    // The new document is saved first, so it has its file from the beginning
    // (the changes are then saved by the Save command, as in any other document)
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString fileName = QFileDialog::getSaveFileName(m_mainWindow, tr("New Document"), QDir(directory).filePath(tr("New document.pdf")), tr("PDF document (*.pdf)"));
    if (fileName.isEmpty())
    {
        return;
    }

    if (!fileName.endsWith(".pdf", Qt::CaseInsensitive))
    {
        fileName += ".pdf";
    }

    // Empty page of the size A4 (in points)
    pdf::PDFDocumentBuilder builder;
    builder.appendPage(QRectF(0.0, 0.0, 595.276, 841.89));
    pdf::PDFDocument document = builder.build();

    pdf::PDFDocumentWriter writer(nullptr);
    pdf::PDFOperationResult result = writer.write(fileName, &document, true);
    if (!result)
    {
        QMessageBox::critical(m_mainWindow, tr("Error"), result.getErrorMessage());
        return;
    }

    m_programController->openDocument(fileName);
}

void PDFWelcomePage::onCreatePdf()
{
    startPdfTool({ "--add-files" });
}

void PDFWelcomePage::onMergeOrSplitPdf()
{
    // The opened document is passed to the tool, otherwise the tool asks for the files
    const QString fileName = m_programController->getOriginalFileName();
    if (!fileName.isEmpty() && QFileInfo::exists(fileName))
    {
        startPdfTool({ fileName });
    }
    else
    {
        startPdfTool({ "--add-files" });
    }
}

void PDFWelcomePage::startPdfTool(const QStringList& arguments)
{
    QString program = QDir(QCoreApplication::applicationDirPath()).filePath("Pdf4QtPageMaster");
#ifdef Q_OS_WIN
    program += ".exe";
#endif

    if (!QFileInfo::exists(program) || !QProcess::startDetached(program, arguments))
    {
        QMessageBox::critical(m_mainWindow, tr("Error"), tr("The tool for merging and splitting of PDF documents cannot be started (%1).").arg(QDir::toNativeSeparators(program)));
    }
}

}   // namespace pdfviewer
