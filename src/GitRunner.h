#pragma once

#include <QObject>
#include <QStringList>

class QProcess;
class QTimer;

/// 异步执行 git 命令，避免 pull / push 这种网络操作把界面卡住。
class GitRunner : public QObject
{
    Q_OBJECT

public:
    explicit GitRunner(QObject *parent = nullptr);

    void setWorkingDirectory(const QString &directory);
    bool isBusy() const;

    void run(const QStringList &arguments, const QString &label);

signals:
    void started(const QString &label);
    void finished(bool ok, const QString &label, const QString &output);

private:
    void report(bool ok);

    QString m_directory;
    QProcess *m_process = nullptr;
    QTimer *m_timeout = nullptr;
    QString m_label;
    QString m_output;
    bool m_reported = false;
};
