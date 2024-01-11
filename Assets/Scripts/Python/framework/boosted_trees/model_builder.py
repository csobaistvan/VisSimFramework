
# custom packages
from framework.helpers import logging_helper as lh

# core packages
import json

# data & machine learning packages
import tensorflow as tf

# local packages
from framework.enums import TrainingType
from framework.base.model_builder import ModelBuilder
from framework.boosted_trees.training_datasets_builder import BoostedTreesTrainingDatasetsBuilder
from framework.boosted_trees.training_params_builder import BoostedTreesTrainingParamsBuilder
from framework.boosted_trees.model_elements_builder import BoostedTreesModelElementsBuilder

# create a new, module-level logger
logger = lh.get_main_module_logger()

# helper class for managing the Gradient Boosted Tress model
class BoostedTreesModelBuilder(ModelBuilder):
    def __init__(self, *args, **kwargs):
        super().__init__(
            *args, **kwargs,
            training_dataset_builder=BoostedTreesTrainingDatasetsBuilder, 
            training_params_builder=BoostedTreesTrainingParamsBuilder, 
            model_elements_builder=BoostedTreesModelElementsBuilder)

    # prepares the training data
    def prepare_training_data(self, training_type=TrainingType.Train):
        result = super().prepare_training_data(training_type=training_type)
        return result

    def _write_train_config_summary(self, model, model_folder):
        # print out the run configuration
        with open(model_folder + 'run_config.txt', 'w') as params_file:
            params_file.write(json.dumps(self.config, indent=4))
    
    # trains the model, using the parameter training data
    def train(self, training_data):
        # write out the run config summary
        self._write_train_config_summary(training_data.model, training_data.model_folder)

        # train the network
        logger.info('Training model...')
        training_data.train_history = tf.estimator.train_and_evaluate(
            estimator=training_data.model,
            train_spec=training_data.train_spec,
            eval_spec=training_data.eval_spec)

        # return the training data with the training model
        return training_data

    # predicts via the model, using the parameter X data
    def predict(self, test_data, test_x):
        raise RuntimeError('Predict currently unimplemented for BoostedTrees models')
