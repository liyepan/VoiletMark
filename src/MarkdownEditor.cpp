#include "MarkdownEditor.h"

#include "MarkdownHighlighter.h"
#include "ThemeManager.h"

#include <QAbstractTextDocumentLayout>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFontMetricsF>
#include <QKeyEvent>
#include <QMimeData>
#include <QPainter>
#include <QTextBlock>
#include <QTextOption>
#include <QUrl>
#include <QWheelEvent>

namespace {

constexpr int kLineNumberRightPadding = 14;

class LineNumberArea : public QWidget
{
public:
    explicit LineNumberArea(MarkdownEditor *editor)
        : QWidget(editor)
        , m_editor(editor)
    {
        setAttribute(Qt::WA_OpaquePaintEvent, true);
    }

    QSize sizeHint() const override
    {
        return QSize(m_editor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        m_editor->lineNumberAreaPaintEvent(event);
    }

private:
    MarkdownEditor *m_editor;
};

} // namespace

MarkdownEditor::MarkdownEditor(QWidget *parent)
    : QPlainTextEdit(parent)
{
    setObjectName(QStringLiteral("markdownEditor"));
    setFrameShape(QFrame::NoFrame);
    setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    setLineWrapMode(QPlainTextEdit::WidgetWidth);
    setCursorWidth(2);
    setAcceptDrops(true);
    document()->setDocumentMargin(18);

    m_lineNumberArea = new LineNumberArea(this);
    m_highlighter = new MarkdownHighlighter(document());

    connect(this, &QPlainTextEdit::blockCountChanged,
            this, &MarkdownEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest,
            this, &MarkdownEditor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged,
            this, &MarkdownEditor::updateCurrentLineHighlight);
    connect(ThemeManager::instance(), &ThemeManager::schemeChanged,
            this, &MarkdownEditor::applyTheme);

    applyEditorFont();
    updateLineNumberAreaWidth(0);
    updateCurrentLineHighlight();
}

int MarkdownEditor::lineNumberAreaWidth() const
{
    int digits = 1;
    int blocks = qMax(1, blockCount());
    while (blocks >= 10) {
        blocks /= 10;
        ++digits;
    }
    return 18 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

void MarkdownEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(m_lineNumberArea);
    const ThemeColors &colors = ThemeManager::instance()->colors();
    painter.fillRect(event->rect(), colors.gutterBackground);

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());
    const int currentLine = textCursor().blockNumber();

    QFont numberFont = font();
    if (numberFont.pointSizeF() > 8.5)
        numberFont.setPointSizeF(numberFont.pointSizeF() - 1.5);
    painter.setFont(numberFont);
    const QFontMetrics metrics(numberFont);

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            painter.setPen(blockNumber == currentLine ? colors.gutterActiveText : colors.gutterText);
            painter.drawText(0, top, m_lineNumberArea->width() - kLineNumberRightPadding,
                             metrics.height(), Qt::AlignRight | Qt::AlignVCenter,
                             QString::number(blockNumber + 1));
        }
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void MarkdownEditor::setEditorFontPointSize(int points)
{
    const int clamped = qBound(8, points, 48);
    if (clamped == m_fontPointSize)
        return;
    m_fontPointSize = clamped;
    applyEditorFont();
}

void MarkdownEditor::applyEditorFont()
{
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setFamilies({QStringLiteral("JetBrains Mono"),
                      QStringLiteral("Cascadia Code"),
                      QStringLiteral("Source Han Mono SC"),
                      QStringLiteral("Noto Sans Mono CJK SC"),
                      QStringLiteral("Sarasa Mono SC"),
                      QStringLiteral("Consolas"),
                      QStringLiteral("DejaVu Sans Mono")});
    font.setPointSize(m_fontPointSize);
    font.setStyleHint(QFont::Monospace);
    font.setFixedPitch(true);

    setFont(font);
    document()->setDefaultFont(font);
    setTabStopDistance(QFontMetricsF(font).horizontalAdvance(QLatin1Char(' ')) * 4.0);
    updateLineNumberAreaWidth(0);
}

void MarkdownEditor::updateLineNumberAreaWidth(int)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void MarkdownEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy != 0)
        m_lineNumberArea->scroll(0, dy);
    else
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void MarkdownEditor::updateCurrentLineHighlight()
{
    QTextEdit::ExtraSelection selection;
    selection.format.setBackground(ThemeManager::instance()->colors().currentLine);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = textCursor();
    selection.cursor.clearSelection();
    setExtraSelections({selection});
}

void MarkdownEditor::applyTheme()
{
    updateCurrentLineHighlight();
    m_lineNumberArea->update();
    viewport()->update();
}

void MarkdownEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);
    const QRect rect = contentsRect();
    m_lineNumberArea->setGeometry(QRect(rect.left(), rect.top(),
                                        lineNumberAreaWidth(), rect.height()));
}

void MarkdownEditor::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Tab) {
        if (textCursor().hasSelection())
            indentSelection(false);
        else
            insertPlainText(QStringLiteral("    "));
        return;
    }
    if (event->key() == Qt::Key_Backtab) {
        indentSelection(true);
        return;
    }
    QPlainTextEdit::keyPressEvent(event);
}

void MarkdownEditor::indentSelection(bool outdent)
{
    QTextCursor cursor = textCursor();
    const int anchor = cursor.hasSelection() ? cursor.selectionStart() : cursor.position();
    const int position = cursor.hasSelection() ? cursor.selectionEnd() : cursor.position();

    QTextCursor startCursor(document());
    startCursor.setPosition(anchor);
    QTextCursor endCursor(document());
    endCursor.setPosition(position);

    const int firstBlock = startCursor.blockNumber();
    const int lastBlock = endCursor.blockNumber();
    const bool hadSelection = cursor.hasSelection();

    cursor.beginEditBlock();
    for (int index = firstBlock; index <= lastBlock; ++index) {
        QTextBlock block = document()->findBlockByNumber(index);
        QTextCursor blockCursor(block);
        if (outdent) {
            const QString text = block.text();
            int removable = 0;
            while (removable < 4 && removable < text.size()
                   && text.at(removable) == QLatin1Char(' ')) {
                ++removable;
            }
            if (removable > 0) {
                blockCursor.setPosition(block.position());
                blockCursor.setPosition(block.position() + removable, QTextCursor::KeepAnchor);
                blockCursor.removeSelectedText();
            }
        } else {
            blockCursor.setPosition(block.position());
            blockCursor.insertText(QStringLiteral("    "));
        }
    }
    cursor.endEditBlock();

    // 还原选区（行宽变化后按块首尾重新计算）。
    if (hadSelection) {
        QTextCursor from(document());
        from.setPosition(document()->findBlockByNumber(firstBlock).position());
        QTextCursor to(document());
        const QTextBlock last = document()->findBlockByNumber(lastBlock);
        to.setPosition(last.position() + last.length() - 1);
        from.setPosition(to.position(), QTextCursor::KeepAnchor);
        setTextCursor(from);
    }
}

void MarkdownEditor::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers().testFlag(Qt::ControlModifier)) {
        const int delta = event->angleDelta().y();
        if (delta != 0) {
            emit zoomStepRequested(delta > 0 ? 1 : -1);
            event->accept();
            return;
        }
    }
    QPlainTextEdit::wheelEvent(event);
}

bool MarkdownEditor::acceptsLocalFiles(const QMimeData *mime) const
{
    if (!mime->hasUrls())
        return false;
    const QList<QUrl> urls = mime->urls();
    for (const QUrl &url : urls) {
        if (url.isLocalFile() && QFileInfo(url.toLocalFile()).isFile())
            return true;
    }
    return false;
}

void MarkdownEditor::dragEnterEvent(QDragEnterEvent *event)
{
    if (acceptsLocalFiles(event->mimeData())) {
        event->acceptProposedAction();
        return;
    }
    QPlainTextEdit::dragEnterEvent(event);
}

void MarkdownEditor::dragMoveEvent(QDragMoveEvent *event)
{
    if (acceptsLocalFiles(event->mimeData())) {
        event->acceptProposedAction();
        return;
    }
    QPlainTextEdit::dragMoveEvent(event);
}

void MarkdownEditor::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        const QList<QUrl> urls = event->mimeData()->urls();
        for (const QUrl &url : urls) {
            if (url.isLocalFile() && QFileInfo(url.toLocalFile()).isFile()) {
                event->acceptProposedAction();
                emit fileDropped(url.toLocalFile());
                return;
            }
        }
    }
    QPlainTextEdit::dropEvent(event);
}
