/* test-google-translator.c
 * Tests para Google Translator
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
#include "google-translator.h"
#include "translation-service.h"

static void test_google_translator_create_null(void)
{
    GoogleTranslator *translator = google_translator_create(NULL);
    g_assert_null(translator);
}

static void test_google_translator_create_with_settings(void)
{
    GSettings *settings = g_settings_new("net.sapples.LiveCaptions");
    g_settings_set_string(settings, "google-api-key", "test-api-key");
    
    GoogleTranslator *translator = google_translator_create(settings);
    g_assert_nonnull(translator);
    
    google_translator_free(translator);
    g_object_unref(settings);
}

static void test_google_translator_is_configured_with_key(void)
{
    GSettings *settings = g_settings_new("net.sapples.LiveCaptions");
    g_settings_set_string(settings, "google-api-key", "AIzaSyTestKey123456789");
    
    GoogleTranslator *translator = google_translator_create(settings);
    g_assert_nonnull(translator);
    
    gboolean configured = google_translator_is_configured(translator);
    g_assert_true(configured);
    
    google_translator_free(translator);
    g_object_unref(settings);
}

static void test_google_translator_is_configured_without_key(void)
{
    GSettings *settings = g_settings_new("net.sapples.LiveCaptions");
    g_settings_set_string(settings, "google-api-key", "");
    
    GoogleTranslator *translator = google_translator_create(settings);
    g_assert_nonnull(translator);
    
    gboolean configured = google_translator_is_configured(translator);
    g_assert_false(configured);
    
    google_translator_free(translator);
    g_object_unref(settings);
}

static void test_google_translator_get_name(void)
{
    GSettings *settings = g_settings_new("net.sapples.LiveCaptions");
    g_settings_set_string(settings, "google-api-key", "test-key");
    
    GoogleTranslator *translator = google_translator_create(settings);
    g_assert_nonnull(translator);
    
    const char *name = google_translator_get_name(translator);
    g_assert_nonnull(name);
    g_assert_cmpstr(name, ==, "Google Translate");
    
    google_translator_free(translator);
    g_object_unref(settings);
}

static void test_google_translator_free_null(void)
{
    // No debería crashear al liberar NULL
    google_translator_free(NULL);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);
    
    g_test_add_func("/google-translator/create_null", test_google_translator_create_null);
    g_test_add_func("/google-translator/create_with_settings", test_google_translator_create_with_settings);
    g_test_add_func("/google-translator/is_configured_with_key", test_google_translator_is_configured_with_key);
    g_test_add_func("/google-translator/is_configured_without_key", test_google_translator_is_configured_without_key);
    g_test_add_func("/google-translator/get_name", test_google_translator_get_name);
    g_test_add_func("/google-translator/free_null", test_google_translator_free_null);
    
    return g_test_run();
}

