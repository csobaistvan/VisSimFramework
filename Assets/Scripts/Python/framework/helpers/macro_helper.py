
from . import dotdict as dd

import re

# represents a single macro
class Macro(object):
    def __init__(self, name, value):
        self.name = name
        self.value = value

        self.format = '${{{name}}}'.format(name=name)
        self.count = 1

    def resolve(self, string):
        if callable(self.value):
            return string.replace(self.format, str(self.value()))
        return string.replace(self.format, str(self.value))

# global list of macros
_MACROS = dd.dotdict()

# resolves all the macro references in str
def resolve_macros(string, validate_result=True):
    # resolve all known macros
    for macro in _MACROS.values():
        string = macro.resolve(string)

    # Make sure all macros were resolved successfully
    if validate_result:
        match = re.search("\\$\\{\w*\\}", string)
        if match:
            raise RuntimeError("Unable to resolve macro reference '{}'".format(match[0]))

    # return the result
    return string

# registers a macro with the given name and value
def add_macro(name, value):
    global _MACROS
    if name in _MACROS:
        _MACROS[name].count += 1
    else:
        _MACROS[name] = Macro(name, value)

# unregisters a macro with a given name
def remove_macro(name):
    global _MACROS
    if not name in _MACROS:
        raise RuntimeError("Attempting to remove unknown macro '{name}'.".format(name))
    _MACROS[name].count -= 1
    if _MACROS[name].count == 0:
        del _MACROS[name]

# creates a scope with custom macros
class ScopedMacros(object):
    def __init__(self, macros):
        self.macros = macros

    def __enter__(self):
        for macro_name, macro_value in self.macros.items():
            add_macro(macro_name, macro_value)
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        for macro_name in self.macros.keys():
            remove_macro(macro_name)