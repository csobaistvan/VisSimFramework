# custom packages
from framework.helpers import logging_helper as lh

# data & machine learning packages
import tensorflow as tf

# local packages
from framework.enums import TrainingType
from framework.base.training_datasets_builder import TrainingDatasetsBuilder

# create a new, module-level logger
logger = lh.get_main_module_logger()

# helper class for building training datasets
class ANNTrainingDatasetsBuilder(TrainingDatasetsBuilder):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def _get_train_datasets(self, datasets):
        return (datasets.x_train, datasets.y_train) if self.training_type == TrainingType.Test else (datasets.x_train_normalized, datasets.y_train_normalized)

    def _get_eval_datasets(self, datasets):
        return (datasets.x_eval, datasets.y_eval)

    def build(self):
        # get the common training parameters
        result = super().build()

        # construct the training dataset
        result.train_dataset = tf.data.Dataset \
            .from_tensor_slices(self._get_train_datasets(result)) \
            .cache() \
            .shuffle(result.num_train_samples, seed=self.network_params.dataset.random_seeds.shuffle, reshuffle_each_iteration=True) \
            .batch(result.train_batch_size)

        # construct the eval dataset
        result.eval_dataset = tf.data.Dataset \
            .from_tensor_slices(self._get_eval_datasets(result)) \
            .cache() \
            .shuffle(result.num_eval_samples, seed=self.network_params.dataset.random_seeds.shuffle, reshuffle_each_iteration=True) \
            .batch(result.eval_batch_size)

        # return the final result
        return result
