/* test-translation-service.c
 * Tests para el servicio de traducción
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
#include "translation-service.h"
#include "amazon-translator.h"
#include "google-translator.h"
#include "microsoft-translator.h"

static void test_translation_service_create_null(void)
{
    TranslationService *service = translation_service_create(NULL, NULL);
    g_assert_null(service);
}

static void test_translation_service_create_invalid_type(void)
{
    GSettings *settings = g_settings_new("net.sapples.LiveCaptions");
    TranslationService *service = translation_service_create("invalid_service", settings);
    g_assert_null(service);
    g_object_unref(settings);
}

static void test_translation_service_create_amazon(void)
{
    GSettings *settings = g_settings_new("net.sapples.LiveCaptions");
    
    // Configurar credenciales básicas para el test
    g_settings_set_string(settings, "amazon-access-key", "test-key");
    g_settings_set_string(settings, "amazon-secret-key", "test-secret");
    g_settings_set_string(settings, "amazon-region", "us-east-1");
    
    TranslationService *service = translation_service_create("amazon", settings);
    g_assert_nonnull(service);
    
    const char *name = translation_service_get_name(service);
    g_assert_nonnull(name);
    g_assert_cmpstr(name, ==, "Amazon Translate");
    
    translation_service_free(service);
    g_object_unref(settings);
}

static void test_translation_service_create_google(void)
{
    GSettings *settings = g_settings_new("net.sapples.LiveCaptions");
    
    // Configurar API key para el test
    g_settings_set_string(settings, "google-api-key", "test-api-key");
    
    TranslationService *service = translation_service_create("google", settings);
    g_assert_nonnull(service);
    
    const char *name = translation_service_get_name(service);
    g_assert_nonnull(name);
    g_assert_cmpstr(name, ==, "Google Translate");
    
    translation_service_free(service);
    g_object_unref(settings);
}

static void test_translation_service_create_microsoft(void)
{
    GSettings *settings = g_settings_new("net.sapples.LiveCaptions");
    
    // Configurar subscription key para el test
    g_settings_set_string(settings, "microsoft-subscription-key", "test-subscription-key");
    g_settings_set_string(settings, "microsoft-region", "global");
    
    TranslationService *service = translation_service_create("microsoft", settings);
    g_assert_nonnull(service);
    
    const char *name = translation_service_get_name(service);
    g_assert_nonnull(name);
    g_assert_cmpstr(name, ==, "Microsoft Translator");
    
    translation_service_free(service);
    g_object_unref(settings);
}

static void test_translation_service_is_configured(void)
{
    GSettings *settings = g_settings_new("net.sapples.LiveCaptions");
    
    // Test sin credenciales
    g_settings_set_string(settings, "amazon-access-key", "");
    g_settings_set_string(settings, "amazon-secret-key", "");
    TranslationService *service1 = translation_service_create("amazon", settings);
    if (service1 != NULL) {
        gboolean configured = translation_service_is_configured(service1);
        g_assert_false(configured);
        translation_service_free(service1);
    }
    
    // Test con credenciales
    g_settings_set_string(settings, "amazon-access-key", "test-key");
    g_settings_set_string(settings, "amazon-secret-key", "test-secret");
    TranslationService *service2 = translation_service_create("amazon", settings);
    if (service2 != NULL) {
        gboolean configured = translation_service_is_configured(service2);
        g_assert_true(configured);
        translation_service_free(service2);
    }
    
    g_object_unref(settings);
}

static void test_translation_service_free_null(void)
{
    // No debería crashear al liberar NULL
    translation_service_free(NULL);
}

static void test_translation_service_get_name(void)
{
    GSettings *settings = g_settings_new("net.sapples.LiveCaptions");
    g_settings_set_string(settings, "amazon-access-key", "test-key");
    g_settings_set_string(settings, "amazon-secret-key", "test-secret");
    
    TranslationService *service = translation_service_create("amazon", settings);
    if (service != NULL) {
        const char *name = translation_service_get_name(service);
        g_assert_nonnull(name);
        g_assert_cmpstr(name, ==, "Amazon Translate");
        translation_service_free(service);
    }
    
    g_object_unref(settings);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);
    
    g_test_add_func("/translation-service/create_null", test_translation_service_create_null);
    g_test_add_func("/translation-service/create_invalid_type", test_translation_service_create_invalid_type);
    g_test_add_func("/translation-service/create_amazon", test_translation_service_create_amazon);
    g_test_add_func("/translation-service/create_google", test_translation_service_create_google);
    g_test_add_func("/translation-service/create_microsoft", test_translation_service_create_microsoft);
    g_test_add_func("/translation-service/is_configured", test_translation_service_is_configured);
    g_test_add_func("/translation-service/free_null", test_translation_service_free_null);
    g_test_add_func("/translation-service/get_name", test_translation_service_get_name);
    
    return g_test_run();
}

