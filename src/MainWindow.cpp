#include "MainWindow.h"

#include "GitRunner.h"
#include "LanguageManager.h"
#include "MarkdownEditor.h"
#include "MarkdownRenderer.h"
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
#include <QGridLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPageLayout>
#include <QPageSize>
#include <QPixmap>
#include <QProcess>
#include <QPrinter>
#include <QPushButton>
#include <QSaveFile>
#include <QSettings>
#include <QSignalBlocker>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTextBlock>
#include <QTextStream>
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

| 功能 | 状态 |
| --- | :---: |
| 语法高亮 | 完成 |
| 导出 PDF | 完成 |
| Git 同步 | 完成 |

- [x] 自动深浅色主题
- [x] Markdown 语法高亮
- [x] 导出 PDF

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

const char kWelcomeTextEn[] = R"VOILETMARK(
# Welcome to VoiletMark

A lightweight cross-platform Markdown editor built with **Qt 5** and C++.

## Quick start

- `Ctrl+O` opens a file, `Ctrl+S` saves, `Ctrl+Shift+S` saves as
- Drag a Markdown file into the window to open it
- `Ctrl+1` – `Ctrl+6` set heading levels, `Ctrl+/` wraps a code block
- `Ctrl+E` exports the document to PDF (A4)

## Modes

The default view is the editor: Markdown markers are hidden, headings are sized,
and the line under the cursor reveals its raw markers.
Click `<src>` in the status bar to see the source with line numbers.

## Theme

Follows the system light / dark setting by default;
switch it under **View → Theme**.

> Quotes, lists, `inline code`, **bold**, *italic*, ~~strikethrough~~
> and [links](https://example.com) all render in place.

| Feature | Status |
| --- | :---: |
| Syntax highlighting | Done |
| PDF export | Done |
| Git sync | Done |

- [x] Syntax highlighting
- [x] Folder sidebar
- [x] PDF export

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

    m_git = new GitRunner(this);
    connect(m_git, &GitRunner::finished, this, &MainWindow::onGitFinished);

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
    connect(LanguageManager::instance(), &LanguageManager::languageChanged,
            this, &MainWindow::retranslateUi);

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
    m_fileList->setContextMenuPolicy(Qt::CustomContextMenu);
    layout->addWidget(m_fileList, 1);

    // ---- Git 工具条：只有打开的是仓库时才显示 ----
    m_gitBar = new QWidget(content);
    QGridLayout *gitLayout = new QGridLayout(m_gitBar);
    gitLayout->setContentsMargins(0, 2, 0, 0);
    gitLayout->setSpacing(4);

    m_gitStatusLabel = new QLabel(tr("Git 仓库"), m_gitBar);
    m_gitStatusLabel->setForegroundRole(QPalette::PlaceholderText);
    gitLayout->addWidget(m_gitStatusLabel, 0, 0, 1, 3);

    auto makeGitButton = [this](const QString &text, const QString &tip) {
        QPushButton *button = new QPushButton(text, m_gitBar);
        button->setToolTip(tip);
        button->setCursor(Qt::PointingHandCursor);
        button->setStyleSheet(QStringLiteral("padding: 3px 6px; min-width: 0;"));
        return button;
    };

    m_gitFetchButton = makeGitButton(tr("获取"), tr("git fetch --all --prune"));
    m_gitPullButton = makeGitButton(tr("拉取"), tr("git pull --ff-only"));
    m_gitStageButton = makeGitButton(tr("添加"), tr("git add -A"));
    m_gitCommitButton = makeGitButton(tr("提交"), tr("把暂存区的内容提交"));
    m_gitPushButton = makeGitButton(tr("推送"), tr("git push"));

    gitLayout->addWidget(m_gitFetchButton, 1, 0);
    gitLayout->addWidget(m_gitPullButton, 1, 1);
    gitLayout->addWidget(m_gitStageButton, 1, 2);
    gitLayout->addWidget(m_gitCommitButton, 2, 0, 1, 2);
    gitLayout->addWidget(m_gitPushButton, 2, 2);

    layout->addWidget(m_gitBar);
    m_gitBar->setVisible(false);

    connect(m_gitPullButton, &QPushButton::clicked, this, &MainWindow::gitPull);
    connect(m_gitFetchButton, &QPushButton::clicked, this, &MainWindow::gitFetch);
    connect(m_gitPushButton, &QPushButton::clicked, this, &MainWindow::gitPush);
    connect(m_gitStageButton, &QPushButton::clicked, this, &MainWindow::gitStageAll);
    connect(m_gitCommitButton, &QPushButton::clicked, this, &MainWindow::gitCommit);
    connect(m_fileList, &QListWidget::customContextMenuRequested,
            this, &MainWindow::showFileContextMenu);

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
    m_fileMenu = menuBar()->addMenu(tr("文件"));
    m_fileMenu->addAction(m_newAction);
    m_fileMenu->addAction(m_openAction);
    m_fileMenu->addAction(m_openFolderAction);
    m_recentMenu = m_fileMenu->addMenu(tr("最近打开"));
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_saveAction);
    m_fileMenu->addAction(m_saveAsAction);
    m_fileMenu->addAction(m_exportPdfAction);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_quitAction);

    m_editMenu = menuBar()->addMenu(tr("编辑"));
    m_editMenu->addAction(m_undoAction);
    m_editMenu->addAction(m_redoAction);
    m_editMenu->addSeparator();
    m_editMenu->addAction(m_cutAction);
    m_editMenu->addAction(m_copyAction);
    m_editMenu->addAction(m_pasteAction);
    m_editMenu->addSeparator();
    m_editMenu->addAction(m_selectAllAction);

    m_viewMenu = menuBar()->addMenu(tr("视图"));
    m_viewMenu->addAction(m_sidebarAction);
    m_viewMenu->addAction(m_sourceModeAction);
    m_viewMenu->addSeparator();
    m_viewMenu->addAction(m_wrapAction);
    m_viewMenu->addSeparator();
    m_viewMenu->addAction(m_zoomInAction);
    m_viewMenu->addAction(m_zoomOutAction);
    m_viewMenu->addAction(m_zoomResetAction);
    m_viewMenu->addSeparator();

    m_themeMenu = m_viewMenu->addMenu(tr("主题"));
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
        m_themeMenu->addAction(action);
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

    m_languageMenu = m_viewMenu->addMenu(tr("语言"));
    QActionGroup *languageGroup = new QActionGroup(this);
    languageGroup->setExclusive(true);
    m_languageChineseAction =
        new QAction(LanguageManager::displayName(QStringLiteral("zh_CN")), this);
    m_languageEnglishAction =
        new QAction(LanguageManager::displayName(QStringLiteral("en")), this);
    for (QAction *action : {m_languageChineseAction, m_languageEnglishAction}) {
        action->setCheckable(true);
        languageGroup->addAction(action);
        m_languageMenu->addAction(action);
    }
    connect(m_languageChineseAction, &QAction::triggered, this,
            [this] { applyLanguage(QStringLiteral("zh_CN")); });
    connect(m_languageEnglishAction, &QAction::triggered, this,
            [this] { applyLanguage(QStringLiteral("en")); });

    m_formatMenu = menuBar()->addMenu(tr("格式"));
    m_headingMenu = m_formatMenu->addMenu(tr("标题"));
    for (QAction *headingAction : m_headingActions) {
        if (headingAction)
            m_headingMenu->addAction(headingAction);
    }
    m_formatMenu->addAction(m_codeBlockAction);

    m_helpMenu = menuBar()->addMenu(tr("帮助"));
    m_aboutAction = m_helpMenu->addAction(tr("关于 VoiletMark"), this, &MainWindow::showAbout);
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

    updateGitState();
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
    const bool english = LanguageManager::instance()->language() == QLatin1String("en");
    QString text = QString::fromUtf8(english ? kWelcomeTextEn : kWelcomeText);
    if (text.startsWith(QLatin1Char('\n')))
        text.remove(0, 1);
    m_welcomeText = text;
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
        tr("Markdown 文件 (*.md *.markdown *.mdown *.mkd);;文本文件 (*.txt);;所有文件 (*)"),
        nullptr, QFileDialog::DontUseNativeDialog);
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
        tr("Markdown 文件 (*.md);;文本文件 (*.txt);;所有文件 (*)"),
        nullptr, QFileDialog::DontUseNativeDialog);
    if (path.isEmpty())
        return false;

    if (QFileInfo(path).suffix().isEmpty())
        path += QStringLiteral(".md");

    return writeToFile(path);
}

void MainWindow::exportPdf()
{
    QString suggested = m_currentPath.isEmpty() ? tr("未命名")
                                                : QFileInfo(m_currentPath).completeBaseName();
    suggested += QStringLiteral(".pdf");

    QString target = QFileDialog::getSaveFileName(
        this, tr("导出 PDF"), QDir(startDirectory()).filePath(suggested),
        tr("PDF 文件 (*.pdf)"), nullptr, QFileDialog::DontUseNativeDialog);
    if (target.isEmpty())
        return;
    if (!target.endsWith(QStringLiteral(".pdf"), Qt::CaseInsensitive))
        target += QStringLiteral(".pdf");

    statusBar()->showMessage(tr("正在导出 PDF…"));

    QTextDocument document;
    MarkdownRenderer::render(m_editor->toPlainText(), &document);
    document.setPageSize(QSizeF(672.76, 1001.57));

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(target);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageMargins(QMarginsF(16, 16, 16, 16), QPageLayout::Millimeter);
    printer.setDocName(QFileInfo(target).completeBaseName());

    QApplication::setOverrideCursor(Qt::WaitCursor);
    document.print(&printer);
    QApplication::restoreOverrideCursor();

    if (!QFileInfo::exists(target) || QFileInfo(target).size() <= 0) {
        QMessageBox::warning(this, tr("导出失败"),
                             tr("没能写出「%1」，换个位置再试一次。")
                                 .arg(QDir::toNativeSeparators(target)));
        return;
    }
    statusBar()->showMessage(tr("已导出 %1").arg(QDir::toNativeSeparators(target)), 4000);
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

void MainWindow::applyLanguage(const QString &code)
{
    LanguageManager::instance()->setLanguage(code);
}

void MainWindow::retranslateUi()
{
    m_fileMenu->setTitle(tr("文件"));
    m_editMenu->setTitle(tr("编辑"));
    m_viewMenu->setTitle(tr("视图"));
    m_formatMenu->setTitle(tr("格式"));
    m_helpMenu->setTitle(tr("帮助"));
    m_recentMenu->setTitle(tr("最近打开"));
    m_themeMenu->setTitle(tr("主题"));
    m_headingMenu->setTitle(tr("标题"));
    m_languageMenu->setTitle(tr("语言"));

    m_newAction->setText(tr("新建"));
    m_openAction->setText(tr("打开…"));
    m_openFolderAction->setText(tr("打开文件夹…"));
    m_saveAction->setText(tr("保存"));
    m_saveAsAction->setText(tr("另存为…"));
    m_exportPdfAction->setText(tr("导出 PDF…"));
    m_quitAction->setText(tr("退出"));

    m_undoAction->setText(tr("撤销"));
    m_redoAction->setText(tr("重做"));
    m_cutAction->setText(tr("剪切"));
    m_copyAction->setText(tr("复制"));
    m_pasteAction->setText(tr("粘贴"));
    m_selectAllAction->setText(tr("全选"));

    m_sidebarAction->setText(tr("侧栏"));
    m_sourceModeAction->setText(tr("源代码模式"));
    m_wrapAction->setText(tr("自动换行"));
    m_zoomInAction->setText(tr("放大"));
    m_zoomOutAction->setText(tr("缩小"));
    m_zoomResetAction->setText(tr("实际大小"));

    m_themeSystemAction->setText(ThemeManager::modeLabel(ThemeManager::Mode::System));
    m_themeLightAction->setText(ThemeManager::modeLabel(ThemeManager::Mode::Light));
    m_themeDarkAction->setText(ThemeManager::modeLabel(ThemeManager::Mode::Dark));
    m_themeScheduleAction->setText(ThemeManager::modeLabel(ThemeManager::Mode::Schedule));
    m_themeScheduleAction->setToolTip(tr("6:00–18:00 使用浅色，其余时间使用深色"));

    for (int level = 1; level <= 6; ++level) {
        if (m_headingActions[level - 1])
            m_headingActions[level - 1]->setText(tr("标题 %1").arg(level));
    }
    m_codeBlockAction->setText(tr("代码块"));
    m_aboutAction->setText(tr("关于 VoiletMark"));

    if (m_sidebarButton) {
        m_sidebarButton->setText(tr("侧栏"));
        m_sidebarButton->setToolTip(tr("显示 / 隐藏文件夹侧栏（Ctrl+B）"));
    }
    if (m_sourceButton)
        m_sourceButton->setToolTip(tr("源代码模式：关闭语法高亮与行号（Ctrl+Shift+M）"));

    const QString language = LanguageManager::instance()->language();
    m_languageChineseAction->setChecked(language == QLatin1String("zh_CN"));
    m_languageEnglishAction->setChecked(language == QLatin1String("en"));

    // 侧栏、最近打开、欢迎文档都跟着换
    m_displayedFolder.clear();
    refreshRecentMenu();
    refreshFolder();

    if (m_currentPath.isEmpty() && !m_editor->document()->isModified()
        && m_editor->toPlainText() == m_welcomeText) {
        showWelcomeDocument();
    }

    updateWindowTitle();
    updateStatusBar();
}

void MainWindow::showAbout()
{
    QMessageBox box(this);
    box.setWindowTitle(tr("关于 VoiletMark"));
    box.setIconPixmap(QPixmap(QStringLiteral(":/packaging/voiletmark.png"))
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
        this, tr("打开文件夹"), startDirectory(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontUseNativeDialog);
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
    updateGitState();

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

// ---------------------------------------------------------------- Git

void MainWindow::updateGitState()
{
    m_gitRoot.clear();
    m_gitBranch.clear();

    if (!m_folderPath.isEmpty() && QFileInfo(m_folderPath).isDir()) {
        QProcess process;
        process.setWorkingDirectory(m_folderPath);
        process.start(QStringLiteral("git"),
                      {QStringLiteral("rev-parse"), QStringLiteral("--show-toplevel")});
        if (process.waitForFinished(3000) && process.exitCode() == 0)
            m_gitRoot = QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
    }

    if (!m_gitRoot.isEmpty()) {
        QProcess branch;
        branch.setWorkingDirectory(m_gitRoot);
        branch.start(QStringLiteral("git"), {QStringLiteral("rev-parse"),
                                             QStringLiteral("--abbrev-ref"),
                                             QStringLiteral("HEAD")});
        if (branch.waitForFinished(3000) && branch.exitCode() == 0)
            m_gitBranch = QString::fromLocal8Bit(branch.readAllStandardOutput()).trimmed();
    }

    if (m_git)
        m_git->setWorkingDirectory(m_gitRoot.isEmpty() ? m_folderPath : m_gitRoot);

    if (m_gitBar) {
        m_gitBar->setVisible(!m_gitRoot.isEmpty());
        m_gitStatusLabel->setText(m_gitBranch.isEmpty() ? tr("Git 仓库") : m_gitBranch);
        m_gitStatusLabel->setToolTip(m_gitRoot);
    }
}

void MainWindow::setGitBusy(bool busy)
{
    m_gitBusy = busy;
    const QList<QPushButton *> buttons = {m_gitPullButton, m_gitFetchButton, m_gitPushButton,
                                          m_gitStageButton, m_gitCommitButton};
    for (QPushButton *button : buttons) {
        if (button)
            button->setEnabled(!busy);
    }
}

void MainWindow::enqueueGit(const QStringList &arguments, const QString &label)
{
    GitStep step;
    step.arguments = arguments;
    step.label = label;
    m_gitQueue.enqueue(step);
    if (!m_gitBusy)
        runNextGit();
}

void MainWindow::runNextGit()
{
    if (m_gitQueue.isEmpty()) {
        setGitBusy(false);
        statusBar()->clearMessage();
        return;
    }

    const GitStep step = m_gitQueue.dequeue();
    setGitBusy(true);
    statusBar()->showMessage(tr("正在%1…").arg(step.label));
    m_git->run(step.arguments, step.label);
}

void MainWindow::onGitFinished(bool ok, const QString &label, const QString &output)
{
    if (!ok) {
        m_gitQueue.clear();
        setGitBusy(false);
        statusBar()->clearMessage();
        QMessageBox::warning(this, tr("Git 操作失败"),
                             tr("%1失败：\n\n%2")
                                 .arg(label, output.isEmpty() ? tr("（没有输出）") : output));
        return;
    }

    if (m_gitQueue.isEmpty()) {
        setGitBusy(false);
        statusBar()->showMessage(tr("%1完成").arg(label), 3000);
        refreshFolder();
    } else {
        runNextGit();
    }
}

void MainWindow::gitPull()
{
    if (m_gitRoot.isEmpty())
        return;
    enqueueGit({QStringLiteral("pull"), QStringLiteral("--ff-only")}, tr("拉取"));
}

void MainWindow::gitFetch()
{
    if (m_gitRoot.isEmpty())
        return;
    enqueueGit({QStringLiteral("fetch"), QStringLiteral("--all"), QStringLiteral("--prune")},
               tr("获取"));
}

void MainWindow::gitPush()
{
    if (m_gitRoot.isEmpty())
        return;
    enqueueGit({QStringLiteral("push")}, tr("推送"));
}

void MainWindow::gitStageAll()
{
    if (m_gitRoot.isEmpty())
        return;
    enqueueGit({QStringLiteral("add"), QStringLiteral("-A")}, tr("暂存"));
}

void MainWindow::gitCommit()
{
    if (m_gitRoot.isEmpty())
        return;

    QProcess staged;
    staged.setWorkingDirectory(m_gitRoot);
    staged.start(QStringLiteral("git"), {QStringLiteral("diff"), QStringLiteral("--cached"),
                                         QStringLiteral("--quiet")});
    if (staged.waitForFinished(5000) && staged.exitCode() == 0) {
        QMessageBox::information(this, tr("提交"),
                                 tr("暂存区是空的，先点「暂存」把所有改动加进去。"));
        return;
    }

    bool ok = false;
    const QString message = QInputDialog::getMultiLineText(
        this, tr("提交"), tr("提交说明："), QString(), &ok);
    if (!ok || message.trimmed().isEmpty())
        return;

    enqueueGit({QStringLiteral("commit"), QStringLiteral("-m"), message.trimmed()}, tr("提交"));
}

void MainWindow::gitAddFile(const QString &path)
{
    if (m_gitRoot.isEmpty())
        return;
    const QString relative = QDir(m_gitRoot).relativeFilePath(path);
    enqueueGit({QStringLiteral("add"), QStringLiteral("--"), relative}, tr("添加到提交"));
}

void MainWindow::gitIgnoreFile(const QString &path)
{
    if (m_gitRoot.isEmpty())
        return;

    const QString relative = QDir(m_gitRoot).relativeFilePath(path);
    const QString ignorePath = QDir(m_gitRoot).filePath(QStringLiteral(".gitignore"));

    QStringList lines;
    QFile ignoreFile(ignorePath);
    if (ignoreFile.open(QIODevice::ReadOnly | QIODevice::Text))
        lines = QString::fromUtf8(ignoreFile.readAll()).split(QLatin1Char('\n'));
    ignoreFile.close();

    for (const QString &line : lines) {
        if (line.trimmed() == relative) {
            statusBar()->showMessage(tr("「%1」已经在 .gitignore 里了").arg(relative), 4000);
            return;
        }
    }

    if (!ignoreFile.open(QIODevice::Append | QIODevice::Text)) {
        QMessageBox::warning(this, tr("忽略文件"),
                             tr("打不开 .gitignore：%1").arg(ignoreFile.errorString()));
        return;
    }
    QTextStream stream(&ignoreFile);
    stream << relative << "\n";
    ignoreFile.close();

    statusBar()->showMessage(tr("已把 %1 写进 .gitignore").arg(relative), 4000);
}

void MainWindow::gitRemoveRemote(const QString &path)
{
    if (m_gitRoot.isEmpty())
        return;

    const QString relative = QDir(m_gitRoot).relativeFilePath(path);

    QProcess check;
    check.setWorkingDirectory(m_gitRoot);
    check.start(QStringLiteral("git"), {QStringLiteral("ls-files"), QStringLiteral("--error-unmatch"),
                                        QStringLiteral("--"), relative});
    if (!(check.waitForFinished(5000) && check.exitCode() == 0)) {
        QMessageBox::information(this, tr("删除远端"),
                                 tr("「%1」没有被 git 跟踪，远端本来就没有它。").arg(relative));
        return;
    }

    QMessageBox box(QMessageBox::Warning, tr("删除远端"),
                    tr("从仓库删除「%1」并推送到远端？\n\n"
                       "本地文件会保留，远端和别人 pull 下来的仓库里会删掉它。")
                        .arg(relative),
                    QMessageBox::NoButton, this);
    QPushButton *confirm = box.addButton(tr("删除并推送"), QMessageBox::AcceptRole);
    box.addButton(tr("取消"), QMessageBox::RejectRole);
    box.exec();
    if (box.clickedButton() != confirm)
        return;

    enqueueGit({QStringLiteral("rm"), QStringLiteral("--cached"), QStringLiteral("--"), relative},
               tr("从仓库移除"));
    enqueueGit({QStringLiteral("commit"), QStringLiteral("-m"),
                tr("chore: 从仓库删除 %1").arg(relative)},
               tr("提交"));
    enqueueGit({QStringLiteral("push")}, tr("推送"));
}

void MainWindow::showFileContextMenu(const QPoint &pos)
{
    QListWidgetItem *item = m_fileList->itemAt(pos);
    if (!item)
        return;
    const QString path = item->data(Qt::UserRole).toString();
    if (path.isEmpty())
        return;

    QMenu menu(this);
    menu.addAction(tr("打开"), this, [this, path] { openPath(path); });

    if (!m_gitRoot.isEmpty()) {
        menu.addSeparator();
        QAction *addAction = menu.addAction(tr("添加"), this, [this, path] { gitAddFile(path); });
        addAction->setToolTip(tr("git add 这个文件"));
        QAction *ignoreAction = menu.addAction(tr("忽略"), this, [this, path] { gitIgnoreFile(path); });
        ignoreAction->setToolTip(tr("把路径写进 .gitignore（不改仓库里已跟踪的文件）"));
        QAction *removeAction =
            menu.addAction(tr("删除远端"), this, [this, path] { gitRemoveRemote(path); });
        removeAction->setToolTip(tr("git rm --cached 后提交并推送：远端删除，本地保留"));
    }

    menu.exec(m_fileList->viewport()->mapToGlobal(pos));
}

