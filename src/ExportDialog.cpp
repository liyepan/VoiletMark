#include "ExportDialog.h"

#include "MarkdownRenderer.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPageSize>
#include <QPrinter>
#include <QPushButton>
#include <QTextBrowser>
#include <QTextDocument>
#include <QVBoxLayout>

ExportDialog::ExportDialog(const QString &markdown, const QString &documentName,
                           const QString &startDirectory, QWidget *parent)
    : QDialog(parent)
    , m_documentName(documentName)
    , m_startDirectory(startDirectory)
{
    setWindowTitle(tr("导出 PDF — %1")
                       .arg(documentName.isEmpty() ? tr("未命名") : documentName));
    resize(900, 840);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 12);
    layout->setSpacing(10);

    m_document = new QTextDocument(this);
    MarkdownRenderer::render(markdown, m_document);
    // A4 去掉 16mm 页边距后的正文宽度（96dpi 下的像素）：
    // 预览和打印用同一个页宽，看到的就是导出的样子。
    m_document->setPageSize(QSizeF(672.76, 1001.57));

    m_preview = new QTextBrowser(this);
    m_preview->setFrameShape(QFrame::NoFrame);
    m_preview->setOpenExternalLinks(true);
    m_preview->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_preview->setFixedWidth(700);
    m_preview->setDocument(m_document);
    m_preview->setStyleSheet(QStringLiteral(
        "QTextBrowser { background: #ffffff; color: #1f2328;"
        " border: 1px solid #d8dde3; border-radius: 4px; }"));

    QHBoxLayout *center = new QHBoxLayout;
    center->addStretch(1);
    center->addWidget(m_preview);
    center->addStretch(1);
    layout->addLayout(center, 1);

    QLabel *hint = new QLabel(tr("A4 纸、16mm 页边距，内容和预览一致。"), this);
    hint->setForegroundRole(QPalette::PlaceholderText);

    QPushButton *saveButton = new QPushButton(tr("保存 PDF…"), this);
    saveButton->setDefault(true);
    QPushButton *closeButton = new QPushButton(tr("关闭"), this);

    QHBoxLayout *buttons = new QHBoxLayout;
    buttons->addWidget(hint);
    buttons->addStretch(1);
    buttons->addWidget(closeButton);
    buttons->addWidget(saveButton);
    layout->addLayout(buttons);

    connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(saveButton, &QPushButton::clicked, this, &ExportDialog::savePdf);
}

void ExportDialog::savePdf()
{
    QString suggested = m_documentName.isEmpty()
                            ? tr("未命名")
                            : QFileInfo(m_documentName).completeBaseName();
    suggested += QStringLiteral(".pdf");

    QString target = QFileDialog::getSaveFileName(
        this, tr("保存 PDF"), QDir(m_startDirectory).filePath(suggested),
        tr("PDF 文件 (*.pdf)"));
    if (target.isEmpty())
        return;

    if (!target.endsWith(QStringLiteral(".pdf"), Qt::CaseInsensitive))
        target += QStringLiteral(".pdf");

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(target);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageMargins(QMarginsF(16, 16, 16, 16), QPageLayout::Millimeter);
    printer.setDocName(QFileInfo(target).completeBaseName());

    m_document->print(&printer);

    if (!QFileInfo::exists(target) || QFileInfo(target).size() <= 0) {
        QMessageBox::warning(this, tr("导出失败"),
                             tr("没能写出「%1」，换个位置再试一次。")
                                 .arg(QDir::toNativeSeparators(target)));
        return;
    }

    QMessageBox::information(this, tr("导出完成"),
                             tr("已保存到\n%1").arg(QDir::toNativeSeparators(target)));
    accept();
}
