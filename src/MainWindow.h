#pragma once

#include <QMainWindow>
#include <QQueue>
#include <QStringList>

class QAction;
class QActionGroup;
class QDockWidget;
class QLabel;
class QPushButton;
class QStackedWidget;
class QToolButton;
class GitRunner;
class QListWidget;
class QListWidgetItem;
class QMenu;
class QTextDocument;
class QToolButton;
class MarkdownEditor;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    bool openPath(const QString &path);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    enum class Eol { Lf, Crlf };

    void createActions();
    void createSidebar();
    void createMenus();
    void createStatusBar();
    void loadSettings();
    void saveSettings();

    void newFile();
    void openFile();
    bool saveFile();
    bool saveFileAs();
    bool maybeSave();
    bool writeToFile(const QString &path);
    void setCurrentFile(const QString &path);
    void showWelcomeDocument();

    void addRecentFile(const QString &path);
    void refreshRecentMenu();
    void loadRecentFiles();

    void chooseFolder();
    void openFolder(const QString &path);
    void refreshFolder();
    void openSidebarItem(QListWidgetItem *item);
    void selectInSidebar(const QString &path);
    QStringList scanFolder() const;
    void setSourceModeEnabled(bool enabled);
    void exportPdf();
    void logExport(const QString &message);

    // ---- Git ----
    void updateGitState();
    void showFileContextMenu(const QPoint &pos);
    void gitAddFile(const QString &path);
    void gitIgnoreFile(const QString &path);
    void gitRemoveRemote(const QString &path);
    void gitCommit();

    void switchSidebarMode(int mode);
    void rebuildToc();
    void goToTocItem(QListWidgetItem *item);
    void gitPull();
    void gitFetch();
    void gitPush();
    void gitStageAll();
    void enqueueGit(const QStringList &arguments, const QString &label);
    void runNextGit();
    void onGitFinished(bool ok, const QString &label, const QString &output);
    void setGitBusy(bool busy);

    void changeZoom(int direction);
    void resetZoom();
    void setWordWrapEnabled(bool enabled);
    void applyThemeMode(int mode);
    void applyLanguage(const QString &code);
    void retranslateUi();
    void showAbout();

    void updateStatusBar();
    void updateWindowTitle();
    void updateEditActionStates();

    QString displayName() const;
    QString startDirectory() const;

    MarkdownEditor *m_editor = nullptr;
    QLabel *m_statusLabel = nullptr;

    QDockWidget *m_sidebar = nullptr;
    QListWidget *m_fileList = nullptr;
    QLabel *m_folderLabel = nullptr;
    QToolButton *m_sourceButton = nullptr;
    QToolButton *m_sidebarButton = nullptr;

    QStackedWidget *m_sidebarStack = nullptr;
    QToolButton *m_fileModeButton = nullptr;
    QToolButton *m_tocModeButton = nullptr;
    QListWidget *m_tocList = nullptr;
    QTimer *m_tocTimer = nullptr;
    int m_sidebarMode = 0;

    GitRunner *m_git = nullptr;
    QWidget *m_gitBar = nullptr;
    QLabel *m_gitStatusLabel = nullptr;
    QPushButton *m_gitPullButton = nullptr;
    QPushButton *m_gitFetchButton = nullptr;
    QPushButton *m_gitPushButton = nullptr;
    QPushButton *m_gitStageButton = nullptr;
    QPushButton *m_gitCommitButton = nullptr;
    QString m_gitRoot;
    QString m_gitBranch;

    struct GitStep {
        QStringList arguments;
        QString label;
    };
    QQueue<GitStep> m_gitQueue;
    bool m_gitBusy = false;
    QTimer *m_folderRefreshTimer = nullptr;
    QString m_folderPath;
    QString m_displayedFolder;

    QString m_currentPath;
    Eol m_eol = Eol::Lf;
    bool m_hasBom = false;
    QStringList m_recentFiles;
    QString m_lastDirectory;

    QAction *m_newAction = nullptr;
    QAction *m_openAction = nullptr;
    QAction *m_saveAction = nullptr;
    QAction *m_saveAsAction = nullptr;
    QAction *m_exportPdfAction = nullptr;
    QAction *m_quitAction = nullptr;

    QAction *m_undoAction = nullptr;
    QAction *m_redoAction = nullptr;
    QAction *m_cutAction = nullptr;
    QAction *m_copyAction = nullptr;
    QAction *m_pasteAction = nullptr;
    QAction *m_selectAllAction = nullptr;

    QAction *m_wrapAction = nullptr;
    QAction *m_zoomInAction = nullptr;
    QAction *m_zoomOutAction = nullptr;
    QAction *m_zoomResetAction = nullptr;

    QAction *m_themeSystemAction = nullptr;
    QAction *m_themeLightAction = nullptr;
    QAction *m_themeDarkAction = nullptr;
    QAction *m_themeScheduleAction = nullptr;

    QAction *m_openFolderAction = nullptr;
    QAction *m_sidebarAction = nullptr;
    QAction *m_sourceModeAction = nullptr;
    QAction *m_codeBlockAction = nullptr;
    QAction *m_headingActions[6] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};

    QMenu *m_recentMenu = nullptr;
    QMenu *m_fileMenu = nullptr;
    QMenu *m_editMenu = nullptr;
    QMenu *m_viewMenu = nullptr;
    QMenu *m_formatMenu = nullptr;
    QMenu *m_helpMenu = nullptr;
    QMenu *m_themeMenu = nullptr;
    QMenu *m_headingMenu = nullptr;
    QMenu *m_languageMenu = nullptr;

    QAction *m_aboutAction = nullptr;
    QAction *m_languageChineseAction = nullptr;
    QAction *m_languageEnglishAction = nullptr;

    QString m_welcomeText;
};
