#ifndef TRANSLATIONUTILS_H
#define TRANSLATIONUTILS_H

#include <QString>

/**
 * @brief Загружает и применяет переводы для приложения
 * @param languageCode Код языка: "ru", "en" или пустая строка для системного
 * @return true если переводы успешно загружены
 */
bool loadApplicationTranslation(const QString &languageCode);

#endif // TRANSLATIONUTILS_H

