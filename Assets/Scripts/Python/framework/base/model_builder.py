# custom packages
from framework.helpers import logging_helper as lh
from framework.helpers.dotdict import dotdict

# local packages
from framework.enums import TrainingType

# create a new, module-level logger
logger = lh.get_main_module_logger()

# helper class for managing models
class ModelBuilder(object):
    def __init__(self, config, data_generator, network_params, training_dataset_builder, training_params_builder, model_elements_builder):
        self.config = config
        self.data_generator = data_generator
        self.network_params = network_params
        if network_params is None:
            self.network_params = self.config.network
        self.training_dataset_builder = training_dataset_builder
        self.training_params_builder = training_params_builder
        self.model_elements_builder = model_elements_builder

    def write_summary(self, training_data, out_folder=None):
        pass

    # prepares data for the training process
    def prepare_training_data(self, training_type=TrainingType.Train):
        # resulting struct and sub-structs
        result = dotdict()

        logger.info("Preparing training data for '{}'...", self.config.network.type)

        # generate the necessary training data
        training_datasets_builder = self.training_dataset_builder(
            config=self.config,
            network_params=self.network_params,
            param_info=self.data_generator.param_info,
            dataset_name=self.data_generator.get_file_name_prefix(),
            raw_datasets=self.data_generator.generate_training_data(),
            training_type=training_type)
        datasets = training_datasets_builder.build()
        result.update(datasets)

        # get the training parameters
        training_params_builder = self.training_params_builder(
            config=self.config,
            network_params=self.network_params,
            training_type=training_type,
            datasets=datasets)
        training_params = training_params_builder.build()
        result.update(training_params)

        # the various activation functions available
        model_elements_builder = self.model_elements_builder(
            config=self.config,
            network_params=self.network_params,
            training_params=training_params,
            datasets=datasets)
        model_elements = model_elements_builder.build()
        result.update(model_elements)
  
        # return the prepared data
        return result
