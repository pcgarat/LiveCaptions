#!/bin/bash
# Script para ejecutar los tests del sistema de traducción

set -e

echo "=== Compilando schema de GSettings ==="
glib-compile-schemas data/ || {
    echo "Error: No se pudo compilar el schema de GSettings"
    echo "Asegúrate de tener glib-compile-schemas instalado"
    exit 1
}

# Configurar variable de entorno para que GSettings encuentre el schema
export GSETTINGS_SCHEMA_DIR="$(pwd)/data"

echo ""
echo "=== Ejecutando test de GSettings (no requiere build completo) ==="
# Compilar y ejecutar test de GSettings directamente
if command -v gcc &> /dev/null && pkg-config --exists gio-2.0; then
    echo "Compilando test-gsettings..."
    gcc src/test-gsettings.c $(pkg-config --cflags --libs gio-2.0) -o /tmp/test-gsettings 2>&1 | grep -v "warning:" || true
    if [ -f "/tmp/test-gsettings" ]; then
        echo "Ejecutando test-gsettings..."
        /tmp/test-gsettings
        echo ""
    fi
else
    echo "  ⚠️  gcc o gio-2.0 no disponibles para compilar test-gsettings"
fi

echo ""
echo "=== Verificando builddir para otros tests ==="
if [ ! -d "builddir" ] || [ ! -f "builddir/meson-private/build.dat" ]; then
    echo "⚠️  Builddir no está configurado correctamente"
    echo ""
    echo "Para ejecutar todos los tests, necesitas:"
    echo "  1. Instalar dependencias: cmake, libsoup-3.0-dev, libjson-glib-dev"
    echo "  2. Configurar build: meson setup builddir --buildtype debug"
    echo "  3. Compilar: meson compile -C builddir"
    echo "  4. Ejecutar: meson test -C builddir"
    echo ""
    echo "Intentando ejecutar tests compilados si existen..."
else
    echo "✅ Builddir configurado correctamente"
    echo ""
    echo "=== Ejecutando tests usando Meson ==="
    if command -v meson &> /dev/null; then
        meson test -C builddir --verbose 2>&1 || echo "  ⚠️  Error al ejecutar tests con Meson"
    fi
fi

# Intentar ejecutar tests individuales si existen
echo ""
echo "=== Ejecutando tests individuales si están compilados ==="

if [ -f "./builddir/src/test-translation-service" ]; then
    echo "Ejecutando test-translation-service..."
    ./builddir/src/test-translation-service
    echo ""
fi

if [ -f "./builddir/src/test-amazon-translator" ]; then
    echo "Ejecutando test-amazon-translator..."
    ./builddir/src/test-amazon-translator
    echo ""
fi

if [ -f "./builddir/src/test-integration" ]; then
    echo "Ejecutando test-integration..."
    ./builddir/src/test-integration
    echo ""
fi

if [ -f "./builddir/src/test-google-translator" ]; then
    echo "Ejecutando test-google-translator..."
    ./builddir/src/test-google-translator
    echo ""
fi

if [ -f "./builddir/src/test-microsoft-translator" ]; then
    echo "Ejecutando test-microsoft-translator..."
    ./builddir/src/test-microsoft-translator
    echo ""
fi

echo ""
echo "=== Tests completados ==="

