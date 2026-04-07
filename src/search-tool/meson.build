conf.set10('HAVE_SEARCH_TOOL', true)

shared_module('search-tool',
  'library.cc',
  'search-model.cc',
  'search-tool.cc',
  dependencies: [audacious_dep, gtk_dep, audgui_dep],
  name_prefix: '',
  install: true,
  install_dir: general_plugin_dir
)
