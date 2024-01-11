# custom packages
from framework.helpers import logging_helper as lh

# local packages
from framework.enums import TrainingType

# create a new, module-level logger
logger = lh.get_main_module_logger()

# helper class for training models
class ModelTrainer(object):
    def __init__(self, config, data_generator, model_builder_class):
        self.config = config
        self.network_params = self.config.network
        self.data_generator = data_generator
        self.model_builder_class = model_builder_class

    def train(self):
        # instantiate the model builder
        model = self.model_builder_class(
            config=self.config, 
            data_generator=self.data_generator, 
            network_params=self.network_params)

        # prepare the necessary training data
        training_data = model.prepare_training_data(
            training_type=TrainingType.Train)

        # write out the run config summary
        model.write_summary(training_data)

        # perform training
        model.train(training_data=training_data)