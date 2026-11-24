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

#include "pdfpagecontenteditorediteditemsettings.h"
#include "ui_pdfpagecontenteditorediteditemsettings.h"

#include "pdfpagecontentelements.h"
#include "pdfpagecontenteditorprocessor.h"
#include "pdfwidgetutils.h"
#include "pdfpainterutils.h"

#include <QDir>
#include <QFileDialog>
#include <QImageReader>
#include <QStandardPaths>
#include <QColorDialog>
#include <QCheckBox>
#include <QLabel>

namespace pdf
{

PDFPageContentEditorEditedItemSettings::PDFPageContentEditorEditedItemSettings(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::PDFPageContentEditorEditedItemSettings)
{
    ui->setupUi(this);
    connect(ui->loadImageButton, &QPushButton::clicked, this, &PDFPageContentEditorEditedItemSettings::selectImage);

    for (const QString& colorName : QColor::colorNames())
    {
        QColor color(colorName);
        QIcon icon = getIconForColor(color);

        ui->penColorCombo->addItem(icon, colorName, color);
        ui->brushColorCombo->addItem(icon, colorName, color);
    }

    ui->penStyleCombo->addItem(tr("Solid"), int(Qt::SolidLine));
    ui->penStyleCombo->addItem(tr("Dashed"), int(Qt::DashLine));
    ui->penStyleCombo->addItem(tr("Dotted"), int(Qt::DotLine));
    ui->penStyleCombo->addItem(tr("Dash-dot"), int(Qt::DashDotLine));
    ui->penStyleCombo->addItem(tr("Dash-dot-dot"), int(Qt::DashDotDotLine));
    ui->penStyleCombo->addItem(tr("Custom"), int(Qt::CustomDashLine));
    ui->brushStyleCombo->addItem(tr("Solid"), int(Qt::SolidPattern));

    connect(ui->selectPenColorButton, &QToolButton::clicked, this, &PDFPageContentEditorEditedItemSettings::onSelectPenColorButtonClicked);
    connect(ui->selectBrushColorButton, &QToolButton::clicked, this, &PDFPageContentEditorEditedItemSettings::onSelectBrushColorButtonClicked);
    connect(ui->penWidthEdit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PDFPageContentEditorEditedItemSettings::onPenWidthChanged);
    connect(ui->penStyleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PDFPageContentEditorEditedItemSettings::onPenStyleChanged);
    connect(ui->brushStyleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PDFPageContentEditorEditedItemSettings::onBrushStyleChanged);
    connect(ui->penColorCombo->lineEdit(), &QLineEdit::editingFinished, this, &PDFPageContentEditorEditedItemSettings::onPenColorComboTextChanged);
    connect(ui->penColorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PDFPageContentEditorEditedItemSettings::onPenColorComboIndexChanged);
    connect(ui->brushColorCombo->lineEdit(), &QLineEdit::editingFinished, this, &PDFPageContentEditorEditedItemSettings::onBrushColorComboTextChanged);
    connect(ui->brushColorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PDFPageContentEditorEditedItemSettings::onBrushColorComboIndexChanged);

    // The text is edited as plain text, formatting codes are displayed on demand only
    QLabel* textHintLabel = new QLabel(tr("Type the new text and confirm with OK."), ui->textGroupBox);
    textHintLabel->setWordWrap(true);
    ui->verticalLayout_3->insertWidget(0, textHintLabel);

    m_showTextCodesCheckBox = new QCheckBox(tr("Show formatting codes (for experts)"), ui->textGroupBox);
    ui->verticalLayout_3->addWidget(m_showTextCodesCheckBox);
    connect(m_showTextCodesCheckBox, &QCheckBox::toggled, this, &PDFPageContentEditorEditedItemSettings::onShowTextCodesToggled);
}

bool PDFPageContentEditorEditedItemSettings::parseTextMarkup(const QString& markup, std::vector<TextLine>& lines)
{
    lines.clear();

    auto unescape = [](QString text)
    {
        text.replace("&lt;", "<");
        text.replace("&gt;", ">");
        text.replace("&quot;", "\"");
        text.replace("&apos;", "'");
        text.replace("&#39;", "'");
        text.replace("&amp;", "&");
        return text;
    };

    // Advance (in thousandths of the font size, negative value moves the text
    // to the right), which is displayed as a space between the words
    constexpr double WORD_SPACE_ADVANCE = -180.0;

    TextLine currentLine;
    QString pendingCodes;
    bool hasText = false;

    qsizetype i = 0;
    const qsizetype count = markup.size();
    while (i < count)
    {
        if (markup[i] == QChar('<'))
        {
            const qsizetype end = markup.indexOf(QChar('>'), i);
            if (end == -1)
            {
                // Invalid code
                return false;
            }

            const QString code = markup.mid(i, end - i + 1);
            i = end + 1;

            if (code.startsWith("<character"))
            {
                // Character without unicode value, it cannot be typed by the user
                return false;
            }

            if (!hasText)
            {
                currentLine.prefix += code;
                currentLine.markup += code;
                continue;
            }

            if (code.startsWith("<space") && pendingCodes.isEmpty())
            {
                currentLine.markup += code;

                const qsizetype valueStart = code.indexOf(QChar('"'));
                const qsizetype valueEnd = code.indexOf(QChar('"'), valueStart + 1);
                if (valueStart != -1 && valueEnd != -1)
                {
                    bool ok = false;
                    const double advance = code.mid(valueStart + 1, valueEnd - valueStart - 1).toDouble(&ok);
                    if (ok && advance <= WORD_SPACE_ADVANCE && !currentLine.text.endsWith(QChar(' ')))
                    {
                        currentLine.text += QChar(' ');
                    }
                }
                continue;
            }

            pendingCodes += code;
        }
        else
        {
            qsizetype end = markup.indexOf(QChar('<'), i);
            if (end == -1)
            {
                end = count;
            }

            const QString rawText = markup.mid(i, end - i);
            const QString text = unescape(rawText);
            i = end;

            if (text.contains(QChar('\n')))
            {
                // Line break is used as the separator of the lines
                return false;
            }

            if (!pendingCodes.isEmpty())
            {
                if (pendingCodes.contains("<tpos") || pendingCodes.contains("<tmatrix"))
                {
                    // Text is positioned - new line
                    lines.push_back(currentLine);
                    currentLine = TextLine();
                    currentLine.prefix = pendingCodes;
                }

                currentLine.markup += pendingCodes;
                pendingCodes.clear();
            }

            currentLine.markup += rawText;
            currentLine.text += text;
            hasText = true;
        }
    }

    currentLine.markup += pendingCodes;
    lines.push_back(currentLine);
    return true;
}

QString PDFPageContentEditorEditedItemSettings::createTextMarkup(const QString& plainText) const
{
    if (m_textLines.empty())
    {
        return plainText.toHtmlEscaped();
    }

    QStringList texts = plainText.split(QChar('\n'));

    // Lines, which were added by the user, have no position - they continue on the last line
    while (texts.size() > qsizetype(m_textLines.size()))
    {
        const QString text = texts.takeLast();
        texts.back() += QChar(' ') + text;
    }

    QString markup;
    for (size_t i = 0; i < m_textLines.size(); ++i)
    {
        const TextLine& line = m_textLines[i];
        const QString text = qsizetype(i) < texts.size() ? texts[qsizetype(i)] : QString();

        if (text == line.text)
        {
            // Unchanged line keeps all its formatting codes
            markup += line.markup;
        }
        else
        {
            markup += line.prefix;
            markup += text.toHtmlEscaped();
        }
    }

    return markup;
}

QString PDFPageContentEditorEditedItemSettings::getPlainTextOfLines() const
{
    QStringList texts;
    for (const TextLine& line : m_textLines)
    {
        texts << line.text;
    }
    return texts.join(QChar('\n'));
}

void PDFPageContentEditorEditedItemSettings::onShowTextCodesToggled(bool checked)
{
    if (checked)
    {
        ui->plainTextEdit->setPlainText(createTextMarkup(ui->plainTextEdit->toPlainText()));
        return;
    }

    std::vector<TextLine> lines;
    if (parseTextMarkup(ui->plainTextEdit->toPlainText(), lines))
    {
        m_textLines = std::move(lines);
        ui->plainTextEdit->setPlainText(getPlainTextOfLines());
    }
    else
    {
        // Text cannot be displayed without the formatting codes
        QSignalBlocker blocker(m_showTextCodesCheckBox);
        m_showTextCodesCheckBox->setChecked(true);
    }
}

QIcon PDFPageContentEditorEditedItemSettings::getIconForColor(QColor color) const
{
    QIcon icon;

    QSize iconSize = PDFWidgetUtils::scaleDPI(this, QSize(16, 16));

    QPixmap pixmap(iconSize.width(), iconSize.height());
    pixmap.fill(color);
    icon.addPixmap(pixmap);

    return icon;
}

PDFPageContentEditorEditedItemSettings::~PDFPageContentEditorEditedItemSettings()
{
    delete ui;
}

void PDFPageContentEditorEditedItemSettings::loadFromElement(PDFPageContentElementEdited* editedElement)
{
    const PDFEditedPageContentElement* contentElement = editedElement->getElement();

    ui->tabWidget->removeTab(ui->tabWidget->indexOf(ui->imageTab));
    ui->tabWidget->removeTab(ui->tabWidget->indexOf(ui->textTab));
    ui->tabWidget->removeTab(ui->tabWidget->indexOf(ui->styleTab));
    ui->tabWidget->removeTab(ui->tabWidget->indexOf(ui->transformationTab));

    if (const PDFEditedPageContentElementImage* imageElement = contentElement->asImage())
    {
        ui->tabWidget->addTab(ui->imageTab, tr("Image"));
        m_image = imageElement->getImage();
        m_imageChanged = false;
        setImage(imageElement->getImage());
    }

    if (PDFEditedPageContentElementText* textElement = editedElement->getElement()->asText())
    {
        ui->tabWidget->addTab(ui->textTab, tr("Text"));
        QString text = textElement->getItemsAsText();

        QSignalBlocker blocker(m_showTextCodesCheckBox);
        if (parseTextMarkup(text, m_textLines))
        {
            m_showTextCodesCheckBox->setChecked(false);
            m_showTextCodesCheckBox->setEnabled(true);
            ui->plainTextEdit->setPlainText(getPlainTextOfLines());
        }
        else
        {
            // Text cannot be displayed without the formatting codes
            m_textLines.clear();
            m_showTextCodesCheckBox->setChecked(true);
            m_showTextCodesCheckBox->setEnabled(false);
            ui->plainTextEdit->setPlainText(text);
        }
    }

    if (editedElement->getElement()->asText() || editedElement->getElement()->asPath())
    {
        ui->tabWidget->addTab(ui->styleTab, tr("Style"));
    }

    QTransform matrix = editedElement->getElement()->getTransform();
    PDFTransformationDecomposition decomposedTransformation = PDFPainterHelper::decomposeTransform(matrix);

    ui->rotationAngleEdit->setValue(qRadiansToDegrees(decomposedTransformation.rotationAngle));
    ui->scaleInXEdit->setValue(decomposedTransformation.scaleX);
    ui->scaleInYEdit->setValue(decomposedTransformation.scaleY);
    ui->shearFactorEdit->setValue(decomposedTransformation.shearFactor);
    ui->translateInXEdit->setValue(decomposedTransformation.translateX);
    ui->translateInYEdit->setValue(decomposedTransformation.translateY);

    ui->tabWidget->addTab(ui->transformationTab, tr("Transformation"));

    // Style
    const PDFEditedPageContentElement* element = editedElement->getElement();

    StyleFeatures features = None;

    if (element->asPath())
    {
        features.setFlag(Pen);
        features.setFlag(PenColor);
        features.setFlag(Brush);
        features.setFlag(StrokeFill);
    }

    if (element->asText())
    {
        features.setFlag(PenColor);
    }

    const bool hasPen = features.testFlag(Pen);
    const bool hasPenColor = features.testFlag(PenColor);
    const bool hasBrush = features.testFlag(Brush);
    const bool hasStrokeFill = features.testFlag(StrokeFill);

    ui->penWidthEdit->setEnabled(hasPen);
    ui->penWidthLabel->setEnabled(hasPen);

    ui->penStyleCombo->setEnabled(hasPen);
    ui->penStyleLabel->setEnabled(hasPen);

    ui->penColorCombo->setEnabled(hasPenColor);
    ui->penColorLabel->setEnabled(hasPenColor);
    ui->selectPenColorButton->setEnabled(hasPenColor);

    ui->brushStyleLabel->setEnabled(hasBrush);
    ui->brushStyleCombo->setEnabled(hasBrush);

    ui->brushColorCombo->setEnabled(hasBrush);
    ui->brushColorLabel->setEnabled(hasBrush);
    ui->selectBrushColorButton->setEnabled(hasBrush);

    ui->strokePathCheckBox->setEnabled(hasStrokeFill);
    ui->fillPathCheckBox->setEnabled(hasStrokeFill);

    if (const PDFEditedPageContentElementPath* pathElement = element->asPath())
    {
        ui->strokePathCheckBox->setChecked(pathElement->getStrokePath());
        ui->fillPathCheckBox->setChecked(pathElement->getFillPath());
    }

    const PDFPageContentProcessorState& graphicState = element->getState();

    QPen pen = pdf::PDFPainterHelper::createPenFromState(&graphicState, graphicState.getAlphaStroking());
    QBrush brush = pdf::PDFPainterHelper::createBrushFromState(&graphicState, graphicState.getAlphaFilling());

    if (element->asText())
    {
        pen.setColor(brush.color());
    }

    setPen(pen, true);
    setBrush(brush, true);
}

void PDFPageContentEditorEditedItemSettings::setPen(const QPen& pen, bool forceUpdate)
{
    if (m_pen != pen || forceUpdate)
    {
        m_pen = pen;
        ui->penWidthEdit->setValue(pen.widthF());
        ui->penStyleCombo->setCurrentIndex(ui->penStyleCombo->findData(int(pen.style())));
        setColorToComboBox(ui->penColorCombo, pen.color());
    }
}

void PDFPageContentEditorEditedItemSettings::setBrush(const QBrush& brush, bool forceUpdate)
{
    if (m_brush != brush || forceUpdate)
    {
        m_brush = brush;
        ui->brushStyleCombo->setCurrentIndex(ui->brushStyleCombo->findData(int(brush.style())));
        setColorToComboBox(ui->brushColorCombo, brush.color());
    }
}

void PDFPageContentEditorEditedItemSettings::setColorToComboBox(QComboBox* comboBox, QColor color)
{
    if (!color.isValid())
    {
        return;
    }

    QString name = color.name(QColor::HexArgb);

    int index = comboBox->findData(color, Qt::UserRole, Qt::MatchExactly);

    if (index == -1)
    {
        // Jakub Melka: try to find text (color name)
        index = comboBox->findText(name);
    }

    if (index != -1)
    {
        comboBox->setCurrentIndex(index);
    }
    else
    {
        comboBox->addItem(getIconForColor(color), name, color);
        comboBox->setCurrentIndex(comboBox->count() - 1);
    }
}

void PDFPageContentEditorEditedItemSettings::saveToElement(PDFPageContentElementEdited* editedElement)
{
    if (PDFEditedPageContentElementImage* imageElement = editedElement->getElement()->asImage())
    {
        // Replace the image only if the user has selected a new one. Unconditional
        // replacement would discard the original image object and degrade (or even
        // lose, if the original image could not be decoded) the image data.
        if (m_imageChanged)
        {
            imageElement->setImage(m_image);
            imageElement->setImageObject(PDFObject());
        }
    }

    if (PDFEditedPageContentElementText* textElement = editedElement->getElement()->asText())
    {
        const QString text = ui->plainTextEdit->toPlainText();
        textElement->setItemsAsText(m_showTextCodesCheckBox->isChecked() ? text : createTextMarkup(text));
    }

    if (PDFEditedPageContentElementPath* pathElement = editedElement->getElement()->asPath())
    {
        pathElement->setStrokePath(ui->strokePathCheckBox->isChecked());
        pathElement->setFillPath(ui->fillPathCheckBox->isChecked());
    }

    PDFTransformationDecomposition decomposedTransformation;
    decomposedTransformation.rotationAngle = qDegreesToRadians(ui->rotationAngleEdit->value());
    decomposedTransformation.shearFactor = ui->shearFactorEdit->value();
    decomposedTransformation.scaleX = ui->scaleInXEdit->value();
    decomposedTransformation.scaleY = ui->scaleInYEdit->value();
    decomposedTransformation.translateX = ui->translateInXEdit->value();
    decomposedTransformation.translateY = ui->translateInYEdit->value();

    QTransform transform = PDFPainterHelper::composeTransform(decomposedTransformation);
    editedElement->getElement()->setTransform(transform);

    if (editedElement->getElement()->asPath())
    {
        PDFPageContentProcessorState graphicState = editedElement->getElement()->getState();

        PDFPainterHelper::applyPenToGraphicState(&graphicState, m_pen);
        PDFPainterHelper::applyBrushToGraphicState(&graphicState, m_brush);

        editedElement->getElement()->setState(graphicState);
    }

    if (editedElement->getElement()->asText())
    {
        PDFPageContentProcessorState graphicState = editedElement->getElement()->getState();

        QBrush brush(m_pen.color(), Qt::SolidPattern);
        PDFPainterHelper::applyBrushToGraphicState(&graphicState, brush);

        editedElement->getElement()->setState(graphicState);
    }
}

static int PDF_gcd(int a, int b)
{
    if (b == 0)
    {
        return a;
    }

    return PDF_gcd(b, a % b);
}

void PDFPageContentEditorEditedItemSettings::setImage(QImage image)
{
    QSize imageSize = QSize(200, 200);

    int width = image.width();
    int height = image.height();

    int n = width;
    int d = height;

    int divisor = PDF_gcd(n, d);
    if (divisor > 1)
    {
        n /= divisor;
        d /= divisor;
    }

    ui->imageWidthEdit->setText(QString("%1 px").arg(width));
    ui->imageHeightEdit->setText(QString("%1 px").arg(height));
    ui->imageRatioEdit->setText(QString("%1 : %2").arg(n).arg(d));

    image.setDevicePixelRatio(this->devicePixelRatioF());
    image = image.scaled(imageSize * this->devicePixelRatioF(), Qt::KeepAspectRatio);

    ui->imageLabel->setPixmap(QPixmap::fromImage(image));
    ui->imageLabel->setFixedSize(PDFWidgetUtils::scaleDPI(this, imageSize));
}

void PDFPageContentEditorEditedItemSettings::selectImage()
{
    QString imageDirectory;

    QStringList pictureDirectiories = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
    if (!pictureDirectiories.isEmpty())
    {
        imageDirectory = pictureDirectiories.last();
    }
    else
    {
        imageDirectory = QDir::currentPath();
    }

    QList<QByteArray> mimeTypes = QImageReader::supportedMimeTypes();
    QStringList mimeTypeFilters;
    for (const QByteArray& mimeType : mimeTypes)
    {
        mimeTypeFilters.append(mimeType);
    }

    QFileDialog dialog(this, tr("Select Image"));
    dialog.setDirectory(imageDirectory);
    dialog.setMimeTypeFilters(mimeTypeFilters);
    dialog.selectMimeTypeFilter("image/svg+xml");
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setFileMode(QFileDialog::ExistingFile);

    if (dialog.exec() == QFileDialog::Accepted)
    {
        QString fileName = dialog.selectedFiles().constFirst();
        QImageReader reader(fileName);
        QImage image = reader.read();

        if (!image.isNull())
        {
            setImage(image);
            m_image = std::move(image);
            m_imageChanged = true;
        }
    }
}

void PDFPageContentEditorEditedItemSettings::setPenColor(QColor color)
{
    if (color.isValid())
    {
        m_pen.setColor(color);
        setColorToComboBox(ui->penColorCombo, color);
    }
}

void PDFPageContentEditorEditedItemSettings::onSelectPenColorButtonClicked()
{
    QColor color = QColorDialog::getColor(m_pen.color(), this, tr("Select Color for Pen"), QColorDialog::ShowAlphaChannel);
    setPenColor(color);
}

void PDFPageContentEditorEditedItemSettings::setBrushColor(QColor color)
{
    if (color.isValid() && m_brush.color() != color)
    {
        m_brush.setColor(color);
        setColorToComboBox(ui->brushColorCombo, color);
    }
}

const QBrush& PDFPageContentEditorEditedItemSettings::getBrush() const
{
    return m_brush;
}

const QPen& PDFPageContentEditorEditedItemSettings::getPen() const
{
    return m_pen;
}

void PDFPageContentEditorEditedItemSettings::onSelectBrushColorButtonClicked()
{
    QColor color = QColorDialog::getColor(m_pen.color(), this, tr("Select Color for Brush"), QColorDialog::ShowAlphaChannel);
    setBrushColor(color);
}

void PDFPageContentEditorEditedItemSettings::onPenWidthChanged(double value)
{
    m_pen.setWidthF(value);
}

void PDFPageContentEditorEditedItemSettings::onPenStyleChanged()
{
    Qt::PenStyle penStyle = static_cast<Qt::PenStyle>(ui->penStyleCombo->currentData().toInt());
    m_pen.setStyle(penStyle);
}

void PDFPageContentEditorEditedItemSettings::onBrushStyleChanged()
{
    Qt::BrushStyle brushStyle = static_cast<Qt::BrushStyle>(ui->brushStyleCombo->currentData().toInt());
    m_brush.setStyle(brushStyle);
}

void PDFPageContentEditorEditedItemSettings::onPenColorComboTextChanged()
{
    QColor color(ui->penColorCombo->currentText());
    if (color.isValid())
    {
        setColorToComboBox(ui->penColorCombo, color);
        m_pen.setColor(color);
    }
    else if (ui->penColorCombo->currentIndex() != -1)
    {
        ui->penColorCombo->setEditText(ui->penColorCombo->itemText(ui->penColorCombo->currentIndex()));
    }
}

void PDFPageContentEditorEditedItemSettings::onPenColorComboIndexChanged()
{
    const int index = ui->penColorCombo->currentIndex();
    QColor color = ui->penColorCombo->itemData(index, Qt::UserRole).value<QColor>();
    if (color.isValid() && m_pen.color() != color)
    {
        m_pen.setColor(color);
    }
}

void PDFPageContentEditorEditedItemSettings::onBrushColorComboTextChanged()
{
    QColor color(ui->brushColorCombo->currentText());
    if (color.isValid())
    {
        setColorToComboBox(ui->brushColorCombo, color);
        m_brush.setColor(color);
    }
    else if (ui->brushColorCombo->currentIndex() != -1)
    {
        ui->brushColorCombo->setEditText(ui->brushColorCombo->itemText(ui->brushColorCombo->currentIndex()));
    }
}

void PDFPageContentEditorEditedItemSettings::onBrushColorComboIndexChanged()
{
    const int index = ui->brushColorCombo->currentIndex();
    QColor color = ui->brushColorCombo->itemData(index, Qt::UserRole).value<QColor>();
    if (color.isValid())
    {
        m_brush.setColor(color);
    }
}

}   // namespace pdf
