#pragma once

#include <QDialog>

class QTextBrowser;
class QTextDocument;

/// “导出 PDF”窗口：上方是排版后的预览，下方保存为 PDF。
class ExportDialog : public QDialog
{
    Q_OBJECT

public:
    ExportDialog(const QString &markdown, const QString &documentName,
                 const QString &startDirectory, QWidget *parent = nullptr);

private:
    void savePdf();

    QTextBrowser *m_preview = nullptr;
    QTextDocument *m_document = nullptr;
    QString m_documentName;
    QString m_startDirectory;
};
