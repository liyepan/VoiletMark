#pragma once

#include <QMainWindow>
#include <QStringList>

class QAction;
class QActionGroup;
class QLabel;
class QMenu;
class QTextDocument;
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

    void changeZoom(int direction);
    void resetZoom();
    void setWordWrapEnabled(bool enabled);
    void applyThemeMode(int mode);
    void showAbout();

    void updateStatusBar();
    void updateWindowTitle();
    void updateEditActionStates();

    QString displayName() const;
    QString startDirectory() const;

    MarkdownEditor *m_editor = nullptr;
    QLabel *m_statusLabel = nullptr;

    QString m_currentPath;
    Eol m_eol = Eol::Lf;
    bool m_hasBom = false;
    QStringList m_recentFiles;
    QString m_lastDirectory;

    QAction *m_newAction = nullptr;
    QAction *m_openAction = nullptr;
    QAction *m_saveAction = nullptr;
    QAction *m_saveAsAction = nullptr;
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

    QMenu *m_recentMenu = nullptr;
};
