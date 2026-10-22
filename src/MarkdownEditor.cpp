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
#include <QPaintEvent>
#include <QRegularExpression>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextOption>
#include <QUrl>
#include <QWheelEvent>

namespace {

constexpr int kLineNumberRightPadding = 14;
constexpr int kMaxTextWidth = 760;

QStringList proseFamilies()
{
    return {QStringLiteral("Noto Sans CJK SC"), QStringLiteral("Source Han Sans SC"),
            QStringLiteral("WenQuanYi Micro Hei"), QStringLiteral("Microsoft YaHei"),
            QStringLiteral("PingFang SC"), QStringLiteral("Segoe UI"),
            QStringLiteral("DejaVu Sans")};
}

QStringList monospaceFamilies()
{
    return {QStringLiteral("JetBrains Mono"), QStringLiteral("Cascadia Code"),
            QStringLiteral("Source Han Mono SC"), QStringLiteral("Noto Sans Mono CJK SC"),
            QStringLiteral("Sarasa Mono SC"), QStringLiteral("Consolas"),
            QStringLiteral("DejaVu Sans Mono")};
}

const QString &fenceMarker()
{
    static const QString marker = QString(QChar(96)).repeated(3);
    return marker;
}

bool isFenceLine(const QTextBlock &block)
{
    return block.isValid() && block.text().trimmed().startsWith(fenceMarker());
}

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
    m_activeBlock = textCursor().blockNumber();
    m_highlighter->setActiveBlock(m_activeBlock);

    connect(this, &QPlainTextEdit::blockCountChanged,
            this, &MarkdownEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest,
            this, &MarkdownEditor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged,
            this, &MarkdownEditor::updateCurrentLineHighlight);
    connect(this, &QPlainTextEdit::cursorPositionChanged,
            this, &MarkdownEditor::updateActiveBlock);
    connect(ThemeManager::instance(), &ThemeManager::schemeChanged,
            this, &MarkdownEditor::applyTheme);

    m_lineNumberArea->setVisible(m_sourceMode);
    applyEditorFont();
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
    QFont font;
    if (m_sourceMode) {
        font.setFamilies(monospaceFamilies());
        font.setStyleHint(QFont::Monospace);
        font.setFixedPitch(true);
    } else {
        font.setFamilies(proseFamilies());
        font.setStyleHint(QFont::SansSerif);
    }
    font.setPointSize(m_fontPointSize);

    setFont(font);
    document()->setDefaultFont(font);
    setTabStopDistance(QFontMetricsF(font).horizontalAdvance(QLatin1Char(' ')) * 4.0);
    updateViewportMargins();

    // 标题字号是根据正文大小算的，缩放 / 换字体后要让高亮层重建格式。
    if (m_highlighter)
        m_highlighter->refreshAppearance();
}

void MarkdownEditor::updateViewportMargins()
{
    const int width = contentsRect().width();
    int left = 0;
    int right = 0;

    if (m_sourceMode) {
        left = lineNumberAreaWidth();
    } else if (width > kMaxTextWidth + 120) {
        // 编辑模式：正文限制在一段舒适宽度里并居中，像一页文档。
        left = right = (width - kMaxTextWidth) / 2;
    }
    setViewportMargins(left, 0, right, 0);
}

void MarkdownEditor::updateLineNumberAreaWidth(int)
{
    updateViewportMargins();
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

void MarkdownEditor::applyBlockRhythm()
{
    if (!document())
        return;

    QTextBlockFormat format;
    format.setLineHeight(150, QTextBlockFormat::ProportionalHeight);
    format.setTopMargin(2);
    format.setBottomMargin(6);

    const bool wasModified = document()->isModified();

    QTextCursor cursor(document());
    cursor.beginEditBlock();
    for (QTextBlock block = document()->firstBlock(); block.isValid(); block = block.next()) {
        QTextCursor blockCursor(block);
        blockCursor.setBlockFormat(format);
    }
    cursor.endEditBlock();

    document()->setModified(wasModified);
}

void MarkdownEditor::setSourceMode(bool enabled)
{
    if (m_sourceMode == enabled)
        return;

    m_sourceMode = enabled;
    m_activeBlock = textCursor().blockNumber();
    m_highlighter->setActiveBlock(m_activeBlock);
    m_highlighter->setRendered(!enabled);
    m_lineNumberArea->setVisible(enabled);

    // 字体、居中和标题格式都跟模式绑定，换模式时整套重建。
    applyEditorFont();
    updateViewportMargins();
    updateCurrentLineHighlight();
    viewport()->update();
    emit sourceModeChanged(enabled);
}

void MarkdownEditor::updateActiveBlock()
{
    if (m_sourceMode)
        return;

    const int blockNumber = textCursor().blockNumber();
    if (blockNumber == m_activeBlock)
        return;

    const QTextBlock previous = document()->findBlockByNumber(m_activeBlock);
    m_activeBlock = blockNumber;
    m_highlighter->setActiveBlock(blockNumber);

    // 只重排两行：离开的那行重新隐藏标记，进入的那行露出来。
    if (previous.isValid())
        m_highlighter->rehighlightBlock(previous);
    m_highlighter->rehighlightBlock(textCursor().block());
}

void MarkdownEditor::selectedBlockRange(int *firstBlock, int *lastBlock) const
{
    const QTextCursor cursor = textCursor();
    int first = cursor.blockNumber();
    int last = first;

    if (cursor.hasSelection()) {
        QTextCursor start(document());
        start.setPosition(cursor.selectionStart());
        QTextCursor end(document());
        end.setPosition(cursor.selectionEnd());
        first = start.blockNumber();
        last = end.blockNumber();
        // 选区正好停在下一行行首时，不要把那一行算进来。
        if (end.positionInBlock() == 0 && last > first)
            --last;
    }

    if (firstBlock) *firstBlock = first;
    if (lastBlock) *lastBlock = last;
}

void MarkdownEditor::selectBlocks(int firstBlock, int lastBlock)
{
    const QTextBlock first = document()->findBlockByNumber(firstBlock);
    const QTextBlock last = document()->findBlockByNumber(lastBlock);
    if (!first.isValid() || !last.isValid())
        return;

    QTextCursor cursor(document());
    cursor.setPosition(first.position());
    cursor.setPosition(last.position() + last.text().length(), QTextCursor::KeepAnchor);
    setTextCursor(cursor);
}

void MarkdownEditor::setHeadingLevel(int level)
{
    level = qBound(0, level, 6);

    int firstBlock = 0;
    int lastBlock = 0;
    selectedBlockRange(&firstBlock, &lastBlock);

    static const QRegularExpression headingPattern(QStringLiteral("^\\s{0,3}#{1,6}\\s+"));

    bool alreadySame = level > 0;
    if (alreadySame) {
        const QString prefix = QString(level, QChar('#')).append(QLatin1Char(' '));
        for (int index = firstBlock; index <= lastBlock; ++index) {
            if (!document()->findBlockByNumber(index).text().startsWith(prefix)) {
                alreadySame = false;
                break;
            }
        }
    }

    const int target = alreadySame ? 0 : level;
    const QString newPrefix = target > 0
                                  ? QString(target, QChar('#')).append(QLatin1Char(' '))
                                  : QString();

    QTextCursor edit(document());
    edit.beginEditBlock();
    for (int index = firstBlock; index <= lastBlock; ++index) {
        const QTextBlock block = document()->findBlockByNumber(index);
        const QString original = block.text();
        QString text = original;
        text.remove(headingPattern);
        if (!newPrefix.isEmpty())
            text.prepend(newPrefix);

        QTextCursor blockCursor(block);
        blockCursor.setPosition(block.position());
        blockCursor.setPosition(block.position() + original.length(), QTextCursor::KeepAnchor);
        blockCursor.insertText(text);
    }
    edit.endEditBlock();

    selectBlocks(firstBlock, lastBlock);
}

void MarkdownEditor::toggleCodeBlock()
{
    int firstBlockNumber = 0;
    int lastBlockNumber = 0;
    selectedBlockRange(&firstBlockNumber, &lastBlockNumber);

    const QTextBlock first = document()->findBlockByNumber(firstBlockNumber);
    const QTextBlock last = document()->findBlockByNumber(lastBlockNumber);
    if (!first.isValid() || !last.isValid())
        return;

    // 整块删掉一行（含行尾换行）。
    const auto removeBlock = [this](const QTextBlock &block) {
        QTextCursor cursor(block);
        cursor.setPosition(block.position());
        const int end = qMin(block.position() + block.length(), document()->characterCount() - 1);
        cursor.setPosition(end, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
    };

    QTextCursor edit(document());

    // 1) 选中的首尾行本身就是围栏 → 拆掉这两行。
    if (lastBlockNumber > firstBlockNumber && isFenceLine(first) && isFenceLine(last)) {
        edit.beginEditBlock();
        removeBlock(last);
        removeBlock(first);
        edit.endEditBlock();
        return;
    }

    // 2) 选中内容整体落在某个围栏块内部 → 拆掉外层围栏。
    QTextBlock opener;
    QTextBlock closer;
    if (enclosingFence(firstBlockNumber, lastBlockNumber, &opener, &closer)) {
        edit.beginEditBlock();
        removeBlock(closer);
        removeBlock(opener);
        edit.endEditBlock();
        return;
    }

    // 3) 包成围栏代码块。
    const int startPosition = first.position();
    const int endPosition = last.position() + last.text().length();

    edit.beginEditBlock();
    QTextCursor tail(document());
    tail.setPosition(endPosition);
    tail.insertText(QLatin1Char('\n') + fenceMarker());
    QTextCursor head(document());
    head.setPosition(startPosition);
    head.insertText(fenceMarker() + QLatin1Char('\n'));
    edit.endEditBlock();

    QTextCursor selection(document());
    selection.setPosition(startPosition);
    selection.setPosition(endPosition + 8, QTextCursor::KeepAnchor);
    setTextCursor(selection);
}

bool MarkdownEditor::enclosingFence(int firstBlockNumber, int lastBlockNumber,
                                    QTextBlock *opener, QTextBlock *closer) const
{
    bool inFence = false;
    bool captured = false;
    QTextBlock candidate;

    for (QTextBlock block = document()->firstBlock(); block.isValid(); block = block.next()) {
        const int number = block.blockNumber();
        const bool fenceLine = isFenceLine(block);

        if (number == firstBlockNumber) {
            captured = inFence;
            if (opener)
                *opener = candidate;
        }
        if (captured && number > lastBlockNumber && fenceLine) {
            if (closer)
                *closer = block;
            return true;
        }
        if (fenceLine) {
            if (!inFence) {
                inFence = true;
                candidate = block;
            } else {
                inFence = false;
                candidate = QTextBlock();
            }
        }
    }
    return false;
}

void MarkdownEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);
    updateViewportMargins();
    const QRect rect = contentsRect();
    m_lineNumberArea->setGeometry(QRect(rect.left(), rect.top(),
                                        lineNumberAreaWidth(), rect.height()));
}

void MarkdownEditor::paintEvent(QPaintEvent *event)
{
    QPlainTextEdit::paintEvent(event);
    if (!m_sourceMode)
        return;

    // 源代码模式：每行底下画一条极细的分隔线，像稿纸一样。
    QPainter painter(viewport());
    QColor lineColor = ThemeManager::instance()->colors().border;
    lineColor.setAlpha(120);
    painter.setPen(lineColor);

    QTextBlock block = firstVisibleBlock();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());
    const int width = viewport()->width();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top())
            painter.drawLine(0, bottom - 1, width, bottom - 1);
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
    }
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
