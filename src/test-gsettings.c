/* test-gsettings.c
 * Tests para las configuraciones GSettings de traducción
 *
 * Copyright 2024
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <glib.h>
#include <gio/gio.h>

static void test_translation_settings_defaults(void)
{
    GSettings *settings = g_settings_new("net.sapples.LiveCaptions");
    
    // Verificar valores por defecto
    gboolean enabled = g_settings_get_boolean(settings, "translation-enabled");
    g_assert_false(enabled);
    
    gchar *service = g_settings_get_string(settings, "translation-service");
    g_assert_cmpstr(service, ==, "amazon");
    g_free(service);
    
    gchar *target_lang = g_settings_get_string(settings, "translation-target-language");
    g_assert_cmpstr(target_lang, ==, "es");
    g_free(target_lang);
    
    gchar *source_lang = g_settings_get_string(settings, "translation-source-language");
    g_assert_cmpstr(source_lang, ==, "auto");
    g_free(source_lang);
    
    gchar *mode = g_settings_get_string(settings, "translation-mode");
    g_assert_cmpstr(mode, ==, "final-only");
    g_free(mode);
    
    gchar *region = g_settings_get_string(settings, "amazon-region");
    g_assert_cmpstr(region, ==, "us-east-1");
    g_free(region);
    
    gchar *google_key = g_settings_get_string(settings, "google-api-key");
    g_assert_cmpstr(google_key, ==, "");
    g_free(google_key);
    
    gchar *microsoft_key = g_settings_get_string(settings, "microsoft-subscription-key");
    g_assert_cmpstr(microsoft_key, ==, "");
    g_free(microsoft_key);
    
    gchar *microsoft_region = g_settings_get_string(settings, "microsoft-region");
    g_assert_cmpstr(microsoft_region, ==, "global");
    g_free(microsoft_region);
    
    g_object_unref(settings);
}

static void test_translation_settings_values(void)
{
    GSettings *settings = g_settings_new("net.sapples.LiveCaptions");
    
    // Test habilitar traducción
    g_settings_set_boolean(settings, "translation-enabled", TRUE);
    g_assert_true(g_settings_get_boolean(settings, "translation-enabled"));
    
    // Test cambiar idioma destino
    g_settings_set_string(settings, "translation-target-language", "fr");
    gchar *target_lang = g_settings_get_string(settings, "translation-target-language");
    g_assert_cmpstr(target_lang, ==, "fr");
    g_free(target_lang);
    
    // Test cambiar modo
    g_settings_set_string(settings, "translation-mode", "realtime");
    gchar *mode = g_settings_get_string(settings, "translation-mode");
    g_assert_cmpstr(mode, ==, "realtime");
    g_free(mode);
    
    // Test credenciales AWS
    g_settings_set_string(settings, "amazon-access-key", "AKIAIOSFODNN7EXAMPLE");
    g_settings_set_string(settings, "amazon-secret-key", "wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY");
    g_settings_set_string(settings, "amazon-region", "eu-west-1");
    
    gchar *access_key = g_settings_get_string(settings, "amazon-access-key");
    g_assert_cmpstr(access_key, ==, "AKIAIOSFODNN7EXAMPLE");
    g_free(access_key);
    
    gchar *secret_key = g_settings_get_string(settings, "amazon-secret-key");
    g_assert_cmpstr(secret_key, ==, "wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY");
    g_free(secret_key);
    
    gchar *region = g_settings_get_string(settings, "amazon-region");
    g_assert_cmpstr(region, ==, "eu-west-1");
    g_free(region);
    
    // Test Google credentials
    g_settings_set_string(settings, "google-api-key", "AIzaSyTestKey123456789");
    gchar *google_key = g_settings_get_string(settings, "google-api-key");
    g_assert_cmpstr(google_key, ==, "AIzaSyTestKey123456789");
    g_free(google_key);
    
    // Test Microsoft credentials
    g_settings_set_string(settings, "microsoft-subscription-key", "a1b2c3d4e5f6g7h8i9j0");
    g_settings_set_string(settings, "microsoft-region", "eastus");
    
    gchar *microsoft_key = g_settings_get_string(settings, "microsoft-subscription-key");
    g_assert_cmpstr(microsoft_key, ==, "a1b2c3d4e5f6g7h8i9j0");
    g_free(microsoft_key);
    
    gchar *microsoft_region = g_settings_get_string(settings, "microsoft-region");
    g_assert_cmpstr(microsoft_region, ==, "eastus");
    g_free(microsoft_region);
    
    g_object_unref(settings);
}

static void test_translation_settings_modes(void)
{
    GSettings *settings = g_settings_new("net.sapples.LiveCaptions");
    
    // Test modo final-only
    g_settings_set_string(settings, "translation-mode", "final-only");
    gchar *mode = g_settings_get_string(settings, "translation-mode");
    g_assert_cmpstr(mode, ==, "final-only");
    g_free(mode);
    
    // Test modo realtime
    g_settings_set_string(settings, "translation-mode", "realtime");
    mode = g_settings_get_string(settings, "translation-mode");
    g_assert_cmpstr(mode, ==, "realtime");
    g_free(mode);
    
    g_object_unref(settings);
}

static void test_translation_settings_languages(void)
{
    GSettings *settings = g_settings_new("net.sapples.LiveCaptions");
    
    const char *languages[] = {"es", "fr", "de", "it", "pt", "zh", "ja", "ru"};
    
    for (int i = 0; i < G_N_ELEMENTS(languages); i++) {
        g_settings_set_string(settings, "translation-target-language", languages[i]);
        gchar *lang = g_settings_get_string(settings, "translation-target-language");
        g_assert_cmpstr(lang, ==, languages[i]);
        g_free(lang);
    }
    
    // Test auto-detección
    g_settings_set_string(settings, "translation-source-language", "auto");
    gchar *source_lang = g_settings_get_string(settings, "translation-source-language");
    g_assert_cmpstr(source_lang, ==, "auto");
    g_free(source_lang);
    
    g_object_unref(settings);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);
    
    g_test_add_func("/gsettings/translation_defaults", test_translation_settings_defaults);
    g_test_add_func("/gsettings/translation_values", test_translation_settings_values);
    g_test_add_func("/gsettings/translation_modes", test_translation_settings_modes);
    g_test_add_func("/gsettings/translation_languages", test_translation_settings_languages);
    
    return g_test_run();
}

