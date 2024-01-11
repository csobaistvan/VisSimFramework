from . import dotdict as dd

# formats the input value
def _format_fn(val, level, formats, marker=None):
    # apply the formatting recursively
    if isinstance(val, list):
        return '\n'.join(_format_fn(item, level, formats, formats[type(val)]) for item in val)

    if isinstance(val, dict):
        return '\n'.join(_format_fn(item, level, formats, formats[type(val)]) for item in val.items())

    # simply return it in a 'key:value' format
    if isinstance(val, tuple):
        sep, ld = ('\n', 1) if (isinstance(val[1], list) or isinstance(val[1], dict)) and len(val[1]) > 0 else ('', 0)
        return '{indent}{marker} {bd}{name}{bd}: {sep}{value}'.format(
            indent=' ' * level * formats['indent_depth'], 
            marker=marker,
            bd=formats['bd'],
            name=val[0], 
            sep=sep,
            value=_format_fn(val[1], level + ld, formats))

    # simply return it in a 'value' format, without indentation
    if marker is None:
        return '{value:{format}}'.format(value=val, format=formats[type(val)])

    # return the formatted list element
    return '{indent}{marker} {value:{format}}'.format(
        indent=' ' * level * formats['indent_depth'], 
        marker=marker,
        value=val, 
        format=formats[type(val)])

# converts the parameter dict to a single markdown text
def dict_to_markdown(vals, heading=None, indent_depth=4, style='regular'):
    # type dependent formats
    formats_style = { 
        'regular': { 'it': '*', 'bd': '**', dict: '-', dd.dotdict: '-', list: '1.' }, 
        'slack': { 'it': '_', 'bd': '*', dict: '•', dd.dotdict: '•', list: '•' } 
        }
    formats = { 'indent_depth': indent_depth, int: ',', float: '', str: '', bool: '', **formats_style[style] }

    # convert the dict to text
    txt = _format_fn(vals, 0, formats)

    # format and args for the final formatting
    fmt = '{vals}'
    args = { 'vals': txt }

    # apply a header, if requested
    if not heading is None:
        fmt = '#{header}\n{vals}'
        args['header'] = heading

    # return the final formatted message
    return fmt.format(**args)

if __name__ == '__main__':
    vals = { 
        'a': 15, 
        'b': [ 20, 30 ], 
        'c': { 
            'foo': 15,
            'bar': 'baz',
            'lista': [ 20, 30 ],
            'listb': [ [ 20, 30 ], [ 40, 50 ] ]
            }
        }
    print(dict_to_markdown(vals, heading='Test dict.'))