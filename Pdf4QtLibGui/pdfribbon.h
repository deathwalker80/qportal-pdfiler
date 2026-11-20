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

#ifndef PDFRIBBON_H
#define PDFRIBBON_H

#include "pdfviewerglobal.h"

#include <QFrame>
#include <QToolButton>

#include <vector>

class QLabel;
class QTabBar;
class QHBoxLayout;
class QVBoxLayout;
class QScrollArea;
class QStackedWidget;

namespace pdfviewer
{

/// Button of the ribbon, which displays an action. The button has its own label
/// and tool tip (a short title and a description, like the screen tips of the
/// office applications), and it follows the state of the action (enabled,
/// visible, checked, icon).
class PDFRibbonButton : public QToolButton
{
    Q_OBJECT

public:
    enum class Size
    {
        Large,  ///< Icon above the label (label can have two lines)
        Small   ///< Icon beside the label
    };

    /// \param fallbackIcon Icon displayed, if the action has no icon
    explicit PDFRibbonButton(QAction* action, Size size, const QString& label, const QString& description, const QIcon& fallbackIcon, QWidget* parent);

    QAction* action() const { return m_action; }

    /// Sets the action, which must be turned on before the action of the button can
    /// be used (for example the editing of the page content must be turned on before
    /// a text box is inserted). The button is then available also while the
    /// prerequisite is turned off, a click turns it on first.
    void setPrerequisite(QAction* prerequisite);

private:
    void updateFromAction();
    void onClicked();

    QAction* m_action;
    QAction* m_prerequisite = nullptr;
    QString m_label;
    QString m_description;
    QIcon m_fallbackIcon;
};

/// Group of the ribbon page (for example "Clipboard" or "Font"), the controls
/// are placed in a row and the title of the group is displayed below them.
class PDFRibbonGroup : public QFrame
{
    Q_OBJECT

public:
    explicit PDFRibbonGroup(const QString& title, QWidget* parent);

    /// Adds large button for the action. Null action is ignored. The fallback
    /// icon is displayed, if the action has no icon.
    PDFRibbonButton* addLargeAction(QAction* action, const QString& label, const QString& description, const QIcon& fallbackIcon = QIcon());

    /// Adds small button for the action. Small buttons are stacked in columns of three.
    /// Null action is ignored.
    PDFRibbonButton* addSmallAction(QAction* action, const QString& label, const QString& description);

    /// Adds large button, which opens the menu with the actions. Null actions are
    /// ignored, the button is not created if no action remains.
    QToolButton* addLargeMenu(const QIcon& icon, const QString& label, const QString& description, const QList<QAction*>& actions);

    /// Adds small button, which opens the menu with the actions. Null actions are
    /// ignored, the button is not created if no action remains.
    QToolButton* addSmallMenu(const QIcon& icon, const QString& label, const QString& description, const QList<QAction*>& actions);

    /// Adds a row of widgets into the current column of the small controls.
    void addWidgetRow(const QList<QWidget*>& widgets);

    /// Finishes the current column of the small controls, the next small
    /// control starts a new column.
    void finishColumn();

    /// Returns true, if the group doesn't contain any control
    bool isEmpty() const { return m_controlCount == 0; }

    /// Sets the prerequisite action for the controls added after this call,
    /// see PDFRibbonButton::setPrerequisite. Nullptr removes the prerequisite.
    void setPrerequisite(QAction* prerequisite) { m_prerequisite = prerequisite; }

    static QString createToolTip(const QString& title, const QString& description, const QKeySequence& shortcut);
    static QString cleanText(QString text);

    /// Label of a large button is wrapped into two lines (at the space nearest
    /// to the middle of the label), if it is longer than a single word. A label
    /// of a single line gets an empty second line, so the icons of all large
    /// buttons are at the same height.
    static QString wrapLargeLabel(const QString& label);

private:
    QVBoxLayout* getSmallColumn();
    QToolButton* createMenuButton(const QIcon& icon, const QString& label, const QString& description, const QList<QAction*>& actions, bool large);

    QHBoxLayout* m_controlsLayout;
    QAction* m_prerequisite = nullptr;
    QVBoxLayout* m_currentSmallColumn;
    int m_currentSmallColumnCount;
    int m_controlCount;
};

/// Page of the ribbon (tab), contains groups
class PDFRibbonPage : public QWidget
{
    Q_OBJECT

public:
    explicit PDFRibbonPage(QWidget* parent);

    PDFRibbonGroup* addGroup(const QString& title);

    /// Removes empty groups and finishes the layout of the page
    void finish();

    /// Returns true, if the page has no group (all groups were empty)
    bool isEmpty() const { return m_isEmpty; }

private:
    QHBoxLayout* m_layout;
    std::vector<PDFRibbonGroup*> m_groups;
    bool m_isEmpty = true;
};

/// Ribbon: tabs with the pages of the commands, divided into groups, as in the
/// office applications. Quick access buttons are displayed left of the tabs,
/// the corner widget (search) right of the tabs.
class PDF4QTLIBGUILIBSHARED_EXPORT PDFRibbon : public QFrame
{
    Q_OBJECT

public:
    explicit PDFRibbon(QWidget* parent);

    PDFRibbonPage* addPage(const QString& title);

    /// Adds a quick access button (icon only) left of the tabs
    void addQuickAccessAction(QAction* action, const QString& label, const QString& description);

    void setCornerWidget(QWidget* widget);

    /// Finishes all pages, removes the empty groups and the empty pages (an application
    /// without some commands, for example the viewer, has fewer pages). Call it after all
    /// controls are added.
    void finish();

    void setCurrentPage(PDFRibbonPage* page);

signals:
    /// The palette was changed (for example the light or dark theme of the system),
    /// the colors of the ribbon are already updated. Icons drawn by the user of the
    /// ribbon should be drawn again.
    void paletteChanged();

protected:
    virtual bool eventFilter(QObject* watched, QEvent* event) override;
    virtual void resizeEvent(QResizeEvent* event) override;
    virtual void changeEvent(QEvent* event) override;

private:
    void updateStyleSheet();
    static QColor mixColors(const QColor& first, const QColor& second, qreal ratio);

    void updateScrollButtons();
    void scrollBy(int delta);

    QTabBar* m_tabBar;
    QStackedWidget* m_pages;
    QHBoxLayout* m_quickAccessLayout;
    QHBoxLayout* m_cornerLayout;
    std::vector<QScrollArea*> m_scrollAreas;
    std::vector<PDFRibbonPage*> m_ribbonPages;
    QToolButton* m_scrollLeftButton;
    QToolButton* m_scrollRightButton;
    bool m_isUpdatingStyleSheet = false;
};

}   // namespace pdfviewer

#endif // PDFRIBBON_H
