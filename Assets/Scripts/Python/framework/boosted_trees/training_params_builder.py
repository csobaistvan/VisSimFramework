
# custom packages
from framework.helpers import logging_helper as lh

# local packages
from framework.base.training_params_builder import TrainingParamsBuilder

# create a new, module-level logger
logger = lh.get_main_module_logger()

# helper class for building training parameters
class BoostedTreesTrainingParamsBuilder(TrainingParamsBuilder):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def build(self):
        # get the common training parameters
        result = super().build()

        # return the final result
        return result
