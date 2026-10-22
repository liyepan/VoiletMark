#include "MarkdownHighlighter.h"

#include "ThemeManager.h"

#include <QTextDocument>

namespace {
constexpr int StateNormal = 0;
constexpr int StateInFence = 1;
} // namespace

MarkdownHighlighter::MarkdownHighlighter(QTextDocument *document)
    : QSyntaxHighlighter(document)
{
    rebuildFormats();
    connect(ThemeManager::instance(), &ThemeManager::schemeChanged, this, [this] {
        rebuildFormats();
        rehighlight();
    });
}

void MarkdownHighlighter::rebuildFormats()
{
    const ThemeColors &c = ThemeManager::instance()->colors();

    m_headingFormat = QTextCharFormat();
    m_headingFormat.setForeground(c.heading);
    m_headingFormat.setFontWeight(QFont::DemiBold);

    m_headingMarkerFormat = QTextCharFormat();
    m_headingMarkerFormat.setForeground(c.headingMarker);

    m_strongFormat = QTextCharFormat();
    m_strongFormat.setForeground(c.strong);
    m_strongFormat.setFontWeight(QFont::Bold);

    m_emphasisFormat = QTextCharFormat();
    m_emphasisFormat.setForeground(c.emphasis);
    m_emphasisFormat.setFontItalic(true);

    m_codeFormat = QTextCharFormat();
    m_codeFormat.setForeground(c.code);
    m_codeFormat.setBackground(c.codeBackground);
    m_codeFormat.setFontWeight(QFont::Normal);
    m_codeFormat.setFontItalic(false);

    m_linkFormat = QTextCharFormat();
    m_linkFormat.setForeground(c.link);

    m_urlFormat = QTextCharFormat();
    m_urlFormat.setForeground(c.linkUrl);

    m_quoteFormat = QTextCharFormat();
    m_quoteFormat.setForeground(c.quote);
    m_quoteFormat.setFontItalic(true);

    m_quoteMarkerFormat = QTextCharFormat();
    m_quoteMarkerFormat.setForeground(c.quoteMarker);
    m_quoteMarkerFormat.setFontItalic(false);

    m_listMarkerFormat = QTextCharFormat();
    m_listMarkerFormat.setForeground(c.listMarker);
    m_listMarkerFormat.setFontWeight(QFont::DemiBold);

    m_ruleFormat = QTextCharFormat();
    m_ruleFormat.setForeground(c.rule);

    m_taskDoneFormat = QTextCharFormat();
    m_taskDoneFormat.setForeground(c.taskDone);
    m_taskDoneFormat.setFontWeight(QFont::DemiBold);

    m_strikeFormat = QTextCharFormat();
    m_strikeFormat.setFontStrikeOut(true);
    m_strikeFormat.setForeground(c.quote);
}

void MarkdownHighlighter::apply(int start, int length, const QTextCharFormat &format)
{
    if (length > 0)
        setFormat(start, length, format);
}

void MarkdownHighlighter::applyPattern(const QRegularExpression &pattern, const QString &text,
                                       const QTextCharFormat &format)
{
    auto it = pattern.globalMatch(text);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        apply(match.capturedStart(), match.capturedLength(), format);
    }
}

void MarkdownHighlighter::highlightBlock(const QString &text)
{
    setCurrentBlockState(StateNormal);
    if (text.isEmpty())
        return;

    static const QRegularExpression fencePattern(QStringLiteral("^\\s{0,3}(?:```|~~~)"));

    // ---- 围栏代码块：跨行状态机 ----------------------------------------
    if (previousBlockState() == StateInFence) {
        apply(0, text.length(), m_codeFormat);
        if (!fencePattern.match(text).hasMatch())
            setCurrentBlockState(StateInFence);
        return;
    }
    if (fencePattern.match(text).hasMatch()) {
        apply(0, text.length(), m_codeFormat);
        setCurrentBlockState(StateInFence);
        return;
    }

    // ---- 块级语法 ------------------------------------------------------
    static const QRegularExpression headingPattern(
        QStringLiteral("^\\s{0,3}(#{1,6})(?:\\s+|$)"));
    const QRegularExpressionMatch heading = headingPattern.match(text);
    if (heading.hasMatch()) {
        apply(0, text.length(), m_headingFormat);
        apply(heading.capturedStart(1), heading.capturedLength(1), m_headingMarkerFormat);
    }

    static const QRegularExpression quotePattern(QStringLiteral("^\\s{0,3}(>+)"));
    const QRegularExpressionMatch quote = quotePattern.match(text);
    if (quote.hasMatch()) {
        apply(0, text.length(), m_quoteFormat);
        apply(quote.capturedStart(1), quote.capturedLength(1), m_quoteMarkerFormat);
    }

    static const QRegularExpression rulePattern(
        QStringLiteral("^\\s{0,3}(?:(?:\\*\\s*){3,}|(?:-\\s*){3,}|(?:_\\s*){3,})$"));
    const QRegularExpressionMatch rule = rulePattern.match(text);
    if (rule.hasMatch()) {
        apply(0, text.length(), m_ruleFormat);
    } else {
        static const QRegularExpression taskPattern(
            QStringLiteral("^\\s*[-+*]\\s+\\[([ xX])\\]"));
        const QRegularExpressionMatch task = taskPattern.match(text);
        if (task.hasMatch()) {
            apply(task.capturedStart(), task.capturedLength(), m_listMarkerFormat);
            if (task.capturedLength(1) > 0) {
                const bool done = task.captured(1).compare(QLatin1String("x"), Qt::CaseInsensitive) == 0;
                apply(task.capturedStart(1), task.capturedLength(1),
                      done ? m_taskDoneFormat : m_listMarkerFormat);
            }
        } else {
            static const QRegularExpression listPattern(
                QStringLiteral("^(\\s*)([-+*]|\\d{1,9}[.)])(\\s+)"));
            const QRegularExpressionMatch list = listPattern.match(text);
            if (list.hasMatch())
                apply(list.capturedStart(2), list.capturedLength(2), m_listMarkerFormat);
        }
    }

    // ---- 行内语法 ------------------------------------------------------
    static const QRegularExpression linkPattern(
        QStringLiteral("!?\\[([^\\]]*)\\]\\(([^)\\s]*)(?:\\s+\"([^\"]*)\")?\\)"));
    auto links = linkPattern.globalMatch(text);
    while (links.hasNext()) {
        const QRegularExpressionMatch match = links.next();
        apply(match.capturedStart(1), match.capturedLength(1), m_linkFormat);
        apply(match.capturedStart(2), match.capturedLength(2), m_urlFormat);
    }

    static const QRegularExpression autoLinkPattern(QStringLiteral("<(https?://[^>]+)>"));
    auto autolinks = autoLinkPattern.globalMatch(text);
    while (autolinks.hasNext()) {
        const QRegularExpressionMatch match = autolinks.next();
        apply(match.capturedStart(1), match.capturedLength(1), m_urlFormat);
    }

    static const QRegularExpression boldItalicPattern(
        QStringLiteral("(\\*{3}|_{3})(?=\\S)(.+?)(?<=\\S)\\1"));
    applyPattern(boldItalicPattern, text, m_strongFormat);
    applyPattern(boldItalicPattern, text, m_emphasisFormat);

    static const QRegularExpression boldPattern(QStringLiteral("(\\*\\*|__)(?=\\S)(.+?)(?<=\\S)\\1"));
    applyPattern(boldPattern, text, m_strongFormat);

    static const QRegularExpression italicPattern(
        QStringLiteral("(?<![*_\\w])([*_])(?=\\S)(.+?)(?<=\\S)\\1(?![*_\\w])"));
    applyPattern(italicPattern, text, m_emphasisFormat);

    static const QRegularExpression strikePattern(QStringLiteral("~~(?=\\S)(.+?)(?<=\\S)~~"));
    applyPattern(strikePattern, text, m_strikeFormat);

    // 行内代码最后着色，压住内部的加粗 / 斜体。
    static const QRegularExpression inlineCodePattern(QStringLiteral("`[^`\\n]+`"));
    applyPattern(inlineCodePattern, text, m_codeFormat);
}
