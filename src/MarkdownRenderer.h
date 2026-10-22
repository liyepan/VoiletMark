#pragma once

#include <QString>

class QTextDocument;

/// 把 Markdown 渲染成排版好的 QTextDocument，供导出预览和打印 PDF 共用。
/// 不依赖任何外部库，只是在这里做一遍块级 / 行内解析。
namespace MarkdownRenderer {

void render(const QString &markdown, QTextDocument *document);

} // namespace MarkdownRenderer
