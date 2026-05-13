#pragma once
#include "TranscriptionEngine.h"
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QStringList>


class TranslationEngine : public QObject {
  Q_OBJECT
public:
  explicit TranslationEngine(QObject *parent = nullptr);

  void setEndpoint(const QString &url);
  void setApiKey(const QString &key);

  bool translate(QList<TranscriptSegment> &segments, const QString &sourceLang,
                 const QString &targetLang);

  static QStringList supportedLanguages();
  static QString languageCode(const QString &name);

signals:
  void progress(int percent);
  void logMessage(const QString &msg);

private:
  QString translateText(const QString &text, const QString &src,
                        const QString &tgt);

  QNetworkAccessManager m_net;
  QString m_endpoint = "http://localhost:5000";
  QString m_apiKey;
};
