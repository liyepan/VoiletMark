#pragma once

#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>

/// Markdown 语法着色。
///
/// 只做“逐行着色”，不改动文档内容：标题、引用、列表、行内样式、
/// 链接以及跨行的围栏代码块。颜色全部取自 ThemeManager，
/// 主题切换后自动重建格式并重新着色。
class MarkdownHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit MarkdownHighlighter(QTextDocument *document = nullptr);

    /// 源代码模式会关掉着色，但不改文档内容（rehighlight 会清掉旧格式）。
    void setHighlightingEnabled(bool enabled);

protected:
    void highlightBlock(const QString &text) override;

private:
    void rebuildFormats();
    void apply(int start, int length, const QTextCharFormat &format);
    void applyPattern(const QRegularExpression &pattern, const QString &text,
                      const QTextCharFormat &format);

    QTextCharFormat m_headingFormat;
    QTextCharFormat m_headingMarkerFormat;
    QTextCharFormat m_strongFormat;
    QTextCharFormat m_emphasisFormat;
    QTextCharFormat m_codeFormat;
    QTextCharFormat m_linkFormat;
    QTextCharFormat m_urlFormat;
    QTextCharFormat m_quoteFormat;
    QTextCharFormat m_quoteMarkerFormat;
    QTextCharFormat m_listMarkerFormat;
    QTextCharFormat m_ruleFormat;
    QTextCharFormat m_taskDoneFormat;
    QTextCharFormat m_strikeFormat;
    bool m_enabled = true;
};
