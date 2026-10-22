#pragma once

#include "CodeHighlight.h"

#include <QColor>
#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>

/// Markdown 着色与“就地渲染”。
///
/// 两种形态：
///  - 渲染模式（默认，对应界面的“编辑器模式”）：隐藏 #、**、反引号等标记，
///    标题按级别放大、加粗文本真加粗、链接只留文字；光标所在行会露出原始标记，
///    方便直接修改。
///  - 源码模式：原样显示所有标记，只做着色。
///
/// 注意：QSyntaxHighlighter::setFormat 是“整段替换”而不是合并，所以这里
/// 每一段都先和当前行的基础格式（标题 / 引用 / 代码块）合并后再设置。
class MarkdownHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit MarkdownHighlighter(QTextDocument *document = nullptr);

    void setRendered(bool rendered);
    bool isRendered() const { return m_rendered; }

    /// 光标所在行；渲染模式下这一行不隐藏标记。
    void setActiveBlock(int blockNumber);
    int activeBlock() const { return m_activeBlock; }

    /// 主题或字号变化后重建格式并重新着色。
    void refreshAppearance();

protected:
    void highlightBlock(const QString &text) override;

private:
    void rebuildFormats();
    void apply(int start, int length, const QTextCharFormat &format);
    void applyDelimited(const QRegularExpression &pattern, const QString &text,
                        const QTextCharFormat &contentFormat, int contentGroup,
                        const QColor &hideAgainst);
    QTextCharFormat compose(const QTextCharFormat &specific) const;
    const QTextCharFormat &tokenFormat(CodeHighlight::Token token) const;
    QTextCharFormat hiddenFormat(const QColor &background) const;
    bool isActiveLine() const;

    bool m_rendered = true;
    int m_activeBlock = -1;
    QTextCharFormat m_blockBase;

    QTextCharFormat m_headingFormats[6];
    QTextCharFormat m_headingMarkerFormat;
    QTextCharFormat m_strongFormat;
    QTextCharFormat m_boldItalicFormat;
    QTextCharFormat m_emphasisFormat;
    QTextCharFormat m_codeFormat;
    QTextCharFormat m_fenceFormat;
    QTextCharFormat m_fenceMarkerFormat;
    QTextCharFormat m_codeKeywordFormat;
    QTextCharFormat m_codeStringFormat;
    QTextCharFormat m_codeNumberFormat;
    QTextCharFormat m_codeCommentFormat;
    QTextCharFormat m_codePreprocessorFormat;
    QTextCharFormat m_linkFormat;
    QTextCharFormat m_urlFormat;
    QTextCharFormat m_quoteFormat;
    QTextCharFormat m_quoteMarkerFormat;
    QTextCharFormat m_listMarkerFormat;
    QTextCharFormat m_ruleFormat;
    QTextCharFormat m_taskDoneFormat;
    QTextCharFormat m_strikeFormat;
};
