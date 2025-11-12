/* google-translator.c
 * Implementación de Google Translate API
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

#include <string.h>
#include <glib.h>
#include <gio/gio.h>
#include <libsoup/soup.h>
#include <libsoup/soup-message.h>
#include <libsoup/soup-session.h>
#include <json-glib/json-glib.h>

#include "google-translator.h"
#include "translation-service.h"

// Definición de TranslationService para acceso a impl_data
// (normalmente está en translation-service.c, pero la necesitamos aquí para acceder a impl_data)
struct TranslationService {
    GSettings *settings;
    char *service_type;
    void *impl_data;
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

struct GoogleTranslator {
    GSettings *settings;
    SoupSession *session;
    char *api_key;
};

// Estructura para pasar datos al callback asíncrono
typedef struct {
    TranslationCallback callback;
    void *user_data;
    char *text;
    SoupMessage *msg; // Guardar referencia al mensaje
} TranslateData;

static void translate_data_free(TranslateData *data)
{
    if (data) {
        g_free(data->text);
        if (data->msg) {
            g_object_unref(data->msg);
        }
        g_free(data);
    }
}

// Callback para cuando la petición HTTP completa
static void translate_response_callback(GObject *source_object,
                                        GAsyncResult *result,
                                        gpointer user_data)
{
    TranslateData *data = (TranslateData*)user_data;
    GError *error = NULL;
    char *translated_text = NULL;
    GBytes *response_body = NULL;
    SoupMessage *msg = data->msg;

    response_body = soup_session_send_and_read_finish(SOUP_SESSION(source_object), result, &error);
    
    if (error) {
        goto cleanup;
    }

    // Verificar status code
    if (msg && !SOUP_STATUS_IS_SUCCESSFUL(soup_message_get_status(msg))) {
        error = g_error_new(G_IO_ERROR,
                           G_IO_ERROR_FAILED,
                           "Error de traducción: %s (%d)",
                           soup_status_get_phrase(soup_message_get_status(msg)),
                           soup_message_get_status(msg));
        goto cleanup;
    }

    // Parsear respuesta JSON
    gsize body_size;
    const guchar *body_data = g_bytes_get_data(response_body, &body_size);
    
    JsonParser *parser = json_parser_new();
    if (!json_parser_load_from_data(parser,
                                    (const gchar*)body_data,
                                    body_size,
                                    &error)) {
        goto cleanup;
    }

    JsonNode *root = json_parser_get_root(parser);
    JsonReader *reader = json_reader_new(root);

    // Google Translate API v2 formato:
    // {
    //   "data": {
    //     "translations": [
    //       {
    //         "translatedText": "..."
    //       }
    //     ]
    //   }
    // }
    if (json_reader_read_member(reader, "data")) {
        if (json_reader_read_member(reader, "translations")) {
            if (json_reader_read_element(reader, 0)) {
                if (json_reader_read_member(reader, "translatedText")) {
                    translated_text = g_strdup(json_reader_get_string_value(reader));
                    json_reader_end_member(reader);
                }
                json_reader_end_element(reader);
            }
            json_reader_end_member(reader);
        }
        json_reader_end_member(reader);
    } else {
        // Verificar si hay error en la respuesta
        if (json_reader_read_member(reader, "error")) {
            gchar *error_message = NULL;
            if (json_reader_read_member(reader, "message")) {
                error_message = g_strdup(json_reader_get_string_value(reader));
                json_reader_end_member(reader);
            }
            json_reader_end_member(reader);
            
            error = g_error_new(G_IO_ERROR,
                               G_IO_ERROR_FAILED,
                               "Error de Google Translate: %s",
                               error_message ? error_message : "Error desconocido");
            g_free(error_message);
        } else {
            error = g_error_new_literal(G_IO_ERROR,
                                        G_IO_ERROR_FAILED,
                                        "Respuesta de traducción inválida");
        }
    }

    g_object_unref(reader);
    g_object_unref(parser);

cleanup:
    if (data->callback) {
        data->callback(translated_text, error, data->user_data);
    }

    if (error) {
        g_error_free(error);
    }
    if (translated_text) {
        g_free(translated_text);
    }
    if (response_body) {
        g_bytes_unref(response_body);
    }

    translate_data_free(data);
}

GoogleTranslator* google_translator_create(GSettings *settings)
{
    if (settings == NULL) {
        return NULL;
    }

    GoogleTranslator *translator = g_new0(GoogleTranslator, 1);
    translator->settings = g_object_ref(settings);

    // Obtener API key de configuración
    gchar *raw_api_key = g_settings_get_string(settings, "google-api-key");
    if (raw_api_key != NULL && strlen(raw_api_key) > 0) {
        // Hacer una copia y limpiar: eliminar espacios en blanco y saltos de línea
        translator->api_key = g_strdup(raw_api_key);
        g_strstrip(translator->api_key);
        // Eliminar cualquier salto de línea o retorno de carro
        for (gchar *p = translator->api_key; *p; p++) {
            if (*p == '\n' || *p == '\r') {
                *p = '\0';
                break;
            }
        }
        // Si después de limpiar está vacío, liberar
        if (strlen(translator->api_key) == 0) {
            g_free(translator->api_key);
            translator->api_key = NULL;
        }
        g_free(raw_api_key);
    } else {
        translator->api_key = NULL;
        if (raw_api_key != NULL) {
            g_free(raw_api_key);
        }
    }

    // Crear sesión HTTP
    translator->session = soup_session_new();

    return translator;
}

void google_translator_free(GoogleTranslator *translator)
{
    if (translator == NULL) {
        return;
    }

    if (translator->session) {
        g_object_unref(translator->session);
    }

    g_free(translator->api_key);
    g_object_unref(translator->settings);
    g_free(translator);
}

void google_translator_translate_async(TranslationService *service,
                                       const char *text,
                                       const char *source_lang,
                                       const char *target_lang,
                                       TranslationCallback callback,
                                       void *user_data)
{
    if (service == NULL || text == NULL || target_lang == NULL) {
        if (callback) {
            GError *error = g_error_new_literal(G_IO_ERROR,
                                                G_IO_ERROR_INVALID_ARGUMENT,
                                                "Parámetros inválidos");
            callback(NULL, error, user_data);
            g_error_free(error);
        }
        return;
    }

    // Obtener la implementación de Google Translator
    // La estructura TranslationService está definida arriba cuando TEST_MODE está definido
    // o en translation-service.c cuando se compila normalmente
    struct TranslationService *service_impl = (struct TranslationService*)service;
    GoogleTranslator *translator = (GoogleTranslator*)service_impl->impl_data;
    if (translator == NULL) {
        if (callback) {
            GError *error = g_error_new_literal(G_IO_ERROR,
                                                G_IO_ERROR_FAILED,
                                                "Servicio no inicializado");
            callback(NULL, error, user_data);
            g_error_free(error);
        }
        return;
    }

    // Verificar configuración
    if (!google_translator_is_configured(translator)) {
        if (callback) {
            GError *error = g_error_new_literal(G_IO_ERROR,
                                                G_IO_ERROR_FAILED,
                                                "Google Translator no está configurado correctamente");
            callback(NULL, error, user_data);
            g_error_free(error);
        }
        return;
    }

    // Construir URL con API key
    char *url = g_strdup_printf("https://translation.googleapis.com/language/translate/v2?key=%s",
                                translator->api_key);

    // Crear mensaje HTTP POST
    SoupMessage *msg = soup_message_new("POST", url);
    g_free(url);

    // Preparar datos para el callback
    TranslateData *data = g_new0(TranslateData, 1);
    data->callback = callback;
    data->user_data = user_data;
    data->text = g_strdup(text);
    data->msg = g_object_ref(msg); // Guardar referencia al mensaje

    // Construir payload JSON
    JsonBuilder *builder = json_builder_new();
    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "q");
    json_builder_add_string_value(builder, text);
    json_builder_set_member_name(builder, "target");
    json_builder_add_string_value(builder, target_lang);
    if (source_lang && g_strcmp0(source_lang, "auto") != 0) {
        json_builder_set_member_name(builder, "source");
        json_builder_add_string_value(builder, source_lang);
    }
    json_builder_set_member_name(builder, "format");
    json_builder_add_string_value(builder, "text");
    json_builder_end_object(builder);

    JsonGenerator *gen = json_generator_new();
    JsonNode *root = json_builder_get_root(builder);
    json_generator_set_root(gen, root);
    char *payload = json_generator_to_data(gen, NULL);

    // Establecer headers y body
    GBytes *body = g_bytes_new(payload, strlen(payload));
    soup_message_set_request_body_from_bytes(msg, "application/json", body);
    g_bytes_unref(body);

    // Enviar petición asíncrona
    soup_session_send_and_read_async(translator->session,
                                     msg,
                                     G_PRIORITY_DEFAULT,
                                     NULL,
                                     translate_response_callback,
                                     data);

    // Limpiar
    g_free(payload);
    json_node_free(root);
    g_object_unref(gen);
    g_object_unref(builder);
    g_object_unref(msg); // Liberar referencia (data->msg mantiene otra referencia)
}

gboolean google_translator_is_configured(GoogleTranslator *translator)
{
    if (translator == NULL) {
        return FALSE;
    }

    // Verificar que tenemos API key
    if (translator->api_key == NULL || strlen(translator->api_key) == 0) {
        return FALSE;
    }

    return TRUE;
}

const char* google_translator_get_name(GoogleTranslator *translator)
{
    (void)translator; // No usado
    return "Google Translate";
}

