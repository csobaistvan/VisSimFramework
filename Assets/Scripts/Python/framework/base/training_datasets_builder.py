# custom packages
from framework.helpers import tensorflow_helper as tfh
from framework.helpers import logging_helper as lh
from framework.helpers.dotdict import dotdict

# core packages
import math
import json

# data & machine learning packages
import numpy as np

import tensorflow as tf

# create a new, module-level logger
logger = lh.get_main_module_logger()

# helper class for building training datasets
class TrainingDatasetsBuilder(object):
    def __init__(self, config, network_params, param_info, dataset_name, raw_datasets, training_type):
        self.config = config
        self.network_params = network_params
        self.param_info = param_info
        self.dataset_name = dataset_name
        self.raw_datasets = raw_datasets
        self.training_type = training_type

    def _create_feature_column(self, feature_name):
        return tf.feature_column.numeric_column(key=feature_name, dtype=np.float)

    def _sample_dataset(self, dataset, num_samples):
        if type(num_samples) is int:
            frac = np.clip(dataset.shape[0] / num_samples, 0.0, 1.0)
            return dataset.sample(frac=frac, random_state=self.network_params.dataset.random_seeds.split)
        elif type(num_samples) is float:
            frac = np.clip(num_samples, 0.0, 1.0)
            return dataset.sample(frac=frac, random_state=self.network_params.dataset.random_seeds.split)
        return dataset

    def _compute_param_normalization_terms(self, dframe, col_names, normalization):
        result = dotdict()

        # normalization terms
        if normalization == 'minmax_positive' or normalization == 'minmax':
            val_min = np.amin(dframe, axis=0)
            val_extent = np.amax(dframe, axis=0) - np.amin(dframe, axis=0)
            result.normalization_bias = [ -mn for mn, ex in zip(val_min, val_extent) ]
            result.normalization_scale = [ 1.0 / ex for mn, ex in zip(val_min, val_extent) ]

        if normalization == 'minmax_symmetric':
            val_min = np.amin(dframe, axis=0)
            val_extent = np.amax(dframe, axis=0) - np.amin(dframe, axis=0)
            result.normalization_bias = [ -mn - ex / 2 for mn, ex in zip(val_min, val_extent) ]
            result.normalization_scale = [ 1.0 / (ex / 2) for mn, ex in zip(val_min, val_extent) ]
        
        elif normalization == 'standardize':
            val_mean = np.mean(dframe, axis=0)
            val_std = np.std(dframe, axis=0)
            result.normalization_bias = [ -value for value in val_mean ]
            result.normalization_scale = [ 1.0 / value for value in val_std ]
        
        elif normalization == 'disable':
            result.normalization_bias = [ 0.0 for _ in range(dframe.shape[1]) ]
            result.normalization_scale = [ 1.0 for _ in range(dframe.shape[1]) ]

        # generate the normalized data
        result.data = ((dframe + result.normalization_bias) * result.normalization_scale).astype(tfh.float_np)

        # return the resulting values
        return result

    def _train_eval_split(self, features, targets):
        # split the data to train and test batches
        np.random.seed(self.network_params.dataset.random_seeds.split)
        rows = np.random.binomial(1, self.network_params.dataset.train_eval_ratio, size=len(features)).astype(bool)

        x_train = features[rows].to_numpy(dtype=tfh.float_np)
        y_train = targets[rows].to_numpy(dtype=tfh.float_np)
        x_eval = features[~rows].to_numpy(dtype=tfh.float_np)
        y_eval = targets[~rows].to_numpy(dtype=tfh.float_np)

        # return everything as numpy arrays
        return (x_train, y_train, x_eval, y_eval)
    
    def build(self, fixed_params={}):
        result = dotdict()

        # update the result with the default parameters
        if len(fixed_params) > 0:
            result.set_fix_items(fixed_params)

        # append the param info to the dataset object
        result.param_info = self.param_info

        # store the name of the source dataset
        result.dataset_name = self.dataset_name

        #print(self.raw_datasets['eye'].shape)

        # build the features dataframe
        result.features = self.param_info.extract_columns(self.raw_datasets, self.network_params.type, 'feature').astype(tfh.float_np)
        result.feature_names = result.features.keys().tolist()
        result.feature_columns = list(map(self._create_feature_column, result.feature_names))
        result.num_features = len(result.feature_names)

        logger.debug('Training with the following features (#{}): {}', result.num_features, json.dumps(result.features.keys().to_list(), indent=4))

        # build the targets dataframe
        result.targets = self.param_info.extract_columns(self.raw_datasets, self.network_params.type, 'target').astype(tfh.float_np)
        result.target_names = result.targets.keys().tolist()
        result.num_targets = len(result.target_names)

        logger.debug('Training with the following targets (#{}): {}', result.num_targets, json.dumps(result.targets.keys().to_list(), indent=4))

        #print(result.features.shape)

        # sample the datasets
        result.features = self._sample_dataset(result.features, self.network_params.dataset.num_samples)
        result.targets = self._sample_dataset(result.targets, self.network_params.dataset.num_samples)

        #print(result.features.shape)

        # perform the train-eval split
        result.x_train, result.y_train, result.x_eval, result.y_eval = self._train_eval_split(result.features, result.targets)
        
        #print(result.x_train.shape)

        # normalize the datasets
        result.x_train_normalization_terms = self._compute_param_normalization_terms(
            result.x_train, result.feature_names, self.network_params.dataset.normalization.features)
        result.x_train_normalized = result.x_train_normalization_terms.data

        result.x_eval_normalization_terms = self._compute_param_normalization_terms(
            result.x_eval, result.feature_names, self.network_params.dataset.normalization.features)
        result.x_eval_normalized = result.x_eval_normalization_terms.data

        result.y_train_normalization_terms = self._compute_param_normalization_terms(
            result.y_train, result.target_names, self.network_params.dataset.normalization.targets)
        result.y_train_normalized = result.y_train_normalization_terms.data

        result.y_eval_normalization_terms = self._compute_param_normalization_terms(
            result.y_eval, result.target_names, self.network_params.dataset.normalization.targets)
        result.y_eval_normalized = result.y_eval_normalization_terms.data
        
        #logger.debug('Normalizing x_train with bias: {}', json.dumps(result.x_train_normalization_terms.normalization_bias, indent=4))
        #logger.debug('Normalizing x_train with scale: {}', json.dumps(result.x_train_normalization_terms.normalization_scale, indent=4))
        #logger.debug('Normalizing x_eval with bias: {}', json.dumps(result.x_eval_normalization_terms.normalization_bias, indent=4))
        #logger.debug('Normalizing x_eval with scale: {}', json.dumps(result.x_eval_normalization_terms.normalization_scale, indent=4))
        #logger.debug('Normalizing y_train with bias: {}', json.dumps(result.y_train_normalization_terms.normalization_bias, indent=4))
        #logger.debug('Normalizing y_train with scale: {}', json.dumps(result.y_train_normalization_terms.normalization_scale, indent=4))
        #logger.debug('Normalizing y_eval with bias: {}', json.dumps(result.y_eval_normalization_terms.normalization_bias, indent=4))
        #logger.debug('Normalizing y_eval with scale: {}', json.dumps(result.y_eval_normalization_terms.normalization_scale, indent=4))

        # cache the dataset shapes
        result.num_train_samples = result.x_train.shape[0]
        result.num_eval_samples = result.x_eval.shape[0]

        # list of loss weight terms
        result.loss_weights = self.param_info.extract_target_weights(self.network_params.type)

        # batch size
        result.batch_size = pow(2, math.ceil(math.log2(self.network_params.batch.size)))

        # batch sizes for training and evaluation
        result.train_batch_size = min(result.batch_size, result.num_train_samples)
        result.eval_batch_size = min(result.batch_size, result.num_eval_samples)

        return result
