/* amazon-translator.c
 * Implementación de Amazon Translate API
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
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <time.h>

#include "amazon-translator.h"
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

struct AmazonTranslator {
    GSettings *settings;
    SoupSession *session;
    char *access_key;
    char *secret_key;
    char *region;
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

// Genera la firma AWS Signature Version 4 (versión simplificada)
static char* generate_aws_signature(const char *secret_key,
                                     const char *region,
                                     const char *service,
                                     const char *method,
                                     const char *uri,
                                     const char *query_string,
                                     const char *payload,
                                     const char *date_stamp,
                                     const char *amz_date)
{
    // Esta es una implementación simplificada
    // Para producción, se debería usar una librería completa de AWS SDK
    // Por ahora, usaremos un enfoque más simple con credenciales temporales
    
    // Nota: La implementación completa de AWS Signature V4 es compleja
    // Para esta versión inicial, asumiremos que el usuario puede usar
    // credenciales IAM o un enfoque simplificado
    
    return NULL; // Placeholder - se implementará con librería AWS o enfoque alternativo
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

    if (json_reader_read_member(reader, "TranslatedText")) {
        translated_text = g_strdup(json_reader_get_string_value(reader));
        json_reader_end_member(reader);
    } else {
        error = g_error_new_literal(G_IO_ERROR,
                                    G_IO_ERROR_FAILED,
                                    "Respuesta de traducción inválida");
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

AmazonTranslator* amazon_translator_create(GSettings *settings)
{
    if (settings == NULL) {
        return NULL;
    }

    AmazonTranslator *translator = g_new0(AmazonTranslator, 1);
    translator->settings = g_object_ref(settings);

    // Obtener credenciales de configuración
    gchar *raw_access_key = g_settings_get_string(settings, "amazon-access-key");
    if (raw_access_key != NULL && strlen(raw_access_key) > 0) {
        translator->access_key = g_strdup(raw_access_key);
        g_strstrip(translator->access_key);
        for (gchar *p = translator->access_key; *p; p++) {
            if (*p == '\n' || *p == '\r') {
                *p = '\0';
                break;
            }
        }
        if (strlen(translator->access_key) == 0) {
            g_free(translator->access_key);
            translator->access_key = NULL;
        }
        g_free(raw_access_key);
    } else {
        translator->access_key = NULL;
        if (raw_access_key != NULL) {
            g_free(raw_access_key);
        }
    }
    
    gchar *raw_secret_key = g_settings_get_string(settings, "amazon-secret-key");
    if (raw_secret_key != NULL && strlen(raw_secret_key) > 0) {
        translator->secret_key = g_strdup(raw_secret_key);
        g_strstrip(translator->secret_key);
        for (gchar *p = translator->secret_key; *p; p++) {
            if (*p == '\n' || *p == '\r') {
                *p = '\0';
                break;
            }
        }
        if (strlen(translator->secret_key) == 0) {
            g_free(translator->secret_key);
            translator->secret_key = NULL;
        }
        g_free(raw_secret_key);
    } else {
        translator->secret_key = NULL;
        if (raw_secret_key != NULL) {
            g_free(raw_secret_key);
        }
    }
    
    gchar *raw_region = g_settings_get_string(settings, "amazon-region");
    if (raw_region != NULL && strlen(raw_region) > 0) {
        translator->region = g_strdup(raw_region);
        g_strstrip(translator->region);
        for (gchar *p = translator->region; *p; p++) {
            if (*p == '\n' || *p == '\r') {
                *p = '\0';
                break;
            }
        }
        g_free(raw_region);
    } else {
        translator->region = NULL;
    }

    // Si no hay región configurada, usar default
    if (translator->region == NULL || strlen(translator->region) == 0) {
        if (translator->region != NULL) {
            g_free(translator->region);
        }
        translator->region = g_strdup("us-east-1");
    }

    // Crear sesión HTTP
    translator->session = soup_session_new();

    return translator;
}

void amazon_translator_free(AmazonTranslator *translator)
{
    if (translator == NULL) {
        return;
    }

    if (translator->session) {
        g_object_unref(translator->session);
    }

    g_free(translator->access_key);
    g_free(translator->secret_key);
    g_free(translator->region);
    g_object_unref(translator->settings);
    g_free(translator);
}

void amazon_translator_translate_async(TranslationService *service,
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

    // Obtener la implementación de Amazon Translator
    // La estructura TranslationService está definida arriba cuando TEST_MODE está definido
    // o en translation-service.c cuando se compila normalmente
    struct TranslationService *service_impl = (struct TranslationService*)service;
    AmazonTranslator *translator = (AmazonTranslator*)service_impl->impl_data;
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
    if (!amazon_translator_is_configured(translator)) {
        if (callback) {
            GError *error = g_error_new_literal(G_IO_ERROR,
                                                G_IO_ERROR_FAILED,
                                                "Amazon Translator no está configurado correctamente");
            callback(NULL, error, user_data);
            g_error_free(error);
        }
        return;
    }

    // Construir URL
    char *url = g_strdup_printf("https://translate.%s.amazonaws.com/",
                                translator->region);

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
    json_builder_set_member_name(builder, "Text");
    json_builder_add_string_value(builder, text);
    json_builder_set_member_name(builder, "SourceLanguageCode");
    if (source_lang && g_strcmp0(source_lang, "auto") != 0) {
        json_builder_add_string_value(builder, source_lang);
    } else {
        json_builder_add_string_value(builder, "auto");
    }
    json_builder_set_member_name(builder, "TargetLanguageCode");
    json_builder_add_string_value(builder, target_lang);
    json_builder_end_object(builder);

    JsonGenerator *gen = json_generator_new();
    JsonNode *root = json_builder_get_root(builder);
    json_generator_set_root(gen, root);
    char *payload = json_generator_to_data(gen, NULL);

    // Establecer headers y body
    GBytes *body = g_bytes_new(payload, strlen(payload));
    soup_message_set_request_body_from_bytes(msg, "application/x-amz-json-1.1", body);
    g_bytes_unref(body);
    SoupMessageHeaders *headers = soup_message_get_request_headers(msg);
    soup_message_headers_append(headers,
                               "X-Amz-Target",
                               "AWSShineFrontendService_20170701.TranslateText");

    // Autenticación AWS (simplificada - en producción usar AWS SDK)
    // Por ahora, asumimos que las credenciales están en variables de entorno
    // o se usarán credenciales IAM si se ejecuta en EC2
    
    // Nota: Para una implementación completa, se necesita:
    // 1. Generar firma AWS Signature V4
    // 2. O usar AWS SDK for C
    // 3. O usar credenciales temporales con STS
    
    // Por ahora, usaremos un enfoque que requiere que el usuario configure
    // las credenciales correctamente. En el futuro se puede mejorar.

    // Enviar petición asíncrona
    // Nota: msg será liberado automáticamente por libsoup
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

gboolean amazon_translator_is_configured(AmazonTranslator *translator)
{
    if (translator == NULL) {
        return FALSE;
    }

    // Verificar que tenemos credenciales básicas
    // Nota: En producción, también se debería verificar que las credenciales son válidas
    if (translator->access_key == NULL || strlen(translator->access_key) == 0) {
        return FALSE;
    }

    if (translator->secret_key == NULL || strlen(translator->secret_key) == 0) {
        return FALSE;
    }

    return TRUE;
}

const char* amazon_translator_get_name(AmazonTranslator *translator)
{
    (void)translator; // No usado
    return "Amazon Translate";
}

