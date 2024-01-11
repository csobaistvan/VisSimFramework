import argparse, json, jstyleson, re
from pathlib import Path

from . import dotdict as dd
from . import logging_helper as lh
from . import modules_helper as mh

# access the main logger
logger = lh.get_main_module_logger()
args = None

def _config_file_exists(filename):
    return Path(filename).exists()

def _base_config_filepath(filename):
    return 'Data/Config/{config}.json'.format(
        config=filename)

def _common_config_filepath(filename):
    return 'Data/Config/common/{config}.json'.format(
        config=filename)

def _module_config_filepath(filename):
    return 'Data/Config/{module}/{module}.{config}.json'.format(
        module=mh.get_main_module(), 
        config=filename)

# list of possible filepaths to search for
def _possible_filepaths(filename):
    return [ 
        _base_config_filepath(filename), 
        _common_config_filepath(filename), 
        _module_config_filepath(filename) 
    ]

# try to search for an existing filepath
def _config_file_path(filename):
    return next((filepath for filepath in _possible_filepaths(filename) if _config_file_exists(filepath)), None)

def _open_config_file(filename):
    # make sure at least one of them exists
    config_filepath = _config_file_path(filename)
        
    # make sure the config file exists
    if config_filepath is None:
        logger.error("No configuration file exists for '{}'; expected file location: '{}'", filename, _possible_filepaths(filename))
        raise ValueError("Invalid config file name.")

    logger.info("Reading configuration from '{}'...", config_filepath)

    return jstyleson.load(open(config_filepath))

# turn all the dicts to dotdicts
def _postprocess_config(elem):
    # wrap dicts in a dotdict and apply this transformation recursively
    if type(elem) is dict:
        return dd.dotdict({ key: _postprocess_config(val) for key, val in elem.items() })

    # do this recursively for arrays
    elif type(elem) is list:
        return [ _postprocess_config(val) for val in elem ]

    # expand command-line arguments
    if type(elem) is str:
        pattern = '\\$([0-9])+'
        match = re.search(pattern, elem)
        while match:
            arg_id = int(match[1])
            arg = args.config_params[arg_id] if arg_id < len(args.config_params) else ''
            elem = elem[:match.start(0)] + arg + elem[match.end(0):]
            match = re.search(pattern, elem)

    # load extra config files
    if type(elem) is str and 'config:' in elem:
        filepath = elem.replace('config:', '')
        return _postprocess_config(_open_config_file(filepath))
    elif type(elem) is str and 'config?:' in elem:
        filepath = elem.replace('config?:', '')
        if _config_file_path(filepath) is None:
            return ""
        return _postprocess_config(_open_config_file(filepath))

    # return everything else unmodified
    return elem

def load_config(default_config_name=None):
    # parse the current run mode
    logger.info('Parsing arguments...')

    parser = argparse.ArgumentParser(
        description='''Performs the learning step of the ML eye reconstruction & aberration estimation process.''',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument(
        'config_name', type=str, default=default_config_name, nargs='?',
        help='Name of the configuration to run.'
    )
    parser.add_argument(
        'config_params', type=str, default='', nargs='*',
        help='Additional config parameters.'
    )

    global args
    args = parser.parse_args()

    # read the config file and perform post-processing on it
    config = _postprocess_config(_open_config_file(args.config_name))

    logger.debug('Run parameters: {}', json.dumps(config, indent=4))

    # return the result
    return config