/* asrproc.c
 * This file implements asr_thread which takes in audio, passes it to aprilasr,
 * and passes the output to line_generator to update the GtkLabel.
 *
 * Copyright 2022 abb128
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

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/mman.h>
#include <glib.h>
#include <time.h>

#include <stdbool.h>
#include <april_api.h>

#include "asrproc.h"
#include "line-gen.h"
#include "livecaptions-window.h"
#include "livecaptions-application.h"
#include "history.h"
#include "common.h"
#include "translation-service.h"

struct asr_thread_i {
    volatile size_t sound_counter;
    size_t silence_counter;

    GThread * thread_id;

    struct line_generator line;

    GMutex text_mutex;
    char text_buffer[32768];

    AprilASRModel model;
    AprilASRSession session;

    LiveCaptionsWindow *window;

    size_t layout_counter;

    volatile bool text_stream_active;
    volatile bool pause;

    volatile time_t last_silence_time;

    volatile bool ending;

    bool errored;

    // Translation service
    TranslationService *translation_service;
    GSettings *settings;
    char *translated_text;
    GMutex translation_mutex;
};


static gboolean main_thread_update_label(void *userdata);

static void *run_asr_thread(void *userdata){
    asr_thread data = (asr_thread)userdata;

    while(!data->ending){
        sleep(1);

        if(data->last_silence_time == 0) continue;

        time_t current_time = time(NULL);

        if(difftime(current_time, data->last_silence_time) >= 6.0) {
            g_mutex_lock(&data->text_mutex);
            data->last_silence_time = 0;
            for(int i=1; i<AC_LINE_COUNT; i++) line_generator_break(&data->line);
            g_mutex_unlock(&data->text_mutex);
            g_idle_add(main_thread_update_label, data);
        }
    }

    //sleep(40);

    //if(data->sound_counter < 512) {
    //    gtk_label_set_text(data->label, "[No audio is being received. If you are playing audio,\nmost likely the audio recording isn't working]");
    //}

    return NULL;
}

// Callback cuando la traducción está lista
static void translation_callback(const char *translated_text,
                                 GError *error,
                                 void *user_data)
{
    asr_thread data = (asr_thread)user_data;

    g_mutex_lock(&data->translation_mutex);
    
    if (error != NULL) {
        // En caso de error, mantener el texto original o mostrar mensaje
        g_warning("Error de traducción: %s", error->message);
        g_free(data->translated_text);
        data->translated_text = NULL;
    } else if (translated_text != NULL && strlen(translated_text) > 0) {
        g_free(data->translated_text);
        data->translated_text = g_strdup(translated_text);
        g_mutex_unlock(&data->translation_mutex);
        // Actualizar la UI con el texto traducido
        g_idle_add(main_thread_update_label, data);
        return;
    } else {
        g_free(data->translated_text);
        data->translated_text = NULL;
    }
    
    g_mutex_unlock(&data->translation_mutex);
}

static gboolean main_thread_update_label(void *userdata){
    asr_thread data = userdata;

    if((data->window == NULL) || (data->pause)) return G_SOURCE_REMOVE;

    g_mutex_lock(&data->text_mutex);
    
    // Verificar si la traducción está habilitada
    bool translation_enabled = g_settings_get_boolean(data->settings, "translation-enabled") &&
                               data->translation_service != NULL &&
                               translation_service_is_configured(data->translation_service);
    
    if (translation_enabled) {
        // Si la traducción está habilitada, solo mostrar texto traducido
        g_mutex_lock(&data->translation_mutex);
        if (data->translated_text != NULL && strlen(data->translated_text) > 0) {
            // Mostrar texto traducido como texto plano (sin markup)
            gtk_label_set_text(data->window->label, data->translated_text);
        }
        // Si no hay traducción aún, NO mostrar nada (mantener el texto anterior)
        g_mutex_unlock(&data->translation_mutex);
    } else {
        // Si la traducción NO está habilitada, mostrar texto original
        line_generator_set_text(&data->line, data->window->label);
    }
    
    if(data->text_stream_active) {
        LiveCaptionsApplication *application = LIVECAPTIONS_APPLICATION(gtk_window_get_application(GTK_WINDOW(data->window)));
        livecaptions_application_stream_text(application, line_generator_get_plaintext(&data->line));
    }

    g_mutex_unlock(&data->text_mutex);

    return G_SOURCE_REMOVE;
}

static void april_result_handler(void* userdata, AprilResultType result, size_t count, const AprilToken* tokens) {
    asr_thread data = userdata;
    if((data->window == NULL) || (data->pause)) return;

    switch(result) {
        case APRIL_RESULT_RECOGNITION_PARTIAL:
        case APRIL_RESULT_RECOGNITION_FINAL:
        {
            g_mutex_lock(&data->text_mutex);
            data->last_silence_time = 0;

            if((data->layout_counter != data->window->font_layout_counter) || (data->line.layout == NULL)) {
                if(data->line.layout != NULL) g_object_unref(data->line.layout);

                data->line.layout = pango_layout_copy(data->window->font_layout);
                data->line.max_text_width = data->window->max_text_width;

                data->layout_counter = data->window->font_layout_counter;
            }

            line_generator_update(&data->line, count, tokens);
            if(result == APRIL_RESULT_RECOGNITION_FINAL) {
                line_generator_finalize(&data->line);
                commit_tokens_to_current_history(tokens, count);
            }

            // Verificar si la traducción está habilitada
            bool translation_enabled = data->translation_service != NULL && 
                                       g_settings_get_boolean(data->settings, "translation-enabled") &&
                                       translation_service_is_configured(data->translation_service);
            
            // Iniciar traducción si está habilitada
            if (translation_enabled) {
                const char *translation_mode = g_settings_get_string(data->settings, "translation-mode");
                bool should_translate = FALSE;
                
                if (g_str_equal(translation_mode, "realtime")) {
                    // Traducir tanto parciales como finales
                    should_translate = TRUE;
                } else if (g_str_equal(translation_mode, "final-only")) {
                    // Traducir solo finales
                    should_translate = (result == APRIL_RESULT_RECOGNITION_FINAL);
                }
                
                if (should_translate) {
                    const char *plaintext = line_generator_get_plaintext(&data->line);
                    if (plaintext != NULL && strlen(plaintext) > 0) {
                        gchar *source_lang = g_settings_get_string(data->settings, "translation-source-language");
                        gchar *target_lang = g_settings_get_string(data->settings, "translation-target-language");
                        
                        // Limpiar traducción anterior
                        g_mutex_lock(&data->translation_mutex);
                        g_free(data->translated_text);
                        data->translated_text = NULL;
                        g_mutex_unlock(&data->translation_mutex);
                        
                        // Iniciar traducción asíncrona
                        translation_service_translate_async(data->translation_service,
                                                           plaintext,
                                                           source_lang ? source_lang : "auto",
                                                           target_lang ? target_lang : "es",
                                                           translation_callback,
                                                           data);
                        
                        g_free(source_lang);
                        g_free(target_lang);
                    }
                }
                
                g_free(translation_mode);
                
                // Si la traducción está habilitada, NO actualizar la UI aquí
                // La UI se actualizará cuando llegue la traducción en el callback
            } else {
                // Si la traducción NO está habilitada, actualizar la UI con el texto original
                g_mutex_unlock(&data->text_mutex);
                g_idle_add(main_thread_update_label, data);
                return; // Salir temprano para evitar el unlock duplicado
            }

            g_mutex_unlock(&data->text_mutex);
            break;
        }

        case APRIL_RESULT_ERROR_CANT_KEEP_UP: {
            livecaptions_window_warn_slow(data->window);
            break;
        }

        case APRIL_RESULT_SILENCE: {
            g_mutex_lock(&data->text_mutex);
            data->last_silence_time = time(NULL);

            line_generator_break(&data->line);
            save_silence_to_history();

            g_mutex_unlock(&data->text_mutex);
            g_idle_add(main_thread_update_label, data);
            break;
        }
    }
}

void asr_thread_enqueue_audio(asr_thread thread, short *data, size_t num_shorts) {
    if((thread->window == NULL) || thread->pause) return;
    if((thread->session == NULL) || (thread->model == NULL)) return;


    bool found_nonzero = false;
    for(size_t i=0; i<num_shorts; i++){
        if((data[i] > 16) || (data[i] < -16)){
            found_nonzero = true;
            break;
        }
    }

    thread->silence_counter = found_nonzero ? 0 : (thread->silence_counter + num_shorts);

    if(thread->silence_counter >= 24000){
        thread->silence_counter = 24000;
        return aas_flush(thread->session);
    }
    
    thread->sound_counter += num_shorts;
    aas_feed_pcm16(thread->session, data, num_shorts); // TODO?
}

gpointer asr_thread_get_model(asr_thread thread) {
    return thread->model;
}

gpointer asr_thread_get_session(asr_thread thread) {
    return thread->session;
}

void asr_thread_pause(asr_thread thread, bool pause) {
    thread->pause = pause;
}

void asr_thread_set_text_stream_active(asr_thread thread, bool active) {
    thread->text_stream_active = active;
}

int asr_thread_samplerate(asr_thread thread) {
    return aam_get_sample_rate(thread->model);
}

// Wrapper para el callback de GSettings
static void on_translation_settings_changed(GSettings *settings,
                                            const gchar *key,
                                            gpointer user_data)
{
    (void)settings; // No usado
    (void)key; // No usado
    asr_thread thread = (asr_thread)user_data;
    asr_thread_update_translation_service(thread);
}

asr_thread create_asr_thread(const char *model_path){
    asr_thread data = calloc(1, sizeof(struct asr_thread_i));

    line_generator_init(&data->line);

    // Inicializar settings y servicio de traducción
    data->settings = g_settings_new("net.sapples.LiveCaptions");
    
    // Inicializar mutexes
    g_mutex_init(&data->text_mutex);
    g_mutex_init(&data->translation_mutex);
    
    // Crear servicio de traducción
    const char *service_type = g_settings_get_string(data->settings, "translation-service");
    if (service_type != NULL) {
        data->translation_service = translation_service_create(service_type, data->settings);
    }
    g_free((gpointer)service_type);
    
    // Conectar cambios de configuración para actualizar el servicio
    g_signal_connect(data->settings, "changed::translation-service",
                    G_CALLBACK(on_translation_settings_changed), data);
    g_signal_connect(data->settings, "changed::amazon-access-key",
                    G_CALLBACK(on_translation_settings_changed), data);
    g_signal_connect(data->settings, "changed::amazon-secret-key",
                    G_CALLBACK(on_translation_settings_changed), data);
    g_signal_connect(data->settings, "changed::google-api-key",
                    G_CALLBACK(on_translation_settings_changed), data);
    g_signal_connect(data->settings, "changed::microsoft-subscription-key",
                    G_CALLBACK(on_translation_settings_changed), data);
    g_signal_connect(data->settings, "changed::deepl-api-key",
                    G_CALLBACK(on_translation_settings_changed), data);
    g_signal_connect(data->settings, "changed::deepl-use-free-api",
                    G_CALLBACK(on_translation_settings_changed), data);

    if(!asr_thread_update_model(data, model_path)){
        char *model_default = GET_MODEL_PATH();
        if(!asr_thread_update_model(data, model_default)) {
            // Limpiar en caso de error
            if (data->translation_service) {
                translation_service_free(data->translation_service);
            }
            g_mutex_clear(&data->translation_mutex);
            g_mutex_clear(&data->text_mutex);
            g_object_unref(data->settings);
            free(data);
            return NULL;
        }

        g_settings_set_string(data->settings, "active-model", model_default);
    }

    data->thread_id = g_thread_new("lcap-audiothread", run_asr_thread, data);

    data->text_stream_active = false;

    return data;
}

bool asr_thread_update_model(asr_thread data, const char *model_path) {
    // Freeing model frees token list, which may be being accessed during
    // line generation
    g_mutex_lock(&data->text_mutex);

    data->pause = true;

    AprilASRModel old_model = data->model;
    AprilASRSession old_session = data->session;

    data->model = NULL;
    data->session = NULL;

    if(old_session != NULL)
        aas_free(old_session);

    if(old_model != NULL)
        aam_free(old_model);


    AprilConfig config = {
        .handler = april_result_handler,
        .flags = APRIL_CONFIG_FLAG_ASYNC_RT_BIT,
        .userdata = data
    };

    AprilASRModel new_model = aam_create_model(model_path);
    if(new_model == NULL) {
        printf("Loading model %s failed!\n", model_path);
        data->errored = true;
        g_mutex_unlock(&data->text_mutex);
        return false;
    }

    {
        printf("\n-- Model metadata --\n");
        printf("Name: %s\n", aam_get_name(new_model));
        char *description = (char*)aam_get_description(new_model);
        for(int i=0; description[i]; i++){
            if((description[i] == ' ') && (description[i+1] == 'D') && (description[i+2] == 'i')){
                description[i] = 0;
                break;
            }
        }
        printf("Description: %s\n", description);
        printf("Language: %s\n", aam_get_language(new_model));
        printf("-- --\n\n");
    }

    line_generator_set_language(&data->line, aam_get_language(new_model));

    AprilASRSession new_session = aas_create_session(new_model, config);
    if(new_session == NULL) {
        printf("Creating session %s failed!\n", model_path);
        data->errored = true;
        g_mutex_unlock(&data->text_mutex);
        return false;
    }

    data->model = new_model;
    data->session = new_session;

    data->errored = false;
    data->ending = false;
    data->pause = false;

    line_generator_finalize(&data->line);

    g_mutex_unlock(&data->text_mutex);

    return true;
}

bool asr_thread_is_errored(asr_thread thread) {
    return thread->errored;
}

void asr_thread_set_main_window(asr_thread thread, LiveCaptionsWindow *window) {
    thread->window = window;
}

void asr_thread_flush(asr_thread thread) {
    if(thread->session == NULL) return;
    aas_flush(thread->session);
}

void asr_thread_update_translation_service(asr_thread thread) {
    if (thread == NULL || thread->settings == NULL) {
        return;
    }

    g_mutex_lock(&thread->translation_mutex);
    
    // Liberar servicio anterior
    if (thread->translation_service != NULL) {
        translation_service_free(thread->translation_service);
        thread->translation_service = NULL;
    }
    
    // Limpiar traducción anterior
    g_free(thread->translated_text);
    thread->translated_text = NULL;
    
    // Crear nuevo servicio con la configuración actual
    const char *service_type = g_settings_get_string(thread->settings, "translation-service");
    if (service_type != NULL) {
        thread->translation_service = translation_service_create(service_type, thread->settings);
    }
    g_free((gpointer)service_type);
    
    g_mutex_unlock(&thread->translation_mutex);
}

void free_asr_thread(asr_thread thread) {
    thread->ending = true;

    g_mutex_lock(&thread->text_mutex);

    g_thread_join(thread->thread_id);

    if(thread->session != NULL)
        aas_free(thread->session);
    
    if(thread->model != NULL)
        aam_free(thread->model);

    g_thread_unref(thread->thread_id); // ?

    // Limpiar servicio de traducción
    g_mutex_lock(&thread->translation_mutex);
    if (thread->translation_service != NULL) {
        translation_service_free(thread->translation_service);
        thread->translation_service = NULL;
    }
    g_free(thread->translated_text);
    thread->translated_text = NULL;
    g_mutex_unlock(&thread->translation_mutex);
    
    g_mutex_clear(&thread->translation_mutex);
    g_mutex_unlock(&thread->text_mutex);
    g_mutex_clear(&thread->text_mutex);
    
    if (thread->settings != NULL) {
        g_object_unref(thread->settings);
    }

    free(thread);
}
