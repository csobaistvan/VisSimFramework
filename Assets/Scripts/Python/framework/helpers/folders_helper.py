import os
from pathlib import Path

import json, jstyleson

from . import macro_helper as mh

def _norm_path_posix(path):
    return str(Path(os.path.normpath(path)).as_posix())

def get_root_folder():
    current_folder = Path(os.getcwd()).absolute()
    while not (current_folder / 'premake5.lua').exists():
        current_folder = current_folder.parent
    return str(current_folder)

def get_assets_folder():
    return _norm_path_posix(get_root_folder() + "/Assets")

def get_exported_networks_folder():
    return _norm_path_posix(get_root_folder() + "/Assets/Generated/Networks")

def get_config_folder():
    return _norm_path_posix(get_root_folder() + "/.config")
    
def get_scripts_folder():
    return _norm_path_posix(get_assets_folder() + "/Scripts")

def get_python_folder():
    return _norm_path_posix(get_assets_folder() + "/Scripts/Python")

def get_matlab_folder():
    return _norm_path_posix(get_assets_folder() + "/Scripts/Matlab")

def get_large_files_folder():
    return _norm_path_posix(get_assets_folder() + "/Scripts/Python/Data")

# registers all the common macros
def _register_common_macros():
    mh.add_macro('workspace_root', get_root_folder())
    mh.add_macro('exported_networks', get_exported_networks_folder())
    mh.add_macro('assets', get_assets_folder())
    mh.add_macro('scrips', get_scripts_folder())
    mh.add_macro('python_scripts', get_python_folder())
    mh.add_macro('matlab_scripts', get_matlab_folder())
    mh.add_macro('large_file_storage', get_large_files_folder())

_register_common_macros()