# custom packages
from framework.helpers import logging_helper as lh
from framework.helpers.dotdict import dotdict

# create a new, module-level logger
logger = lh.get_main_module_logger()

# helper class for building the various model elements
class ModelElementsBuilder(object):
    def __init__(self, config, network_params, training_params, datasets):
        self.config = config
        self.network_params = network_params
        self.training_params = training_params
        self.datasets = datasets

    def build(self):
        result = dotdict()
        return result
