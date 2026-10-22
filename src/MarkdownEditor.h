#pragma once

#include <QPlainTextEdit>

class MarkdownHighlighter;

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

signals:
    void zoomStepRequested(int direction);
    void fileDropped(const QString &path);

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
    void applyTheme();

private:
    void applyEditorFont();
    void indentSelection(bool outdent);
    bool acceptsLocalFiles(const QMimeData *mime) const;

    QWidget *m_lineNumberArea = nullptr;
    MarkdownHighlighter *m_highlighter = nullptr;
    int m_fontPointSize = 12;
};
