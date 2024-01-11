# custom packages
from framework.helpers import logging_helper as lh

# local packages
from framework.base.model_tuner import ModelTuner

# create a new, module-level logger
logger = lh.get_main_module_logger()

# helper class for building training datasets
class BoostedTreesModelTuner(ModelTuner):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    # performs hyperparameter tuning
    def tune(self):
        raise RuntimeError('Hyperparameter tuning is currently unsupported for BoostedTrees models')