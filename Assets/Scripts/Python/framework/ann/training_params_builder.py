# custom packages
from framework.helpers import logging_helper as lh

# local packages
from framework.base.training_params_builder import TrainingParamsBuilder

# create a new, module-level logger
logger = lh.get_main_module_logger()

# helper class for building training parameters
class ANNTrainingParamsBuilder(TrainingParamsBuilder):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def build(self):
        # get the common training parameters
        result = super().build()
        
        # how many steps to take per execution
        steps_per_execution = 1
        if 'noneager_steps_per_exec' in self.network_params:
            if isinstance(self.network_params.noneager_steps_per_exec, int):
                steps_per_execution = self.network_params.noneager_steps_per_exec
            elif isinstance(self.network_params.noneager_steps_per_exec, float):
                steps_per_execution = int(result.train_steps_per_epoch * self.network_params.noneager_steps_per_exec)
            steps_per_execution = min(result.train_steps_per_epoch, steps_per_execution)
        result.steps_per_execution = steps_per_execution

        # return the final result
        return result
