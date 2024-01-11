# custom packages
from framework.helpers import logging_helper as lh

# create a new, module-level logger
logger = lh.get_main_module_logger()

# helper class for training models
class ModelTuner(object):
    def __init__(self, config, data_generator, model_tuner_class):
        self.config = config
        self.data_generator = data_generator
        self.network_params = self.config.network
        self.tune_params = self.config.tuning
        self.model_tuner_class = model_tuner_class

    def tune(self):
        # instantiate the tuner class
        tuner = self.model_tuner_class(
            config=self.config, 
            data_generator=self.data_generator, 
            network_params=self.network_params,
            tune_params=self.tune_params)

        # perform tuning on it
        tuner.tune()
        