/* test-integration.c
 * Tests de integración para el sistema de traducción
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

static void test_translation_service_factory_pattern_amazon(void)
{
    GSettings *settings = g_settings_new("net.sapples.LiveCaptions");
    g_settings_set_string(settings, "amazon-access-key", "test-key");
    g_settings_set_string(settings, "amazon-secret-key", "test-secret");
    
    // Test que el factory crea el servicio correcto
    TranslationService *service = translation_service_create("amazon", settings);
    g_assert_nonnull(service);
    
    const char *name = translation_service_get_name(service);
    g_assert_cmpstr(name, ==, "Amazon Translate");
    
    // Test que el servicio verifica configuración
    gboolean configured = translation_service_is_configured(service);
    g_assert_true(configured);
    
    translation_service_free(service);
    g_object_unref(settings);
}

static void test_translation_service_factory_pattern_google(void)
{
    GSettings *settings = g_settings_new("net.sapples.LiveCaptions");
    g_settings_set_string(settings, "google-api-key", "test-api-key");
    
    // Test que el factory crea el servicio correcto
    TranslationService *service = translation_service_create("google", settings);
    g_assert_nonnull(service);
    
    const char *name = translation_service_get_name(service);
    g_assert_cmpstr(name, ==, "Google Translate");
    
    // Test que el servicio verifica configuración
    gboolean configured = translation_service_is_configured(service);
    g_assert_true(configured);
    
    translation_service_free(service);
    g_object_unref(settings);
}

static void test_translation_service_factory_pattern_microsoft(void)
{
    GSettings *settings = g_settings_new("net.sapples.LiveCaptions");
    g_settings_set_string(settings, "microsoft-subscription-key", "test-subscription-key");
    
    // Test que el factory crea el servicio correcto
    TranslationService *service = translation_service_create("microsoft", settings);
    g_assert_nonnull(service);
    
    const char *name = translation_service_get_name(service);
    g_assert_cmpstr(name, ==, "Microsoft Translator");
    
    // Test que el servicio verifica configuración
    gboolean configured = translation_service_is_configured(service);
    g_assert_true(configured);
    
    translation_service_free(service);
    g_object_unref(settings);
}

static void test_translation_service_settings_sync(void)
{
    GSettings *settings = g_settings_new("net.sapples.LiveCaptions");
    
    // Cambiar configuración después de crear el servicio
    g_settings_set_string(settings, "amazon-access-key", "new-key");
    g_settings_set_string(settings, "amazon-secret-key", "new-secret");
    
    TranslationService *service = translation_service_create("amazon", settings);
    g_assert_nonnull(service);
    
    // El servicio debería usar las nuevas credenciales
    gboolean configured = translation_service_is_configured(service);
    g_assert_true(configured);
    
    translation_service_free(service);
    g_object_unref(settings);
}

static void test_translation_service_multiple_instances(void)
{
    GSettings *settings1 = g_settings_new("net.sapples.LiveCaptions");
    g_settings_set_string(settings1, "amazon-access-key", "key1");
    g_settings_set_string(settings1, "amazon-secret-key", "secret1");
    
    GSettings *settings2 = g_settings_new("net.sapples.LiveCaptions");
    g_settings_set_string(settings2, "amazon-access-key", "key2");
    g_settings_set_string(settings2, "amazon-secret-key", "secret2");
    
    TranslationService *service1 = translation_service_create("amazon", settings1);
    TranslationService *service2 = translation_service_create("amazon", settings2);
    
    g_assert_nonnull(service1);
    g_assert_nonnull(service2);
    
    // Ambos servicios deberían ser independientes
    g_assert_true(translation_service_is_configured(service1));
    g_assert_true(translation_service_is_configured(service2));
    
    translation_service_free(service1);
    translation_service_free(service2);
    
    g_object_unref(settings1);
    g_object_unref(settings2);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);
    
    g_test_add_func("/integration/factory_pattern_amazon", test_translation_service_factory_pattern_amazon);
    g_test_add_func("/integration/factory_pattern_google", test_translation_service_factory_pattern_google);
    g_test_add_func("/integration/factory_pattern_microsoft", test_translation_service_factory_pattern_microsoft);
    g_test_add_func("/integration/settings_sync", test_translation_service_settings_sync);
    g_test_add_func("/integration/multiple_instances", test_translation_service_multiple_instances);
    
    return g_test_run();
}

