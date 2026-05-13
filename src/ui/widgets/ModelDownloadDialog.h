#pragma once
#include <QDialog>
#include <QFile>
#include <QLabel>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProgressBar>
#include <QPushButton>
#include <QTableWidget>


struct ModelInfo {
  QString id;
  QString displayName;
  QString filename;
  QString description;
  qint64 sizeBytes;
  int ramRequiredMB;
  QString tier;
  bool multilingual;
  QString downloadUrl;
};

class ModelDownloadDialog : public QDialog {
  Q_OBJECT
public:
  explicit ModelDownloadDialog(const QString &modelsDir,
                               QWidget *parent = nullptr);

  QString lastDownloadedPath() const;

private slots:
  void onSelectionChanged();
  void onDownloadClicked();
  void onDownloadProgress(qint64 received, qint64 total);
  void onDownloadFinished();
  void onCancelDownload();

private:
  void buildCatalog();
  void populateTable();
  void highlightRecommended();
  int detectSystemRamMB() const;
  bool hasGpu() const;
  QString recommendedModelId() const;
  void setDownloading(bool active);
  QString formatSize(qint64 bytes) const;
  QString modelsDir() const;

  QString m_modelsDir;
  QString m_lastDownloaded;
  QList<ModelInfo> m_catalog;
  QString m_recommendedId;
  int m_selectedRow = -1;

  QLabel *m_systemInfoLabel;
  QLabel *m_recommendBadge;
  QTableWidget *m_table;
  QLabel *m_descLabel;
  QProgressBar *m_progressBar;
  QLabel *m_progressLabel;
  QPushButton *m_downloadBtn;
  QPushButton *m_cancelBtn;
  QPushButton *m_closeBtn;

  QNetworkAccessManager m_net;
  QNetworkReply *m_reply = nullptr;
  QFile m_outFile;
  ModelInfo m_downloading;
};
