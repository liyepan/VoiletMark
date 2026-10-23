#pragma once

#include "CodeHighlight.h"

#include <QPlainTextEdit>
#include <QVector>

class MarkdownHighlighter;
class QTextBlock;
class QTimer;
class QTableWidget;

/// 编辑器本体：等宽字体、行号栏、当前行高亮、Tab 缩进、
/// Ctrl+滚轮缩放，以及把拖入的文件转交给主窗口打开。
class MarkdownEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit MarkdownEditor(QWidget *parent = nullptr);

    int lineNumberAreaWidth() const;
    void lineNumberAreaPaintEvent(QPaintEvent *event);

    void setEditorFontPointSize(int points);
    int editorFontPointSize() const { return m_fontPointSize; }

    MarkdownHighlighter *highlighter() const { return m_highlighter; }

    bool isSourceMode() const { return m_sourceMode; }
    /// 源代码模式：等宽字体 + 行号 + 每行一条细线，原样显示标记。
    void setSourceMode(bool enabled);

    /// 给全文套一层统一的段落节奏（行高、段间距），加载文档后调用一次。
    void applyBlockRhythm();

    /// 把选中的行设为 N 级标题（1–6）；整段已经是这一级时改回正文。
    void setHeadingLevel(int level);
    /// 把选中内容包成围栏代码块；如果已经围起来了则拆掉。
    void toggleCodeBlock();

    /// 把光标移到指定行并居中显示（目录跳转用）。
    void goToBlock(int blockNumber);

signals:
    void zoomStepRequested(int direction);
    void fileDropped(const QString &path);
    void sourceModeChanged(bool enabled);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect &rect, int dy);
    void updateCurrentLineHighlight();
    void updateActiveBlock();
    void applyTheme();
    void rebuildCodeFences();

private:
    struct TableInfo;

    void applyEditorFont();
    void updateViewportMargins();
    void indentSelection(bool outdent);
    QRectF codeFenceRect(int index) const;
    QRectF copyButtonRect(int index) const;
    QRectF languageRect(int index) const;
    QString codeFenceText(int index) const;
    QString languageLabel(int index) const;
    void showLanguageMenu(int index);
    void setCodeFenceLanguage(int index, const QString &info);

    // ---- 表格 ----
    void rebuildTables();
    void positionTableWidgets();
    QRectF tableRect(const TableInfo &table) const;
    void createTableWidget(int index);
    void applyTableEdit(int index);
    void replaceTableText(int index, const QStringList &lines);
    int tableIndexFor(const QTableWidget *widget) const;
    bool acceptsLocalFiles(const QMimeData *mime) const;
    void selectedBlockRange(int *firstBlock, int *lastBlock) const;
    void selectBlocks(int firstBlock, int lastBlock);
    bool enclosingFence(int firstBlock, int lastBlock,
                        QTextBlock *opener, QTextBlock *closer) const;

    struct CodeFence {
        int open = -1;   // 开围栏所在行
        int close = -1;  // 闭围栏所在行，未闭合时为 -1
        QString info;    // 围栏上的语言标记
        CodeHighlight::Language language = CodeHighlight::Language::None;
    };

    struct TableInfo {
        int firstBlock = -1;
        int lastBlock = -1;
        QTableWidget *widget = nullptr;
        QVector<Qt::Alignment> alignments;
    };

    QWidget *m_lineNumberArea = nullptr;
    MarkdownHighlighter *m_highlighter = nullptr;
    int m_fontPointSize = 12;
    bool m_sourceMode = false;
    int m_activeBlock = -1;

    QVector<CodeFence> m_codeFences;
    QVector<TableInfo> m_tables;
    bool m_updatingTable = false;
    bool m_skipTableRebuild = false;
    QTimer *m_fenceTimer = nullptr;
    int m_hoveredFence = -1;
    int m_hoveredLanguage = -1;
    int m_copiedFence = -1;
};
