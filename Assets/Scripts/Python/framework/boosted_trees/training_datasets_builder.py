
# custom packages
from framework.helpers import logging_helper as lh
from framework.helpers.dotdict import dotdict

#local packages
from framework.base.training_datasets_builder import TrainingDatasetsBuilder

# create a new, module-level logger
logger = lh.get_main_module_logger()

# helper class for building training datasets
class BoostedTreesTrainingDatasetsBuilder(TrainingDatasetsBuilder):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def build(self):
        # Manually set the batch size if 'train_in_memory' is enabled
        fixed_params = dotdict()
        if self.network_params.train_in_memory:
            fixed_params.batch_size = 2 ** 31

        # get the common training parameters
        result = super().build(fixed_params=fixed_params)

        # return the final result
        return result
