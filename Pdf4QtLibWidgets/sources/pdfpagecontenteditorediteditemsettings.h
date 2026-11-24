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

#ifndef PDFPAGECONTENTEDITOREDITEDITEMSETTINGS_H
#define PDFPAGECONTENTEDITOREDITEDITEMSETTINGS_H

#include <QDialog>
#include <QPen>
#include <QBrush>

#include <vector>

class QComboBox;
class QCheckBox;

namespace Ui
{
class PDFPageContentEditorEditedItemSettings;
}

namespace pdf
{
class PDFPageContentElementEdited;

class PDFPageContentEditorEditedItemSettings : public QWidget
{
    Q_OBJECT

public:
    explicit PDFPageContentEditorEditedItemSettings(QWidget* parent);
    virtual ~PDFPageContentEditorEditedItemSettings() override;

    void loadFromElement(PDFPageContentElementEdited* editedElement);
    void saveToElement(PDFPageContentElementEdited* editedElement);

    void setPen(const QPen& pen, bool forceUpdate);
    void setBrush(const QBrush& brush, bool forceUpdate);

    const QPen& getPen() const;
    const QBrush& getBrush() const;

private:
    void onSelectPenColorButtonClicked();
    void onSelectBrushColorButtonClicked();
    void onPenWidthChanged(double value);
    void onPenStyleChanged();
    void onBrushStyleChanged();
    void onPenColorComboTextChanged();
    void onPenColorComboIndexChanged();
    void onBrushColorComboTextChanged();
    void onBrushColorComboIndexChanged();

    void setImage(QImage image);
    void selectImage();

    enum StyleFeature
    {
        None = 0,
        Pen = 1 << 0,
        PenColor = 1 << 1,
        Brush = 1 << 2,
        StrokeFill = 1 << 3,
    };
    Q_DECLARE_FLAGS(StyleFeatures, StyleFeature)

    void setColorToComboBox(QComboBox* comboBox, QColor color);
    QIcon getIconForColor(QColor color) const;

    void setPenColor(QColor color);
    void setBrushColor(QColor color);

    /// Line of the edited text. The text of a text element is stored with formatting
    /// codes (position, font, spacing...), the user edits the plain text of the lines.
    struct TextLine
    {
        QString markup; ///< Original text of the line with all formatting codes
        QString prefix; ///< Formatting codes at the beginning of the line
        QString text;   ///< Plain text of the line
    };

    /// Splits the text with formatting codes into the lines of plain text.
    /// \param markup Text with formatting codes
    /// \param[out] lines Lines of the text
    /// \returns False, if the text cannot be edited as plain text
    static bool parseTextMarkup(const QString& markup, std::vector<TextLine>& lines);

    /// Creates the text with formatting codes from the edited plain text. Lines, which
    /// were not changed, keep their original formatting codes.
    QString createTextMarkup(const QString& plainText) const;

    /// Returns the plain text of the lines
    QString getPlainTextOfLines() const;

    void onShowTextCodesToggled(bool checked);

    Ui::PDFPageContentEditorEditedItemSettings* ui;
    QCheckBox* m_showTextCodesCheckBox = nullptr;
    std::vector<TextLine> m_textLines;
    QImage m_image;
    bool m_imageChanged = false;
    QPen m_pen;
    QBrush m_brush;
};

}   // namespace pdf

#endif // PDFPAGECONTENTEDITOREDITEDITEMSETTINGS_H
