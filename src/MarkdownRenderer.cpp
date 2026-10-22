#include "MarkdownRenderer.h"

#include "CodeHighlight.h"

#include <QColor>
#include <QFont>
#include <QRegularExpression>
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QTextFrame>
#include <QTextTable>
#include <QTextCursor>
#include <QTextDocument>

namespace {

QStringList proseFamilies()
{
    return {QStringLiteral("Noto Sans CJK SC"), QStringLiteral("Source Han Sans SC"),
            QStringLiteral("WenQuanYi Micro Hei"), QStringLiteral("Microsoft YaHei"),
            QStringLiteral("PingFang SC"), QStringLiteral("Segoe UI"),
            QStringLiteral("DejaVu Sans")};
}

QStringList monoFamilies()
{
    return {QStringLiteral("JetBrains Mono"), QStringLiteral("Cascadia Code"),
            QStringLiteral("Source Han Mono SC"), QStringLiteral("Noto Sans Mono CJK SC"),
            QStringLiteral("Sarasa Mono SC"), QStringLiteral("Consolas"),
            QStringLiteral("DejaVu Sans Mono")};
}

QColor hex(const char *value)
{
    return QColor(QString::fromLatin1(value));
}

struct Style
{
    QColor text = hex("#1f2328");
    QColor heading = hex("#11151a");
    QColor muted = hex("#5b6570");
    QColor rule = hex("#d0d7de");
    QColor codeText = hex("#24292f");
    QColor codeBg = hex("#f3f4f6");
    QColor border = hex("#d0d7de");
    QColor tableHeader = hex("#f6f8fa");
    QColor keyword = hex("#0550ae");
    QColor string = hex("#0a7d3a");
    QColor number = hex("#953800");
    QColor comment = hex("#6e7781");
    QColor preprocessor = hex("#8250df");
    QColor link = hex("#0969da");
    QColor marker = hex("#0969da");
};

constexpr qreal kBodySize = 11.5;
constexpr qreal kCodeSize = 10.5;

bool isTableSeparator(const QString &text)
{
    static const QRegularExpression pattern(
        QStringLiteral(R"(^\s*\|?\s*:?-{2,}:?\s*(?:\|\s*:?-{2,}:?\s*)+\|?\s*$)"));
    return pattern.match(text).hasMatch();
}

QStringList splitTableRow(const QString &line)
{
    QString text = line.trimmed();
    if (text.startsWith(QLatin1Char('|')))
        text.remove(0, 1);
    if (text.endsWith(QLatin1Char('|')))
        text.chop(1);
    QStringList cells = text.split(QLatin1Char('|'));
    for (QString &cell : cells)
        cell = cell.trimmed();
    return cells;
}

class Renderer
{
public:
    explicit Renderer(QTextDocument *document)
        : m_document(document)
        , m_cursor(document)
    {
    }

    void run(const QString &markdown);

private:
    void beginBlock(const QTextBlockFormat &blockFormat, const QTextCharFormat &charFormat);
    void renderHeading(int level, const QString &text);
    void renderParagraph(const QString &text, int quoteDepth);
    void renderListItem(const QString &marker, const QString &text, int indentLevel);
    void renderCodeBlock(const QStringList &lines, const QString &info);
    void renderTable(const QStringList &rows, const QString &separator);
    QColor tokenColor(CodeHighlight::Token token) const;
    void renderRule();
    void renderInline(const QString &text, const QTextCharFormat &base);

    QTextCharFormat bodyFormat(const QColor &color = QColor()) const;
    QTextCharFormat codeFormat() const;

    QTextDocument *m_document = nullptr;
    QTextCursor m_cursor;
    bool m_firstBlock = true;
    Style m_style;
};

QTextCharFormat Renderer::bodyFormat(const QColor &color) const
{
    QTextCharFormat format;
    format.setFontFamilies(proseFamilies());
    format.setFontPointSize(kBodySize);
    format.setForeground(color.isValid() ? color : m_style.text);
    return format;
}

QTextCharFormat Renderer::codeFormat() const
{
    QTextCharFormat format;
    format.setFontFamilies(monoFamilies());
    format.setFontFixedPitch(true);
    format.setFontPointSize(kCodeSize);
    format.setForeground(m_style.codeText);
    format.setBackground(m_style.codeBg);
    return format;
}

void Renderer::beginBlock(const QTextBlockFormat &blockFormat, const QTextCharFormat &charFormat)
{
    if (m_firstBlock) {
        m_cursor.setBlockFormat(blockFormat);
        m_cursor.setBlockCharFormat(charFormat);
        m_firstBlock = false;
    } else {
        m_cursor.insertBlock(blockFormat, charFormat);
    }
    m_cursor.setCharFormat(charFormat);
}

void Renderer::renderHeading(int level, const QString &text)
{
    static const qreal scales[6] = {2.0, 1.55, 1.3, 1.15, 1.05, 1.0};
    QTextBlockFormat blockFormat;
    blockFormat.setLineHeight(135, QTextBlockFormat::ProportionalHeight);
    blockFormat.setTopMargin(level <= 2 ? 20 : 14);
    blockFormat.setBottomMargin(8);

    QTextCharFormat format;
    format.setFontFamilies(proseFamilies());
    format.setFontPointSize(kBodySize * scales[qBound(1, level, 6) - 1]);
    format.setFontWeight(QFont::Bold);
    format.setForeground(m_style.heading);

    beginBlock(blockFormat, format);
    m_cursor.insertText(text, format);
}

void Renderer::renderParagraph(const QString &text, int quoteDepth)
{
    QTextBlockFormat blockFormat;
    blockFormat.setLineHeight(150, QTextBlockFormat::ProportionalHeight);
    blockFormat.setBottomMargin(8);
    if (quoteDepth > 0) {
        blockFormat.setLeftMargin(18 + (quoteDepth - 1) * 16);
    }
    beginBlock(blockFormat, bodyFormat(quoteDepth > 0 ? m_style.muted : QColor()));

    if (quoteDepth > 0) {
        QTextCharFormat bar = bodyFormat(m_style.rule);
        bar.setFontWeight(QFont::Bold);
        m_cursor.insertText(QStringLiteral("│ "), bar);
    }
    renderInline(text, bodyFormat(quoteDepth > 0 ? m_style.muted : QColor()));
}

void Renderer::renderListItem(const QString &marker, const QString &text, int indentLevel)
{
    QTextBlockFormat blockFormat;
    blockFormat.setLineHeight(150, QTextBlockFormat::ProportionalHeight);
    blockFormat.setBottomMargin(6);
    blockFormat.setLeftMargin(18 + indentLevel * 18);
    blockFormat.setTextIndent(-14);
    beginBlock(blockFormat, bodyFormat());

    QTextCharFormat markerFormat = bodyFormat(m_style.marker);
    markerFormat.setFontWeight(QFont::DemiBold);
    m_cursor.insertText(marker + QLatin1Char(' '), markerFormat);
    renderInline(text, bodyFormat());
}

void Renderer::renderCodeBlock(const QStringList &lines, const QString &info)
{
    if (m_firstBlock) {
        // 文档一开头就是代码块：先垫一个空段落，frame 才有地方插。
        QTextBlockFormat emptyFormat;
        beginBlock(emptyFormat, bodyFormat());
    }

    QTextFrameFormat frameFormat;
    frameFormat.setBackground(m_style.codeBg);
    frameFormat.setBorder(1);
    frameFormat.setBorderBrush(m_style.border);
    frameFormat.setBorderStyle(QTextFrameFormat::BorderStyle_Solid);
    frameFormat.setPadding(8);
    frameFormat.setTopMargin(6);
    frameFormat.setBottomMargin(12);

    QTextFrame *frame = m_cursor.insertFrame(frameFormat);
    QTextCursor frameCursor(m_document);
    frameCursor.setPosition(frame->firstPosition());
    bool firstInside = true;

    const auto insideBlock = [&](const QTextBlockFormat &blockFormat,
                                 const QTextCharFormat &charFormat) {
        if (firstInside) {
            frameCursor.setBlockFormat(blockFormat);
            frameCursor.setBlockCharFormat(charFormat);
            firstInside = false;
        } else {
            frameCursor.insertBlock(blockFormat, charFormat);
        }
    };

    const QString language = CodeHighlight::displayName(CodeHighlight::fromInfoString(info));
    if (!language.isEmpty()) {
        QTextBlockFormat labelFormat;
        labelFormat.setAlignment(Qt::AlignRight);
        labelFormat.setBottomMargin(2);

        QTextCharFormat labelChar;
        labelChar.setForeground(m_style.muted);
        labelChar.setFontPointSize(kBodySize * 0.8);

        insideBlock(labelFormat, labelChar);
        frameCursor.insertText(language, labelChar);
    }

    QTextBlockFormat codeBlock;
    codeBlock.setLineHeight(130, QTextBlockFormat::ProportionalHeight);
    const QTextCharFormat format = codeFormat();

    QStringList content = lines;
    if (content.isEmpty())
        content << QString();

    const CodeHighlight::Language codeLanguage = CodeHighlight::fromInfoString(info);
    bool inComment = false;

    for (const QString &line : content) {
        insideBlock(codeBlock, format);

        bool endsInComment = inComment;
        int position = 0;

        if (codeLanguage != CodeHighlight::Language::None) {
            const QVector<CodeHighlight::Span> spans =
                CodeHighlight::tokenize(line, codeLanguage, inComment, &endsInComment);
            for (const CodeHighlight::Span &span : spans) {
                if (span.start > position)
                    frameCursor.insertText(line.mid(position, span.start - position), format);

                QTextCharFormat tokenFormat = format;
                tokenFormat.setForeground(tokenColor(span.token));
                frameCursor.insertText(line.mid(span.start, span.length), tokenFormat);
                position = span.start + span.length;
            }
        }
        inComment = endsInComment;

        if (position < line.size())
            frameCursor.insertText(line.mid(position), format);
        if (line.isEmpty())
            frameCursor.insertText(QStringLiteral(" "), format);
    }

    // 光标挪到边框外面，后面的内容接着写
    m_cursor = QTextCursor(m_document);
    m_cursor.setPosition(frame->lastPosition());
}

QColor Renderer::tokenColor(CodeHighlight::Token token) const
{
    switch (token) {
    case CodeHighlight::Token::Keyword:
    case CodeHighlight::Token::Type:
        return m_style.keyword;
    case CodeHighlight::Token::String:
        return m_style.string;
    case CodeHighlight::Token::Number:
        return m_style.number;
    case CodeHighlight::Token::Comment:
        return m_style.comment;
    case CodeHighlight::Token::Preprocessor:
        return m_style.preprocessor;
    }
    return m_style.codeText;
}

void Renderer::renderTable(const QStringList &rows, const QString &separator)
{
    if (rows.isEmpty())
        return;

    if (m_firstBlock) {
        QTextBlockFormat emptyFormat;
        beginBlock(emptyFormat, bodyFormat());
    }

    const QStringList headerCells = splitTableRow(rows.first());
    const int columns = qMax(1, headerCells.size());

    QVector<Qt::Alignment> alignments;
    for (const QString &cell : splitTableRow(separator)) {
        const bool left = cell.startsWith(QLatin1Char(':'));
        const bool right = cell.endsWith(QLatin1Char(':'));
        if (left && right)
            alignments.append(Qt::AlignHCenter);
        else if (right)
            alignments.append(Qt::AlignRight);
        else
            alignments.append(Qt::AlignLeft);
    }

    QTextTableFormat tableFormat;
    tableFormat.setBorder(1);
    tableFormat.setBorderBrush(m_style.border);
    tableFormat.setBorderStyle(QTextFrameFormat::BorderStyle_Solid);
    tableFormat.setCellSpacing(0);
    tableFormat.setCellPadding(6);
    tableFormat.setWidth(QTextLength(QTextLength::PercentageLength, 100));
    tableFormat.setTopMargin(6);
    tableFormat.setBottomMargin(12);

    QTextTable *table = m_cursor.insertTable(rows.size(), columns, tableFormat);
    for (int row = 0; row < rows.size(); ++row) {
        const QStringList cells = (row == 0) ? headerCells : splitTableRow(rows.at(row));
        for (int column = 0; column < columns; ++column) {
            QTextTableCell cell = table->cellAt(row, column);

            QTextTableCellFormat cellFormat = cell.format().toTableCellFormat();
            cellFormat.setBackground(row == 0 ? m_style.tableHeader : m_style.codeBg.lighter(104));
            cellFormat.setTopBorder(1);
            cellFormat.setBottomBorder(1);
            cellFormat.setLeftBorder(1);
            cellFormat.setRightBorder(1);
            cellFormat.setTopBorderBrush(m_style.border);
            cellFormat.setBottomBorderBrush(m_style.border);
            cellFormat.setLeftBorderBrush(m_style.border);
            cellFormat.setRightBorderBrush(m_style.border);
            cellFormat.setTopBorderStyle(QTextFrameFormat::BorderStyle_Solid);
            cellFormat.setBottomBorderStyle(QTextFrameFormat::BorderStyle_Solid);
            cellFormat.setLeftBorderStyle(QTextFrameFormat::BorderStyle_Solid);
            cellFormat.setRightBorderStyle(QTextFrameFormat::BorderStyle_Solid);
            cell.setFormat(cellFormat);

            QTextCursor cellCursor = cell.firstCursorPosition();
            QTextBlockFormat blockFormat;
            blockFormat.setAlignment(alignments.value(column, Qt::AlignLeft));
            cellCursor.setBlockFormat(blockFormat);

            QTextCharFormat charFormat;
            charFormat.setFontFamilies(proseFamilies());
            charFormat.setFontPointSize(kBodySize);
            charFormat.setForeground(m_style.text);
            if (row == 0) {
                charFormat.setFontWeight(QFont::Bold);
                charFormat.setForeground(m_style.heading);
            }
            cellCursor.insertText(cells.value(column), charFormat);
        }
    }

    m_cursor = QTextCursor(m_document);
    m_cursor.setPosition(table->lastPosition());
}

void Renderer::renderRule()
{
    QTextBlockFormat blockFormat;
    blockFormat.setLineHeight(140, QTextBlockFormat::ProportionalHeight);
    blockFormat.setTopMargin(6);
    blockFormat.setBottomMargin(12);
    beginBlock(blockFormat, bodyFormat(m_style.rule));

    QTextCharFormat format = bodyFormat(m_style.rule);
    format.setFontPointSize(8);
    m_cursor.insertText(QString(60, QChar(0x2500)), format);
}

void Renderer::renderInline(const QString &text, const QTextCharFormat &base)
{
    struct Rule {
        QRegularExpression pattern;
        int kind;
    };

    static const QVector<Rule> rules = {
        {QRegularExpression(QStringLiteral("`([^`\\n]+)`")), 0},
        {QRegularExpression(QStringLiteral("!\\[([^\\]]*)\\]\\(([^)]*)\\)")), 1},
        {QRegularExpression(QStringLiteral("\\[([^\\]]*)\\]\\(([^)]*)\\)")), 2},
        {QRegularExpression(QStringLiteral("\\*\\*\\*([^*]+)\\*\\*\\*")), 3},
        {QRegularExpression(QStringLiteral("\\*\\*([^*]+)\\*\\*")), 4},
        {QRegularExpression(QStringLiteral("__([^_]+)__")), 4},
        {QRegularExpression(QStringLiteral("\\*([^*]+)\\*")), 5},
        {QRegularExpression(QStringLiteral("(?<![_\\w])_([^_]+)_(?![_\\w])")), 5},
        {QRegularExpression(QStringLiteral("~~([^~]+)~~")), 6},
        {QRegularExpression(QStringLiteral("<(https?://[^>]+)>")), 7},
    };

    int position = 0;
    while (position < text.size()) {
        int bestStart = -1;
        int bestKind = -1;
        QRegularExpressionMatch best;
        for (const Rule &rule : rules) {
            const QRegularExpressionMatch match = rule.pattern.match(text, position);
            if (!match.hasMatch())
                continue;
            if (bestStart < 0 || match.capturedStart() < bestStart) {
                bestStart = match.capturedStart();
                bestKind = rule.kind;
                best = match;
            }
        }

        if (bestStart < 0) {
            m_cursor.insertText(text.mid(position), base);
            break;
        }
        if (bestStart > position)
            m_cursor.insertText(text.mid(position, bestStart - position), base);

        const QString content = best.capturedLength(1) > 0 ? best.captured(1) : best.captured(0);
        switch (bestKind) {
        case 0: { // 行内代码
            m_cursor.insertText(content, codeFormat());
            break;
        }
        case 1: { // 图片：显示 alt
            QTextCharFormat format = base;
            format.setForeground(m_style.muted);
            format.setFontItalic(true);
            m_cursor.insertText(content.isEmpty() ? QStringLiteral("[图片]") : content, format);
            break;
        }
        case 2: { // 链接
            QTextCharFormat format = base;
            format.setForeground(m_style.link);
            format.setFontUnderline(true);
            format.setAnchor(true);
            format.setAnchorHref(best.captured(2));
            m_cursor.insertText(best.captured(1), format);
            break;
        }
        case 3: { // 粗斜体
            QTextCharFormat format = base;
            format.setFontWeight(QFont::Bold);
            format.setFontItalic(true);
            m_cursor.insertText(content, format);
            break;
        }
        case 4: { // 粗体
            QTextCharFormat format = base;
            format.setFontWeight(QFont::Bold);
            format.setForeground(m_style.heading);
            m_cursor.insertText(content, format);
            break;
        }
        case 5: { // 斜体
            QTextCharFormat format = base;
            format.setFontItalic(true);
            m_cursor.insertText(content, format);
            break;
        }
        case 6: { // 删除线
            QTextCharFormat format = base;
            format.setFontStrikeOut(true);
            format.setForeground(m_style.muted);
            m_cursor.insertText(content, format);
            break;
        }
        default: { // 裸链接
            QTextCharFormat format = base;
            format.setForeground(m_style.link);
            format.setFontUnderline(true);
            format.setAnchor(true);
            format.setAnchorHref(best.captured(1));
            m_cursor.insertText(best.captured(1), format);
            break;
        }
        }

        position = bestStart + best.capturedLength();
    }
}

void Renderer::run(const QString &markdown)
{
    const QStringList lines = markdown.split(QLatin1Char('\n'));

    static const QRegularExpression headingPattern(QStringLiteral("^(#{1,6})\\s+(.*)$"));
    static const QRegularExpression fencePattern(QStringLiteral("^\\s{0,3}(?:```|~~~)(.*)$"));
    static const QRegularExpression quotePattern(QStringLiteral("^\\s{0,3}(>+)\\s?(.*)$"));
    static const QRegularExpression bulletPattern(
        QStringLiteral("^(\\s*)[-+*]\\s+\\[([ xX])\\]\\s?(.*)$"));
    static const QRegularExpression listPattern(
        QStringLiteral("^(\\s*)([-+*]|\\d{1,9}[.)])\\s+(.*)$"));
    static const QRegularExpression rulePattern(
        QStringLiteral("^\\s{0,3}(?:(?:\\*\\s*){3,}|(?:-\\s*){3,}|(?:_\\s*){3,})$"));

    int index = 0;
    while (index < lines.size()) {
        const QString line = lines.at(index);

        // 表格：当前行有 |，下一行是 |---|
        if (line.contains(QLatin1Char('|')) && index + 1 < lines.size()
            && isTableSeparator(lines.at(index + 1))) {
            QStringList rows{line};
            int next = index + 2;
            while (next < lines.size() && !lines.at(next).trimmed().isEmpty()
                   && lines.at(next).contains(QLatin1Char('|'))) {
                rows << lines.at(next);
                ++next;
            }
            renderTable(rows, lines.at(index + 1));
            index = next;
            continue;
        }

        const QRegularExpressionMatch fence = fencePattern.match(line);
        if (fence.hasMatch()) {
            QStringList code;
            ++index;
            while (index < lines.size() && !fencePattern.match(lines.at(index)).hasMatch())
                code << lines.at(index++);
            if (index < lines.size())
                ++index; // 跳过闭合围栏
            renderCodeBlock(code, fence.captured(1));
            continue;
        }

        if (line.trimmed().isEmpty()) {
            ++index;
            continue;
        }

        const QRegularExpressionMatch heading = headingPattern.match(line);
        if (heading.hasMatch()) {
            renderHeading(heading.capturedLength(1), heading.captured(2).trimmed());
            ++index;
            continue;
        }

        if (rulePattern.match(line).hasMatch()) {
            renderRule();
            ++index;
            continue;
        }

        const QRegularExpressionMatch quote = quotePattern.match(line);
        if (quote.hasMatch()) {
            renderParagraph(quote.captured(2).trimmed(), quote.capturedLength(1));
            ++index;
            continue;
        }

        const QRegularExpressionMatch task = bulletPattern.match(line);
        if (task.hasMatch()) {
            const int indent = task.captured(1).size() / 2;
            const bool done = task.captured(2).compare(QLatin1String("x"), Qt::CaseInsensitive) == 0;
            renderListItem(done ? QStringLiteral("☑") : QStringLiteral("☐"),
                           task.captured(3), indent);
            ++index;
            continue;
        }

        const QRegularExpressionMatch list = listPattern.match(line);
        if (list.hasMatch()) {
            const int indent = list.captured(1).size() / 2;
            QString marker = list.captured(2);
            if (marker.size() == 1 && (marker == QLatin1String("-") || marker == QLatin1String("+")
                                       || marker == QLatin1String("*")))
                marker = QStringLiteral("•");
            renderListItem(marker, list.captured(3), indent);
            ++index;
            continue;
        }

        renderParagraph(line, 0);
        ++index;
    }
}

} // namespace

void MarkdownRenderer::render(const QString &markdown, QTextDocument *document)
{
    if (!document)
        return;

    document->clear();

    QFont body;
    body.setFamilies(proseFamilies());
    body.setPointSizeF(kBodySize);
    document->setDefaultFont(body);
    document->setDocumentMargin(0);

    Renderer renderer(document);
    renderer.run(markdown);
}
