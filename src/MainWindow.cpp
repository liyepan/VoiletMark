#include "MainWindow.h"

#include "ExportDialog.h"
#include "MarkdownEditor.h"
#include "ThemeManager.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QDir>
#include <QDirIterator>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPixmap>
#include <QSaveFile>
#include <QSettings>
#include <QSignalBlocker>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>

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
    createSidebar();
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

    m_exportPdfAction = new QAction(tr("导出 PDF…"), this);
    m_exportPdfAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+E")));
    connect(m_exportPdfAction, &QAction::triggered, this, &MainWindow::exportPdf);

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

    m_openFolderAction = new QAction(tr("打开文件夹…"), this);
    m_openFolderAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+O")));
    connect(m_openFolderAction, &QAction::triggered, this, &MainWindow::chooseFolder);

    m_sidebarAction = new QAction(tr("侧栏"), this);
    m_sidebarAction->setCheckable(true);
    m_sidebarAction->setChecked(true);
    m_sidebarAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+B")));
    connect(m_sidebarAction, &QAction::toggled, this, [this](bool visible) {
        if (m_sidebar)
            m_sidebar->setVisible(visible);
        if (m_sidebarButton) {
            const QSignalBlocker blocker(m_sidebarButton);
            m_sidebarButton->setChecked(visible);
        }
        QSettings().setValue(QStringLiteral("window/sidebarVisible"), visible);
    });

    m_sourceModeAction = new QAction(tr("源代码模式"), this);
    m_sourceModeAction->setCheckable(true);
    m_sourceModeAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+M")));
    connect(m_sourceModeAction, &QAction::toggled, this, &MainWindow::setSourceModeEnabled);

    m_codeBlockAction = new QAction(tr("代码块"), this);
    m_codeBlockAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+/")));
    connect(m_codeBlockAction, &QAction::triggered, this, [this] {
        m_editor->toggleCodeBlock();
    });

    for (int level = 1; level <= 6; ++level) {
        QAction *action = new QAction(tr("标题 %1").arg(level), this);
        action->setShortcut(QKeySequence(QStringLiteral("Ctrl+%1").arg(level)));
        connect(action, &QAction::triggered, this, [this, level] {
            m_editor->setHeadingLevel(level);
        });
        m_headingActions[level - 1] = action;
    }
}

void MainWindow::createSidebar()
{
    m_sidebar = new QDockWidget(tr("文件夹"), this);
    m_sidebar->setObjectName(QStringLiteral("folderDock"));
    m_sidebar->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_sidebar->setFeatures(QDockWidget::NoDockWidgetFeatures);
    m_sidebar->setTitleBarWidget(new QWidget(m_sidebar));

    QWidget *content = new QWidget(m_sidebar);
    QVBoxLayout *layout = new QVBoxLayout(content);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    m_folderLabel = new QLabel(tr("未打开文件夹"), content);
    m_folderLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    layout->addWidget(m_folderLabel);

    m_fileList = new QListWidget(content);
    m_fileList->setFrameShape(QFrame::NoFrame);
    m_fileList->setUniformItemSizes(true);
    m_fileList->setTextElideMode(Qt::ElideMiddle);
    m_fileList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    layout->addWidget(m_fileList, 1);

    m_sidebar->setWidget(content);
    addDockWidget(Qt::LeftDockWidgetArea, m_sidebar);
    resizeDocks({m_sidebar}, {250}, Qt::Horizontal);

    connect(m_fileList, &QListWidget::itemClicked, this, &MainWindow::openSidebarItem);

    // 列表自己刷新：没有“刷新”按钮，每 5 秒比对一次文件集合，变了才重建。
    m_folderRefreshTimer = new QTimer(this);
    m_folderRefreshTimer->setInterval(5000);
    connect(m_folderRefreshTimer, &QTimer::timeout, this, &MainWindow::refreshFolder);
    m_folderRefreshTimer->start();
}

void MainWindow::createMenus()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("文件"));
    fileMenu->addAction(m_newAction);
    fileMenu->addAction(m_openAction);
    fileMenu->addAction(m_openFolderAction);
    m_recentMenu = fileMenu->addMenu(tr("最近打开"));
    fileMenu->addSeparator();
    fileMenu->addAction(m_saveAction);
    fileMenu->addAction(m_saveAsAction);
    fileMenu->addAction(m_exportPdfAction);
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
    viewMenu->addAction(m_sidebarAction);
    viewMenu->addAction(m_sourceModeAction);
    viewMenu->addSeparator();
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
    m_themeScheduleAction = new QAction(ThemeManager::modeLabel(ThemeManager::Mode::Schedule), this);
    m_themeScheduleAction->setToolTip(tr("6:00–18:00 使用浅色，其余时间使用深色"));
    for (QAction *action : {m_themeSystemAction, m_themeLightAction, m_themeDarkAction,
                            m_themeScheduleAction}) {
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
    connect(m_themeScheduleAction, &QAction::triggered, this,
            [this] { applyThemeMode(static_cast<int>(ThemeManager::Mode::Schedule)); });

    QMenu *formatMenu = menuBar()->addMenu(tr("格式"));
    QMenu *headingMenu = formatMenu->addMenu(tr("标题"));
    for (QAction *headingAction : m_headingActions) {
        if (headingAction)
            headingMenu->addAction(headingAction);
    }
    formatMenu->addAction(m_codeBlockAction);

    QMenu *helpMenu = menuBar()->addMenu(tr("帮助"));
    helpMenu->addAction(tr("关于 VoiletMark"), this, &MainWindow::showAbout);
}

void MainWindow::createStatusBar()
{
    // 左下角：侧栏开关 + 源代码模式开关。
    m_sidebarButton = new QToolButton(this);
    m_sidebarButton->setText(tr("侧栏"));
    m_sidebarButton->setCheckable(true);
    m_sidebarButton->setChecked(true);
    m_sidebarButton->setCursor(Qt::PointingHandCursor);
    m_sidebarButton->setToolTip(tr("显示 / 隐藏文件夹侧栏（Ctrl+B）"));
    statusBar()->addWidget(m_sidebarButton);
    connect(m_sidebarButton, &QToolButton::toggled, m_sidebarAction, &QAction::setChecked);

    m_sourceButton = new QToolButton(this);
    m_sourceButton->setText(QStringLiteral("<src>"));
    m_sourceButton->setCheckable(true);
    m_sourceButton->setCursor(Qt::PointingHandCursor);
    m_sourceButton->setToolTip(tr("源代码模式：关闭语法高亮与行号（Ctrl+Shift+M）"));
    statusBar()->addWidget(m_sourceButton);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setContentsMargins(8, 0, 10, 0);
    statusBar()->addPermanentWidget(m_statusLabel);
    statusBar()->setSizeGripEnabled(false);

    connect(m_sourceButton, &QToolButton::toggled, this, &MainWindow::setSourceModeEnabled);
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
    m_themeScheduleAction->setChecked(mode == ThemeManager::Mode::Schedule);

    m_folderPath = settings.value(QStringLiteral("files/folder")).toString();
    if (!m_folderPath.isEmpty() && QFileInfo(m_folderPath).isDir())
        refreshFolder();
    else
        m_folderPath.clear();

    const bool sidebarVisible =
        settings.value(QStringLiteral("window/sidebarVisible"), true).toBool();
    m_sidebar->setVisible(sidebarVisible);
    m_sidebarAction->setChecked(sidebarVisible);

    const bool sourceMode = settings.value(QStringLiteral("editor/sourceMode"), false).toBool();
    setSourceModeEnabled(sourceMode);
}

void MainWindow::saveSettings()
{
    QSettings settings;
    settings.setValue(QStringLiteral("window/geometry"), saveGeometry());
    settings.setValue(QStringLiteral("editor/fontSize"), m_editor->editorFontPointSize());
    settings.setValue(QStringLiteral("editor/wordWrap"), m_wrapAction->isChecked());
    settings.setValue(QStringLiteral("files/lastDirectory"), m_lastDirectory);
    settings.setValue(QStringLiteral("files/recent"), m_recentFiles);
    settings.setValue(QStringLiteral("files/folder"), m_folderPath);
    settings.setValue(QStringLiteral("window/sidebarVisible"), m_sidebar->isVisible());
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
    m_editor->applyBlockRhythm();
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
    m_editor->applyBlockRhythm();
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
    m_editor->applyBlockRhythm();
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

void MainWindow::exportPdf()
{
    ExportDialog dialog(m_editor->toPlainText(), displayName(), startDirectory(), this);
    dialog.exec();
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
    selectInSidebar(path);
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
    QMessageBox box(this);
    box.setWindowTitle(tr("关于 VoiletMark"));
    box.setIconPixmap(QPixmap(QStringLiteral(":/logo.png"))
                          .scaled(72, 72, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    box.setTextFormat(Qt::RichText);
    box.setText(tr("<h3>VoiletMark %1</h3>"
                   "<p>一个轻量的跨平台 Markdown 编辑器。</p>"
                   "<p>基于 Qt %2 与 C++17 构建。</p>")
                    .arg(QCoreApplication::applicationVersion(),
                         QString::fromLatin1(qVersion())));
    box.setStandardButtons(QMessageBox::Ok);
    box.exec();
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

void MainWindow::chooseFolder()
{
    const QString directory = QFileDialog::getExistingDirectory(
        this, tr("打开文件夹"), startDirectory(), QFileDialog::ShowDirsOnly);
    if (!directory.isEmpty())
        openFolder(directory);
}

void MainWindow::openFolder(const QString &path)
{
    const QFileInfo info(path);
    if (!info.isDir())
        return;

    m_folderPath = info.absoluteFilePath();
    m_lastDirectory = m_folderPath;
    QSettings().setValue(QStringLiteral("files/folder"), m_folderPath);
    refreshFolder();

    m_sidebar->setVisible(true);
    m_sidebarAction->setChecked(true);
}

QStringList MainWindow::scanFolder() const
{
    QStringList files;
    if (m_folderPath.isEmpty() || !QFileInfo(m_folderPath).isDir())
        return files;

    const QDir root(m_folderPath);
    QDirIterator iterator(m_folderPath,
                          {QStringLiteral("*.md"), QStringLiteral("*.markdown"),
                           QStringLiteral("*.mdown"), QStringLiteral("*.mkd")},
                          QDir::Files, QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        const QString filePath = iterator.next();
        const QString relative = root.relativeFilePath(filePath);
        if (relative.startsWith(QLatin1Char('.')) || relative.contains(QLatin1String("/.")))
            continue;
        files.append(filePath);
    }

    std::sort(files.begin(), files.end(), [&root](const QString &left, const QString &right) {
        return root.relativeFilePath(left).compare(root.relativeFilePath(right),
                                                   Qt::CaseInsensitive) < 0;
    });
    return files;
}

void MainWindow::refreshFolder()
{
    if (!m_fileList)
        return;

    if (m_folderPath.isEmpty() || !QFileInfo(m_folderPath).isDir()) {
        if (m_fileList->count() > 0 || m_displayedFolder != m_folderPath) {
            m_fileList->clear();
            m_displayedFolder = m_folderPath;
            m_folderLabel->setText(tr("未打开文件夹"));
            m_folderLabel->setToolTip(tr("文件 → 打开文件夹…"));
        }
        return;
    }

    const QDir root(m_folderPath);
    if (m_displayedFolder != m_folderPath) {
        m_displayedFolder = m_folderPath;
        m_fileList->clear();
        m_folderLabel->setText(root.dirName().isEmpty() ? m_folderPath : root.dirName());
        m_folderLabel->setToolTip(QDir::toNativeSeparators(m_folderPath));
    }

    const QStringList files = scanFolder();

    // 和当前显示的内容一致就什么都不做，避免每 5 秒闪一下、丢失选中项。
    QStringList shown;
    shown.reserve(m_fileList->count());
    for (int index = 0; index < m_fileList->count(); ++index) {
        const QString path = m_fileList->item(index)->data(Qt::UserRole).toString();
        if (!path.isEmpty())
            shown.append(path);
    }
    if (files == shown)
        return;

    const QString selectedPath = m_currentPath;
    m_fileList->clear();

    for (const QString &filePath : files) {
        QListWidgetItem *item = new QListWidgetItem(root.relativeFilePath(filePath), m_fileList);
        item->setData(Qt::UserRole, filePath);
        item->setToolTip(QDir::toNativeSeparators(filePath));
    }

    if (files.isEmpty()) {
        QListWidgetItem *empty = new QListWidgetItem(tr("（没有 Markdown 文件）"), m_fileList);
        empty->setFlags(Qt::NoItemFlags);
    }

    selectInSidebar(selectedPath);
}

void MainWindow::openSidebarItem(QListWidgetItem *item)
{
    if (!item)
        return;

    const QString path = item->data(Qt::UserRole).toString();
    if (path.isEmpty())
        return;

    if (!m_currentPath.isEmpty()
        && QFileInfo(path).absoluteFilePath() == QFileInfo(m_currentPath).absoluteFilePath()
        && !m_editor->document()->isModified()) {
        return;
    }
    openPath(path);
}

void MainWindow::selectInSidebar(const QString &path)
{
    if (!m_fileList || m_fileList->count() == 0)
        return;

    const QString absolute = path.isEmpty() ? QString() : QFileInfo(path).absoluteFilePath();
    if (!absolute.isEmpty()) {
        for (int index = 0; index < m_fileList->count(); ++index) {
            QListWidgetItem *item = m_fileList->item(index);
            const QString itemPath = item->data(Qt::UserRole).toString();
            if (itemPath.isEmpty())
                continue;
            if (QFileInfo(itemPath).absoluteFilePath() == absolute) {
                m_fileList->setCurrentItem(item);
                m_fileList->scrollToItem(item);
                return;
            }
        }
    }

    m_fileList->clearSelection();
    m_fileList->setCurrentItem(nullptr);
}

void MainWindow::setSourceModeEnabled(bool enabled)
{
    m_editor->setSourceMode(enabled);

    if (m_sourceButton) {
        const QSignalBlocker blocker(m_sourceButton);
        m_sourceButton->setChecked(enabled);
        m_sourceButton->setText(enabled ? QStringLiteral("</src>") : QStringLiteral("<src>"));
    }
    if (m_sourceModeAction) {
        const QSignalBlocker blocker(m_sourceModeAction);
        m_sourceModeAction->setChecked(enabled);
    }

    QSettings().setValue(QStringLiteral("editor/sourceMode"), enabled);
}
