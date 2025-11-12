/* microsoft-translator.h
 * Implementación concreta de TranslationService para Microsoft Translator
 *
 * Copyright 2024
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <glib.h>
#include <gio/gio.h>
#include "translation-service.h"

// Estructura opaca para la implementación de Microsoft Translator
typedef struct MicrosoftTranslator MicrosoftTranslator;

/**
 * Crea una nueva instancia de Microsoft Translator
 * @param settings GSettings con la configuración (subscription key, región)
 * @return Nueva instancia o NULL si falla
 */
MicrosoftTranslator* microsoft_translator_create(GSettings *settings);

/**
 * Libera los recursos de Microsoft Translator
 * @param translator La instancia a liberar
 */
void microsoft_translator_free(MicrosoftTranslator *translator);

/**
 * Traduce texto usando Microsoft Translator API
 * @param service El servicio de traducción (se pasa para compatibilidad)
 * @param text Texto a traducir
 * @param source_lang Idioma origen (ISO 639-1) o "auto"
 * @param target_lang Idioma destino (ISO 639-1)
 * @param callback Callback a invocar cuando la traducción esté lista
 * @param user_data Datos de usuario
 */
void microsoft_translator_translate_async(TranslationService *service,
                                          const char *text,
                                          const char *source_lang,
                                          const char *target_lang,
                                          TranslationCallback callback,
                                          void *user_data);

/**
 * Verifica si Microsoft Translator está configurado correctamente
 * @param translator La instancia
 * @return TRUE si está configurado, FALSE en caso contrario
 */
gboolean microsoft_translator_is_configured(MicrosoftTranslator *translator);

/**
 * Obtiene el nombre del servicio
 * @param translator La instancia
 * @return "Microsoft Translator"
 */
const char* microsoft_translator_get_name(MicrosoftTranslator *translator);

