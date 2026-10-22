#include "MarkdownRenderer.h"

#include <QColor>
#include <QFont>
#include <QRegularExpression>
#include <QTextBlockFormat>
#include <QTextCharFormat>
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
    QColor codeText = hex("#b4295a");
    QColor codeBg = hex("#f3f4f6");
    QColor link = hex("#0969da");
    QColor marker = hex("#0969da");
};

constexpr qreal kBodySize = 11.5;
constexpr qreal kCodeSize = 10.5;

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
    void renderCodeBlock(const QStringList &lines);
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

void Renderer::renderCodeBlock(const QStringList &lines)
{
    QTextBlockFormat blockFormat;
    blockFormat.setLineHeight(135, QTextBlockFormat::ProportionalHeight);
    blockFormat.setTopMargin(6);
    blockFormat.setBottomMargin(10);
    blockFormat.setLeftMargin(14);
    blockFormat.setRightMargin(14);
    blockFormat.setBackground(m_style.codeBg);

    const QTextCharFormat format = codeFormat();
    QStringList content = lines;
    if (content.isEmpty())
        content << QString();

    for (int index = 0; index < content.size(); ++index) {
        QTextBlockFormat lineFormat = blockFormat;
        lineFormat.setTopMargin(index == 0 ? 6 : 0);
        lineFormat.setBottomMargin(index == content.size() - 1 ? 10 : 0);
        beginBlock(lineFormat, format);
        const QString line = content.at(index);
        m_cursor.insertText(line.isEmpty() ? QStringLiteral(" ") : line, format);
    }
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

        const QRegularExpressionMatch fence = fencePattern.match(line);
        if (fence.hasMatch()) {
            QStringList code;
            ++index;
            while (index < lines.size() && !fencePattern.match(lines.at(index)).hasMatch())
                code << lines.at(index++);
            if (index < lines.size())
                ++index; // 跳过闭合围栏
            renderCodeBlock(code);
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
