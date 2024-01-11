# custom packages
from framework.helpers.dotdict import dotdict
from framework.helpers import logging_helper as lh

# create a new, module-level logger
logger = lh.get_main_module_logger()

# helper class for training models
class ModelTuner(object):
    def __init__(self, config, data_generator, network_params, tune_params):
        self.config = config
        self.data_generator = data_generator
        self.network_params = network_params
        self.tune_params = tune_params

    def tune(self):
        result = dotdict()
        return result
        