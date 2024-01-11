
# Regular dotdict (dictionary with support for dict.item syntax)
# Also allows certain items to be fixed, so that they cannot be overridden
class dotdict(dict):
    __getattr__ = dict.__getitem__
    __delattr__ = dict.__delitem__

    def set_fix_items(self, items):
        self.update(items)
        self._fixed_items = items

    def __setattr__(self, name, value):
        if '_fixed_items' not in self or ('_fixed_items' in self and name not in self._fixed_items):
            super().__setitem__(name, value)
