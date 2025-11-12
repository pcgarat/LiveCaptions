/* deepl-translator.h
 * Implementación concreta de TranslationService para DeepL
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

// Estructura opaca para la implementación de DeepL Translator
typedef struct DeepLTranslator DeepLTranslator;

/**
 * Crea una nueva instancia de DeepL Translator
 * @param settings GSettings con la configuración (API key)
 * @return Nueva instancia o NULL si falla
 */
DeepLTranslator* deepl_translator_create(GSettings *settings);

/**
 * Libera los recursos de DeepL Translator
 * @param translator La instancia a liberar
 */
void deepl_translator_free(DeepLTranslator *translator);

/**
 * Traduce texto usando DeepL API
 * @param service El servicio de traducción (se pasa para compatibilidad)
 * @param text Texto a traducir
 * @param source_lang Idioma origen (ISO 639-1) o "auto"
 * @param target_lang Idioma destino (ISO 639-1)
 * @param callback Callback a invocar cuando la traducción esté lista
 * @param user_data Datos de usuario
 */
void deepl_translator_translate_async(TranslationService *service,
                                       const char *text,
                                       const char *source_lang,
                                       const char *target_lang,
                                       TranslationCallback callback,
                                       void *user_data);

/**
 * Verifica si DeepL Translator está configurado correctamente
 * @param translator La instancia
 * @return TRUE si está configurado, FALSE en caso contrario
 */
gboolean deepl_translator_is_configured(DeepLTranslator *translator);

/**
 * Obtiene el nombre del servicio
 * @param translator La instancia
 * @return "DeepL"
 */
const char* deepl_translator_get_name(DeepLTranslator *translator);

