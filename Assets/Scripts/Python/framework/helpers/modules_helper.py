import inspect

from . import macro_helper as mh

# returns the name of the main module
def get_main_module():
    return inspect.getmodulename(inspect.stack()[-1].filename)

# registers all the common macros
def _register_common_macros():
    mh.add_macro('module_name', get_main_module())

_register_common_macros()