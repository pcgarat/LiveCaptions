/* translation-service.h
 * Interfaz abstracta para servicios de traducción siguiendo el principio
 * abierto/cerrado. Permite añadir nuevos servicios sin modificar código existente.
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

typedef struct TranslationService TranslationService;

/**
 * Callback que se invoca cuando la traducción está lista
 * @param translated_text El texto traducido (o NULL en caso de error)
 * @param error GError con información del error si ocurrió alguno
 * @param user_data Datos de usuario pasados a translate_async
 */
typedef void (*TranslationCallback)(const char *translated_text,
                                    GError *error,
                                    void *user_data);

/**
 * Crea una instancia de un servicio de traducción según el tipo especificado
 * @param service_type Tipo de servicio ("amazon", "google", etc.)
 * @param settings GSettings con la configuración del servicio
 * @return Nueva instancia de TranslationService o NULL si el tipo no es válido
 */
TranslationService* translation_service_create(const char *service_type,
                                               GSettings *settings);

/**
 * Libera los recursos de un servicio de traducción
 * @param service El servicio a liberar
 */
void translation_service_free(TranslationService *service);

/**
 * Traduce texto de forma asíncrona
 * @param service El servicio de traducción a usar
 * @param text El texto a traducir
 * @param source_lang Código de idioma origen (ISO 639-1) o "auto" para auto-detección
 * @param target_lang Código de idioma destino (ISO 639-1)
 * @param callback Función a llamar cuando la traducción esté lista
 * @param user_data Datos de usuario a pasar al callback
 */
void translation_service_translate_async(TranslationService *service,
                                         const char *text,
                                         const char *source_lang,
                                         const char *target_lang,
                                         TranslationCallback callback,
                                         void *user_data);

/**
 * Verifica si el servicio está configurado correctamente
 * @param service El servicio a verificar
 * @return TRUE si está configurado, FALSE en caso contrario
 */
gboolean translation_service_is_configured(TranslationService *service);

/**
 * Obtiene el nombre del servicio
 * @param service El servicio
 * @return Nombre del servicio (no debe ser liberado)
 */
const char* translation_service_get_name(TranslationService *service);

