#include "TranslationEngine.h"
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>


TranslationEngine::TranslationEngine(QObject *parent) : QObject(parent) {}

void TranslationEngine::setEndpoint(const QString &url) { m_endpoint = url; }
void TranslationEngine::setApiKey(const QString &key) { m_apiKey = key; }

bool TranslationEngine::translate(QList<TranscriptSegment> &segments,
                                  const QString &sourceLang,
                                  const QString &targetLang) {
  if (segments.isEmpty())
    return true;
  emit logMessage(QString("Translating %1 segments [%2 -> %3]")
                      .arg(segments.size())
                      .arg(sourceLang)
                      .arg(targetLang));

  for (int i = 0; i < segments.size(); ++i) {
    TranscriptSegment &seg = segments[i];
    QString translated = translateText(seg.text, sourceLang, targetLang);
    if (!translated.isEmpty())
      seg.text = translated;
    emit progress(static_cast<int>(100.0 * i / segments.size()));
  }

  emit progress(100);
  emit logMessage("Translation complete");
  return true;
}

QString TranslationEngine::translateText(const QString &text,
                                         const QString &src,
                                         const QString &tgt) {
  QUrl url(m_endpoint + "/translate");
  QNetworkRequest request(url);
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  if (!m_apiKey.isEmpty())
    request.setRawHeader("Authorization", ("Bearer " + m_apiKey).toUtf8());

  QJsonObject body;
  body["q"] = text;
  body["source"] = src.isEmpty() ? "auto" : src;
  body["target"] = tgt;
  body["format"] = "text";

  QByteArray payload = QJsonDocument(body).toJson();
  QNetworkReply *reply = m_net.post(request, payload);

  QEventLoop loop;
  QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  loop.exec();

  QString result;
  if (reply->error() == QNetworkReply::NoError) {
    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    result = doc.object().value("translatedText").toString();
  } else {
    emit logMessage(QString("Translation error: %1").arg(reply->errorString()));
  }

  reply->deleteLater();
  return result;
}

QStringList TranslationEngine::supportedLanguages() {
  return {"Auto Detect",
          "English",
          "Spanish",
          "French",
          "German",
          "Italian",
          "Portuguese",
          "Russian",
          "Japanese",
          "Chinese (Simplified)",
          "Chinese (Traditional)",
          "Korean",
          "Arabic",
          "Hindi",
          "Dutch",
          "Polish",
          "Turkish",
          "Ukrainian",
          "Vietnamese",
          "Thai",
          "Indonesian",
          "Malay",
          "Swedish",
          "Norwegian",
          "Danish"};
}

QString TranslationEngine::languageCode(const QString &name) {
  static QHash<QString, QString> codes = {{"Auto Detect", "auto"},
                                          {"English", "en"},
                                          {"Spanish", "es"},
                                          {"French", "fr"},
                                          {"German", "de"},
                                          {"Italian", "it"},
                                          {"Portuguese", "pt"},
                                          {"Russian", "ru"},
                                          {"Japanese", "ja"},
                                          {"Chinese (Simplified)", "zh"},
                                          {"Chinese (Traditional)", "zh"},
                                          {"Korean", "ko"},
                                          {"Arabic", "ar"},
                                          {"Hindi", "hi"},
                                          {"Dutch", "nl"},
                                          {"Polish", "pl"},
                                          {"Turkish", "tr"},
                                          {"Ukrainian", "uk"},
                                          {"Vietnamese", "vi"},
                                          {"Thai", "th"},
                                          {"Indonesian", "id"},
                                          {"Malay", "ms"},
                                          {"Swedish", "sv"},
                                          {"Norwegian", "no"},
                                          {"Danish", "da"}};
  return codes.value(name, "en");
}
