# custom packages
from framework.helpers import tensorflow_helper as tfh
from framework.helpers import logging_helper as lh
from framework.helpers import modules_helper as mh
from framework.helpers.dotdict import dotdict

# core packages
from pathlib import Path

# local packages
from framework.enums import TrainingType

# create a new, module-level logger
logger = lh.get_main_module_logger()

# helper class for building training parameters
class TrainingParamsBuilder(object):
    def __init__(self, config, network_params, training_type, datasets):
        self.config = config
        self.network_params = network_params
        self.training_type = training_type
        self.datasets = datasets

    def build(self, fixed_params={}):
        result = dotdict()

        # update the result with the default parameters
        if len(fixed_params) > 0:
            result.set_fix_items(fixed_params)
    
        # store the network parameters
        result.network_params = self.network_params

        # store the training flag
        result.training_type  = self.training_type

        # compute the number of steps per epoch
        result.train_steps_per_epoch = (self.datasets.num_train_samples + self.datasets.train_batch_size - 1) // self.datasets.train_batch_size
        result.eval_steps_per_epoch = (self.datasets.num_eval_samples + self.datasets.eval_batch_size - 1) // self.datasets.eval_batch_size

        logger.debug('Number of training steps per epoch: {}', result.train_steps_per_epoch)
        logger.debug('Number of eval steps per epoch: {}', result.eval_steps_per_epoch)

        # total number of epochs
        result.num_epochs = self.network_params.num_epochs

        # compute the total number of training and test steps
        result.num_train_steps = None if self.network_params.num_epochs==0 else self.network_params.num_epochs * result.train_steps_per_epoch
        result.num_test_steps = None if self.network_params.num_epochs==0 else self.network_params.num_epochs * result.eval_steps_per_epoch
        
        # how often to eval
        result.num_eval_epochs = 1
        result.num_eval_steps = result.num_eval_epochs * result.train_steps_per_epoch

        logger.debug('Number of total training steps: {}', result.num_train_steps)
        
        # how often to save checkpoints
        result.num_checkpoint_save_epochs = max(self.network_params.num_epochs // (self.network_params.num_checkpoints + 1), 5)
        result.num_checkpoint_save_steps = result.num_checkpoint_save_epochs * result.train_steps_per_epoch

        # maximum number of checkpoints to keep
        result.num_max_checkpoints = 5

        # how often to log the loss
        result.num_log_steps = result.train_steps_per_epoch

        # how often to save summaries
        result.num_save_summaries_steps = result.train_steps_per_epoch
        
        # generate the folder name for the network itself
        result.network_folder = tfh.get_network_type_folder(
            module_name=mh.get_main_module(),
            network_type=self.network_params.type, 
            model_type=self.network_params.model_type)

        # generate a name for the model
        result.model_name = tfh.get_network_name(
            data_generator=self.config.data_generator, 
            network_params=self.network_params)

        # generate the folder name to store the files
        result.model_folder = tfh.get_network_folder(
            network_module=mh.get_main_module(),
            network_params=result.network_params,
            network_name=result.model_name)
        
        # generate the train and validation data folder paths
        result.model_folder_train = result.model_folder + 'train/'
        result.model_folder_validation = result.model_folder + 'validation/'

        # make sure the model folder exists
        if result.training_type != TrainingType.Tune:
            Path(result.model_folder).mkdir(parents=True, exist_ok=True)

        # model checkpoint paths
        result.checkpoints_path = result.model_folder + tfh.checkpoint_format(result.num_epochs)

        return result
