/* test-amazon-translator.c
 * Tests para Amazon Translator
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
#include "amazon-translator.h"
#include "translation-service.h"

static void test_amazon_translator_create_null(void)
{
    AmazonTranslator *translator = amazon_translator_create(NULL);
    g_assert_null(translator);
}

static void test_amazon_translator_create_with_settings(void)
{
    GSettings *settings = g_settings_new("net.sapples.LiveCaptions");
    g_settings_set_string(settings, "amazon-access-key", "test-access-key");
    g_settings_set_string(settings, "amazon-secret-key", "test-secret-key");
    g_settings_set_string(settings, "amazon-region", "us-east-1");
    
    AmazonTranslator *translator = amazon_translator_create(settings);
    g_assert_nonnull(translator);
    
    amazon_translator_free(translator);
    g_object_unref(settings);
}

static void test_amazon_translator_is_configured_with_credentials(void)
{
    GSettings *settings = g_settings_new("net.sapples.LiveCaptions");
    g_settings_set_string(settings, "amazon-access-key", "AKIAIOSFODNN7EXAMPLE");
    g_settings_set_string(settings, "amazon-secret-key", "wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY");
    g_settings_set_string(settings, "amazon-region", "us-east-1");
    
    AmazonTranslator *translator = amazon_translator_create(settings);
    g_assert_nonnull(translator);
    
    gboolean configured = amazon_translator_is_configured(translator);
    g_assert_true(configured);
    
    amazon_translator_free(translator);
    g_object_unref(settings);
}

static void test_amazon_translator_is_configured_without_credentials(void)
{
    GSettings *settings = g_settings_new("net.sapples.LiveCaptions");
    g_settings_set_string(settings, "amazon-access-key", "");
    g_settings_set_string(settings, "amazon-secret-key", "");
    
    AmazonTranslator *translator = amazon_translator_create(settings);
    g_assert_nonnull(translator);
    
    gboolean configured = amazon_translator_is_configured(translator);
    g_assert_false(configured);
    
    amazon_translator_free(translator);
    g_object_unref(settings);
}

static void test_amazon_translator_get_name(void)
{
    GSettings *settings = g_settings_new("net.sapples.LiveCaptions");
    g_settings_set_string(settings, "amazon-access-key", "test-key");
    g_settings_set_string(settings, "amazon-secret-key", "test-secret");
    
    AmazonTranslator *translator = amazon_translator_create(settings);
    g_assert_nonnull(translator);
    
    const char *name = amazon_translator_get_name(translator);
    g_assert_nonnull(name);
    g_assert_cmpstr(name, ==, "Amazon Translate");
    
    amazon_translator_free(translator);
    g_object_unref(settings);
}

static void test_amazon_translator_free_null(void)
{
    // No debería crashear al liberar NULL
    amazon_translator_free(NULL);
}

static void test_amazon_translator_default_region(void)
{
    GSettings *settings = g_settings_new("net.sapples.LiveCaptions");
    g_settings_set_string(settings, "amazon-access-key", "test-key");
    g_settings_set_string(settings, "amazon-secret-key", "test-secret");
    g_settings_set_string(settings, "amazon-region", ""); // Región vacía
    
    AmazonTranslator *translator = amazon_translator_create(settings);
    g_assert_nonnull(translator);
    
    // Debería usar región por defecto
    amazon_translator_free(translator);
    g_object_unref(settings);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);
    
    g_test_add_func("/amazon-translator/create_null", test_amazon_translator_create_null);
    g_test_add_func("/amazon-translator/create_with_settings", test_amazon_translator_create_with_settings);
    g_test_add_func("/amazon-translator/is_configured_with_credentials", test_amazon_translator_is_configured_with_credentials);
    g_test_add_func("/amazon-translator/is_configured_without_credentials", test_amazon_translator_is_configured_without_credentials);
    g_test_add_func("/amazon-translator/get_name", test_amazon_translator_get_name);
    g_test_add_func("/amazon-translator/free_null", test_amazon_translator_free_null);
    g_test_add_func("/amazon-translator/default_region", test_amazon_translator_default_region);
    
    return g_test_run();
}

