# Tests del Sistema de Traducción

Este documento describe los tests implementados para el sistema de traducción de subtítulos.

## Estructura de Tests

Los tests están organizados en los siguientes archivos:

### 1. `test-translation-service.c`
Tests para la interfaz abstracta `TranslationService` y el patrón Factory:

- **test_translation_service_create_null**: Verifica que crear un servicio con parámetros NULL retorna NULL
- **test_translation_service_create_invalid_type**: Verifica que tipos de servicio inválidos retornan NULL
- **test_translation_service_create_amazon**: Verifica la creación exitosa de un servicio Amazon
- **test_translation_service_is_configured**: Verifica la detección de configuración correcta
- **test_translation_service_free_null**: Verifica que liberar NULL no causa crash
- **test_translation_service_get_name**: Verifica que se obtiene el nombre correcto del servicio

### 2. `test-amazon-translator.c`
Tests para la implementación concreta de Amazon Translator:

- **test_amazon_translator_create_null**: Verifica creación con settings NULL
- **test_amazon_translator_create_with_settings**: Verifica creación con configuración válida
- **test_amazon_translator_is_configured_with_credentials**: Verifica detección de credenciales válidas
- **test_amazon_translator_is_configured_without_credentials**: Verifica detección de falta de credenciales
- **test_amazon_translator_get_name**: Verifica nombre del servicio
- **test_amazon_translator_free_null**: Verifica liberación segura de NULL
- **test_amazon_translator_default_region**: Verifica uso de región por defecto

### 3. `test-gsettings.c`
Tests para las configuraciones GSettings:

- **test_translation_settings_defaults**: Verifica valores por defecto del schema
- **test_translation_settings_values**: Verifica lectura/escritura de valores
- **test_translation_settings_modes**: Verifica modos de traducción (final-only, realtime)
- **test_translation_settings_languages**: Verifica códigos de idioma soportados

### 4. `test-integration.c`
Tests de integración del sistema completo:

- **test_translation_service_factory_pattern**: Verifica el patrón Factory
- **test_translation_service_settings_sync**: Verifica sincronización con GSettings
- **test_translation_service_multiple_instances**: Verifica múltiples instancias independientes

## Cobertura de Tests

### Componentes Cubiertos

✅ **TranslationService (Interfaz)**
- Creación y destrucción
- Factory pattern
- Validación de parámetros
- Obtención de nombre del servicio

✅ **AmazonTranslator (Implementación)**
- Inicialización con GSettings
- Validación de credenciales
- Manejo de regiones
- Liberación de recursos

✅ **GSettings (Configuración)**
- Valores por defecto
- Lectura/escritura de configuraciones
- Modos de traducción
- Idiomas soportados

✅ **Integración**
- Patrón Factory
- Sincronización con configuración
- Múltiples instancias

### Componentes No Cubiertos (Por Limitaciones)

⚠️ **Traducción Asíncrona Real**
- No se testean llamadas HTTP reales a AWS (requiere credenciales válidas y conexión)
- No se testea el parsing de respuestas JSON reales
- No se testea el manejo de errores de red

⚠️ **Autenticación AWS Signature V4**
- La implementación completa de la firma AWS no está implementada aún
- Se requiere para hacer peticiones reales a AWS Translate API

## Ejecutar Tests

### Prerrequisitos

1. Compilar el schema de GSettings:
```bash
glib-compile-schemas data/
```

2. Configurar y compilar el proyecto:
```bash
meson setup builddir --buildtype debug
meson compile -C builddir
```

### Ejecutar Todos los Tests

```bash
meson test -C builddir
```

O usar el script proporcionado:
```bash
./run-tests.sh
```

### Ejecutar Tests Individuales

```bash
# Test de TranslationService
./builddir/src/test-translation-service

# Test de AmazonTranslator
./builddir/src/test-amazon-translator

# Test de GSettings
./builddir/src/test-gsettings

# Test de integración
./builddir/src/test-integration
```

### Ejecutar con Verbose Output

```bash
meson test -C builddir --verbose
```

## Agregar Nuevos Tests

Para añadir tests para un nuevo servicio de traducción:

1. Crear `test-<servicio>-translator.c` siguiendo el patrón de `test-amazon-translator.c`
2. Añadir el ejecutable en `src/meson.build`
3. Añadir el test con `test('nombre', ejecutable)`

Ejemplo:
```meson
test_new_service_sources = [
  'test-new-service-translator.c',
  'new-service-translator.c',
  'translation-service.c'
]

test_new_service = executable('test-new-service',
  test_new_service_sources,
  dependencies: livecaptions_deps,
)

test('new-service', test_new_service)
```

## Principios de Testing Aplicados

1. **Aislamiento**: Cada test es independiente y no depende de otros
2. **Principio AAA**: Arrange, Act, Assert en cada test
3. **Cobertura de Casos Límite**: Tests para NULL, valores vacíos, etc.
4. **Tests Unitarios**: Cada componente se testea de forma aislada
5. **Tests de Integración**: Verificación del funcionamiento conjunto

## Notas

- Los tests no requieren conexión a internet ni credenciales AWS reales
- Los tests verifican la estructura y lógica, no las llamadas HTTP reales
- Para tests end-to-end con AWS real, se necesitarían credenciales de prueba y un entorno de testing aislado

