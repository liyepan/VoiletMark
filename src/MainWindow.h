#pragma once

#include <QMainWindow>
#include <QStringList>

class QAction;
class QActionGroup;
class QDockWidget;
class QLabel;
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
