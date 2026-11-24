/* Photo Snap is licensed under the GNU General Public License, version 3 or later. */
#pragma once

#include <QTranslator>

QStringList supportedLanguages();
QString languageName(const QString &language);
QString normalizedLanguage(const QString &language);

class AppTranslator final : public QTranslator
{
public:
    explicit AppTranslator(QString language, QObject *parent = nullptr);
    QString translate(const char *context, const char *sourceText,
                      const char *disambiguation = nullptr, int n = -1) const override;

private:
    QString m_language;
};
