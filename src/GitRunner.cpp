#include "GitRunner.h"

#include <QProcess>
#include <QTimer>

GitRunner::GitRunner(QObject *parent)
    : QObject(parent)
    , m_process(new QProcess(this))
    , m_timeout(new QTimer(this))
{
    m_process->setProcessChannelMode(QProcess::MergedChannels);

    connect(m_process, &QProcess::readyReadStandardOutput, this, [this] {
        m_output += QString::fromLocal8Bit(m_process->readAllStandardOutput());
    });

    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this](int code, QProcess::ExitStatus status) {
                report(code == 0 && status == QProcess::NormalExit);
            });

    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart)
            report(false);
    });

    m_timeout->setSingleShot(true);
    m_timeout->setInterval(90000);
    connect(m_timeout, &QTimer::timeout, this, [this] {
        if (m_process->state() == QProcess::NotRunning)
            return;
        m_output += tr("\n（超时，已中止）");
        m_process->kill();
    });
}

void GitRunner::setWorkingDirectory(const QString &directory)
{
    m_directory = directory;
}

bool GitRunner::isBusy() const
{
    return m_process->state() != QProcess::NotRunning;
}

void GitRunner::run(const QStringList &arguments, const QString &label)
{
    if (isBusy())
        return;

    m_label = label;
    m_output.clear();
    m_reported = false;

    m_process->setWorkingDirectory(m_directory);
    m_process->start(QStringLiteral("git"), arguments);
    m_timeout->start();

    emit started(label);
}

void GitRunner::report(bool ok)
{
    if (m_reported)
        return;
    m_reported = true;
    m_timeout->stop();
    emit finished(ok, m_label, m_output.trimmed());
}
