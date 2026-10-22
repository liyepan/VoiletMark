#include "MainWindow.h"

#include "MarkdownEditor.h"
#include "ThemeManager.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTimer>

namespace {

constexpr int kMaxRecentFiles = 8;
constexpr int kDefaultFontSize = 12;

const char kWelcomeText[] = R"VOILETMARK(
# 欢迎使用 VoiletMark

一款轻量的跨平台 Markdown 编辑器，基于 **Qt 5** 与 C++ 构建。

## 快速开始

- `Ctrl+O` 打开文件，`Ctrl+S` 保存，`Ctrl+Shift+S` 另存为
- 也可以直接把 Markdown 文件拖进窗口
- `Tab` / `Shift+Tab` 缩进整行，`Ctrl+滚轮` 调整字号

## 主题

界面默认跟随系统的浅色 / 深色设置自动切换，
也可以在 **视图 → 主题** 中手动锁定。

## 语法预览

> 引用、列表、`行内代码`、**粗体**、*斜体*、~~删除线~~
> 以及 [链接](https://example.com) 都会实时着色。

1. 有序列表项
2. 另一个有序列表项

- [x] 自动深浅色主题
- [x] Markdown 语法高亮
- [ ] 实时预览（下一版）

```cpp
#include <cstdio>

int main()
{
    std::printf("Hello, VoiletMark!\n");
    return 0;
}
```

---
)VOILETMARK";

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setMinimumSize(760, 520);
    resize(1024, 720);

    m_editor = new MarkdownEditor(this);
    setCentralWidget(m_editor);

    createActions();
    createMenus();
    createStatusBar();
    loadSettings();
    refreshRecentMenu();

    connect(m_editor->document(), &QTextDocument::modificationChanged,
            this, &MainWindow::updateWindowTitle);
    connect(m_editor, &QPlainTextEdit::cursorPositionChanged,
            this, &MainWindow::updateStatusBar);
    connect(m_editor, &MarkdownEditor::zoomStepRequested,
            this, &MainWindow::changeZoom);
    connect(m_editor, &MarkdownEditor::fileDropped,
            this, &MainWindow::openPath);

    showWelcomeDocument();
    updateEditActionStates();
    updateStatusBar();
    m_editor->setFocus();
}

void MainWindow::createActions()
{
    m_newAction = new QAction(tr("新建"), this);
    m_newAction->setShortcut(QKeySequence::New);
    connect(m_newAction, &QAction::triggered, this, &MainWindow::newFile);

    m_openAction = new QAction(tr("打开…"), this);
    m_openAction->setShortcut(QKeySequence::Open);
    connect(m_openAction, &QAction::triggered, this, &MainWindow::openFile);

    m_saveAction = new QAction(tr("保存"), this);
    m_saveAction->setShortcut(QKeySequence::Save);
    connect(m_saveAction, &QAction::triggered, this, [this] { saveFile(); });

    m_saveAsAction = new QAction(tr("另存为…"), this);
    m_saveAsAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+S")));
    connect(m_saveAsAction, &QAction::triggered, this, [this] { saveFileAs(); });

    m_quitAction = new QAction(tr("退出"), this);
    m_quitAction->setShortcut(QKeySequence::Quit);
    connect(m_quitAction, &QAction::triggered, this, &QWidget::close);

    m_undoAction = new QAction(tr("撤销"), this);
    m_undoAction->setShortcut(QKeySequence::Undo);
    connect(m_undoAction, &QAction::triggered, m_editor, &QPlainTextEdit::undo);

    m_redoAction = new QAction(tr("重做"), this);
    m_redoAction->setShortcut(QKeySequence::Redo);
    connect(m_redoAction, &QAction::triggered, m_editor, &QPlainTextEdit::redo);

    m_cutAction = new QAction(tr("剪切"), this);
    m_cutAction->setShortcut(QKeySequence::Cut);
    connect(m_cutAction, &QAction::triggered, m_editor, &QPlainTextEdit::cut);

    m_copyAction = new QAction(tr("复制"), this);
    m_copyAction->setShortcut(QKeySequence::Copy);
    connect(m_copyAction, &QAction::triggered, m_editor, &QPlainTextEdit::copy);

    m_pasteAction = new QAction(tr("粘贴"), this);
    m_pasteAction->setShortcut(QKeySequence::Paste);
    connect(m_pasteAction, &QAction::triggered, m_editor, &QPlainTextEdit::paste);

    m_selectAllAction = new QAction(tr("全选"), this);
    m_selectAllAction->setShortcut(QKeySequence::SelectAll);
    connect(m_selectAllAction, &QAction::triggered, m_editor, &QPlainTextEdit::selectAll);

    m_wrapAction = new QAction(tr("自动换行"), this);
    m_wrapAction->setCheckable(true);
    m_wrapAction->setShortcut(QKeySequence(QStringLiteral("Alt+Z")));
    connect(m_wrapAction, &QAction::toggled, this, &MainWindow::setWordWrapEnabled);

    m_zoomInAction = new QAction(tr("放大"), this);
    m_zoomInAction->setShortcut(QKeySequence::ZoomIn);
    connect(m_zoomInAction, &QAction::triggered, this, [this] { changeZoom(1); });

    m_zoomOutAction = new QAction(tr("缩小"), this);
    m_zoomOutAction->setShortcut(QKeySequence::ZoomOut);
    connect(m_zoomOutAction, &QAction::triggered, this, [this] { changeZoom(-1); });

    m_zoomResetAction = new QAction(tr("实际大小"), this);
    m_zoomResetAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+0")));
    connect(m_zoomResetAction, &QAction::triggered, this, &MainWindow::resetZoom);

    connect(m_editor, &QPlainTextEdit::undoAvailable, m_undoAction, &QAction::setEnabled);
    connect(m_editor, &QPlainTextEdit::redoAvailable, m_redoAction, &QAction::setEnabled);
    connect(m_editor, &QPlainTextEdit::copyAvailable, m_cutAction, &QAction::setEnabled);
    connect(m_editor, &QPlainTextEdit::copyAvailable, m_copyAction, &QAction::setEnabled);
}

void MainWindow::createMenus()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("文件"));
    fileMenu->addAction(m_newAction);
    fileMenu->addAction(m_openAction);
    m_recentMenu = fileMenu->addMenu(tr("最近打开"));
    fileMenu->addSeparator();
    fileMenu->addAction(m_saveAction);
    fileMenu->addAction(m_saveAsAction);
    fileMenu->addSeparator();
    fileMenu->addAction(m_quitAction);

    QMenu *editMenu = menuBar()->addMenu(tr("编辑"));
    editMenu->addAction(m_undoAction);
    editMenu->addAction(m_redoAction);
    editMenu->addSeparator();
    editMenu->addAction(m_cutAction);
    editMenu->addAction(m_copyAction);
    editMenu->addAction(m_pasteAction);
    editMenu->addSeparator();
    editMenu->addAction(m_selectAllAction);

    QMenu *viewMenu = menuBar()->addMenu(tr("视图"));
    viewMenu->addAction(m_wrapAction);
    viewMenu->addSeparator();
    viewMenu->addAction(m_zoomInAction);
    viewMenu->addAction(m_zoomOutAction);
    viewMenu->addAction(m_zoomResetAction);
    viewMenu->addSeparator();

    QMenu *themeMenu = viewMenu->addMenu(tr("主题"));
    QActionGroup *themeGroup = new QActionGroup(this);
    themeGroup->setExclusive(true);

    m_themeSystemAction = new QAction(ThemeManager::modeLabel(ThemeManager::Mode::System), this);
    m_themeLightAction = new QAction(ThemeManager::modeLabel(ThemeManager::Mode::Light), this);
    m_themeDarkAction = new QAction(ThemeManager::modeLabel(ThemeManager::Mode::Dark), this);
    for (QAction *action : {m_themeSystemAction, m_themeLightAction, m_themeDarkAction}) {
        action->setCheckable(true);
        themeGroup->addAction(action);
        themeMenu->addAction(action);
    }
    m_themeSystemAction->setChecked(true);

    connect(m_themeSystemAction, &QAction::triggered, this,
            [this] { applyThemeMode(static_cast<int>(ThemeManager::Mode::System)); });
    connect(m_themeLightAction, &QAction::triggered, this,
            [this] { applyThemeMode(static_cast<int>(ThemeManager::Mode::Light)); });
    connect(m_themeDarkAction, &QAction::triggered, this,
            [this] { applyThemeMode(static_cast<int>(ThemeManager::Mode::Dark)); });

    QMenu *helpMenu = menuBar()->addMenu(tr("帮助"));
    helpMenu->addAction(tr("关于 VoiletMark"), this, &MainWindow::showAbout);
}

void MainWindow::createStatusBar()
{
    m_statusLabel = new QLabel(this);
    m_statusLabel->setContentsMargins(8, 0, 10, 0);
    statusBar()->addPermanentWidget(m_statusLabel);
    statusBar()->setSizeGripEnabled(false);
}

void MainWindow::loadSettings()
{
    QSettings settings;

    const QByteArray geometry = settings.value(QStringLiteral("window/geometry")).toByteArray();
    if (!geometry.isEmpty())
        restoreGeometry(geometry);

    m_lastDirectory = settings.value(QStringLiteral("files/lastDirectory")).toString();
    if (m_lastDirectory.isEmpty())
        m_lastDirectory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

    const int fontSize = settings.value(QStringLiteral("editor/fontSize"), kDefaultFontSize).toInt();
    m_editor->setEditorFontPointSize(fontSize);

    const bool wrap = settings.value(QStringLiteral("editor/wordWrap"), true).toBool();
    m_wrapAction->setChecked(wrap);
    setWordWrapEnabled(wrap);

    loadRecentFiles();

    const ThemeManager::Mode mode = ThemeManager::instance()->mode();
    m_themeSystemAction->setChecked(mode == ThemeManager::Mode::System);
    m_themeLightAction->setChecked(mode == ThemeManager::Mode::Light);
    m_themeDarkAction->setChecked(mode == ThemeManager::Mode::Dark);
}

void MainWindow::saveSettings()
{
    QSettings settings;
    settings.setValue(QStringLiteral("window/geometry"), saveGeometry());
    settings.setValue(QStringLiteral("editor/fontSize"), m_editor->editorFontPointSize());
    settings.setValue(QStringLiteral("editor/wordWrap"), m_wrapAction->isChecked());
    settings.setValue(QStringLiteral("files/lastDirectory"), m_lastDirectory);
    settings.setValue(QStringLiteral("files/recent"), m_recentFiles);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!maybeSave()) {
        event->ignore();
        return;
    }
    saveSettings();
    event->accept();
}

QString MainWindow::displayName() const
{
    if (m_currentPath.isEmpty())
        return tr("未命名");
    return QFileInfo(m_currentPath).fileName();
}

QString MainWindow::startDirectory() const
{
    if (!m_currentPath.isEmpty())
        return QFileInfo(m_currentPath).absolutePath();
    if (QFileInfo::exists(m_lastDirectory))
        return m_lastDirectory;
    return QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
}

void MainWindow::showWelcomeDocument()
{
    QString text = QString::fromUtf8(kWelcomeText);
    if (text.startsWith(QLatin1Char('\n')))
        text.remove(0, 1);
    m_editor->setPlainText(text);
    m_editor->document()->setModified(false);
    m_editor->document()->clearUndoRedoStacks();
    setCurrentFile(QString());
    m_editor->moveCursor(QTextCursor::Start);
}

void MainWindow::newFile()
{
    if (!maybeSave())
        return;
    m_editor->clear();
    m_editor->document()->setModified(false);
    m_editor->document()->clearUndoRedoStacks();
    m_hasBom = false;
    m_eol = Eol::Lf;
    setCurrentFile(QString());
    updateStatusBar();
    m_editor->setFocus();
}

void MainWindow::openFile()
{
    if (!maybeSave())
        return;

    const QString path = QFileDialog::getOpenFileName(
        this, tr("打开 Markdown 文件"), startDirectory(),
        tr("Markdown 文件 (*.md *.markdown *.mdown *.mkd);;文本文件 (*.txt);;所有文件 (*)"));
    if (path.isEmpty())
        return;
    openPath(path);
}

bool MainWindow::openPath(const QString &path)
{
    if (path.isEmpty())
        return false;

    const QString absolutePath = QFileInfo(path).absoluteFilePath();
    if (!maybeSave())
        return false;

    QFile file(absolutePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("无法打开文件"),
                             tr("读取「%1」失败：%2")
                                 .arg(QDir::toNativeSeparators(absolutePath), file.errorString()));
        return false;
    }
    const QByteArray raw = file.readAll();
    file.close();

    m_hasBom = raw.startsWith("\xEF\xBB\xBF");
    QString text = QString::fromUtf8(m_hasBom ? raw.mid(3) : raw);
    m_eol = text.contains(QStringLiteral("\r\n")) ? Eol::Crlf : Eol::Lf;
    if (m_eol == Eol::Crlf)
        text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));

    m_editor->setPlainText(text);
    m_editor->document()->setModified(false);
    m_editor->document()->clearUndoRedoStacks();
    setCurrentFile(absolutePath);
    addRecentFile(absolutePath);
    updateStatusBar();
    m_editor->setFocus();
    return true;
}

bool MainWindow::saveFile()
{
    if (m_currentPath.isEmpty())
        return saveFileAs();
    return writeToFile(m_currentPath);
}

bool MainWindow::saveFileAs()
{
    QString path = QFileDialog::getSaveFileName(
        this, tr("另存为"), startDirectory(),
        tr("Markdown 文件 (*.md);;文本文件 (*.txt);;所有文件 (*)"));
    if (path.isEmpty())
        return false;

    if (QFileInfo(path).suffix().isEmpty())
        path += QStringLiteral(".md");

    return writeToFile(path);
}

bool MainWindow::writeToFile(const QString &path)
{
    QString text = m_editor->toPlainText();
    if (m_eol == Eol::Crlf)
        text.replace(QLatin1Char('\n'), QStringLiteral("\r\n"));

    QByteArray data = text.toUtf8();
    if (m_hasBom)
        data.prepend("\xEF\xBB\xBF");

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, tr("无法保存文件"),
                             tr("写入「%1」失败：%2")
                                 .arg(QDir::toNativeSeparators(path), file.errorString()));
        return false;
    }
    if (file.write(data) != data.size()) {
        file.cancelWriting();
        QMessageBox::warning(this, tr("无法保存文件"),
                             tr("写入「%1」时发生错误。").arg(QDir::toNativeSeparators(path)));
        return false;
    }
    if (!file.commit()) {
        QMessageBox::warning(this, tr("无法保存文件"),
                             tr("提交「%1」失败：%2")
                                 .arg(QDir::toNativeSeparators(path), file.errorString()));
        return false;
    }

    m_editor->document()->setModified(false);
    setCurrentFile(path);
    addRecentFile(path);
    statusBar()->showMessage(tr("已保存 %1").arg(QDir::toNativeSeparators(path)), 2500);
    return true;
}

bool MainWindow::maybeSave()
{
    if (!m_editor->document()->isModified())
        return true;

    const QMessageBox::StandardButton button = QMessageBox::warning(
        this, tr("VoiletMark"),
        tr("「%1」有未保存的修改，是否保存？").arg(displayName()),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
        QMessageBox::Save);

    if (button == QMessageBox::Save)
        return saveFile();
    return button == QMessageBox::Discard;
}

void MainWindow::setCurrentFile(const QString &path)
{
    m_currentPath = path;
    if (!path.isEmpty()) {
        m_lastDirectory = QFileInfo(path).absolutePath();
        setWindowFilePath(path);
    } else {
        setWindowFilePath(QString());
    }
    updateWindowTitle();
}

void MainWindow::updateWindowTitle()
{
    setWindowTitle(tr("%1[*] — VoiletMark").arg(displayName()));
    setWindowModified(m_editor->document()->isModified());
}

void MainWindow::updateEditActionStates()
{
    m_undoAction->setEnabled(m_editor->document()->isUndoAvailable());
    m_redoAction->setEnabled(m_editor->document()->isRedoAvailable());
    m_cutAction->setEnabled(m_editor->textCursor().hasSelection());
    m_copyAction->setEnabled(m_editor->textCursor().hasSelection());
}

void MainWindow::updateStatusBar()
{
    const QTextCursor cursor = m_editor->textCursor();
    const int line = cursor.blockNumber() + 1;
    const int column = cursor.positionInBlock() + 1;
    const int characters = m_editor->document()->characterCount() - 1;

    QString text = tr("行 %1，列 %2").arg(line).arg(column);
    if (cursor.hasSelection())
        text += tr("    已选 %1 字").arg(cursor.selectedText().size());
    text += tr("    %1 字").arg(qMax(0, characters));
    text += QStringLiteral("    UTF-8");
    m_statusLabel->setText(text);
}

void MainWindow::changeZoom(int direction)
{
    const int size = (direction == 0) ? kDefaultFontSize
                                      : m_editor->editorFontPointSize() + direction;
    m_editor->setEditorFontPointSize(size);
    QSettings().setValue(QStringLiteral("editor/fontSize"), m_editor->editorFontPointSize());
}

void MainWindow::resetZoom()
{
    changeZoom(0);
}

void MainWindow::setWordWrapEnabled(bool enabled)
{
    m_editor->setLineWrapMode(enabled ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
    QSettings().setValue(QStringLiteral("editor/wordWrap"), enabled);
}

void MainWindow::applyThemeMode(int mode)
{
    ThemeManager::instance()->setMode(static_cast<ThemeManager::Mode>(mode), true);
}

void MainWindow::showAbout()
{
    QMessageBox::about(
        this, tr("关于 VoiletMark"),
        tr("<h3>VoiletMark %1</h3>"
           "<p>一个轻量的跨平台 Markdown 编辑器。</p>"
           "<p>基于 Qt %2 与 C++17 构建，界面会跟随系统自动切换浅色 / 深色主题。</p>")
            .arg(QCoreApplication::applicationVersion(), QString::fromLatin1(qVersion())));
}

void MainWindow::addRecentFile(const QString &path)
{
    const QString absolutePath = QFileInfo(path).absoluteFilePath();
    m_recentFiles.removeAll(absolutePath);
    m_recentFiles.prepend(absolutePath);
    while (m_recentFiles.size() > kMaxRecentFiles)
        m_recentFiles.removeLast();
    refreshRecentMenu();
    QSettings().setValue(QStringLiteral("files/recent"), m_recentFiles);
}

void MainWindow::loadRecentFiles()
{
    m_recentFiles = QSettings().value(QStringLiteral("files/recent")).toStringList();
    m_recentFiles.removeDuplicates();
    while (m_recentFiles.size() > kMaxRecentFiles)
        m_recentFiles.removeLast();
}

void MainWindow::refreshRecentMenu()
{
    if (!m_recentMenu)
        return;

    m_recentMenu->clear();
    if (m_recentFiles.isEmpty()) {
        QAction *empty = m_recentMenu->addAction(tr("（暂无记录）"));
        empty->setEnabled(false);
        return;
    }

    for (const QString &path : m_recentFiles) {
        QAction *action = m_recentMenu->addAction(QDir::toNativeSeparators(path));
        action->setToolTip(path);
        connect(action, &QAction::triggered, this, [this, path] {
            openPath(path);
        });
    }
    m_recentMenu->addSeparator();
    m_recentMenu->addAction(tr("清空列表"), this, [this] {
        m_recentFiles.clear();
        QSettings().setValue(QStringLiteral("files/recent"), m_recentFiles);
        refreshRecentMenu();
    });
}
