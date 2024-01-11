# custom packages
from numpy.lib.function_base import average
from framework.enums import TrainingType
from framework.helpers import tensorflow_helper as tfh
from framework.helpers import logging_helper as lh

# data & machine learning packages
import numpy as np

import tensorflow as tf
import tensorflow_addons as tfa

# local packages
from framework.base.model_elements_builder import ModelElementsBuilder
from framework.ann.model_layers_builder import ANNModelLayersBuilder

# create a new, module-level logger
logger = lh.get_main_module_logger()

# helper class for building the various model elements
class ANNModelElementsBuilder(ModelElementsBuilder):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def _num_steps(self, steps):
        if type(steps) is int:
            return steps
        return steps * self.training_params.train_steps_per_epoch

    def _decay_schedule(self, param_name, rate, warmup, schedule_name, schedule_params):
        # the various decay schedule available
        learning_rate_schedules = {
            'disable': tfh.ConstantLearningRate(
                learning_rate=1.0 * rate),
            'cosine': tf.optimizers.schedules.CosineDecay(
                name=param_name,
                initial_learning_rate=schedule_params.cosine.initial * rate,
                decay_steps=self._num_steps(float(self.network_params.num_epochs)),
                alpha=schedule_params.cosine.alpha),
            'cosine_restarts': tf.optimizers.schedules.CosineDecayRestarts(
                name=param_name,
                initial_learning_rate=schedule_params.cosine_restarts.initial * rate,
                first_decay_steps=self._num_steps(schedule_params.cosine_restarts.first_epochs),
                t_mul=schedule_params.cosine_restarts.t_mul,
                m_mul=schedule_params.cosine_restarts.m_mul,
                alpha=schedule_params.cosine_restarts.alpha),
            'exponential': tf.optimizers.schedules.ExponentialDecay(
                name=param_name,
                initial_learning_rate=schedule_params.exponential.initial * rate,
                decay_steps=self._num_steps(schedule_params.exponential.epochs),
                decay_rate=schedule_params.exponential.rate,
                staircase=not schedule_params.exponential.smooth),
            'inverse_time': tf.optimizers.schedules.InverseTimeDecay(
                name=param_name,
                initial_learning_rate=schedule_params.inverse_time.initial * rate,
                decay_steps=self._num_steps(schedule_params.inverse_time.epochs),
                decay_rate=schedule_params.inverse_time.rate,
                staircase=not schedule_params.inverse_time.smooth),
            'polynomial': tf.optimizers.schedules.PolynomialDecay(
                name=param_name,
                initial_learning_rate=schedule_params.polynomial.initial * rate,
                end_learning_rate=schedule_params.polynomial.final * rate,
                decay_steps=self._num_steps(float(self.network_params.num_epochs)),
                power=schedule_params.polynomial.power),
            'piecewise': tf.optimizers.schedules.PiecewiseConstantDecay(
                name=param_name,
                boundaries=[ self._num_steps(b) for b in schedule_params.piecewise.boundaries ],
                values=[ r * rate for r in schedule_params.piecewise.rates ]),
            'cyclical_triangular': tfa.optimizers.TriangularCyclicalLearningRate(
                name=param_name,
                initial_learning_rate=schedule_params.cyclical_triangular.initial * rate,
                maximal_learning_rate=schedule_params.cyclical_triangular.maximal * rate,
                step_size=self._num_steps(schedule_params.cyclical_triangular.length),
            ),
            'cyclical_triangular2': tfa.optimizers.Triangular2CyclicalLearningRate(
                name=param_name,
                initial_learning_rate=schedule_params.cyclical_triangular2.initial * rate,
                maximal_learning_rate=schedule_params.cyclical_triangular2.maximal * rate,
                step_size=self._num_steps(schedule_params.cyclical_triangular2.length),
            ),
            'cyclical_exponential': tfa.optimizers.ExponentialCyclicalLearningRate(
                name=param_name,
                initial_learning_rate=schedule_params.cyclical_exponential.initial * rate,
                maximal_learning_rate=schedule_params.cyclical_exponential.maximal * rate,
                step_size=self._num_steps(schedule_params.cyclical_exponential.length),
            ),
        }

        # extract the appropriate schedule
        schedule = learning_rate_schedules[schedule_name]

        # apply warmup
        if warmup > 0.0:
            schedule = tfh.LearningRateWarmUp(
                initial_learning_rate=rate,
                warmup_steps=self._num_steps(warmup),
                decay_schedule_fn=schedule)

        # return the final schedule
        return schedule

    def build(self):
        # get the common elements
        result = super().build()
        
        # build the list of layers
        model_layers_builder = ANNModelLayersBuilder(
            config=self.config,
            network_params=self.network_params,
            datasets=self.datasets)
        result.layers = model_layers_builder.build()

        # list of available optimizers
        result.optimizers = {
            'rmsprop': (
                tf.keras.optimizers.RMSprop, 
                { 
                    'learning_rate': self.network_params.learning_rate.rate
                }
            ),
            'sgd': (
                tfa.optimizers.SGDW, 
                { 
                    'learning_rate': self.network_params.learning_rate.rate, 
                    'weight_decay': self.network_params.regularization.weight_decay
                }
            ),
            'lamb': (
                tfa.optimizers.LAMB, 
                { 
                    'learning_rate': self.network_params.learning_rate.rate, 
                    'weight_decay_rate': self.network_params.regularization.weight_decay
                }
            ),
            'adam': (
                tfa.optimizers.AdamW, 
                { 
                    'learning_rate': self.network_params.learning_rate.rate, 
                    'weight_decay': self.network_params.regularization.weight_decay
                }
            ),
            'adamax': (
                tfh.AdamaxW, 
                { 
                    'learning_rate': self.network_params.learning_rate.rate, 
                    'weight_decay': self.network_params.regularization.weight_decay
                }
            ),
            'adagrad': (
                tfh.AdagradW, 
                { 
                    'learning_rate': self.network_params.learning_rate.rate, 
                    'weight_decay': self.network_params.regularization.weight_decay
                }
            ),
            'adadelta': (
                tfh.AdadeltaW, 
                { 
                    'learning_rate': self.network_params.learning_rate.rate, 
                    'weight_decay': self.network_params.regularization.weight_decay
                }
            ),
            'radam': (
                tfh.RectifiedAdam, 
                { 
                    'learning_rate': self.network_params.learning_rate.rate, 
                    'weight_decay': self.network_params.regularization.weight_decay, 
                    'total_steps': self.training_params.num_train_steps
                }
            ),
            'nadam': (
                tf.keras.optimizers.Nadam, 
                { 
                    'learning_rate': self.network_params.learning_rate.rate 
                }
            ),
            'adahessian': (
                tfh.AdaHessian,
                {
                    'learning_rate': self.network_params.learning_rate.rate, 
                    'weight_decay': self.network_params.regularization.weight_decay,
                }
            ),
            'qhm': (
                tfh.QHMW,
                {
                    'learning_rate': self.network_params.learning_rate.rate, 
                    'weight_decay': self.network_params.regularization.weight_decay,
                }
            ),
            'qhadam': (
                tfh.QHAdamW,
                {
                    'learning_rate': self.network_params.learning_rate.rate, 
                    'weight_decay': self.network_params.regularization.weight_decay,
                }
            )
        }
        # extract the current optimizer
        optimizer_class, optimizer_params = result.optimizers[self.network_params.optimizer.lower()]

        # build the list of optimizer parameters
        if self.network_params.optimizer in self.network_params.optimizer_params:
            optimizer_params.update(self.network_params.optimizer_params[self.network_params.optimizer])

        # handle decaying optimizer parameters
        if 'decayed_params' in optimizer_params:
            for param in optimizer_params['decayed_params']:
                optimizer_params[param] = self._decay_schedule(
                    param_name=param,
                    rate=optimizer_params[param], 
                    warmup=self.network_params.decay_schedule.warmup,
                    schedule_name=self.network_params.decay_schedule.name,
                    schedule_params=self.network_params.decay_schedule.schedule_params)
            del optimizer_params['decayed_params']
            
        # construct the actual optimizer object
        result.optimizer = optimizer_class(**optimizer_params)

        # apply lookahead, if requested
        if 'lookahead' in self.network_params and self.network_params.lookahead.enable:
            result.optimizer = tfa.optimizers.Lookahead(
                optimizer=result.optimizer,
                sync_period=self.network_params.lookahead.sync_period,
                slow_step_size=self.network_params.lookahead.slow_step_size)

        if 'model_average' in self.network_params and self.network_params.model_average.enable:
            average_types = {
                'moving_average': (
                    tfa.optimizers.MovingAverage,
                    {
                        'optimizer': result.optimizer,
                        'average_decay': self.network_params.model_average.moving_average.average_decay,
                        'start_step': self._num_steps(self.network_params.model_average.moving_average.start_step)
                    }
                ),
                'stocastic_average': (
                    tfa.optimizers.SWA,
                    {
                        'optimizer': result.optimizer,
                        'average_period': self.network_params.model_average.stocastic_average.average_period,
                        'start_step': self._num_steps(self.network_params.self.network_params.model_average.stocastic_average.start_step)
                    }
                ),
            }
            average_cls, average_params = average_types[self.network_params.model_average.type]
            result.optimizer = average_cls(**average_params)

        # construct the loss object
        result.losses = {
            'mae': tf.keras.losses.mae,
            'mape': tf.keras.losses.mape,
            'mse': tf.keras.losses.mse,
            'log_cosh': tf.keras.losses.log_cosh,
            'rmse': tfh.root_mean_squared_error,
            'huber': tf.keras.losses.Huber(1.35 if 'huber_delta' not in self.network_params.loss else self.network_params.loss.huber_delta)
        }
        result.loss = result.losses[self.network_params.loss.function.lower()]
        
        # build the loss input generator object
        result.loss_input_generator = None
        if 'transform_network' in self.network_params.loss:
            # input and output columns of the network
            network_type = self.network_params.loss.transform_network.network_type
            network_inputs = self.datasets.param_info.filter_param_list(network_type, 'feature', extract_attrib='name')
            network_outputs = self.datasets.param_info.filter_param_list(network_type, 'target', extract_attrib='name')
            
            # extract the corresponding column IDs
            def extract_column_ids(column_names, network_inputs):
                return [ param_id for param_id, param_name in enumerate(column_names) if param_name in network_inputs ]

            result.loss_transform_network = tfh.get_network_folder(
                network_module=self.network_params.loss.transform_network.network_module,
                network_type=self.network_params.loss.transform_network.network_type, 
                model_type=self.network_params.loss.transform_network.model_type, 
                network_name=self.network_params.loss.transform_network.network_name)
            result.loss_transform_network_input_cols = (
                extract_column_ids(self.datasets.feature_names, network_inputs),
                extract_column_ids(self.datasets.target_names, network_inputs))
            result.loss_function_input_cols = (
                extract_column_ids(self.datasets.feature_names, network_outputs),
                extract_column_ids(self.datasets.target_names, network_outputs))

            if self.network_params.loss.transform_network.enable:
                result.loss_input_generator = tfh.EnhancedLossInputGenerator(
                    network_folder=result.loss_transform_network,
                    network_input_cols=result.loss_transform_network_input_cols,
                    loss_function_input_cols=result.loss_function_input_cols)
        
        # get the loss weight parameters
        #result.loss_weights = np.array(self.datasets.loss_weights, dtype=tfh.float_np)

        # get the gradient transformation method
        result.gradient_transformer = None
        if 'gradient' in self.network_params:
            result.gradient_transformer = tfh.EnhancedGradientTransformer(
                center=self.network_params.gradient.centralization,
                add_noise=self.network_params.gradient.noise
            )

        # create the data transformer
        result.data_transformer = tfh.EnhancedDataTransformer(
            input_bias=self.datasets.x_train_normalization_terms.normalization_bias,
            input_scale=self.datasets.x_train_normalization_terms.normalization_scale,
            target_bias=self.datasets.y_train_normalization_terms.normalization_bias,
            target_scale=self.datasets.y_train_normalization_terms.normalization_scale
        )

        # define the metrics; these do not work properly with a custom loss fn., but still give an estimate of how 
        # similar the true and predicted outputs are
        result.metrics = []
        result.metrics.append(tf.keras.metrics.MeanAbsoluteError(name='MAE'))
        result.metrics.append(tf.keras.metrics.MeanAbsolutePercentageError(name='MAPE'))
        #result.metrics.append(tf.keras.metrics.MeanSquaredError(name='MSE'))
        #result.metrics.append(tf.keras.metrics.RootMeanSquaredError(name='RMSE'))

        # define the callbacks
        result.callbacks = []

        #result.callbacks.append(tfa.callbacks.TQDMProgressBar())                             # TQDM progress bar

        result.callbacks.append(tf.keras.callbacks.TensorBoard(                              # TensorBoard data exporting callback
            log_dir=self.training_params.model_folder,
            update_freq=self.training_params.train_steps_per_epoch,
            histogram_freq=1,
            write_graph=True,
            profile_batch=0,
            embeddings_freq=0))

        if self.training_params.training_type != TrainingType.Tune:                           # handle tuning mode
            result.callbacks.append(tfh.NetworkParamsLoggingCallback(                         # network param logging callback
                network_params=self.network_params,
                features=self.datasets.feature_names,
                targets=self.datasets.target_names))

            result.callbacks.append(tf.keras.callbacks.ModelCheckpoint(                       # checkpoint saving callback
                filepath=self.training_params.checkpoints_path,
                period=self.training_params.num_checkpoint_save_epochs,
                verbose=1,
                save_weights_only=False,
                save_best_only=False,
                mode='min'))

            result.callbacks.append(tfh.EnhancedKerasCheckpointsCallback(
                save_epochs=self.training_params.num_checkpoint_save_epochs,
                total_epochs=self.training_params.num_epochs, 
                model_folder=self.training_params.model_folder, 
                metrics=result.metrics,
                save_model=True,
                save_metrics=self.config.slack_notifications,
                log_via_slack=self.config.slack_notifications))

        if self.network_params.early_stopping.baseline.enable:
            result.callbacks.append(tf.keras.callbacks.EarlyStopping(                        # early stopping: baseline
                monitor=self.network_params.early_stopping.baseline.metric,
                patience=self.network_params.early_stopping.baseline.epochs,
                baseline=self.network_params.early_stopping.baseline.threshold,
                mode='min',
                verbose=1))
                
        if self.network_params.early_stopping.min_delta.enable:
            result.callbacks.append(tf.keras.callbacks.EarlyStopping(                        # early stopping: min delta
                monitor=self.network_params.early_stopping.min_delta.metric,
                patience=self.network_params.early_stopping.min_delta.epochs,
                min_delta=self.network_params.early_stopping.min_delta.threshold,
                mode='min',
                verbose=1))

        if self.network_params.early_stopping.num_epochs.enable:
            result.callbacks.append(tf.keras.callbacks.EarlyStopping(                        # early stopping: epochs
                monitor='MAE',
                patience=self.network_params.early_stopping.num_epochs.epochs,
                baseline=1e-10,
                mode='min',
                verbose=1))

        # create the model
        result.model = tfh.EnhancedKerasModel(
            inputs=result.layers[0],
            outputs=result.layers[-1])
        result.model_compile_args = {
            'optimizer': result.optimizer,
            'loss': result.loss,
            'metrics': result.metrics,
            'run_eagerly': self.network_params.eager_execution,
            'data_transformer': result.data_transformer,
            'loss_input_generator': result.loss_input_generator,
            'gradient_transformer': result.gradient_transformer }
        
        if not self.network_params.eager_execution:
            result.model_compile_args['steps_per_execution'] = self.training_params.steps_per_execution
        
        # create the train parameters
        has_progressbar_callback = any(type(cb) is tfa.callbacks.TQDMProgressBar for cb in result.callbacks)
        result.model_train_args = {
            'x': self.datasets.train_dataset, 
            'epochs': self.training_params.num_epochs,
            'initial_epoch': 0,
            'verbose': 0 if has_progressbar_callback else 1,
            'shuffle': True,
            'validation_data': self.datasets.eval_dataset,
            'validation_freq': self.training_params.num_eval_epochs,
            'callbacks': result.callbacks
        }

        # create the predict parameters
        result.model_predict_args = {
            'batch_size': self.datasets.batch_size,
            'verbose': 1
        }

        # return the final result
        return result
