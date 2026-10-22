#pragma once

#include <QString>
#include <QVector>

/// 围栏代码块里的轻量语法高亮。
///
/// 不依赖任何第三方库，按语言给出一组关键字 + 注释 / 字符串规则，
/// 逐行扫描出 token，由 MarkdownHighlighter 上色。
namespace CodeHighlight {

enum class Language {
    None = 0,
    C,
    Cpp,
    Python,
    JavaScript,
    TypeScript,
    Java,
    Rust,
    Go,
    Json,
    Bash,
    Sql,
    Yaml,
    Count
};

enum class Token { Keyword, Type, String, Number, Comment, Preprocessor };

struct Span {
    int start = 0;
    int length = 0;
    Token token = Token::Keyword;
};

struct LanguageEntry {
    Language language;
    QString info;  // 写进围栏里的标记，例如 cpp
    QString name;  // 显示名，例如 C++
};

/// 供语言下拉菜单使用（顺序即菜单顺序）。
const QVector<LanguageEntry> &languageEntries();

QString displayName(Language language);

/// 从 ```cpp 这类 info string 里识别语言（大小写不敏感）。
Language fromInfoString(const QString &info);

/// 扫描一行，返回需要着色的区间；inBlockComment 传入 / 传出跨行注释状态。
QVector<Span> tokenize(const QString &text, Language language, bool inBlockComment,
                       bool *newBlockComment);

} // namespace CodeHighlight
