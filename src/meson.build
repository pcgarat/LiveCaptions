livecaptions_sources = [
  'main.c',
  'livecaptions-window.c',
  'livecaptions-welcome.c',
  'livecaptions-settings.c',
  'livecaptions-application.c',
  'audiocap.c',
  'audiocap-pa.c',
  'audiocap-pw.c',
  'asrproc.c',
  'line-gen.c',
  'profanity-filter.c',
  'window-helper.c',
  'history.c',
  'livecaptions-history-window.c',
  'dbus-interface.c',
  'translation-service.c',
  'amazon-translator.c',
  'google-translator.c',
  'microsoft-translator.c',
  'deepl-translator.c'
]

cc = meson.get_compiler('c')

livecaptions_deps = [
  dependency('libadwaita-1', version: '>= 1.0'),
  #dependency('libpipewire-0.3', version: '>=0.3.41'),
  dependency('libpulse'),
  dependency('x11'),
  dependency('libsoup-3.0', version: '>= 3.0'),
  dependency('json-glib-1.0', version: '>= 1.0'),
  dependency('openssl'),

  cc.find_library('m', required: false),
  april_lib
]

gnome = import('gnome')

livecaptions_sources += gnome.compile_resources('livecaptions-resources',
  'livecaptions.gresource.xml',
  c_name: 'livecaptions'
)

executable('livecaptions', livecaptions_sources,
  dependencies: livecaptions_deps,
  install: true,
)

# Tests
test_translation_service_sources = [
  'test-translation-service.c',
  'translation-service.c',
  'amazon-translator.c',
  'google-translator.c',
  'microsoft-translator.c',
  'deepl-translator.c'
]

test_translation_service = executable('test-translation-service',
  test_translation_service_sources,
  dependencies: livecaptions_deps,
  c_args: ['-DTEST_MODE'],
)

test('translation-service', test_translation_service)

test_amazon_translator_sources = [
  'test-amazon-translator.c',
  'amazon-translator.c',
  'translation-service.c',
  'google-translator.c',
  'microsoft-translator.c',
  'deepl-translator.c'
]

test_amazon_translator = executable('test-amazon-translator',
  test_amazon_translator_sources,
  dependencies: livecaptions_deps,
  c_args: ['-DTEST_MODE'],
)

test('amazon-translator', test_amazon_translator)

test_google_translator_sources = [
  'test-google-translator.c',
  'google-translator.c',
  'translation-service.c',
  'amazon-translator.c',
  'microsoft-translator.c',
  'deepl-translator.c'
]

test_google_translator = executable('test-google-translator',
  test_google_translator_sources,
  dependencies: livecaptions_deps,
  c_args: ['-DTEST_MODE'],
)

test('google-translator', test_google_translator)

test_microsoft_translator_sources = [
  'test-microsoft-translator.c',
  'microsoft-translator.c',
  'translation-service.c',
  'amazon-translator.c',
  'google-translator.c',
  'deepl-translator.c'
]

test_microsoft_translator = executable('test-microsoft-translator',
  test_microsoft_translator_sources,
  dependencies: livecaptions_deps,
  c_args: ['-DTEST_MODE'],
)

test('microsoft-translator', test_microsoft_translator)

test_gsettings_sources = [
  'test-gsettings.c'
]

test_gsettings = executable('test-gsettings',
  test_gsettings_sources,
  dependencies: [
    dependency('gio-2.0', version: '>= 2.0'),
  ],
)

test('gsettings-translation', test_gsettings)

test_integration_sources = [
  'test-integration.c',
  'translation-service.c',
  'amazon-translator.c',
  'google-translator.c',
  'microsoft-translator.c',
  'deepl-translator.c'
]

test_integration = executable('test-integration',
  test_integration_sources,
  dependencies: livecaptions_deps,
  c_args: ['-DTEST_MODE'],
)

test('integration', test_integration)
