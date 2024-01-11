import logging as log
import psutil, os, sys, io, time
import threading
from pathlib import Path
from tensorflow.python.platform import tf_logging

from . import modules_helper as mh
from . import datetime_helper as dth

# generates a full path for a log file based on the input name
def get_log_path(logfile_name):
    # generate the log folder first
    result_folder = '{logfolder}/{date}'.format(
        logfolder='Log',
        date=dth.get_startup_time(fmt='filename'))

    # the full file path to the actual log file
    result_filename = '{logfolder}/{filename}.log'.format(
        logfolder=result_folder,
        filename=logfile_name)

    # create the corresponding directory structure
    result_folder_path = Path(result_folder)
    result_folder_path.mkdir(parents=True, exist_ok=True)

    # return the generated file path
    return result_filename

# custom wrapper class that acts as a StringIO object, but redirects messages to a logger
class LoggerWrapper(io.StringIO):
    def __init__(self, logger, log_level):
        super().__init__()
        self.buffer = ""
        self.logger = logger
        self.log_level = log_level

    def write(self, b):
        if b and not b.isspace():
            self.logger.log(self.log_level, b)

# class that only lets main thread messages through
class MainThreadFilter(log.Filter):
    def filter(self, record):
        return record.threadName == threading.main_thread().name

# from: https://stackoverflow.com/questions/45684364/how-to-log-number-with-commas-as-thousands-separators
class Message(object):
    def __init__(self, fmt, args):
        self.fmt = fmt
        self.args = args

    def __str__(self):
        return self.fmt.format(*self.args)

class StyleAdapter(log.LoggerAdapter):
    def __init__(self, logger, extra=None):
        super(StyleAdapter, self).__init__(logger, extra or {})

    def log(self, level, msg, *args, **kwargs):
        if self.isEnabledFor(level):
            msg, kwargs = self.process(msg, kwargs)
            self.logger._log(level, Message(msg, args), (), **kwargs)

# initializes the parameter logger object
def setup_logger(logger, module_name):
    # set the log level to the lowest possible
    logger.setLevel(log.DEBUG)
    
    # create the formatters
    chformatter = log.Formatter('[%(asctime)s][%(levelname)s][%(threadName)s]: %(message)s', '%Y-%m-%d %H:%M:%S')
    fileformatter = log.Formatter('[%(asctime)s][%(levelname)s][%(threadName)s][%(module)s::%(funcName)s@%(lineno)s]: %(message)s', '%Y-%m-%d %H:%M:%S')

    # create the filters
    chfilter = MainThreadFilter()

    # create the console handlers
    chstd = log.StreamHandler(stream=sys.__stdout__) # stdout
    chstd.setLevel(log.INFO)
    chstd.setFormatter(chformatter)
    chstd.addFilter(chfilter)
    logger.addHandler(chstd)
    cherr = log.StreamHandler(stream=sys.__stderr__) # stderr
    cherr.setLevel(log.ERROR)
    cherr.setFormatter(chformatter)
    cherr.addFilter(chfilter)
    logger.addHandler(cherr)

    # create the file handlers
    fhinf = log.FileHandler(get_log_path(module_name + '.info'), 'w') # log file for INFO+
    fhinf.setLevel(log.INFO)
    fhinf.setFormatter(fileformatter)
    logger.addHandler(fhinf)

    fhdeb = log.FileHandler(get_log_path(module_name + '.debug'), 'w') # log file for EVERYTHING
    fhdeb.setLevel(log.DEBUG)
    fhdeb.setFormatter(fileformatter)
    logger.addHandler(fhdeb)

    # wrap the logger in a style adapter
    logger = StyleAdapter(logger)

    return logger

# custom init routine for setting up logging
def setup_module_logger(module_name=None, override_stdio=False):
    # get the parent module name if no module name is supplied
    if module_name is None:
        module_name = mh.get_main_module()

    # create the logger object
    logger = log.getLogger(module_name)

    # setup logging
    logger = setup_logger(logger, module_name)

    # redirect stdout and stderr to these if we are in the main module
    if override_stdio:
        sys.stdout = LoggerWrapper(logger, log.INFO)
        sys.stderr = LoggerWrapper(logger, log.ERROR)

    return logger

# returns the main module logger
def get_main_module_logger():
    return StyleAdapter(log.getLogger(mh.get_main_module()))