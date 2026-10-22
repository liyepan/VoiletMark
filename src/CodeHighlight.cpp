#include "CodeHighlight.h"

#include <QHash>
#include <QSet>

namespace CodeHighlight {
namespace {

struct LanguageDef
{
    QSet<QString> keywords;
    QSet<QString> types;
    QStringList lineComments;
    QString blockStart;
    QString blockEnd;
    QString quotes = QStringLiteral("\"'");
    bool preprocessor = false;
    bool caseInsensitive = false;
};

QSet<QString> words(const char *list)
{
    QSet<QString> set;
    const QString text = QString::fromLatin1(list);
    for (const QString &word : text.split(QLatin1Char(' '), Qt::SkipEmptyParts))
        set.insert(word);
    return set;
}

const QHash<int, LanguageDef> &definitions()
{
    static const QHash<int, LanguageDef> table = [] {
        QHash<int, LanguageDef> defs;

        LanguageDef c;
        c.keywords = words("auto break case const continue default do else enum extern for goto if "
                           "inline long register restrict return short signed sizeof static struct "
                           "switch typedef union unsigned void volatile while _Bool _Static_assert");
        c.types = words("char double float int size_t ssize_t ptrdiff_t FILE int8_t int16_t int32_t "
                        "int64_t uint8_t uint16_t uint32_t uint64_t bool");
        c.lineComments = QStringList{QStringLiteral("//")};
        c.blockStart = QStringLiteral("/*");
        c.blockEnd = QStringLiteral("*/");
        c.preprocessor = true;
        defs.insert(int(Language::C), c);

        LanguageDef cpp = c;
        cpp.keywords.unite(words("alignas alignof and asm bool catch class concept constexpr const_cast "
                                 "co_await co_return co_yield decltype delete dynamic_cast explicit export "
                                 "false friend mutable namespace new noexcept not nullptr operator or "
                                 "override private protected public reinterpret_cast static_cast template "
                                 "this throw true try typeid typename using virtual xor final requires"));
        cpp.types.unite(words("std string vector map set unique_ptr shared_ptr size_t"));
        defs.insert(int(Language::Cpp), cpp);

        LanguageDef py;
        py.keywords = words("and as assert async await break class continue def del elif else except False "
                            "finally for from global if import in is lambda None nonlocal not or pass raise "
                            "return True try while with yield match case");
        py.types = words("int float str bytes list dict set tuple bool object");
        py.lineComments = QStringList{QStringLiteral("#")};
        py.quotes = QStringLiteral("\"'");
        defs.insert(int(Language::Python), py);

        LanguageDef js;
        js.keywords = words("async await break case catch class const continue debugger default delete do "
                            "else export extends finally for function if import in instanceof let new of "
                            "return static super switch this throw try typeof var void while with yield "
                            "null true false undefined");
        js.types = words("String Number Boolean Object Array Promise Map Set JSON Math Date RegExp");
        js.lineComments = QStringList{QStringLiteral("//")};
        js.blockStart = QStringLiteral("/*");
        js.blockEnd = QStringLiteral("*/");
        js.quotes = QStringLiteral("\"'\x60");
        defs.insert(int(Language::JavaScript), js);

        LanguageDef ts = js;
        ts.keywords.unite(words("abstract declare enum implements interface keyof namespace never readonly "
                                "satisfies type unknown infer is asserts override private protected public "
                                "any symbol bigint"));
        ts.types.unite(words("string number boolean any unknown never void"));
        defs.insert(int(Language::TypeScript), ts);

        LanguageDef java;
        java.keywords = words("abstract assert break case catch class const continue default do else enum "
                              "extends final finally for goto if implements import instanceof interface "
                              "native new package private protected public return static strictfp super "
                              "switch synchronized this throw throws transient try volatile while var "
                              "true false null");
        java.types = words("byte short int long float double char boolean void String Object List Map Set");
        java.lineComments = QStringList{QStringLiteral("//")};
        java.blockStart = QStringLiteral("/*");
        java.blockEnd = QStringLiteral("*/");
        defs.insert(int(Language::Java), java);

        LanguageDef rust;
        rust.keywords = words("as async await break const continue crate dyn else enum extern false fn for "
                              "if impl in let loop match mod move mut pub ref return self Self static "
                              "struct super trait true type unsafe use where while yield");
        rust.types = words("i8 i16 i32 i64 i128 u8 u16 u32 u64 u128 f32 f64 bool char str String Vec Option "
                           "Result Box");
        rust.lineComments = QStringList{QStringLiteral("//")};
        rust.blockStart = QStringLiteral("/*");
        rust.blockEnd = QStringLiteral("*/");
        defs.insert(int(Language::Rust), rust);

        LanguageDef go;
        go.keywords = words("break case chan const continue default defer else fallthrough for func go goto "
                            "if import interface map package range return select struct switch type var "
                            "true false nil iota");
        go.types = words("bool byte complex64 complex128 error float32 float64 int int8 int16 int32 int64 "
                         "rune string uint uint8 uint16 uint32 uint64 uintptr any");
        go.lineComments = QStringList{QStringLiteral("//")};
        go.blockStart = QStringLiteral("/*");
        go.blockEnd = QStringLiteral("*/");
        go.quotes = QStringLiteral("\"'\x60");
        defs.insert(int(Language::Go), go);

        LanguageDef json;
        json.keywords = words("true false null");
        json.lineComments = QStringList();
        json.quotes = QStringLiteral("\"");
        defs.insert(int(Language::Json), json);

        LanguageDef bash;
        bash.keywords = words("if then else elif fi for while until do done case esac function in select "
                              "time return exit local export readonly declare unset echo printf cd source "
                              "alias set shift trap true false");
        bash.lineComments = QStringList{QStringLiteral("#")};
        bash.quotes = QStringLiteral("\"'\x60");
        defs.insert(int(Language::Bash), bash);

        LanguageDef sql;
        sql.keywords = words("select from where insert into values update set delete create table drop alter "
                             "index view join left right inner outer full on group by order having limit "
                             "offset union all as and or not null is in like between distinct case when "
                             "then else end exists primary key foreign references default unique check");
        sql.types = words("int integer bigint smallint varchar char text date time timestamp boolean float "
                          "double decimal numeric");
        sql.lineComments = QStringList{QStringLiteral("--")};
        sql.blockStart = QStringLiteral("/*");
        sql.blockEnd = QStringLiteral("*/");
        sql.caseInsensitive = true;
        defs.insert(int(Language::Sql), sql);

        LanguageDef yaml;
        yaml.keywords = words("true false null yes no on off");
        yaml.lineComments = QStringList{QStringLiteral("#")};
        defs.insert(int(Language::Yaml), yaml);

        return defs;
    }();

    return table;
}

const LanguageDef *definition(Language language)
{
    const auto &defs = definitions();
    const auto it = defs.constFind(int(language));
    return it == defs.constEnd() ? nullptr : &it.value();
}

} // namespace

Language fromInfoString(const QString &info)
{
    QString name = info.trimmed().toLower();
    if (name.isEmpty())
        return Language::None;
    name = name.section(QLatin1Char(' '), 0, 0);

    static const QHash<QString, Language> aliases = {
        {QStringLiteral("c"), Language::C},
        {QStringLiteral("h"), Language::C},
        {QStringLiteral("cpp"), Language::Cpp},
        {QStringLiteral("c++"), Language::Cpp},
        {QStringLiteral("cc"), Language::Cpp},
        {QStringLiteral("cxx"), Language::Cpp},
        {QStringLiteral("hpp"), Language::Cpp},
        {QStringLiteral("python"), Language::Python},
        {QStringLiteral("py"), Language::Python},
        {QStringLiteral("javascript"), Language::JavaScript},
        {QStringLiteral("js"), Language::JavaScript},
        {QStringLiteral("jsx"), Language::JavaScript},
        {QStringLiteral("mjs"), Language::JavaScript},
        {QStringLiteral("typescript"), Language::TypeScript},
        {QStringLiteral("ts"), Language::TypeScript},
        {QStringLiteral("tsx"), Language::TypeScript},
        {QStringLiteral("java"), Language::Java},
        {QStringLiteral("rust"), Language::Rust},
        {QStringLiteral("rs"), Language::Rust},
        {QStringLiteral("go"), Language::Go},
        {QStringLiteral("golang"), Language::Go},
        {QStringLiteral("json"), Language::Json},
        {QStringLiteral("bash"), Language::Bash},
        {QStringLiteral("sh"), Language::Bash},
        {QStringLiteral("shell"), Language::Bash},
        {QStringLiteral("zsh"), Language::Bash},
        {QStringLiteral("sql"), Language::Sql},
        {QStringLiteral("yaml"), Language::Yaml},
        {QStringLiteral("yml"), Language::Yaml},
    };

    return aliases.value(name, Language::None);
}

QVector<Span> tokenize(const QString &text, Language language, bool inBlockComment,
                       bool *newBlockComment)
{
    QVector<Span> spans;
    if (newBlockComment)
        *newBlockComment = inBlockComment;

    const LanguageDef *def = definition(language);
    if (!def)
        return spans;

    const int length = text.size();
    bool comment = inBlockComment;
    int index = 0;

    if (def->preprocessor) {
        int start = 0;
        while (start < length && text.at(start).isSpace())
            ++start;
        if (start < length && text.at(start) == QLatin1Char('#')) {
            int end = start + 1;
            while (end < length
                   && (text.at(end).isLetterOrNumber() || text.at(end) == QLatin1Char('_')))
                ++end;
            spans.append({start, end - start, Token::Preprocessor});
            index = end;
        }
    }

    while (index < length) {
        if (comment) {
            const int end = text.indexOf(def->blockEnd, index);
            if (end < 0) {
                spans.append({index, length - index, Token::Comment});
                index = length;
                break;
            }
            spans.append({index, end + def->blockEnd.size() - index, Token::Comment});
            index = end + def->blockEnd.size();
            comment = false;
            continue;
        }

        bool lineComment = false;
        for (const QString &prefix : def->lineComments) {
            if (text.mid(index, prefix.size()) == prefix) {
                spans.append({index, length - index, Token::Comment});
                index = length;
                lineComment = true;
                break;
            }
        }
        if (lineComment)
            break;

        if (!def->blockStart.isEmpty()
            && text.mid(index, def->blockStart.size()) == def->blockStart) {
            const int end = text.indexOf(def->blockEnd, index + def->blockStart.size());
            if (end < 0) {
                spans.append({index, length - index, Token::Comment});
                comment = true;
                index = length;
            } else {
                spans.append({index, end + def->blockEnd.size() - index, Token::Comment});
                index = end + def->blockEnd.size();
            }
            continue;
        }

        const QChar current = text.at(index);
        if (def->quotes.contains(current)) {
            int end = index + 1;
            while (end < length) {
                if (text.at(end) == QLatin1Char('\\')) {
                    end += 2;
                    continue;
                }
                if (text.at(end) == current) {
                    ++end;
                    break;
                }
                ++end;
            }
            end = qMin(end, length);
            spans.append({index, end - index, Token::String});
            index = end;
            continue;
        }

        if (current.isDigit()) {
            int end = index + 1;
            while (end < length) {
                const QChar ch = text.at(end);
                if (ch.isLetterOrNumber() || ch == QLatin1Char('.') || ch == QLatin1Char('_'))
                    ++end;
                else
                    break;
            }
            spans.append({index, end - index, Token::Number});
            index = end;
            continue;
        }

        if (current.isLetter() || current == QLatin1Char('_')) {
            int end = index + 1;
            while (end < length
                   && (text.at(end).isLetterOrNumber() || text.at(end) == QLatin1Char('_')))
                ++end;
            const QString word = text.mid(index, end - index);
            const QString probe = def->caseInsensitive ? word.toLower() : word;
            if (def->types.contains(probe))
                spans.append({index, end - index, Token::Type});
            else if (def->keywords.contains(probe))
                spans.append({index, end - index, Token::Keyword});
            index = end;
            continue;
        }

        ++index;
    }

    if (newBlockComment)
        *newBlockComment = comment;
    return spans;
}

} // namespace CodeHighlight
