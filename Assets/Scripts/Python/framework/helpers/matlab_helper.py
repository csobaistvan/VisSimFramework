import os, threading
import matlab.engine
from pathlib import Path

import logging as log
from . import folders_helper as fh
from . import logging_helper as lh
from . import winapi_helper as wah
from . import dotdict as dd

# access the main logger
logger = lh.get_main_module_logger()

# converts the thread Id to a name
def _get_thread_name(thread_id=None):
    return threading.current_thread().name if thread_id is None else 'Thread-{}'.format(thread_id)

# generates a unique file name for the current matlab instance
def _get_log_filename(thread_id=None):
    return lh.get_log_path('matlab.{thread}'.format(thread=_get_thread_name(thread_id)))

# get our custom MATLAB scripts folder
def _get_matlab_scripts_folder():
    scripts_folder = os.path.normpath(fh.get_scripts_folder() + "/Matlab")
    scripts_folder_path = Path(scripts_folder)
    return str(scripts_folder_path.as_posix())

# startup options
def _get_startup_options(thread_id=None):
    return '-nosplash -sd "{folder}" -singleCompThread -logfile "{logfile}"'.format(
        folder=fh.get_matlab_folder(),
        logfile=_get_log_filename(thread_id))

def _set_process_affinity(matlab_instance, thread_id):
    if thread_id is not None:
        pid = int(matlab_instance.feature('getpid'))
        wah.set_process_affinity(pid, { thread_id })

def _set_process_priority(matlab_instance, priority):
    pid = int(matlab_instance.feature('getpid'))
    wah.set_process_priority(pid, priority)

# adds the input path to the PATH of the input instance
def _add_folder_to_path(matlab_instance, path):
    matlab_instance.addpath(matlab_instance.genpath(path))

# configures the MATLAB path to make our custom scripts available
def _configure_matlab_path(matlab_instance):
    _add_folder_to_path(matlab_instance, fh.get_matlab_folder()) # custom MATLAB scripts folder

def check_matlab_instance(matlab_instance):
    #return matlab_instance._check_matlab()

    is_alive = True
    try:
        is_alive = (matlab_instance.power(2.0, 2.0)) == 4.0
    except:
        is_alive = False

    return is_alive

# starts a new MATLAB instance asynchronously; must be finalized using 'finish_matlab_async_startup'
def start_matlab_async(thread_id=None, set_processor_affinity=True, process_priority=wah.ProcessPriority.HIGH):
    future = dd.dotdict()
    future.thread_id = thread_id
    future.process_priority = process_priority
    future.set_processor_affinity = set_processor_affinity
    future.future = matlab.engine.start_matlab(background=True, option=_get_startup_options(thread_id))
    return future

# finishes the async startup process
def finish_matlab_async_startup(future):
    matlab_instance = future.future.result()
    _configure_matlab_path(matlab_instance)
    _set_process_priority(matlab_instance, future.process_priority)
    if future.set_processor_affinity:
        _set_process_affinity(matlab_instance, future.thread_id)
    return matlab_instance

# starts a new MATLAB instance with all the custom script folders properly added to PATH
def start_matlab(thread_id=None, set_processor_affinity=True, process_priority=wah.ProcessPriority.HIGH):
    return finish_matlab_async_startup(
        start_matlab_async(
            thread_id=thread_id, 
            set_processor_affinity=set_processor_affinity,
            process_priority=process_priority))

# calls the parameter MATLAB function with the input parameters, automatically wrapping the stdout and stderr values
def call_fn(fn, *args, **kwargs):
    return fn(*args, **kwargs)