# custom packages
from framework.helpers import tensorflow_helper as tfh
from framework.helpers import logging_helper as lh

# core packages
import itertools
import json

# data & machine learning packages
import numpy as np

import tensorflow as tf

# local packages
from framework.enums import TrainingType
from framework.base.model_builder import ModelBuilder
from framework.ann.training_datasets_builder import ANNTrainingDatasetsBuilder
from framework.ann.training_params_builder import ANNTrainingParamsBuilder
from framework.ann.model_elements_builder import ANNModelElementsBuilder

# create a new, module-level logger
logger = lh.get_main_module_logger()

# helper class for managing a simple Artificial Neural Network (ANN) model
class ANNModelBuilder(ModelBuilder):
    def __init__(self, *args, **kwargs):
        super().__init__(
            *args, **kwargs,
            training_dataset_builder=ANNTrainingDatasetsBuilder, 
            training_params_builder=ANNTrainingParamsBuilder, 
            model_elements_builder=ANNModelElementsBuilder)

    # prepares the training data
    def prepare_training_data(self, training_type=TrainingType.Train):
        logger.info('Preparing training data...')

        # invoke the inherited version to get the common properties
        result = super().prepare_training_data(training_type=training_type)
        
        # restore an existing model in test mode
        if training_type == TrainingType.Test:
            logger.info('Looking for an existing model...')
            
            # try to restore a previously fully trained model
            if tfh.keras_model_exists(result.model_folder, 'h5'):
                logger.info('Restoring existing model...')
                result.model = tfh.restore_trained_keras_model(result.model_folder, 'h5')
            
            # try to restore a previous checkpoint
            elif tfh.keras_model_checkpoint_exists(result.model_folder):
                logger.info('Restoring model from last checkpoint...')
                result.model = tfh.restore_keras_model_from_checkpoint(result.model_folder)
            # raise an error if we cannot do it
            else:
                raise ValueError('Unable to find training data in the parameter model folder.', result.model_folder)

        return result
    
    def write_summary(self, training_data, out_folder=None):
        # use the model folder as the default output folder
        if out_folder is None:
            out_folder = training_data.model_folder

        logger.debug('Writing model summary to {}', out_folder)

        # log some model info
        with open(out_folder + '/model_summary.txt', 'w') as summary_file:
            summary_file.write('Model summary:\n')
            training_data.model.summary(print_fn=lambda line: summary_file.write(line + '\n'))

            trainable_count = int(np.sum([tf.keras.backend.count_params(p) for p in training_data.model.trainable_weights]))
            non_trainable_count = int(np.sum([tf.keras.backend.count_params(p) for p in training_data.model.non_trainable_weights]))

            logger.debug('Total number of params: {:,} ({:,} trainable, {:,} non-trainable)', 
                trainable_count + non_trainable_count, trainable_count, non_trainable_count)

        # log the config in the model's output folder
        with open(out_folder + '/run_config.txt', 'w') as params_file:
            params_file.write(json.dumps(self.config, indent=4))

    def _finish_training(self, training_data):
        # store the history entries in a csv file
        tfh.save_history_metric_csv(
                history=training_data.train_history.history, 
                folder=training_data.model_folder)
        
        num_epochs_trained = len(training_data.train_history.history['loss'])
        if (num_epochs_trained % training_data.num_checkpoint_save_epochs) != 0:
            # list of metric names to process
            metric_names = map(lambda metric: metric.name, training_data.metrics)

            # slack plot message pieces
            slack_msg = []

            # store the history entries in plots
            for metric in itertools.chain(['loss'], metric_names):
                # save the metric plot
                charts = tfh.save_history_metric_plot(
                    history=training_data.train_history.history, 
                    metric=metric, 
                    folder=training_data.model_folder, 
                    total_epochs=training_data.num_epochs)
                for filepath, title, description in charts:
                    slack_msg.append({ 'title': title, 'comment': description, 'filepath': filepath })

            # send a slack notification
            if self.config.slack_notifications:
                tfh.send_checkpoint_slack_message(
                    msg='Training finished', 
                    epoch=num_epochs_trained, 
                    total_epochs=training_data.num_epochs, 
                    history=training_data.train_history.history, 
                    metrics=metric_names, 
                    plots=slack_msg)

            # export the model
            logger.info('Saving trained model...')

            tfh.export_keras_model(training_data.model, training_data.model_folder)

    # trains the model, using the parameter training data
    def train(self, training_data):
        logger.info('Inititating model training for network "{}"...', training_data.model_name)

        # construct the network
        logger.info('Constructing model...')
        training_data.model.compile(**training_data.model_compile_args)
        
        # train the model
        logger.info('Training model...')
        training_data.train_history = training_data.model.fit(**training_data.model_train_args)

        # finish the training process
        logger.info('Training finished; performing final steps...')
        self._finish_training(training_data)

        # return the training data with the trained model
        return training_data

    # predicts via the model, using the parameter X data
    def predict(self, test_data, test_x):
        # call predict on the restored model
        logger.info('Predicting via the restored model...')
        return test_data.model.predict(x=test_x, **test_data.model_predict_args)
