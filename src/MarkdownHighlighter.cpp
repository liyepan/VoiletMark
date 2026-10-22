#include "MarkdownHighlighter.h"

#include "ThemeManager.h"

#include <QTextBlock>
#include <QTextDocument>
#include <QTextLayout>

namespace {
constexpr int StateNormal = 0;
constexpr int StateInFence = 1;

QStringList monospaceFamilies()
{
    return {QStringLiteral("JetBrains Mono"), QStringLiteral("Cascadia Code"),
            QStringLiteral("Source Han Mono SC"), QStringLiteral("Noto Sans Mono CJK SC"),
            QStringLiteral("Sarasa Mono SC"), QStringLiteral("Consolas"),
            QStringLiteral("DejaVu Sans Mono")};
}
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

void MarkdownHighlighter::setRendered(bool rendered)
{
    if (m_rendered == rendered)
        return;
    m_rendered = rendered;
    // 源代码模式不放大标题，格式需要按模式重建。
    rebuildFormats();
    rehighlight();
}

void MarkdownHighlighter::setActiveBlock(int blockNumber)
{
    m_activeBlock = blockNumber;
}

void MarkdownHighlighter::refreshAppearance()
{
    rebuildFormats();
    rehighlight();
}

bool MarkdownHighlighter::isActiveLine() const
{
    return m_rendered && currentBlock().blockNumber() == m_activeBlock;
}

void MarkdownHighlighter::rebuildFormats()
{
    const ThemeColors &c = ThemeManager::instance()->colors();

    qreal base = 12.0;
    if (document() && document()->defaultFont().pointSizeF() > 0.5)
        base = document()->defaultFont().pointSizeF();

    static const qreal scales[6] = {1.75, 1.50, 1.30, 1.14, 1.04, 1.0};
    for (int level = 0; level < 6; ++level) {
        m_headingFormats[level] = QTextCharFormat();
        m_headingFormats[level].setForeground(c.heading);
        m_headingFormats[level].setFontWeight(QFont::Bold);
        // 编辑模式按级别放大；源代码模式保持逐行等高，只保留颜色和粗细。
        if (m_rendered)
            m_headingFormats[level].setFontPointSize(base * scales[level]);
    }

    m_headingMarkerFormat = QTextCharFormat();
    m_headingMarkerFormat.setForeground(c.headingMarker);

    m_strongFormat = QTextCharFormat();
    m_strongFormat.setForeground(c.strong);
    m_strongFormat.setFontWeight(QFont::Bold);

    m_boldItalicFormat = QTextCharFormat();
    m_boldItalicFormat.setForeground(c.strong);
    m_boldItalicFormat.setFontWeight(QFont::Bold);
    m_boldItalicFormat.setFontItalic(true);

    m_emphasisFormat = QTextCharFormat();
    m_emphasisFormat.setForeground(c.emphasis);
    m_emphasisFormat.setFontItalic(true);

    m_codeFormat = QTextCharFormat();
    m_codeFormat.setForeground(c.code);
    m_codeFormat.setBackground(c.codeBackground);
    m_codeFormat.setFontWeight(QFont::Normal);
    m_codeFormat.setFontItalic(false);
    m_codeFormat.setFontFamilies(monospaceFamilies());
    m_codeFormat.setFontFixedPitch(true);

    // 围栏代码块：等宽、普通文字色，不带底色——外面由编辑器画框。
    m_fenceFormat = QTextCharFormat();
    m_fenceFormat.setFontFamilies(monospaceFamilies());
    m_fenceFormat.setFontFixedPitch(true);
    m_fenceFormat.setForeground(c.editorText);

    m_linkFormat = QTextCharFormat();
    m_linkFormat.setForeground(c.link);
    m_linkFormat.setFontUnderline(true);

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

QTextCharFormat MarkdownHighlighter::compose(const QTextCharFormat &specific) const
{
    QTextCharFormat result = m_blockBase;
    result.merge(specific);
    return result;
}

QTextCharFormat MarkdownHighlighter::hiddenFormat(const QColor &background) const
{
    // 用“和背景同色 + 极小字号”把标记藏起来：文字仍然留在文档里，
    // 光标移回来时重新显示，所以不影响编辑和撤销。
    QTextCharFormat format;
    format.setForeground(background);
    format.setFontPointSize(0.5);
    return format;
}

void MarkdownHighlighter::apply(int start, int length, const QTextCharFormat &format)
{
    if (length > 0)
        setFormat(start, length, format);
}

void MarkdownHighlighter::applyDelimited(const QRegularExpression &pattern, const QString &text,
                                         const QTextCharFormat &contentFormat, int contentGroup,
                                         const QColor &hideAgainst)
{
    auto iterator = pattern.globalMatch(text);
    while (iterator.hasNext()) {
        const QRegularExpressionMatch match = iterator.next();
        const int start = match.capturedStart(contentGroup);
        const int length = match.capturedLength(contentGroup);
        if (length <= 0)
            continue;

        if (m_rendered && !isActiveLine()) {
            apply(start, length, compose(contentFormat));
            const int matchStart = match.capturedStart();
            const int matchEnd = matchStart + match.capturedLength();
            if (start > matchStart)
                apply(matchStart, start - matchStart, compose(hiddenFormat(hideAgainst)));
            if (matchEnd > start + length)
                apply(start + length, matchEnd - start - length,
                      compose(hiddenFormat(hideAgainst)));
        } else {
            // 源码模式，或光标正停在这一行：整段按内容样式显示，标记保留。
            apply(match.capturedStart(), match.capturedLength(), compose(contentFormat));
        }
    }
}

void MarkdownHighlighter::highlightBlock(const QString &text)
{
    setCurrentBlockState(StateNormal);
    m_blockBase = QTextCharFormat();
    if (text.isEmpty())
        return;

    const ThemeColors &c = ThemeManager::instance()->colors();
    const bool rendered = m_rendered;
    const bool active = isActiveLine();

    static const QRegularExpression fencePattern(QStringLiteral("^\\s{0,3}(?:```|~~~)"));
    const bool inFence = previousBlockState() == StateInFence;
    const bool fenceLine = fencePattern.match(text).hasMatch();

    // ---- 围栏代码块 -----------------------------------------------------
    if (inFence) {
        if (!fenceLine)
            setCurrentBlockState(StateInFence);
        if (rendered) {
            if (fenceLine && !active) {
                apply(0, text.length(), hiddenFormat(c.editorBase));
            } else {
                m_blockBase = m_fenceFormat;
                apply(0, text.length(), m_fenceFormat);
            }
        } else {
            m_blockBase = m_codeFormat;
            apply(0, text.length(), m_codeFormat);
        }
        return;
    }
    if (fenceLine) {
        setCurrentBlockState(StateInFence);
        if (rendered) {
            if (!active)
                apply(0, text.length(), hiddenFormat(c.editorBase));
            else
                apply(0, text.length(), m_fenceFormat);
        } else {
            apply(0, text.length(), m_codeFormat);
        }
        return;
    }

    // ---- 标题 -----------------------------------------------------------
    static const QRegularExpression headingPattern(QStringLiteral("^\\s{0,3}(#{1,6})(\\s+)"));
    const QRegularExpressionMatch heading = headingPattern.match(text);
    if (heading.hasMatch()) {
        const int level = qBound(1, heading.capturedLength(1), 6);
        m_blockBase.merge(m_headingFormats[level - 1]);
        apply(0, text.length(), m_blockBase);

        const int markerStart = heading.capturedStart(1);
        const int markerLength = heading.capturedLength(1) + heading.capturedLength(2);
        if (rendered && !active)
            apply(markerStart, markerLength, compose(hiddenFormat(c.editorBase)));
        else
            apply(markerStart, markerLength, compose(m_headingMarkerFormat));
    }

    // ---- 引用 -----------------------------------------------------------
    static const QRegularExpression quotePattern(QStringLiteral("^\\s{0,3}(>+)"));
    const QRegularExpressionMatch quote = quotePattern.match(text);
    if (quote.hasMatch()) {
        m_blockBase.merge(m_quoteFormat);
        apply(0, text.length(), m_blockBase);
        if (rendered && !active)
            apply(quote.capturedStart(1), quote.capturedLength(1),
                  compose(hiddenFormat(c.editorBase)));
        else
            apply(quote.capturedStart(1), quote.capturedLength(1), compose(m_quoteMarkerFormat));
    }

    // ---- 分隔线 / 列表 ---------------------------------------------------
    static const QRegularExpression rulePattern(
        QStringLiteral("^\\s{0,3}(?:(?:\\*\\s*){3,}|(?:-\\s*){3,}|(?:_\\s*){3,})$"));
    if (rulePattern.match(text).hasMatch()) {
        apply(0, text.length(), compose(m_ruleFormat));
    } else {
        static const QRegularExpression taskPattern(
            QStringLiteral("^\\s*[-+*]\\s+\\[([ xX])\\]"));
        const QRegularExpressionMatch task = taskPattern.match(text);
        if (task.hasMatch()) {
            apply(task.capturedStart(), task.capturedLength(), compose(m_listMarkerFormat));
            if (task.capturedLength(1) > 0) {
                const bool done = task.captured(1).compare(QLatin1String("x"),
                                                           Qt::CaseInsensitive) == 0;
                apply(task.capturedStart(1), task.capturedLength(1),
                      compose(done ? m_taskDoneFormat : m_listMarkerFormat));
            }
        } else {
            static const QRegularExpression listPattern(
                QStringLiteral("^(\\s*)([-+*]|\\d{1,9}[.)])(\\s+)"));
            const QRegularExpressionMatch list = listPattern.match(text);
            if (list.hasMatch())
                apply(list.capturedStart(2), list.capturedLength(2), compose(m_listMarkerFormat));
        }
    }

    // ---- 链接 / 图片 -----------------------------------------------------
    static const QRegularExpression linkPattern(
        QStringLiteral("!?\\[([^\\]]*)\\]\\(([^)\\s]*)(?:\\s+\"([^\"]*)\")?\\)"));
    auto links = linkPattern.globalMatch(text);
    while (links.hasNext()) {
        const QRegularExpressionMatch match = links.next();
        const int labelStart = match.capturedStart(1);
        const int labelLength = match.capturedLength(1);
        const int matchEnd = match.capturedStart() + match.capturedLength();

        apply(labelStart, labelLength, compose(m_linkFormat));

        if (rendered && !active) {
            if (labelStart > match.capturedStart())
                apply(match.capturedStart(), labelStart - match.capturedStart(),
                      compose(hiddenFormat(c.editorBase)));
            const int suffixStart = labelStart + labelLength;
            if (matchEnd > suffixStart)
                apply(suffixStart, matchEnd - suffixStart,
                      compose(hiddenFormat(c.editorBase)));
        } else if (match.capturedLength(2) > 0) {
            apply(match.capturedStart(2), match.capturedLength(2), compose(m_urlFormat));
        }
    }

    static const QRegularExpression autoLinkPattern(QStringLiteral("<(https?://[^>]+)>"));
    auto autoLinks = autoLinkPattern.globalMatch(text);
    while (autoLinks.hasNext()) {
        const QRegularExpressionMatch match = autoLinks.next();
        apply(match.capturedStart(1), match.capturedLength(1), compose(m_urlFormat));
        if (rendered && !active) {
            apply(match.capturedStart(), 1, compose(hiddenFormat(c.editorBase)));
            apply(match.capturedStart() + match.capturedLength() - 1, 1,
                  compose(hiddenFormat(c.editorBase)));
        }
    }

    // ---- 行内强调 --------------------------------------------------------
    // 内容里禁止出现定界符，否则 **粗体** 会被更宽泛的 *斜体* 规则先吃掉。
    static const QRegularExpression boldAsterisk(
        QStringLiteral("\\*\\*(?=\\S)([^*\\n]+?)(?<=\\S)\\*\\*"));
    applyDelimited(boldAsterisk, text, m_strongFormat, 1, c.editorBase);

    static const QRegularExpression boldUnderscore(
        QStringLiteral("__(?=\\S)([^_\\n]+?)(?<=\\S)__"));
    applyDelimited(boldUnderscore, text, m_strongFormat, 1, c.editorBase);

    static const QRegularExpression italicAsterisk(
        QStringLiteral("(?<![*\\w])\\*(?!\\*)(?=\\S)([^*\\n]+?)(?<=\\S)\\*(?!\\*)"));
    applyDelimited(italicAsterisk, text, m_emphasisFormat, 1, c.editorBase);

    static const QRegularExpression italicUnderscore(
        QStringLiteral("(?<![_\\w])_(?!_)(?=\\S)([^_\\n]+?)(?<=\\S)_(?![\\w_])"));
    applyDelimited(italicUnderscore, text, m_emphasisFormat, 1, c.editorBase);

    static const QRegularExpression strikePattern(
        QStringLiteral("~~(?=\\S)([^~\\n]+?)(?<=\\S)~~"));
    applyDelimited(strikePattern, text, m_strikeFormat, 1, c.editorBase);

    // 三重强调最后应用，压住上面的加粗 / 斜体。
    static const QRegularExpression boldItalicPattern(
        QStringLiteral("\\*\\*\\*(?=\\S)([^*\\n]+?)(?<=\\S)\\*\\*\\*"));
    applyDelimited(boldItalicPattern, text, m_boldItalicFormat, 1, c.editorBase);

    // ---- 行内代码（最后，压住内部的强调）---------------------------------
    static const QRegularExpression codePattern(QStringLiteral("(`+)([^`\\n]+)\\1"));
    auto codes = codePattern.globalMatch(text);
    while (codes.hasNext()) {
        const QRegularExpressionMatch match = codes.next();
        const int matchStart = match.capturedStart();
        const int matchEnd = matchStart + match.capturedLength();
        const QTextCharFormat span = compose(m_codeFormat);
        apply(matchStart, match.capturedLength(), span);

        if (rendered && !active) {
            const int contentStart = match.capturedStart(2);
            const int contentLength = match.capturedLength(2);
            if (contentLength <= 0)
                continue;
            QTextCharFormat hidden = span;
            hidden.merge(hiddenFormat(c.codeBackground));
            if (contentStart > matchStart)
                apply(matchStart, contentStart - matchStart, hidden);
            const int suffixStart = contentStart + contentLength;
            if (matchEnd > suffixStart)
                apply(suffixStart, matchEnd - suffixStart, hidden);
        }
    }
}
