import os, multiprocessing, csv
from pathlib import Path
import slackblocks

import tensorflow as tf
from tensorflow.python.keras.optimizer_v2.nadam import Nadam
from tensorflow.python.types import internal
import tensorflow_addons as tfa

from tensorflow.python.eager import backprop
from tensorflow.python.autograph.core import ag_ctx
from tensorflow.python.autograph.impl import api as autograph
from tensorflow.python.framework import dtypes
from tensorflow.python.framework import ops
from tensorflow.python.framework import tensor_util
from tensorflow.python.keras import backend as K
from tensorflow.python.keras.engine import base_layer
from tensorflow.python.keras.engine import base_layer_utils
from tensorflow.python.keras.engine import compile_utils
from tensorflow.python.keras.engine import data_adapter
from tensorflow.python.keras.engine import training_utils
from tensorflow.python.keras.utils import losses_utils
from tensorflow.python.keras.utils import tf_utils
from tensorflow.python.ops import math_ops
from tensorflow.python.ops.losses import util as tf_losses_util
from tensorflow.python.training import training_util
from tensorboard import program

from qhoptim.tf.qhm import QHMOptimizer
from qhoptim.tf.qhadam import QHAdamOptimizer
from framework.modules.tensorflow.RectifiedAdam import RectifiedAdam as RAdam
from framework.modules.tensorflow.AdaHessian import AdaHessian as AdaHessian

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import keras_tuner as kt

from . import logging_helper as lh
from . import markdown_helper as mdh
from . import slack_helper as sh
from . import datetime_helper as dth
from . import folders_helper as fh
from . import filename_helpers as fnh

# access the main logger
logger = lh.get_main_module_logger()

# float type
float = tf.float32
float_np = np.float32

# initializes TF
def init_tensorflow():
    logger.info('Running on TF {}, using Keras {}', tf.__version__, tf.keras.__version__)

    # set the log level
    tf.compat.v1.logging.set_verbosity(tf.compat.v1.logging.INFO)
    os.environ['TF_CPP_MIN_LOG_LEVEL'] = '2'
    os.environ['TF_XLA_FLAGS'] = '--tf_xla_enable_xla_devices'

    # set the float type
    tf.keras.backend.set_floatx(float.name)

    # set the config vars
    tf.config.run_functions_eagerly(False)
    tf.config.optimizer.set_jit(False)
    tf.config.optimizer.set_experimental_options({
        'layout_optimizer': True,
        'constant_folding': True,
        'shape_optimization': True,
        'remapping': True,
        'arithmetic_optimization': True,
        'dependency_optimization': True,
        'loop_optimization': True,
        'function_optimization': True,
        'debug_stripper': True })

    # register all custom objects
    tfa.register.register_all()
    tf.keras.utils.register_keras_serializable()

#region Common model properties

# returns the target folder for the parameter network
def get_network_type_folder(module_name, network_type, model_type):
    return os.path.normpath("{lfs}/Models/{module_name}.{network_type}/{model_type}/".format(
        lfs=fh.get_large_files_folder(),
        module_name=module_name,
        network_type=network_type, 
        model_type=model_type))

# returns the name corresponding to the target network
def get_network_name(data_generator, network_params):
    if 'name' in network_params and network_params.name:
        return network_params.name
    return fnh.hash_filename(str({ **data_generator, **network_params }))

def get_tuner_name(network_params, tune_params):
    return fnh.hash_filename(str({ **network_params, **tune_params }))

# generates folder name for a model
def get_network_folder(network_module, network_name, network_params=None, network_type=None, model_type=None):
    if network_params is not None:
        network_folder = get_network_type_folder(network_module, network_params.type, network_params.model_type)
    else:
        network_folder = get_network_type_folder(network_module, network_type, model_type)

    return "{folder}/{name}/".format(folder=network_folder, name=network_name)

# returns the checkpoint format for the parameter number of epochs
def checkpoint_format(num_epochs, current_epoch=None):
    result = 'model.ckpt-{epoch:0%dd}' % + len(str(num_epochs))
    if not current_epoch is None:
        return result.format(epoch=current_epoch)
    return result

#endregion

#region TensorBoard

def _launch_tensorboard(model_dir, port):
    logger.info("Launching TensorBoard in '{}'; port: {}", model_dir, port)

    # disable logging
    tf.compat.v1.logging.set_verbosity(tf.compat.v1.logging.ERROR)

    # create the tensorboard program and configure it
    tb = program.TensorBoard()
    tb.configure(argv=[None, 
        '--logdir', model_dir,
        '--port', str(port)])
    tb.main()

# launch tensorboard for t he parameter model type
def launch_tensorboard(module_name, network_type, model_type, port, as_daemon=False):
    # construct the model directory
    model_dir = get_network_type_folder(module_name, network_type, model_type)

    logger.info("Launching TensorBoard in '{}'; port: {}", model_dir, port)
    
    # launch TB as a detached daemon
    if as_daemon:
        p = multiprocessing.Process(
            target=_launch_tensorboard,
            daemon=True,
            kwargs={
                'model_dir': model_dir,
                'port': port
            })
        p.start()
    # start TB in this process
    else:
        _launch_tensorboard(model_dir, port)

#endregion


#region Training loss saving

# converts the parameter history entry into a plot and saves it to disk
def save_history_metric_csv(history, folder, **kwargs):
    num_epochs_trained = len(history['loss'])
    metrics = list(history.keys())

    header = ['epoch'] + metrics
    with open('{folder}/metrics.csv'.format(folder=folder), 'w', newline='', encoding='utf-8') as csv_file:
        csv_writer = csv.writer(csv_file, delimiter=';')
        csv_writer.writerow(header)
        for i in range(num_epochs_trained):
            epoch_row = [str(i + 1)] + [history[metric][i] for metric in metrics]
            csv_writer.writerow(epoch_row)

# converts the parameter history entry into a plot and saves it to disk
def save_history_metric_plot(history, metric, folder, total_epochs=0, dpi=400.0, major_ticks=5, minor_ticks=5):
    result = []

    # various metadata for the metrics available
    metrics = {
        'loss': {
            'title': {
                'train': 'Training {metric}',
                'eval': 'Eval {metric}'
            }
        },
        'default': {
            'title': {
                'train': 'Model {metric}',
                'eval': 'Eval {metric}'
            }
        }
    }

    # extract the metadata that we need to use for this metric
    metric_meta = metrics['default']
    metric_name = metric
    if metric_name.lower() in metrics:
        metric_meta = metrics[metric_name.lower()]

    # build the list of charts to save
    charts = { 
        'loss': { 
            'domain': 'train', 
            'prefix': ''
        }, 
        'eval': { 
            'domain': 'eval', 
            'prefix': 'val_'
        }, 
    }
    charts = { key: val for key, val in charts.items() if any(map(lambda metric: val['prefix'] in metric, history.keys())) }

    # how many epochs performed
    num_epochs = len(history['loss'])

    for _, chart_params in charts.items():
        # unpack the chart params
        domain, prefix = (chart_params['domain'], chart_params['prefix'])

        # meta attributes
        filename = 'history_{prefix}{metric}.png'.format(prefix=prefix, metric=metric_name)
        filepath = folder + filename
        title_fmt = metric_meta['title'][domain]
        title = title_fmt.format(metric=metric_name)
        desc_format = title_fmt + " after {epochs}/{total_epochs} epochs."
        description = desc_format.format(metric=metric_name, epochs=num_epochs, total_epochs=total_epochs)

        # create the history plot
        fig = plt.figure(dpi=dpi)
        ax = fig.add_subplot(1, 1, 1)

        # extract and observe the recorded history entries
        hist_entries = history[prefix + metric_name]
        num_hist_entries = len(hist_entries)
        freq = num_epochs // num_hist_entries
        
        # plot them
        plt.plot(list(range(freq, (num_hist_entries + 1) * freq, freq)), hist_entries)

        # compute the min/max x and y values
        x_min, x_max = (1, num_epochs)
        y_min, y_max = (min(hist_entries), max(hist_entries))

        # configure the plot
        plt.title(title)
        plt.ylabel(metric_name)
        plt.xlabel('epoch')
        #plt.legend(charts.keys(), loc='upper right')

        # x ticks
        x_major_steps = np.ceil((x_max - x_min) / major_ticks)
        x_minor_steps = np.ceil((x_max - x_min) / (minor_ticks * major_ticks))
        x_major_ticks = np.arange(x_min, x_max + 1, x_major_steps)
        x_minor_ticks = np.arange(x_min, x_max + 1, x_minor_steps)
        ax.set_xticks(x_major_ticks)
        ax.set_xticks(x_minor_ticks, minor=True)

        # y ticks
        y_major_ticks = np.linspace(y_min, y_max, major_ticks + 1)
        y_minor_ticks = np.linspace(y_min, y_max, (major_ticks + 1) * (minor_ticks + 1))
        ax.set_yticks(y_major_ticks)
        ax.set_yticks(y_minor_ticks, minor=True)

        # grid
        ax.grid(which='minor', alpha=0.2)
        ax.grid(which='major', alpha=0.5)

        # save it
        plt.savefig(filepath)

        # close the figure
        plt.close()

        # add the figure to the list of generated figs
        result.append((filepath, title, description))

    return result

# saves each parameter metric from the history to a plot on disk
def save_history_metrics_plot(history, metrics, folder, **kwargs):
    for metric in metrics:
        save_history_metric_plot(history, metric, folder, **kwargs)

#endregion

#region Slack notification

def send_checkpoint_slack_message(msg, epoch, total_epochs, history, metrics, plots):
    train_status = {
        'Epochs': '{total}/{epochs}'.format(epochs=epoch, total=total_epochs),
        'Start time': dth.get_startup_time(),
        'Current time': dth.get_current_time(),
        'Total elapsed time': dth.get_elapsed_time()
    }
    for metric in metrics:
        metric_name = metric.name
        best_id = np.argmin(history[metric_name])
        best_val_id = np.argmin(history['val_' + metric_name])
        train_status['Best ' + metric_name] = '{} (eval: {})'.format(
            history[metric_name][best_id], 
            history['val_' + metric_name][best_val_id])
    slack_msg = [ 
        slackblocks.HeaderBlock(msg), 
        sh.dict_block(train_status), 
        slackblocks.DividerBlock(),
        *plots ]
    sh.send_message(slack_msg)

#endregion

#region Estimator

#region Estimator loss functions

def _estimator_loss_mae(labels, logits):
    return math_ops.abs(logits - labels)
    
def _estimator_loss_mape(labels, logits):
    return 100.0 * (math_ops.abs(logits - labels) / labels)

def _estimator_loss_mse(labels, logits):
    return math_ops.squared_difference(labels, logits)

def _estimator_loss_fn(labels, logits, loss_fn):
    if labels is None:
        raise ValueError("labels must not be None.")
    if logits is None:
        raise ValueError("logits must not be None.")
    logits = math_ops.cast(logits, dtype=dtypes.float32)
    labels = math_ops.cast(labels, dtype=dtypes.float32)
    logits.get_shape().assert_is_compatible_with(labels.get_shape())
    return loss_fn(logits, labels)

def estimator_loss_mean_absolute_error(labels, logits):
    return _estimator_loss_fn(labels, logits, _estimator_loss_mae)

def estimator_loss_mean_absolute_percentage_error(labels, logits, features):
    return _estimator_loss_fn(labels, logits, _estimator_loss_mape)

def estimator_loss_mean_squared_error(labels, logits, features):
    return _estimator_loss_fn(labels, logits, _estimator_loss_mse)

#endregion

#region Estimator metrics

def _estimator_metric_mae(labels, predictions):
    return math_ops.abs(predictions - labels)
    
def _estimator_metric_mape(labels, predictions):
    return 100.0 * (math_ops.abs(predictions - labels) / labels)

def _estimator_metric_mse(labels, predictions):
    return math_ops.squared_difference(labels, predictions)

def _estimator_metric_fn(labels, predictions, loss_fn):
    if labels is None:
        raise ValueError("labels must not be None.")
    if predictions is None:
        raise ValueError("predictions must not be None.")
    predictions = math_ops.cast(predictions, dtype=dtypes.float32)
    labels = math_ops.cast(labels, dtype=dtypes.float32)
    predictions.get_shape().assert_is_compatible_with(labels.get_shape())
    return loss_fn(predictions, labels)

def estimator_metric_mean_absolute_error(labels, predictions):
    return _estimator_metric_fn(labels, predictions, _estimator_metric_mae)

def estimator_metric_mean_absolute_percentage_error(labels, predictions, features):
    return _estimator_metric_fn(labels, predictions, _estimator_metric_mape)

def estimator_metric_mean_squared_error(labels, predictions, features):
    return _estimator_metric_fn(labels, predictions, _estimator_metric_mse)

#endregion

#region Activation functions

@tf.keras.utils.register_keras_serializable(package="Helpers")
def tanh2(x):
    return 2.0 * tf.nn.tanh(x)

@tf.keras.utils.register_keras_serializable(package="Helpers")
def tanh3(x):
    return 3.0 * tf.nn.tanh(x)

@tf.keras.utils.register_keras_serializable(package="Helpers")
def tanhsq2(x):
    return 1.4142135623730950488016887242097 * tf.nn.tanh(x)

@tf.keras.utils.register_keras_serializable(package="Helpers")
def tanhsq3(x):
    return 1.7320508075688772935274463415059 * tf.nn.tanh(x)

#endregion

#region Estimator input functions

# TF estimator input generator function, using batches and shuffling
def estimator_input_generator_fn_batched(x_df, y_df, batch_size, num_repeats):
    return tf.data.Dataset \
        .from_tensor_slices((x_df.to_dict('list'), y_df.values)) \
        .cache() \
        .shuffle(x_df.shape[0]) \
        .batch(batch_size) \
        .repeat(num_repeats)

# TF estimator input generator function, using batches and shuffling
def estimator_input_generator_fn_oneshot(x_df, y_df, num_repeats):
    return tf.data.Dataset \
        .from_tensor_slices((x_df.to_dict('list'), y_df.values)) \
        .cache() \
        .batch(x_df.shape[0]) \
        .repeat(num_repeats)

# TF estimator input generator function
def estimator_input_generator_fn_inmemory(x_df, y_df, num_repeats):
    return (x_df.to_dict('list'), y_df.values)

# TF estimator input generator function
def estimator_input_generator_fn(x_df, y_df, batch_size, num_repeats, in_memory):
    if in_memory:
        return estimator_input_generator_fn_inmemory(x_df, y_df, num_repeats)
    elif batch_size == None or batch_size <= 0 or batch_size >= x_df.shape[0]:
        return estimator_input_generator_fn_oneshot(x_df, y_df, num_repeats)
    else:
        return estimator_input_generator_fn_batched(x_df, y_df, batch_size, num_repeats)

#endregion

#endregion

#region Keras learning rate schedules

@tf.keras.utils.register_keras_serializable(package="Helpers")
class ConstantLearningRate(tf.keras.optimizers.schedules.LearningRateSchedule):

    def __init__(self, learning_rate, name=None):
        super().__init__()
        self._serialized_properties = [ 'name', 'learning_rate' ]

        self.learning_rate = learning_rate
        self.name = name

    def __call__(self, step):
        with ops.name_scope_v2(self.name or "ExponentialDecay") as name:
            return ops.convert_to_tensor_v2(self.learning_rate, name="learning_rate")

    def get_config(self):
        return get_properties_for_config(self)

# Learning rate warmup
# based on: https://github.com/tensorflow/models/blob/master/official/nlp/optimization.py
@tf.keras.utils.register_keras_serializable(package="Helpers")
class LearningRateWarmUp(tf.keras.optimizers.schedules.LearningRateSchedule):
    """Applies a warmup schedule on a given learning rate decay schedule."""

    def __init__(self,
                initial_learning_rate,
                decay_schedule_fn,
                warmup_steps,
                power=1.0,
                name=None):
        super(LearningRateWarmUp, self).__init__()
        self._serialized_properties = [ 'initial_learning_rate', 'decay_schedule_fn', 'warmup_steps', 'power', 'name' ]

        self.initial_learning_rate = initial_learning_rate
        self.warmup_steps = warmup_steps
        self.power = power
        self.decay_schedule_fn = decay_schedule_fn
        self.name = name

    def __call__(self, step):
        with tf.name_scope(self.name or 'LearningRateWarmUp') as name:
            # Implements polynomial warmup. i.e., if global_step < warmup_steps, the
            # learning rate will be `global_step/num_warmup_steps * init_lr`.
            global_step_float = tf.cast(step, tf.float32)
            warmup_steps_float = tf.cast(self.warmup_steps, tf.float32)
            warmup_percent_done = global_step_float / warmup_steps_float
            warmup_learning_rate = (
                self.initial_learning_rate *
                tf.math.pow(warmup_percent_done, self.power))
            return tf.cond(
                global_step_float < warmup_steps_float,
                lambda: warmup_learning_rate,
                lambda: self.decay_schedule_fn(step),
                name=name)

    def get_config(self):
        return get_properties_for_config(self)

#endregion

#region Keras optimizers

# Quasi-hyperbolic moments optimizer with weight decay
class QHMW(tfa.optimizers.DecoupledWeightDecayExtension, QHMOptimizer):
    def __init__(self, weight_decay, *args, **kwargs):
        super(QHMW, self).__init__(weight_decay, *args, **kwargs)

# Quasi-hyperbolic Adam optimizer with weight decay
class QHAdamW(tfa.optimizers.DecoupledWeightDecayExtension, QHAdamOptimizer):
    def __init__(self, weight_decay, *args, **kwargs):
        super(QHAdamW, self).__init__(weight_decay, *args, **kwargs)

# Rectified Adam (RAdam) optimizer
RectifiedAdam = RAdam

# AdHessian optimizer
AdaHessian = AdaHessian

# Nesterov adam with weight decay
class NadamW(tfa.optimizers.DecoupledWeightDecayExtension, tf.keras.optimizers.Nadam):
    def __init__(self, weight_decay, *args, **kwargs):
        super(NadamW, self).__init__(weight_decay, *args, **kwargs)

# Adagrad with weight decay        
class AdagradW(tfa.optimizers.DecoupledWeightDecayExtension, tf.keras.optimizers.Adagrad):
    def __init__(self, weight_decay, *args, **kwargs):
        super(AdagradW, self).__init__(weight_decay, *args, **kwargs)

# Adadelta with weight decay        
class AdadeltaW(tfa.optimizers.DecoupledWeightDecayExtension, tf.keras.optimizers.Adadelta):
    def __init__(self, weight_decay, *args, **kwargs):
        super(AdadeltaW, self).__init__(weight_decay, *args, **kwargs)
        
# Adamax with weight decay        
class AdamaxW(tfa.optimizers.DecoupledWeightDecayExtension, tf.keras.optimizers.Adamax):
    def __init__(self, weight_decay, *args, **kwargs):
        super(AdamaxW, self).__init__(weight_decay, *args, **kwargs)

#endregion

#region Keras loss functions

@tf.keras.utils.register_keras_serializable(package="Helpers", name="RMSE")
def root_mean_squared_error(y_true, y_pred):
    return K.sqrt(tf.keras.losses.mean_squared_error(y_true, y_pred))

#endregion

#region Keras gradient transform functions

@tf.keras.utils.register_keras_serializable(package="Helpers")
class EnhancedGradientTransformer(object):
    def __init__(self, center=False, add_noise=False):
        self._serialized_properties = [ "center", "add_noise" ]

        self.center = center
        self.add_noise = add_noise
    
    def _centralized_grad(grad):
        rank = len(grad.shape)
        if rank > 1:
            grad -= tf.reduce_mean(grad, axis=list(range(rank-1)), keepdims=True)
        return grad

    def _centralize_gradients(self, gradients):
        return list(map(EnhancedGradientTransformer._centralized_grad, gradients))

    def _add_noise_to_gradients(self, gradients):
        #TODO: implement
        return gradients

    def __call__(self, gradients):
        if self.center:
            gradients = self._centralize_gradients(gradients)

        if self.add_noise:
            gradients = self._add_noise_to_gradients(gradients)

        return gradients

    def get_config(self):
        return get_properties_for_config(self)

#endregion

#region Keras models

@tf.keras.utils.register_keras_serializable(package="Helpers")
class EnhancedDataTransformer(object):
    def __init__(self, input_bias, input_scale, target_bias, target_scale):
        self._serialized_properties = [ "input_bias", "input_scale", "target_bias", "target_scale" ]

        self.input_bias = input_bias
        self.input_scale = input_scale
        self.target_bias = target_bias
        self.target_scale = target_scale

    def _make_constant_column(self, member_name):
        col = getattr(self, member_name)
        internal_name = '_' + member_name
        setattr(self, internal_name, tf.constant(col, dtype=np.float, shape=(1, len(col)), name=internal_name))
        logger.debug('Normalization term {}: {}', internal_name, str(getattr(self, internal_name)))
        
    def compile(self):
        const_columns = ['input_bias', 'input_scale', 'target_bias', 'target_scale']
        for column_name in const_columns:
            self._make_constant_column(column_name)

    def _normalize_data(self, inputs, bias, scale):
        return tf.multiply(tf.add(inputs, bias), scale)

    def _unnormalize_data(self, inputs, bias, scale):
        return tf.subtract(tf.divide(inputs, scale), bias)

    def normalize_feature(self, x):
        return self._normalize_data(x, self._input_bias, self._input_scale)

    def normalize_target(self, y):
        return self._normalize_data(y, self._target_bias, self._target_scale)

    def unnormalize_feature(self, x):
        return self._unnormalize_data(x, self._input_bias, self._input_scale)

    def unnormalize_target(self, y):
        return self._unnormalize_data(y, self._target_bias, self._target_scale)

    def _extract_bias_scale(self, col_ids):
        x_bias = tf.gather(self._input_bias, col_ids[0], axis=1)
        x_scale = tf.gather(self._input_scale, col_ids[0], axis=1)
        y_bias = tf.gather(self._target_bias, col_ids[1], axis=1)
        y_scale = tf.gather(self._target_scale, col_ids[1], axis=1)
        bias = tf.concat([x_bias, y_bias], axis=1)
        scale = tf.concat([x_scale, y_scale], axis=1)
        return (bias, scale)

    def normalize_cols(self, data, col_ids):
        return self._normalize_data(data, *self._extract_bias_scale(col_ids))

    def unnormalize_cols(self, data, col_ids):
        return self._unnormalize_data(data, *self._extract_bias_scale(col_ids))

    def get_config(self):
        return get_properties_for_config(self)

# TODO: consider that the columns might need to be reordered before training
@tf.keras.utils.register_keras_serializable(package="Helpers")
class EnhancedLossInputGenerator(object):
    def __init__(self, network_folder, network_input_cols, loss_function_input_cols):
        self._serialized_properties = [ 'network_folder', 'network_input_cols', 'loss_function_input_cols' ]

        self.network_folder = network_folder
        self.network_input_cols = network_input_cols
        self.loss_function_input_cols = loss_function_input_cols

    def _load_network(self):
        # try to load the network
        self._network = None

        if keras_model_exists(self.network_folder, 'h5'):
            logger.info('Loading trained model from {}', self.network_folder)
            self._network = restore_trained_keras_model(self.network_folder, 'h5')

        elif keras_model_checkpoint_exists(self.network_folder):
            logger.info('Loading latest model checkpoint from {}', self.network_folder)
            self._network = restore_keras_model_from_checkpoint(self.network_folder)

        # make sure actually found a trained network to use
        if self._network is None:
            raise ValueError("No previous training data exists in network folder '{}'.".format(self.network_folder))

    def _make_colid_tensor(self, col_ids, name):
        return tf.constant(col_ids, dtype=np.int32, shape=(len(col_ids)), name=name)

    def _make_colid_tensors(self, col_ids, name):
        suffixes = ['_features', '_targets']
        return [self._make_colid_tensor(c, name + s) for c, s, in zip(col_ids, suffixes)]

    def compile(self):
        self._network_input_cols = self._make_colid_tensors(self.network_input_cols, 'network_input_cols')
        self._loss_function_input_cols = self._make_colid_tensors(self.loss_function_input_cols, 'loss_function_input_cols')

        logger.debug('Loss transform network input column IDs: {}', str(self._network_input_cols))
        logger.debug('Loss function input column IDs: {}', str(self._loss_function_input_cols))
        
        self._load_network()

    def _extract_cols(self, x, y, column_ids):
        x_cols = tf.gather(x, column_ids[0], axis=1)
        y_cols = tf.gather(y, column_ids[1], axis=1)
        return tf.concat([x_cols, y_cols], axis=1)

    def _extract_network_input_cols(self, x, y):
        return self._extract_cols(x, y, self._network_input_cols)

    def _extract_loss_function_input_cols(self, x, y):
        return self._extract_cols(x, y, self._loss_function_input_cols)

    def _normalize_network_inputs(self, x, data_transformer):
        return data_transformer.normalize_cols(x, self._network_input_cols)

    def _normalize_loss_function_inputs(self, x, data_transformer):
        return data_transformer.normalize_cols(x, self._loss_function_input_cols)

    def build_loss_fn_inputs(self, x, y_true, y_pred, data_transformer, normalize_inputs=False, normalize_outputs=False):
        # remove normalization if normalization is used
        if normalize_inputs:
            x = data_transformer.unnormalize_feature(x)
            y_true = data_transformer.unnormalize_target(y_true)
            y_pred = data_transformer.unnormalize_target(y_pred)

        # construct the loss outputs
        y_true_map = self._extract_loss_function_input_cols(x, y_true)
        y_pred_map = self._network(inputs=self._extract_network_input_cols(x, y_pred), training=False)

        # normalize the results if normalization is used
        if normalize_outputs:
            y_true_map = self._normalize_loss_function_inputs(y_true_map, data_transformer)
            y_pred_map = self._normalize_loss_function_inputs(y_pred_map, data_transformer)

        # return the final result
        return (y_true_map, y_pred_map)

    def get_config(self):
        return get_properties_for_config(self)

@tf.keras.utils.register_keras_serializable(package="Helpers")
class EnhancedKerasModel(tf.keras.Model):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        
        self._serialized_properties = [ 'data_transformer', 'loss_input_generator', 'gradient_transformer' ]

        self.data_transformer = None
        self.gradient_transformer = None
        self.loss_input_generator = None

    def compile(self, 
        optimizer='rmsprop', 
        loss=None, 
        metrics=None, 
        loss_weights=None, 
        loss_input_generator=None,
        data_transformer=None,
        gradient_transformer=None, 
        weighted_metrics=None, 
        run_eagerly=None, 
        steps_per_execution=None, 
        **kwargs):

        super().compile(
            optimizer=optimizer,
            loss=loss,
            metrics=metrics,
            loss_weights=loss_weights,
            weighted_metrics=weighted_metrics,
            run_eagerly=run_eagerly,
            steps_per_execution=steps_per_execution,
            **kwargs)

        self.gradient_transformer = gradient_transformer

        self.loss_input_generator = loss_input_generator
        if self.loss_input_generator is not None:
            self.loss_input_generator.compile()

        self.data_transformer = data_transformer
        if self.data_transformer is not None:
            self.data_transformer.compile()

    def _get_loss_inputs(self, x, y, y_hat, training):
        return \
            (y, y_hat) \
            if self.loss_input_generator is None \
            else self.loss_input_generator.build_loss_fn_inputs(x, y, y_hat, self.data_transformer, 
                                                                normalize_inputs=training, 
                                                                normalize_outputs=training)

    def _get_gradients(self, gradient_tape, loss):
        gradients = gradient_tape.gradient(loss, self.trainable_variables)
        return gradients if self.gradient_transformer is None else self.gradient_transformer(gradients)

    def _get_gradients_hessian(self, gradient_tape, loss):
        return self.optimizer.get_gradients_hessian(loss, self.trainable_variables)

    def train_step(self, data):
        data = data_adapter.expand_1d(data)
        x, y, sample_weight = data_adapter.unpack_x_y_sample_weight(data)

        with backprop.GradientTape() as tape:
            y_hat = self(x, training=True)
            y_true, y_pred = self._get_loss_inputs(x, y, y_hat, training=True)

            loss = self.compiled_loss(y_true, y_pred, sample_weight, regularization_losses=self.losses)
            if hasattr(self.optimizer, 'apply_gradients_hessian'):
                gradients, hessian = self._get_gradients_hessian(tape, loss)
                self.optimizer.apply_gradients_hessian(zip(gradients, hessian, self.trainable_variables))
            else:
                gradients = self._get_gradients(tape, loss)
                self.optimizer.apply_gradients(zip(gradients, self.trainable_variables))
        
        self.compiled_metrics.update_state(y_true, y_pred, sample_weight)
        return {m.name: m.result() for m in self.metrics}

    def test_step(self, data):
        data = data_adapter.expand_1d(data)
        x, y, sample_weight = data_adapter.unpack_x_y_sample_weight(data)

        y_hat = self(x, training=False)
        y_true, y_pred = self._get_loss_inputs(x, y, y_hat, training=False)

        self.compiled_loss(y_true, y_pred, sample_weight, regularization_losses=self.losses)
        self.compiled_metrics.update_state(y_true, y_pred, sample_weight)
        return {m.name: m.result() for m in self.metrics}

    def get_config(self):
        return get_properties_for_config(self, base=super().get_config())

#endregion

#region Keras layers

# AdaNorm layer
# This implementation is based on: 
# - https://github.com/keras-team/keras/blob/v2.6.0/keras/layers/normalization/layer_normalization.py#L29-L361
# - https://github.com/lancopku/AdaNorm/blob/546faea0c3297061d743482d690ccd7d51f1ac38/machine%20translation/fairseq/modules/layer_norm.py
@tf.keras.utils.register_keras_serializable(package="Helpers")
class AdaNorm(tf.keras.layers.Layer):
    def __init__(self, axis=-1, scale=1.0, k=0.1, epsilon=1e-6, **kwargs):
        super().__init__(**kwargs)
        self._serialized_properties = [ 'axis', 'scale', 'k', 'epsilon' ]

        self.scale = scale
        self.k = k
        self.axis = axis
        self.epsilon = epsilon

    def build(self, input_shape):
        ndims = len(input_shape)
        if ndims is None:
            raise ValueError('Input shape %s has undefined rank.' % input_shape)

        # Convert axis to list and resolve negatives
        if isinstance(self.axis, int):
            self.axis = [self.axis]
        elif isinstance(self.axis, tuple):
            self.axis = list(self.axis)
        for idx, x in enumerate(self.axis):
            if x < 0:
                self.axis[idx] = ndims + x

        # Validate axes
        for x in self.axis:
            if x < 0 or x >= ndims:
                raise ValueError('Invalid axis: %d' % x)
        if len(self.axis) != len(set(self.axis)):
            raise ValueError('Duplicate axis: {}'.format(tuple(self.axis)))

    def call(self, x, **kwargs):
        mean, variance = tf.nn.moments(x, axes=self.axis, keepdims=True)
        x = x - mean
        mean = tf.math.reduce_mean(x, axis=self.axis, keepdims=True)
        graNorm = tf.stop_gradient(self.k * (x - mean) / (variance + self.epsilon))
        input_norm = (x - x * graNorm) / (variance + self.epsilon)
        return input_norm * self.scale

    def get_config(self):
        return get_properties_for_config(self, base=super().get_config())

# bias-scale layer
@tf.keras.utils.register_keras_serializable(package="Helpers")
class BiasScaleLayer(tf.keras.layers.Layer):
    def __init__(self, bias=None, scale=None, invert=False, at_train=False, at_infer=False, **kwargs):
        super().__init__(**kwargs)
        self._serialized_properties = [ 'bias', 'scale', 'invert', 'at_train', 'at_infer' ]

        self.bias = bias
        self.scale = scale
        self.invert = invert
        self.at_train = at_train
        self.at_infer = at_infer
    
    def build(self, input_shape):
        self._bias = tf.constant(self.bias, dtype=np.float, shape=(1, len(self.bias)), name='bias')
        self._scale = tf.constant(self.scale, dtype=np.float, shape=(1, len(self.scale)), name='scale')

    def call(self, inputs, training=None):
        if (training and not self.at_train) or (not training and not self.at_infer):
            return inputs

        # input / scale - bias
        if self.invert:
            return tf.subtract(tf.divide(inputs, self._scale), self._bias)
        
        # (input + bias) * scale
        else:
            return tf.multiply(tf.add(inputs, self._bias), self._scale)

    def get_config(self):
        return get_properties_for_config(self, base=super().get_config())

# scale-bias layer
@tf.keras.utils.register_keras_serializable(package="Helpers")
class ScaleBiasLayer(tf.keras.layers.Layer):
    def __init__(self, bias=None, scale=None, invert=None, at_train=False, at_infer=False, **kwargs):
        super().__init__(**kwargs)
        self._serialized_properties = [ 'bias', 'scale', 'invert', 'at_train', 'at_infer' ]

        self.bias = bias
        self.scale = scale
        self.invert = invert
        self.at_train = at_train
        self.at_infer = at_infer
    
    def build(self, input_shape):
        self._bias = tf.constant(self.bias, dtype=np.float, shape=(1, len(self.bias)), name='bias')
        self._scale = tf.constant(self.scale, dtype=np.float, shape=(1, len(self.scale)), name='scale')

    def call(self, inputs, training=None):
        if (training and not self.at_train) or (not training and not self.at_infer):
            return inputs

        # (input - bias) / scale
        if self.invert:
            return tf.divide(tf.subtract(input, self._bias), self._scale)
        
        # input * scale + bias
        else:
            return tf.add(tf.multiply(inputs, self._scale), self._bias)

    def get_config(self):
        return get_properties_for_config(self, base=super().get_config())

#endregion

#region Keras callbacks

# callback that updates a keras tuning process
class KerasTunerCallback(tf.keras.callbacks.Callback):
    def __init__(self, tuner, trial):
        super(KerasTunerCallback, self).__init__()
        self.tuner = tuner
        self.trial = trial

    def on_epoch_begin(self, epoch, logs=None):
        #logger.info('on_epoch_begin: {}', self.trial.trial_id)
        self.tuner.on_epoch_begin(self.trial, self.model, epoch, logs=logs)

    def on_batch_begin(self, batch, logs=None):
        #logger.info('on_batch_begin: {}', self.trial.trial_id)
        self.tuner.on_batch_begin(self.trial, self.model, batch, logs)

    def on_batch_end(self, batch, logs=None):
        #logger.info('on_batch_end: {}', self.trial.trial_id)
        self.tuner.on_batch_end(self.trial, self.model, batch, logs)

    def on_epoch_end(self, epoch, logs=None):
        #logger.info('on_epoch_end: {}', self.trial.trial_id)
        self.tuner.on_epoch_end(self.trial, self.model, epoch, logs=logs)

# Callback that adds extra functionality to checkpoints
class EnhancedKerasCheckpointsCallback(tf.keras.callbacks.Callback):
    def __init__(self, save_epochs=None, total_epochs=None, model_folder=None, metrics=[], save_model=False, save_metrics=False, log_via_slack=False):
        super().__init__()
        self.history = {}
        self.save_epochs = save_epochs
        self.total_epochs = total_epochs
        self.model_folder = model_folder
        self.metrics = metrics
        self.save_model = save_model
        self.save_metrics = save_metrics
        self.log_via_slack = log_via_slack

    def on_train_begin(self, logs=None):
        self.epoch = []

    def _ckpt_name(self):
        return checkpoint_format(self.total_epochs, len(self.epoch))

    def _ckpt_folder(self):
        return self.model_folder + self._ckpt_name() + '/'

    def _save_network(self):
        self.model.save(
            filepath='{folder}{ckpt}.h5'.format(folder=self._ckpt_folder(), ckpt=self._ckpt_name()),
            overwrite=True,
            include_optimizer=True,
            save_format='h5')

    def _save_metrics(self):
        # construct the output folder
        slack_msg = []

        # save the metric plots
        for metric in self.metrics:
            charts = save_history_metric_plot(history=self.history, metric=metric.name, 
                folder=self._ckpt_folder(), total_epochs=self.total_epochs)
            for filepath, title, description in charts:
                slack_msg.append({ 'title': title, 'comment': description, 'filepath': filepath })
        
        # send the plots via Slack, if requested
        if self.log_via_slack:
            send_checkpoint_slack_message(
                msg='Checkpoint reached', 
                epoch=len(self.epoch), 
                total_epochs=self.total_epochs, 
                history=self.history, 
                metrics=self.metrics, 
                plots=slack_msg)

    def on_epoch_end(self, epoch, logs=None):
        logs = logs or {}
        self.epoch.append(epoch)
        for k, v in logs.items():
            self.history.setdefault(k, []).append(v)
        
        # save on every save epoch
        if (len(self.epoch) % self.save_epochs) == 0:
            # make sure the ckpt folder exists
            Path(self._ckpt_folder()).mkdir(parents=True, exist_ok=True)

            # save the plots and the network
            if self.save_model:
                self._save_network()
            if self.save_metrics:
                self._save_metrics()

# callback that logs the learning hyperparameters
class NetworkParamsLoggingCallback(tf.keras.callbacks.Callback):

    def __init__(self, network_params, features, targets):
        self.network_params = network_params
        self.features = features
        self.targets = targets

        self._test_written = False
        self._train_written = False

    def _log_params(self):
        # create the string representation of the network variables
        trainable_count = int(np.sum([tf.keras.backend.count_params(p) for p in self.model.trainable_weights]))
        non_trainable_count = int(np.sum([tf.keras.backend.count_params(p) for p in self.model.non_trainable_weights]))
        variables = { 'Total': trainable_count + non_trainable_count, 'Trainable': trainable_count, 'Non-trainable': non_trainable_count }

        # log everything
        tf.summary.text('network_properties', mdh.dict_to_markdown(vals=self.network_params, heading='Network Parameters'), step=0)
        tf.summary.text('variables', mdh.dict_to_markdown(vals=variables, heading='Variables'), step=0)
        tf.summary.text('features', mdh.dict_to_markdown(vals=self.features, heading='Features'), step=0)
        tf.summary.text('targets', mdh.dict_to_markdown(vals=self.targets, heading='Targets'), step=0)

    def on_test_begin(self, logs=None):
        if not self._test_written:
            self._test_written = True
            self._log_params()

    def on_train_begin(self, logs=None):
        if not self._train_written:
            self._train_written = True
            self._log_params()

#endregion

#region Keras serialization helper functions

# extract an object's properties to be used in get_config
def get_properties_for_config(obj, properties=None, base={}):
    if properties is None and hasattr(obj, '_serialized_properties'):
        properties = getattr(obj, '_serialized_properties')
    obj_properties = { name: getattr(obj, name) for name in properties }
    return { **base, **obj_properties }

# a list of all the custom objects available
_custom_objects = { 
    'EnhancedKerasModel': EnhancedKerasModel
}

def _export_keras_model(model, folder, save_format):
    model.save(
        filepath='{folder}/trained_model.{fmt}'.format(folder=folder, fmt=save_format),
        overwrite=True,
        include_optimizer=True,
        save_format=save_format)

# exports the parameter model
def export_keras_model(model, folder, save_formats=['h5', 'tf']):
    # make all the parent folders
    Path(folder).mkdir(parents=True, exist_ok=True)
    
    # export the model using model.save
    for fmt in save_formats:
        _export_keras_model(model, folder, fmt)

# loads back a saved keras model
def _load_keras_model(saved_model_path):
    return tf.keras.models.load_model(saved_model_path, custom_objects=_custom_objects)

# checks if a trained keras model exists given the parameter folder
def keras_model_exists(folder, save_format):
    if save_format == 'h5':
        return Path(folder).exists() and Path(folder + '/trained_model.h5').exists()
    elif save_format == 'tf':
        return Path(folder).exists() and Path(folder + '/trained_model.tf').exists() and Path(folder + '/trained_model.tf/saved_model.pb').exists()
    raise RuntimeError('Unknown save format: ', save_format)

# restores the trained keras model
def restore_trained_keras_model(folder, save_format='h5'):
    # make sure the folder even exists
    if not keras_model_exists(folder, save_format):
        logger.error("Unable to restore model: no exported model data available.")
        return None

    # load back the saved model and return it
    saved_model_path = '{folder}/trained_model.{fmt}'.format(folder=folder, fmt=save_format)
    return _load_keras_model(saved_model_path)

#endregion

#region Keras checkpoint helper functions

# extracts the epoch id from the parameter checkpoint path
def _extract_keras_checkpoint_epoch_id(ckpt):
    return int(str(ckpt.name).split('-')[1])    

def _get_keras_checkpoint_nested_model_path(ckpt):
    ckpt_name = Path(ckpt).name
    nested_ckpt = ckpt + '/' + ckpt_name + '.h5'
    if Path(nested_ckpt).exists():
        return nested_ckpt
    return ckpt

# checks if the parameter path is a keras model folder
def _is_keras_model_checkpoint(path):
    return path.is_dir() and 'model.ckpt' in str(path)

# returns the list of all the checkpoints in the target folder
def _enumerate_keras_model_checkpoints(folder):
    return list(filter(_is_keras_model_checkpoint, Path(folder).iterdir()))

# checks if a previous checkpoint exists given the parameter folder
def keras_model_checkpoint_exists(folder):
    return Path(folder).exists() and len(_enumerate_keras_model_checkpoints(folder)) > 0

# returns the path to the latest checkpoint and the last epoch
def keras_model_latest_checkpoint(folder):
    if not keras_model_checkpoint_exists(folder):
        logger.info("No previous checkpoints available.")
        return None

    checkpoints = _enumerate_keras_model_checkpoints(folder)
    epochs = list(map(_extract_keras_checkpoint_epoch_id, checkpoints))
    latest_version_id = np.argmax(epochs)
    return (str(checkpoints[latest_version_id]), epochs[latest_version_id])

def _restore_keras_checkpoint(folder, use_nested):
    # make sure the folder even exists
    if not keras_model_checkpoint_exists(folder):
        logger.info("Unable to restore model: no previous checkpoints available.")

        return None

    # find the latest checkpoint
    ckpt_path, ckpt_epoch = keras_model_latest_checkpoint(folder)
    if use_nested:
        ckpt_path = _get_keras_checkpoint_nested_model_path(ckpt_path)

    logger.info("Restoring model from '{}'", ckpt_path)

    # load the model
    model = _load_keras_model(ckpt_path)

    return (model, ckpt_epoch)

# restores the model for the parameter checkpoint
def restore_keras_model_checkpoint(folder):
    return _restore_keras_checkpoint(folder, False)
    
# restores the model for the parameter checkpoint
def restore_keras_model_from_checkpoint(folder):
    model, _ = _restore_keras_checkpoint(folder, True)
    return model

#endregion

#endregion