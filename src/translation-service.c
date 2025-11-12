/* translation-service.c
 * Implementación base y factory para servicios de traducción
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

#include "translation-service.h"
#include "amazon-translator.h"
#include "google-translator.h"
#include "microsoft-translator.h"
#include "deepl-translator.h"

struct TranslationService {
    GSettings *settings;
    char *service_type;
    void *impl_data; // Datos específicos de la implementación
    void (*free_impl)(void *impl_data);
    void (*translate_impl)(TranslationService *service,
                          const char *text,
                          const char *source_lang,
                          const char *target_lang,
                          TranslationCallback callback,
                          void *user_data);
    gboolean (*is_configured_impl)(void *impl_data);
    const char* (*get_name_impl)(void *impl_data);
};

// Funciones wrapper para hacer cast de tipos específicos a void*
static void amazon_translator_free_wrapper(void *impl_data) {
    amazon_translator_free((AmazonTranslator*)impl_data);
}

static gboolean amazon_translator_is_configured_wrapper(void *impl_data) {
    return amazon_translator_is_configured((AmazonTranslator*)impl_data);
}

static const char* amazon_translator_get_name_wrapper(void *impl_data) {
    return amazon_translator_get_name((AmazonTranslator*)impl_data);
}

static void google_translator_free_wrapper(void *impl_data) {
    google_translator_free((GoogleTranslator*)impl_data);
}

static gboolean google_translator_is_configured_wrapper(void *impl_data) {
    return google_translator_is_configured((GoogleTranslator*)impl_data);
}

static const char* google_translator_get_name_wrapper(void *impl_data) {
    return google_translator_get_name((GoogleTranslator*)impl_data);
}

static void microsoft_translator_free_wrapper(void *impl_data) {
    microsoft_translator_free((MicrosoftTranslator*)impl_data);
}

static gboolean microsoft_translator_is_configured_wrapper(void *impl_data) {
    return microsoft_translator_is_configured((MicrosoftTranslator*)impl_data);
}

static const char* microsoft_translator_get_name_wrapper(void *impl_data) {
    return microsoft_translator_get_name((MicrosoftTranslator*)impl_data);
}

static void deepl_translator_free_wrapper(void *impl_data) {
    deepl_translator_free((DeepLTranslator*)impl_data);
}

static gboolean deepl_translator_is_configured_wrapper(void *impl_data) {
    return deepl_translator_is_configured((DeepLTranslator*)impl_data);
}

static const char* deepl_translator_get_name_wrapper(void *impl_data) {
    return deepl_translator_get_name((DeepLTranslator*)impl_data);
}

TranslationService* translation_service_create(const char *service_type,
                                               GSettings *settings)
{
    if (service_type == NULL || settings == NULL) {
        return NULL;
    }

    TranslationService *service = g_new0(TranslationService, 1);
    service->settings = g_object_ref(settings);
    service->service_type = g_strdup(service_type);

    // Factory: crear implementación según el tipo
    if (g_str_equal(service_type, "amazon")) {
        service->impl_data = amazon_translator_create(settings);
        if (service->impl_data == NULL) {
            translation_service_free(service);
            return NULL;
        }
        service->free_impl = amazon_translator_free_wrapper;
        service->translate_impl = amazon_translator_translate_async;
        service->is_configured_impl = amazon_translator_is_configured_wrapper;
        service->get_name_impl = amazon_translator_get_name_wrapper;
    } else if (g_str_equal(service_type, "google")) {
        service->impl_data = google_translator_create(settings);
        if (service->impl_data == NULL) {
            translation_service_free(service);
            return NULL;
        }
        service->free_impl = google_translator_free_wrapper;
        service->translate_impl = google_translator_translate_async;
        service->is_configured_impl = google_translator_is_configured_wrapper;
        service->get_name_impl = google_translator_get_name_wrapper;
    } else if (g_str_equal(service_type, "microsoft")) {
        service->impl_data = microsoft_translator_create(settings);
        if (service->impl_data == NULL) {
            translation_service_free(service);
            return NULL;
        }
        service->free_impl = microsoft_translator_free_wrapper;
        service->translate_impl = microsoft_translator_translate_async;
        service->is_configured_impl = microsoft_translator_is_configured_wrapper;
        service->get_name_impl = microsoft_translator_get_name_wrapper;
    } else if (g_str_equal(service_type, "deepl")) {
        service->impl_data = deepl_translator_create(settings);
        if (service->impl_data == NULL) {
            translation_service_free(service);
            return NULL;
        }
        service->free_impl = deepl_translator_free_wrapper;
        service->translate_impl = deepl_translator_translate_async;
        service->is_configured_impl = deepl_translator_is_configured_wrapper;
        service->get_name_impl = deepl_translator_get_name_wrapper;
    } else {
        // Tipo de servicio no soportado
        g_free(service->service_type);
        g_object_unref(service->settings);
        g_free(service);
        return NULL;
    }

    return service;
}

void translation_service_free(TranslationService *service)
{
    if (service == NULL) {
        return;
    }

    if (service->free_impl && service->impl_data) {
        service->free_impl(service->impl_data);
    }

    g_free(service->service_type);
    g_object_unref(service->settings);
    g_free(service);
}

void translation_service_translate_async(TranslationService *service,
                                         const char *text,
                                         const char *source_lang,
                                         const char *target_lang,
                                         TranslationCallback callback,
                                         void *user_data)
{
    if (service == NULL || service->translate_impl == NULL) {
        if (callback) {
            GError *error = g_error_new_literal(G_IO_ERROR,
                                                G_IO_ERROR_FAILED,
                                                "Servicio de traducción no válido");
            callback(NULL, error, user_data);
            g_error_free(error);
        }
        return;
    }

    service->translate_impl(service, text, source_lang, target_lang,
                            callback, user_data);
}

gboolean translation_service_is_configured(TranslationService *service)
{
    if (service == NULL || service->is_configured_impl == NULL) {
        return FALSE;
    }

    return service->is_configured_impl(service->impl_data);
}

const char* translation_service_get_name(TranslationService *service)
{
    if (service == NULL || service->get_name_impl == NULL) {
        return "Unknown";
    }

    return service->get_name_impl(service->impl_data);
}

