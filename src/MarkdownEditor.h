#pragma once

#include <QPlainTextEdit>

class MarkdownHighlighter;
class QTextBlock;

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
    /// 源代码模式：关掉语法高亮、行号与当前行高亮，只看文件原始内容。
    void setSourceMode(bool enabled);

    /// 把选中的行设为 N 级标题（1–6）；整段已经是这一级时改回正文。
    void setHeadingLevel(int level);
    /// 把选中内容包成围栏代码块；如果已经围起来了则拆掉。
    void toggleCodeBlock();

signals:
    void zoomStepRequested(int direction);
    void fileDropped(const QString &path);
    void sourceModeChanged(bool enabled);

protected:
    void resizeEvent(QResizeEvent *event) override;
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

private:
    void applyEditorFont();
    void indentSelection(bool outdent);
    bool acceptsLocalFiles(const QMimeData *mime) const;
    void selectedBlockRange(int *firstBlock, int *lastBlock) const;
    void selectBlocks(int firstBlock, int lastBlock);
    bool enclosingFence(int firstBlock, int lastBlock,
                        QTextBlock *opener, QTextBlock *closer) const;

    QWidget *m_lineNumberArea = nullptr;
    MarkdownHighlighter *m_highlighter = nullptr;
    int m_fontPointSize = 12;
    bool m_sourceMode = false;
    int m_activeBlock = -1;
};
