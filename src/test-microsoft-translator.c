/* test-microsoft-translator.c
 * Tests para Microsoft Translator
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
#include "microsoft-translator.h"
#include "translation-service.h"

static void test_microsoft_translator_create_null(void)
{
    MicrosoftTranslator *translator = microsoft_translator_create(NULL);
    g_assert_null(translator);
}

static void test_microsoft_translator_create_with_settings(void)
{
    GSettings *settings = g_settings_new("net.sapples.LiveCaptions");
    g_settings_set_string(settings, "microsoft-subscription-key", "test-subscription-key");
    g_settings_set_string(settings, "microsoft-region", "global");
    
    MicrosoftTranslator *translator = microsoft_translator_create(settings);
    g_assert_nonnull(translator);
    
    microsoft_translator_free(translator);
    g_object_unref(settings);
}

static void test_microsoft_translator_is_configured_with_key(void)
{
    GSettings *settings = g_settings_new("net.sapples.LiveCaptions");
    g_settings_set_string(settings, "microsoft-subscription-key", "a1b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6");
    g_settings_set_string(settings, "microsoft-region", "global");
    
    MicrosoftTranslator *translator = microsoft_translator_create(settings);
    g_assert_nonnull(translator);
    
    gboolean configured = microsoft_translator_is_configured(translator);
    g_assert_true(configured);
    
    microsoft_translator_free(translator);
    g_object_unref(settings);
}

static void test_microsoft_translator_is_configured_without_key(void)
{
    GSettings *settings = g_settings_new("net.sapples.LiveCaptions");
    g_settings_set_string(settings, "microsoft-subscription-key", "");
    
    MicrosoftTranslator *translator = microsoft_translator_create(settings);
    g_assert_nonnull(translator);
    
    gboolean configured = microsoft_translator_is_configured(translator);
    g_assert_false(configured);
    
    microsoft_translator_free(translator);
    g_object_unref(settings);
}

static void test_microsoft_translator_get_name(void)
{
    GSettings *settings = g_settings_new("net.sapples.LiveCaptions");
    g_settings_set_string(settings, "microsoft-subscription-key", "test-key");
    
    MicrosoftTranslator *translator = microsoft_translator_create(settings);
    g_assert_nonnull(translator);
    
    const char *name = microsoft_translator_get_name(translator);
    g_assert_nonnull(name);
    g_assert_cmpstr(name, ==, "Microsoft Translator");
    
    microsoft_translator_free(translator);
    g_object_unref(settings);
}

static void test_microsoft_translator_free_null(void)
{
    // No debería crashear al liberar NULL
    microsoft_translator_free(NULL);
}

static void test_microsoft_translator_default_region(void)
{
    GSettings *settings = g_settings_new("net.sapples.LiveCaptions");
    g_settings_set_string(settings, "microsoft-subscription-key", "test-key");
    g_settings_set_string(settings, "microsoft-region", ""); // Región vacía
    
    MicrosoftTranslator *translator = microsoft_translator_create(settings);
    g_assert_nonnull(translator);
    
    // Debería usar región por defecto
    microsoft_translator_free(translator);
    g_object_unref(settings);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);
    
    g_test_add_func("/microsoft-translator/create_null", test_microsoft_translator_create_null);
    g_test_add_func("/microsoft-translator/create_with_settings", test_microsoft_translator_create_with_settings);
    g_test_add_func("/microsoft-translator/is_configured_with_key", test_microsoft_translator_is_configured_with_key);
    g_test_add_func("/microsoft-translator/is_configured_without_key", test_microsoft_translator_is_configured_without_key);
    g_test_add_func("/microsoft-translator/get_name", test_microsoft_translator_get_name);
    g_test_add_func("/microsoft-translator/free_null", test_microsoft_translator_free_null);
    g_test_add_func("/microsoft-translator/default_region", test_microsoft_translator_default_region);
    
    return g_test_run();
}

