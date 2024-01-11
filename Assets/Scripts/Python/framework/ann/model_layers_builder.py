# custom packages
import itertools
from re import I
from framework.helpers import tensorflow_helper as tfh
from framework.helpers import logging_helper as lh

# data & machine learning packages
import numpy as np

import tensorflow as tf
import tensorflow_addons as tfa

# create a new, module-level logger
logger = lh.get_main_module_logger()

# helper class for building model layers
class ANNModelLayersBuilder(object):
    def __init__(self, config, network_params, datasets):
        self.config = config
        self.network_params = network_params
        self.datasets = datasets

        # the various activation functions available
        activation_fns = {
            'linear': None,
            'tanh': tf.keras.activations.tanh,
            'tanh2': tfh.tanh2,
            'tanh3': tfh.tanh3,
            'tanhsq2': tfh.tanhsq2,
            'tanhsq3': tfh.tanhsq3,
            'relu': tf.keras.activations.relu,
            'selu': tf.keras.activations.selu,
            'elu': tf.keras.activations.elu,
            'swish': tf.keras.activations.swish,
            'gelu': tfa.activations.gelu,
            'mish': tfa.activations.mish,
            'lisht': tfa.activations.lisht
        }
        self.activation_fn = activation_fns[self.network_params.hidden_layers.activation_function]
        self.output_activation_fn = activation_fns[self.network_params.hidden_layers.output_activation_function]

        # what initializer to use for the kernel weights
        kernel_initializers = {
            'ones': tf.keras.initializers.Ones(),
            'zeros': tf.keras.initializers.Zeros(),
            'identity': tf.keras.initializers.Identity(),
            'random_uniform': tf.keras.initializers.RandomUniform(seed=self.network_params.hidden_layers.random_seeds.kernel_initializer),
            'random_normal': tf.keras.initializers.RandomNormal(seed=self.network_params.hidden_layers.random_seeds.kernel_initializer),
            'truncated_normal': tf.keras.initializers.TruncatedNormal(seed=self.network_params.hidden_layers.random_seeds.kernel_initializer),
            'glorot_uniform': tf.keras.initializers.GlorotUniform(seed=self.network_params.hidden_layers.random_seeds.kernel_initializer),
            'glorot_normal': tf.keras.initializers.GlorotNormal(seed=self.network_params.hidden_layers.random_seeds.kernel_initializer),
            'he_uniform': tf.keras.initializers.HeUniform(seed=self.network_params.hidden_layers.random_seeds.kernel_initializer),
            'he_normal': tf.keras.initializers.HeNormal(seed=self.network_params.hidden_layers.random_seeds.kernel_initializer),
            'lecun_uniform': tf.keras.initializers.LecunUniform(seed=self.network_params.hidden_layers.random_seeds.kernel_initializer),
            'lecun_normal': tf.keras.initializers.LecunNormal(seed=self.network_params.hidden_layers.random_seeds.kernel_initializer)
        }
        bias_initializers = {
            'ones': tf.keras.initializers.Ones(),
            'zeros': tf.keras.initializers.Zeros(),
            'identity': tf.keras.initializers.Identity(),
            'random_uniform': tf.keras.initializers.RandomUniform(seed=self.network_params.hidden_layers.random_seeds.bias_initializer),
            'random_normal': tf.keras.initializers.RandomNormal(seed=self.network_params.hidden_layers.random_seeds.bias_initializer),
            'truncated_normal': tf.keras.initializers.TruncatedNormal(seed=self.network_params.hidden_layers.random_seeds.bias_initializer),
            'glorot_uniform': tf.keras.initializers.GlorotUniform(seed=self.network_params.hidden_layers.random_seeds.bias_initializer),
            'glorot_normal': tf.keras.initializers.GlorotNormal(seed=self.network_params.hidden_layers.random_seeds.bias_initializer),
            'he_uniform': tf.keras.initializers.HeUniform(seed=self.network_params.hidden_layers.random_seeds.bias_initializer),
            'he_normal': tf.keras.initializers.HeNormal(seed=self.network_params.hidden_layers.random_seeds.bias_initializer),
            'lecun_uniform': tf.keras.initializers.LecunUniform(seed=self.network_params.hidden_layers.random_seeds.bias_initializer),
            'lecun_normal': tf.keras.initializers.LecunNormal(seed=self.network_params.hidden_layers.random_seeds.bias_initializer)
        }

        # default initializers for each activation fn.
        default_initializers = {
            'linear': ('he_uniform', 'zeros'),
            'relu': ('he_uniform', 'zeros'),
            'gelu': ('he_uniform', 'zeros'),
            'elu': ('he_uniform', 'zeros'),
            'swish': ('he_uniform', 'zeros'),
            'mish': ('he_uniform', 'zeros'),
            'selu': ('lecun_normal', 'zeros'),
            'default': ('zeros', 'zeros')
        }

        # extract the initializers
        initializer_names = list(default_initializers['default'])
        if 'kernel_initializer' in self.network_params.hidden_layers and self.network_params.hidden_layers.kernel_initializer in kernel_initializers:
            initializer_names[0] = self.network_params.hidden_layers.kernel_initializer
        if 'bias_initializer' in self.network_params.hidden_layers and self.network_params.hidden_layers.bias_initializer in bias_initializers:
            initializer_names[1] = self.network_params.hidden_layers.bias_initializer
        if self.network_params.hidden_layers.activation_function in default_initializers:
            initializer_names = default_initializers[self.network_params.hidden_layers.activation_function]
        self.kernel_initialzer = kernel_initializers[initializer_names[0]]
        self.bias_initializer = bias_initializers[initializer_names[1]]
        
        # default to no regularization
        self.kernel_regularizer = None
        self.bias_regularizer = None
        self.activity_regularizer = None
        self.kernel_constraint = None
        self.bias_constraint = None

        # regularization objects
        def l1_l2_regularizer(regularizer_params):
            return tf.keras.regularizers.l1_l2(l1=1e-3 * regularizer_params.L1, l2=1e-3 * regularizer_params.L2)

        self.kernel_regularizer = l1_l2_regularizer(self.network_params.regularization.kernel) if 'kernel' in self.network_params.regularization else None
        self.bias_regularizer = l1_l2_regularizer(self.network_params.regularization.bias) if 'bias' in self.network_params.regularization else None
        self.activity_regularizer = l1_l2_regularizer(self.network_params.regularization.activity) if 'activity' in self.network_params.regularization else None

        # constraints
        if 'maxnorm' in self.network_params.regularization and self.network_params.regularization.maxnorm > 0.0:
            self.kernel_constraint = tf.keras.constraints.MaxNorm(self.network_params.regularization.maxnorm)

        # list of building block constructors
        self.blocks = {
            'input': self._base_layer_input,
            'inputnormalization': self._base_layer_input_normalization,
            'output': self._base_layer_output,
            'outputnormalization': self._base_layer_output_normalization,
            'dense': self._base_layer_dense,
            'noisydense': self._base_layer_noisy_dense,
            'activation': self._base_layer_activation,
            'denseactivation': self._base_layer_dense_with_activation,
            'dropout': self._base_layer_dropout,
            'alphadropout': self._base_layer_alpha_dropout,
            'adanorm': self._base_layer_adanorm,
            'batchnormalization': self._base_layer_batch_norm,
            'layernormalization': self._base_layer_layer_norm,
            'groupnormalization': self._base_layer_group_norm,
            'instancenormalization': self._base_layer_instance_norm,
            'residual': self._residual_block
        }

        # list of default blocks
        self.default_input_block = [ "Input", "InputNormalization" ]
        self.default_output_block = [ "Output", "OutputNormalization" ]
        self.default_hidden_block = [ "DenseActivation" ]
        self.default_transition_block = [ ]

        # extract the network topology
        self.network_topology = self.network_params.hidden_layers.structure[self.network_params.network_topology.lower()]

    #region Structures

    def _structure_or(self, block_name, default_value):
        return self.network_topology[block_name] if block_name in self.network_topology else default_value

    def _input_structure(self):
        return self._structure_or('input', self.default_input_block)

    def _output_structure(self):
        return self._structure_or('output', self.default_output_block)

    def _hidden_block_structure(self):
        return self._structure_or('hidden', self.default_hidden_block)

    def _transition_structure(self):
        return self._structure_or('transition', self.default_transition_block)

    def _residual_block_structure(self):
        return self.network_topology['residual']

    def _residual_shortcut_structure(self, shortcut_type):
        return self.network_topology['shortcut_' + shortcut_type.lower()]
        
    def _residual_block_transition_structure(self):
        return self.network_topology['transition_block']
        
    #endregion Structures
    
    #region Layer constructors

    def _apply_weight_normalization(self, layer):
        if 'normalize_weights' in self.network_params.hidden_layers and self.network_params.hidden_layers.normalize_weights:
            return tfa.layers.WeightNormalization(
                layer=layer,
                data_init=False)
        return layer

    def _num_hidden_units(self, layer_id):
        if isinstance(self.network_params.hidden_layers.num_units, list):
            return self.network_params.hidden_layers.num_units[min(layer_id, len(self.network_params.hidden_layers.num_units) - 1)]
        return self.network_params.hidden_layers.num_units

    def _base_layer_input(self, layer_id):
        return tf.keras.layers.Input(
            shape=(self.datasets.num_features,),
            dtype=np.float)
            
    def _base_layer_output(self, layer_id):
        return self._apply_weight_normalization(tf.keras.layers.Dense(
            units=self.datasets.num_targets,
            activation=self.output_activation_fn,
            kernel_initializer=self.kernel_initialzer,
            bias_initializer=self.bias_initializer,
            kernel_regularizer=self.kernel_regularizer,
            bias_regularizer=self.bias_regularizer,
            activity_regularizer=self.activity_regularizer,
            kernel_constraint=self.kernel_constraint,
            bias_constraint=self.bias_constraint))

    def _base_layer_input_normalization(self, layer_id):
        return tfh.BiasScaleLayer(
            name='input_normalization',
            at_train=False,
            at_infer=True,
            scale=self.datasets.x_train_normalization_terms.normalization_scale,
            bias=self.datasets.x_train_normalization_terms.normalization_bias)

    def _base_layer_output_normalization(self, layer_id):
        return tfh.BiasScaleLayer(
            name='output_normalization',
            at_train=False,
            at_infer=True,
            invert=True,
            scale=self.datasets.y_train_normalization_terms.normalization_scale,
            bias=self.datasets.y_train_normalization_terms.normalization_bias)

    def _base_layer_dense(self, layer_id):
        return self._apply_weight_normalization(tf.keras.layers.Dense(
            units=self._num_hidden_units(layer_id), 
            kernel_initializer=self.kernel_initialzer,
            bias_initializer=self.bias_initializer,
            kernel_regularizer=self.kernel_regularizer,
            bias_regularizer=self.bias_regularizer,
            activity_regularizer=self.activity_regularizer,
            kernel_constraint=self.kernel_constraint,
            bias_constraint=self.bias_constraint))

    def _base_layer_dense_with_activation(self, layer_id):
        return tf.keras.layers.Dense(
            units=self._num_hidden_units(layer_id), 
            activation=self.activation_fn,
            kernel_initializer=self.kernel_initialzer,
            bias_initializer=self.bias_initializer,
            kernel_regularizer=self.kernel_regularizer,
            bias_regularizer=self.bias_regularizer,
            activity_regularizer=self.activity_regularizer,
            kernel_constraint=self.kernel_constraint,
            bias_constraint=self.bias_constraint)

    def _base_layer_noisy_dense(self, layer_id):
        return tfa.layers.NoisyDense(
            units=self._num_hidden_units(layer_id), 
            sigma=self.network_params.hidden_layers.noise_sigma,
            kernel_initializer=self.kernel_initialzer,
            bias_initializer=self.bias_initializer,
            kernel_regularizer=self.kernel_regularizer,
            bias_regularizer=self.bias_regularizer,
            activity_regularizer=self.activity_regularizer,
            kernel_constraint=self.kernel_constraint,
            bias_constraint=self.bias_constraint)

    def _base_layer_activation(self, layer_id):
        return tf.keras.layers.Activation(
            activation=self.activation_fn)

    def _base_layer_batch_norm(self, layer_id):
        return tf.keras.layers.BatchNormalization(
            momentum=self.network_params.normalization.momentum,
            epsilon=self.network_params.normalization.epsilon,
            center=self.network_params.normalization.center,
            scale=self.network_params.normalization.scale)

    def _base_layer_adanorm(self, layer_id):
        return tfh.AdaNorm(
            epsilon=self.network_params.normalization.epsilon)

    def _base_layer_layer_norm(self, layer_id):
        return tf.keras.layers.LayerNormalization(
            epsilon=self.network_params.normalization.epsilon,
            center=self.network_params.normalization.center,
            scale=self.network_params.normalization.scale)

    def _base_layer_group_norm(self, layer_id):
        return tfa.layers.GroupNormalization(
            groups=self.network_params.normalization.groups,
            epsilon=self.network_params.normalization.epsilon,
            center=self.network_params.normalization.center,
            scale=self.network_params.normalization.scale)

    def _base_layer_instance_norm(self, layer_id):
        return tfa.layers.InstanceNormalization(
            epsilon=self.network_params.normalization.epsilon,
            center=self.network_params.normalization.center,
            scale=self.network_params.normalization.scale)

    def _base_layer_dropout(self, layer_id):
        return tf.keras.layers.Dropout(
            rate=self.network_params.regularization.dropout_rate)

    def _base_layer_alpha_dropout(self, layer_id):
        return tf.keras.layers.AlphaDropout(
            rate=self.network_params.regularization.dropout_rate)

    def _residual_shortcut(self, layer_id, shortcut_type, block_input):
        # create the shortcut layers
        shortcut_layers = self._split_block_spec(layer_id, self._residual_shortcut_structure(shortcut_type))
        block_input = self._concat_layers(block_input, shortcut_layers)

        # add together the two paths and return the new node
        return tf.keras.layers.add([self.input_tensor, block_input])

    def _residual_block(self, layer_id, shortcut_type, num_inner_layers):
        # remember the input layer
        block_input = self.input_tensor

        # generate the first n-1 blocks
        for _ in range(int(num_inner_layers) - 1):
            self._append_layers(self._split_block_spec(layer_id, self._residual_block_structure()))

        # use the 'Dense' shortcut path to define the last block
        self._append_layers(self._split_block_spec(layer_id, self._residual_shortcut_structure("Dense")))

        # sum the two paths
        self.input_tensor = self._residual_shortcut(layer_id, shortcut_type, block_input)

        # append every node to close the residual block end
        self._append_layers(self._split_block_spec(layer_id, self._residual_block_transition_structure()))

        # everything is already built up, don't return anything
        return None

    #endregion Layer constructors

    def _split_layer_name_args(self, layer_desc):
        # split the block specification to a name-args pair
        if '(' in layer_desc and ')' in layer_desc:
            block_name, block_args = layer_desc.split('(')
            block_name = block_name.lower()
            block_args = block_args[0:-1].split(', ')
        else:
            block_name = layer_desc.lower()
            block_args = []
        return (block_name, block_args)

    def _split_block_spec(self, layer_id, block_spec):
        block_specs = map(self._split_layer_name_args, block_spec)
        blocks = [ self.blocks[block_name](layer_id, *block_args) for block_name, block_args in block_specs ]
        blocks = [ block for block in blocks if block is not None ]
        return blocks

    def _concat_layer(self, input_layer, layer):
        return layer if input_layer is None else layer(input_layer)

    def _concat_layers(self, input_layer, layers):
        for layer in layers:
            input_layer = self._concat_layer(input_layer, layer)
        return input_layer

    def _append_layer(self, layer):
        # concatenate the layer
        self.input_tensor = self._concat_layer(self.input_tensor, layer)
        
        # also store the layer in the resulting
        self.result.append(self.input_tensor)

    def _append_layers(self, layers):
        for layer in layers:
            self._append_layer(layer)

    def build(self):
        self.result = []
        self.input_tensor = None

        # input block
        self._append_layers(self._split_block_spec(None, self._input_structure()))
        
        # hidden blocks
        for layer_id in range(self.network_params.hidden_layers.num_layers):
            self._append_layers(self._split_block_spec(layer_id, self._hidden_block_structure()))
            if layer_id < self.network_params.hidden_layers.num_layers - 1:
                self._append_layers(self._split_block_spec(layer_id, self._transition_structure()))

        # output block
        self._append_layers(self._split_block_spec(None, self._output_structure()))

        return self.result
