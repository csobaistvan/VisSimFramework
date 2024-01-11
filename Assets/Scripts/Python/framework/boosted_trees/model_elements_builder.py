
# custom packages
from framework.helpers import tensorflow_helper as tfh
from framework.helpers import logging_helper as lh

# core packages
import functools

# data & machine learning packages
import pandas as pd

import tensorflow as tf

# local packages
from framework.base.model_elements_builder import ModelElementsBuilder

# create a new, module-level logger
logger = lh.get_main_module_logger()

# helper class for building the various model elements
class BoostedTreesModelElementsBuilder(ModelElementsBuilder):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def _create_input_fn(self, dataset, repeats):
        x = getattr(self.datasets, 'x_{dataset}_normalized'.format(dataset=dataset))
        y = getattr(self.datasets, 'y_{dataset}_normalized'.format(dataset=dataset))
        batch_size = getattr(self.datasets, '{dataset}_batch_size'.format(dataset=dataset))
        x_df = pd.DataFrame(x, columns=self.datasets.feature_names)
        y_df = pd.DataFrame(y, columns=self.datasets.target_names)

        return functools.partial(
            tfh.estimator_input_generator_fn, 
            x_df=x_df,
            y_df=y_df,
            batch_size=batch_size,
            num_repeats=repeats,
            in_memory=self.network_params.train_in_memory)

    def build(self):
        # get the common elements
        result = super().build()

        # create the train and eval input fns
        result.train_input_fn = self._create_input_fn('train', None)
        result.eval_input_fn = self._create_input_fn('eval', 1)

        ## Add the metrics to the estimator
        result.metrics = [
            tfh.estimator_metric_mean_absolute_error,
            tfh.estimator_metric_mean_absolute_percentage_error,
            tfh.estimator_metric_mean_squared_error,
        ]
        
        #for metric in result.metrics:
        #    result.model = tf.estimator.add_metrics(result.model, metric)

        # construct the loss object
        #result.losses = {
        #    'MAE': tfh.estimator_mean_absolute_error,
        #    'MAPE': tfh.estimator_mean_absolute_percentage_error,
        #    'MSE': tfh.estimator_mean_squared_error
        #}
        #result.loss = result.losses[self.network_params.loss.function]

        # create the regressor head
        #result.head = tf.estimator.RegressionHead(
        #    label_dimension=self.datasets.num_targets,
        #    loss_fn=result.loss)

        # create a run configuration
        result.run_config = tf.estimator.RunConfig(
            model_dir=self.training_params.model_folder,
            tf_random_seed=self.network_params.random_seed,
            save_summary_steps=self.training_params.num_save_summaries_steps,
            save_checkpoints_steps=self.training_params.num_checkpoint_save_steps,
            keep_checkpoint_max=self.training_params.num_max_checkpoints,
            log_step_count_steps=self.training_params.num_log_steps,
            session_config=None)

        # construct the list of model arguments to use
        result.model_args = {
            'feature_columns': self.datasets.feature_columns,
            'n_batches_per_layer': self.training_params.train_steps_per_epoch,
            'label_dimension': self.datasets.num_targets,
            'model_dir': self.training_params.model_folder,
            'n_trees': self.network_params.tree_params.num_trees,
            'max_depth': self.network_params.tree_params.max_tree_depth,
            'learning_rate': self.network_params.learning_rate,
            'l1_regularization': self.network_params.regularization.L1,
            'l2_regularization': self.network_params.regularization.L2,
            'tree_complexity': self.network_params.tree_params.tree_complexity,
            'min_node_weight': self.network_params.tree_params.min_node_weight,
            'pruning_mode': self.network_params.tree_params.pruning,
            'center_bias': self.network_params.tree_params.center_bias,
            'train_in_memory': self.network_params.train_in_memory,
            'quantile_sketch_epsilon': self.network_params.tree_params.quantile_sketch_epsilon,
            'config': result.run_config }
        if self.network_params.train_in_memory:
            result.model_args['n_batches_per_layer'] = 1

        # construct the model
        result.model = tf.estimator.BoostedTreesRegressor(**result.model_args)

        # construct the list of checkpoint listeners
        result.saving_listeners = []

        # construct the list of hooks to use while training
        result.train_hooks = []
        
        # construct the TrainSpec object for training
        result.train_spec = tf.estimator.TrainSpec(
            input_fn=result.train_input_fn,
            max_steps=self.training_params.num_train_steps,
            hooks=result.train_hooks,
            saving_listeners=result.saving_listeners)

        # construct the list of hooks to use while evaluating
        result.eval_hooks = []

        # construct the EvalSpec object for evaluating
        result.eval_spec = tf.estimator.EvalSpec(
            input_fn=result.eval_input_fn,
            steps=self.training_params.eval_steps_per_epoch,
            hooks=result.eval_hooks,
            start_delay_secs=120,
            throttle_secs=120)

        # return the final result
        return result
