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

#include "pdfribbon.h"
#include "pdfwidgetutils.h"
#include "pdfutils.h"

#include <QMenu>
#include <QApplication>
#include <QLabel>
#include <QAction>
#include <QTabBar>
#include <QEvent>
#include <QWheelEvent>
#include <QScrollBar>
#include <QBoxLayout>
#include <QScrollArea>
#include <QStackedWidget>

#include "pdfdbgheap.h"

namespace pdfviewer
{

PDFRibbonButton::PDFRibbonButton(QAction* action, Size size, const QString& label, const QString& description, const QIcon& fallbackIcon, QWidget* parent) :
    QToolButton(parent),
    m_action(action),
    m_label(label.isEmpty() ? PDFRibbonGroup::cleanText(action->text()) : label),
    m_description(description),
    m_fallbackIcon(fallbackIcon)
{
    setAutoRaise(true);
    setFocusPolicy(Qt::NoFocus);

    if (size == Size::Large)
    {
        setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        setIconSize(pdf::PDFWidgetUtils::scaleDPI(this, QSize(32, 32)));
        setMinimumWidth(pdf::PDFWidgetUtils::scaleDPI_x(this, 56));
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        setProperty("ribbonLarge", true);
        setText(PDFRibbonGroup::wrapLargeLabel(m_label));
    }
    else
    {
        setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        setIconSize(pdf::PDFWidgetUtils::scaleDPI(this, QSize(16, 16)));
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        setProperty("ribbonSmall", true);
        setText(m_label);
    }

    connect(this, &QToolButton::clicked, this, &PDFRibbonButton::onClicked);
    connect(m_action, &QAction::changed, this, &PDFRibbonButton::updateFromAction);
    updateFromAction();
}

void PDFRibbonButton::setPrerequisite(QAction* prerequisite)
{
    if (m_prerequisite)
    {
        disconnect(m_prerequisite, nullptr, this, nullptr);
    }

    m_prerequisite = prerequisite;

    if (m_prerequisite && m_prerequisite != m_action)
    {
        connect(m_prerequisite, &QAction::changed, this, &PDFRibbonButton::updateFromAction);
    }

    updateFromAction();
}

void PDFRibbonButton::onClicked()
{
    if (m_prerequisite && m_prerequisite != m_action && !m_prerequisite->isChecked())
    {
        if (!m_prerequisite->isEnabled())
        {
            updateFromAction();
            return;
        }

        m_prerequisite->trigger();
    }

    if (m_action->isEnabled())
    {
        m_action->trigger();
    }

    updateFromAction();
}

void PDFRibbonButton::updateFromAction()
{
    // The prerequisite turned off makes the action unavailable, but the button
    // stays available - the click turns the prerequisite on first.
    const bool isPrerequisiteUsable = m_prerequisite && m_prerequisite != m_action &&
                                      !m_prerequisite->isChecked() && m_prerequisite->isEnabled();

    setIcon(m_action->icon().isNull() ? m_fallbackIcon : m_action->icon());
    setEnabled(m_action->isEnabled() || isPrerequisiteUsable);
    setVisible(m_action->isVisible());
    setCheckable(m_action->isCheckable());
    setChecked(m_action->isCheckable() && m_action->isChecked());
    setToolTip(PDFRibbonGroup::createToolTip(m_label, m_description, m_action->shortcut()));
}

PDFRibbonGroup::PDFRibbonGroup(const QString& title, QWidget* parent) :
    QFrame(parent),
    m_controlsLayout(nullptr),
    m_currentSmallColumn(nullptr),
    m_currentSmallColumnCount(0),
    m_controlCount(0)
{
    setObjectName("ribbonGroup");

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(pdf::PDFWidgetUtils::scaleDPI_x(this, 4), pdf::PDFWidgetUtils::scaleDPI_y(this, 2),
                               pdf::PDFWidgetUtils::scaleDPI_x(this, 4), pdf::PDFWidgetUtils::scaleDPI_y(this, 2));
    layout->setSpacing(pdf::PDFWidgetUtils::scaleDPI_y(this, 2));

    m_controlsLayout = new QHBoxLayout();
    m_controlsLayout->setContentsMargins(0, 0, 0, 0);
    m_controlsLayout->setSpacing(pdf::PDFWidgetUtils::scaleDPI_x(this, 2));
    layout->addLayout(m_controlsLayout, 1);

    QLabel* titleLabel = new QLabel(title, this);
    titleLabel->setObjectName("ribbonGroupTitle");
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel, 0);
}

PDFRibbonButton* PDFRibbonGroup::addLargeAction(QAction* action, const QString& label, const QString& description, const QIcon& fallbackIcon)
{
    if (!action)
    {
        return nullptr;
    }

    finishColumn();
    PDFRibbonButton* button = new PDFRibbonButton(action, PDFRibbonButton::Size::Large, label, description, fallbackIcon, this);
    button->setPrerequisite(m_prerequisite);
    m_controlsLayout->addWidget(button);
    ++m_controlCount;
    return button;
}

PDFRibbonButton* PDFRibbonGroup::addSmallAction(QAction* action, const QString& label, const QString& description)
{
    if (!action)
    {
        return nullptr;
    }

    PDFRibbonButton* button = new PDFRibbonButton(action, PDFRibbonButton::Size::Small, label, description, QIcon(), this);
    button->setPrerequisite(m_prerequisite);
    getSmallColumn()->addWidget(button, 0, Qt::AlignLeft);
    ++m_currentSmallColumnCount;
    ++m_controlCount;
    return button;
}

QToolButton* PDFRibbonGroup::addLargeMenu(const QIcon& icon, const QString& label, const QString& description, const QList<QAction*>& actions)
{
    finishColumn();
    QToolButton* button = createMenuButton(icon, label, description, actions, true);
    if (button)
    {
        m_controlsLayout->addWidget(button);
        ++m_controlCount;
    }
    return button;
}

QToolButton* PDFRibbonGroup::addSmallMenu(const QIcon& icon, const QString& label, const QString& description, const QList<QAction*>& actions)
{
    QToolButton* button = createMenuButton(icon, label, description, actions, false);
    if (button)
    {
        getSmallColumn()->addWidget(button, 0, Qt::AlignLeft);
        ++m_currentSmallColumnCount;
        ++m_controlCount;
    }
    return button;
}

void PDFRibbonGroup::addWidgetRow(const QList<QWidget*>& widgets)
{
    if (widgets.isEmpty())
    {
        return;
    }

    QHBoxLayout* rowLayout = new QHBoxLayout();
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(pdf::PDFWidgetUtils::scaleDPI_x(this, 2));
    for (QWidget* widget : widgets)
    {
        widget->setParent(this);
        rowLayout->addWidget(widget);
    }
    rowLayout->addStretch(1);

    getSmallColumn()->addLayout(rowLayout);
    ++m_currentSmallColumnCount;
    m_controlCount += int(widgets.size());
}

void PDFRibbonGroup::finishColumn()
{
    if (m_currentSmallColumn)
    {
        m_currentSmallColumn->addStretch(1);
        m_currentSmallColumn = nullptr;
        m_currentSmallColumnCount = 0;
    }
}

QString PDFRibbonGroup::createToolTip(const QString& title, const QString& description, const QKeySequence& shortcut)
{
    QString toolTip = QString("<p style='white-space:pre'><b>%1</b>").arg(title.toHtmlEscaped().replace('\n', ' '));
    if (!shortcut.isEmpty())
    {
        toolTip += QString(" (%1)").arg(shortcut.toString(QKeySequence::NativeText).toHtmlEscaped());
    }
    toolTip += "</p>";

    if (!description.isEmpty())
    {
        toolTip += QString("<p>%1</p>").arg(description.toHtmlEscaped());
    }

    return toolTip;
}

QString PDFRibbonGroup::cleanText(QString text)
{
    // Accelerators of the menus are not used in the ribbon, "&&" is a real ampersand
    text.replace("&&", QChar(0x0001));
    text.remove('&');
    text.replace(QChar(0x0001), "&");

    if (text.endsWith("..."))
    {
        text.chop(3);
    }

    return text.trimmed();
}

QString PDFRibbonGroup::wrapLargeLabel(const QString& label)
{
    if (label.contains('\n'))
    {
        return label;
    }

    if (label.size() <= 8)
    {
        return label + "\n ";
    }

    const int middle = int(label.size()) / 2;
    int bestPosition = -1;
    for (int i = 0; i < label.size(); ++i)
    {
        if (label[i] == ' ' && (bestPosition == -1 || qAbs(i - middle) < qAbs(bestPosition - middle)))
        {
            bestPosition = i;
        }
    }

    if (bestPosition == -1)
    {
        return label + "\n ";
    }

    QString wrappedLabel = label;
    wrappedLabel[bestPosition] = '\n';
    return wrappedLabel;
}

QVBoxLayout* PDFRibbonGroup::getSmallColumn()
{
    if (m_currentSmallColumn && m_currentSmallColumnCount >= 3)
    {
        finishColumn();
    }

    if (!m_currentSmallColumn)
    {
        m_currentSmallColumn = new QVBoxLayout();
        m_currentSmallColumn->setContentsMargins(0, 0, 0, 0);
        m_currentSmallColumn->setSpacing(pdf::PDFWidgetUtils::scaleDPI_y(this, 1));
        m_controlsLayout->addLayout(m_currentSmallColumn);
        m_currentSmallColumnCount = 0;
    }

    return m_currentSmallColumn;
}

QToolButton* PDFRibbonGroup::createMenuButton(const QIcon& icon, const QString& label, const QString& description, const QList<QAction*>& actions, bool large)
{
    QList<QAction*> validActions;
    for (QAction* action : actions)
    {
        if (action)
        {
            validActions << action;
        }
    }

    if (validActions.isEmpty())
    {
        return nullptr;
    }

    QToolButton* button = new QToolButton(this);
    button->setAutoRaise(true);
    button->setFocusPolicy(Qt::NoFocus);
    button->setIcon(icon);
    button->setText(large ? wrapLargeLabel(label) : label);
    button->setToolTip(createToolTip(label, description, QKeySequence()));
    button->setPopupMode(QToolButton::InstantPopup);

    if (large)
    {
        button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        button->setIconSize(pdf::PDFWidgetUtils::scaleDPI(this, QSize(32, 32)));
        button->setMinimumWidth(pdf::PDFWidgetUtils::scaleDPI_x(this, 56));
        button->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        button->setProperty("ribbonLarge", true);
        button->setProperty("ribbonMenu", true);
    }
    else
    {
        button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        button->setIconSize(pdf::PDFWidgetUtils::scaleDPI(this, QSize(16, 16)));
        button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        button->setProperty("ribbonSmall", true);
    }

    QMenu* menu = new QMenu(button);
    menu->setToolTipsVisible(true);
    for (QAction* action : validActions)
    {
        menu->addAction(action);
    }
    button->setMenu(menu);

    if (QAction* prerequisite = m_prerequisite)
    {
        // Actions of the menu are available after the prerequisite is turned on
        connect(menu, &QMenu::aboutToShow, button, [prerequisite]()
        {
            if (!prerequisite->isChecked() && prerequisite->isEnabled())
            {
                prerequisite->trigger();
            }
        });
    }

    return button;
}

PDFRibbonPage::PDFRibbonPage(QWidget* parent) :
    QWidget(parent),
    m_layout(new QHBoxLayout(this))
{
    setObjectName("ribbonPage");
    m_layout->setContentsMargins(pdf::PDFWidgetUtils::scaleDPI_x(this, 4), 0, pdf::PDFWidgetUtils::scaleDPI_x(this, 4), 0);
    m_layout->setSpacing(0);
}

PDFRibbonGroup* PDFRibbonPage::addGroup(const QString& title)
{
    PDFRibbonGroup* group = new PDFRibbonGroup(title, this);
    m_groups.push_back(group);
    return group;
}

void PDFRibbonPage::finish()
{
    for (PDFRibbonGroup* group : m_groups)
    {
        group->finishColumn();

        if (group->isEmpty())
        {
            delete group;
            continue;
        }

        if (m_layout->count() > 0)
        {
            QFrame* separator = new QFrame(this);
            separator->setObjectName("ribbonGroupSeparator");
            separator->setFrameShape(QFrame::VLine);
            separator->setFrameShadow(QFrame::Plain);
            m_layout->addWidget(separator);
        }

        m_layout->addWidget(group);
    }

    m_groups.clear();
    m_layout->addStretch(1);

    // Controls are not squeezed, when the window is narrow - the page is scrolled instead
    setMinimumWidth(sizeHint().width());
}

PDFRibbon::PDFRibbon(QWidget* parent) :
    QFrame(parent),
    m_tabBar(nullptr),
    m_pages(nullptr),
    m_quickAccessLayout(nullptr),
    m_cornerLayout(nullptr),
    m_scrollLeftButton(nullptr),
    m_scrollRightButton(nullptr)
{
    setObjectName("ribbon");

    updateStyleSheet();

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    QHBoxLayout* headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(pdf::PDFWidgetUtils::scaleDPI_x(this, 4), pdf::PDFWidgetUtils::scaleDPI_y(this, 2),
                                     pdf::PDFWidgetUtils::scaleDPI_x(this, 4), 0);
    headerLayout->setSpacing(pdf::PDFWidgetUtils::scaleDPI_x(this, 2));

    m_quickAccessLayout = new QHBoxLayout();
    m_quickAccessLayout->setContentsMargins(0, 0, pdf::PDFWidgetUtils::scaleDPI_x(this, 8), 0);
    m_quickAccessLayout->setSpacing(0);
    headerLayout->addLayout(m_quickAccessLayout);

    m_tabBar = new QTabBar(this);
    m_tabBar->setObjectName("ribbonTabBar");
    m_tabBar->setDrawBase(false);
    m_tabBar->setExpanding(false);
    m_tabBar->setFocusPolicy(Qt::NoFocus);
    m_tabBar->setUsesScrollButtons(true);
    headerLayout->addWidget(m_tabBar, 1);

    m_cornerLayout = new QHBoxLayout();
    m_cornerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->addLayout(m_cornerLayout);
    layout->addLayout(headerLayout);

    QHBoxLayout* pagesLayout = new QHBoxLayout();
    pagesLayout->setContentsMargins(0, 0, 0, 0);
    pagesLayout->setSpacing(0);

    m_scrollLeftButton = new QToolButton(this);
    m_scrollLeftButton->setObjectName("ribbonScrollButton");
    m_scrollLeftButton->setArrowType(Qt::LeftArrow);
    m_scrollLeftButton->setAutoRaise(true);
    m_scrollLeftButton->setFocusPolicy(Qt::NoFocus);
    m_scrollLeftButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_scrollLeftButton->hide();

    m_scrollRightButton = new QToolButton(this);
    m_scrollRightButton->setObjectName("ribbonScrollButton");
    m_scrollRightButton->setArrowType(Qt::RightArrow);
    m_scrollRightButton->setAutoRaise(true);
    m_scrollRightButton->setFocusPolicy(Qt::NoFocus);
    m_scrollRightButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_scrollRightButton->hide();

    m_pages = new QStackedWidget(this);
    m_pages->setObjectName("ribbonPages");

    pagesLayout->addWidget(m_scrollLeftButton);
    pagesLayout->addWidget(m_pages, 1);
    pagesLayout->addWidget(m_scrollRightButton);
    layout->addLayout(pagesLayout);

    connect(m_tabBar, &QTabBar::currentChanged, this, [this](int index)
    {
        m_pages->setCurrentIndex(index);
        updateScrollButtons();
    });

    const int scrollStep = pdf::PDFWidgetUtils::scaleDPI_x(this, 120);
    connect(m_scrollLeftButton, &QToolButton::clicked, this, [this, scrollStep]() { scrollBy(-scrollStep); });
    connect(m_scrollRightButton, &QToolButton::clicked, this, [this, scrollStep]() { scrollBy(scrollStep); });
}

PDFRibbonPage* PDFRibbon::addPage(const QString& title)
{
    QScrollArea* scrollArea = new QScrollArea(m_pages);
    scrollArea->setObjectName("ribbonScrollArea");
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setWidgetResizable(true);
    scrollArea->viewport()->installEventFilter(this);

    PDFRibbonPage* page = new PDFRibbonPage(scrollArea);
    scrollArea->setWidget(page);
    connect(scrollArea->horizontalScrollBar(), &QScrollBar::rangeChanged, this, &PDFRibbon::updateScrollButtons);
    connect(scrollArea->horizontalScrollBar(), &QScrollBar::valueChanged, this, &PDFRibbon::updateScrollButtons);

    m_pages->addWidget(scrollArea);
    m_tabBar->addTab(title);
    m_scrollAreas.push_back(scrollArea);
    m_ribbonPages.push_back(page);
    return page;
}

void PDFRibbon::addQuickAccessAction(QAction* action, const QString& label, const QString& description)
{
    if (!action)
    {
        return;
    }

    PDFRibbonButton* button = new PDFRibbonButton(action, PDFRibbonButton::Size::Small, label, description, QIcon(), this);
    button->setToolButtonStyle(Qt::ToolButtonIconOnly);
    button->setProperty("ribbonQuickAccess", true);
    m_quickAccessLayout->addWidget(button);
}

void PDFRibbon::setCornerWidget(QWidget* widget)
{
    widget->setParent(this);
    m_cornerLayout->addWidget(widget);
}

void PDFRibbon::finish()
{
    int pageHeight = 0;
    for (PDFRibbonPage* page : m_ribbonPages)
    {
        page->finish();
        pageHeight = qMax(pageHeight, page->sizeHint().height());
    }

    // All pages have the same height (the highest one), so the document
    // doesn't move, when another page of the ribbon is selected.
    m_pages->setFixedHeight(pageHeight + 2);

    updateScrollButtons();
}

void PDFRibbon::setCurrentPage(int index)
{
    m_tabBar->setCurrentIndex(index);
}

bool PDFRibbon::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::Wheel)
    {
        // Pages wider than the window are scrolled by the mouse wheel
        // (the scroll bars are hidden, the arrows are shown instead).
        QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
        const int delta = wheelEvent->angleDelta().y() != 0 ? wheelEvent->angleDelta().y() : wheelEvent->angleDelta().x();
        scrollBy(-delta);
        return true;
    }

    return QFrame::eventFilter(watched, event);
}

void PDFRibbon::changeEvent(QEvent* event)
{
    QFrame::changeEvent(event);

    if (event->type() == QEvent::PaletteChange && !m_isUpdatingStyleSheet)
    {
        updateStyleSheet();
    }
}

QColor PDFRibbon::mixColors(const QColor& first, const QColor& second, qreal ratio)
{
    return QColor::fromRgbF(first.redF() * (1.0 - ratio) + second.redF() * ratio,
                            first.greenF() * (1.0 - ratio) + second.greenF() * ratio,
                            first.blueF() * (1.0 - ratio) + second.blueF() * ratio);
}

void PDFRibbon::updateStyleSheet()
{
    // Colors are computed from the palette, so the ribbon follows the light and
    // the dark theme (the roles mid/midlight of the palettes of the themes are
    // not reliable - they can be almost the same as the background).
    const QPalette palette = QApplication::palette(this);
    const QColor window = palette.color(QPalette::Window);
    const QColor text = palette.color(QPalette::WindowText);
    const QColor highlight = palette.color(QPalette::Highlight);

    const QString hoverColor = mixColors(window, text, 0.10).name();
    const QString hoverBorderColor = mixColors(window, text, 0.22).name();
    const QString checkedColor = mixColors(window, highlight, 0.28).name();
    const QString lineColor = mixColors(window, text, 0.22).name();
    const QString titleColor = mixColors(window, text, 0.60).name();
    const QString highlightColor = highlight.name();

    QString styleSheet = QString("QTabBar#ribbonTabBar::tab { padding: 5px 12px 4px 12px; border: none; border-bottom: 3px solid transparent; background: transparent; }"
                                 "QTabBar#ribbonTabBar::tab:hover { background: %1; }"
                                 "QTabBar#ribbonTabBar::tab:selected { border-bottom: 3px solid %6; }"
                                 "QStackedWidget#ribbonPages { border-top: 1px solid %4; border-bottom: 1px solid %4; }"
                                 "QFrame#ribbonGroupSeparator { color: %4; }"
                                 "QLabel#ribbonGroupTitle, QLabel#ribbonHint { color: %5; font-size: 8pt; }"
                                 "QToolButton[ribbonLarge=\"true\"], QToolButton[ribbonSmall=\"true\"], QToolButton[ribbonQuickAccess=\"true\"] { border: 1px solid transparent; border-radius: 3px; padding: 2px; }"
                                 "QToolButton[ribbonLarge=\"true\"]:hover, QToolButton[ribbonSmall=\"true\"]:hover, QToolButton[ribbonQuickAccess=\"true\"]:hover { background: %1; border-color: %2; }"
                                 "QToolButton[ribbonLarge=\"true\"]:checked, QToolButton[ribbonSmall=\"true\"]:checked { background: %3; border: 1px solid %6; }"
                                 "QToolButton[ribbonMenu=\"true\"] { padding-bottom: 10px; }"
                                 "QToolButton[ribbonLarge=\"true\"]::menu-indicator { subcontrol-position: bottom center; subcontrol-origin: padding; }"
                                 "QToolButton#ribbonScrollButton { border: none; }")
                         .arg(hoverColor, hoverBorderColor, checkedColor, lineColor, titleColor, highlightColor);

    pdf::PDFTemporaryValueChange guard(&m_isUpdatingStyleSheet, true);
    setStyleSheet(styleSheet);
}

void PDFRibbon::resizeEvent(QResizeEvent* event)
{
    QFrame::resizeEvent(event);
    updateScrollButtons();
}

void PDFRibbon::updateScrollButtons()
{
    const int index = m_pages->currentIndex();
    if (index < 0 || index >= int(m_scrollAreas.size()))
    {
        return;
    }

    QScrollBar* scrollBar = m_scrollAreas[index]->horizontalScrollBar();
    m_scrollLeftButton->setVisible(scrollBar->value() > scrollBar->minimum());
    m_scrollRightButton->setVisible(scrollBar->value() < scrollBar->maximum());
}

void PDFRibbon::scrollBy(int delta)
{
    const int index = m_pages->currentIndex();
    if (index < 0 || index >= int(m_scrollAreas.size()))
    {
        return;
    }

    QScrollBar* scrollBar = m_scrollAreas[index]->horizontalScrollBar();
    scrollBar->setValue(scrollBar->value() + delta);
}

}   // namespace pdfviewer
