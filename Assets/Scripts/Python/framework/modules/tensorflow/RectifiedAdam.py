import tensorflow as tf
from tensorflow_addons.utils.types import FloatTensorLike

from typing import Union, Callable, Dict
from typeguard import typechecked


# Based on: https://github.com/tensorflow/addons/blob/v0.14.0/tensorflow_addons/optimizers/rectified_adam.py#L24-L326
@tf.keras.utils.register_keras_serializable(package="Helpers")
class RectifiedAdam(tf.keras.optimizers.Optimizer):
    @typechecked
    def __init__(
        self,
        learning_rate: Union[FloatTensorLike, Callable, Dict] = 0.001,
        beta_1: FloatTensorLike = 0.9,
        beta_2: FloatTensorLike = 0.999,
        beta_1_final: FloatTensorLike = 0.9,
        p: FloatTensorLike = 0.5,
        decay_betas: bool = False,
        total_steps: int = 0,
        epsilon: FloatTensorLike = 1e-7,
        weight_decay: Union[FloatTensorLike, Callable, Dict] = 0.0,
        sma_threshold: FloatTensorLike = 5.0,
        name: str = "RectifiedAdam",
        **kwargs,
    ):
        super().__init__(name, **kwargs)

        if isinstance(learning_rate, Dict):
            learning_rate = tf.keras.optimizers.schedules.deserialize(learning_rate)

        if isinstance(weight_decay, Dict):
            weight_decay = tf.keras.optimizers.schedules.deserialize(weight_decay)

        self._set_hyper("learning_rate", kwargs.get("lr", learning_rate))
        self._set_hyper("beta_1", beta_1)
        self._set_hyper("beta_2", beta_2)
        self._set_hyper("beta_1_final", beta_1)
        self._set_hyper("decay", self._initial_decay)
        self._set_hyper("weight_decay", weight_decay)
        self._set_hyper("total_steps", float(total_steps))
        self._set_hyper("sma_threshold", sma_threshold)
        self.decay_betas = decay_betas
        self.p = p
        self.epsilon = epsilon or tf.keras.backend.epsilon()
        self._has_weight_decay = weight_decay != 0.0

    def _create_slots(self, var_list):
        for var in var_list:
            self.add_slot(var, "m")
        for var in var_list:
            self.add_slot(var, "v")

    def set_weights(self, weights):
        params = self.weights
        num_vars = int((len(params) - 1) / 2)
        if len(weights) == 3 * num_vars + 1:
            weights = weights[: len(params)]
        super().set_weights(weights)

    def _decayed_wd(self, var_dtype):
        var_t = self._get_hyper("weight_decay", var_dtype)
        if isinstance(var_t, tf.keras.optimizers.schedules.LearningRateSchedule):
            var_t = tf.cast(var_t(self.iterations), var_dtype)
        return var_t
        
    def _decayed_beta1(self, var_dtype):
        beta_1 = self._get_hyper("beta_1", var_dtype)

        if not self.decay_betas:
            return beta_1

        steps = tf.cast(self.iterations, var_dtype)
        total_steps = self._get_hyper("total_steps", var_dtype)
        beta_1_final = self._get_hyper("beta_1_final", var_dtype)
        steps_left_frac = 1.0 - (steps / total_steps)
        decay = steps_left_frac / ((1.0 - beta_1) + beta_1 * steps_left_frac)
        return (1.0 - decay) * beta_1_final + beta_1 * decay

    def _resource_apply_dense(self, grad, var):
        var_dtype = var.dtype.base_dtype
        lr_t = self._decayed_lr(var_dtype)
        wd_t = self._decayed_wd(var_dtype)
        m = self.get_slot(var, "m")
        v = self.get_slot(var, "v")
        beta_1_t = self._decayed_beta1(var_dtype)
        beta_2_t = self._get_hyper("beta_2", var_dtype)
        epsilon_t = tf.convert_to_tensor(self.epsilon, var_dtype)
        p_t = tf.convert_to_tensor(self.p, var_dtype)
        local_step = tf.cast(self.iterations + 1, var_dtype)
        beta_1_power = tf.pow(beta_1_t, local_step)
        beta_2_power = tf.pow(beta_2_t, local_step)

        sma_inf = 2.0 / (1.0 - beta_2_t) - 1.0
        sma_t = sma_inf - 2.0 * local_step * beta_2_power / (1.0 - beta_2_power)

        m_t = m.assign(
            beta_1_t * m + (1.0 - beta_1_t) * grad, use_locking=self._use_locking
        )
        m_corr_t = m_t / (1.0 - beta_1_power)

        v_t = v.assign(
            beta_2_t * v + (1.0 - beta_2_t) * tf.square(grad),
            use_locking=self._use_locking,
        )
        v_corr_t = tf.pow(v_t / (1.0 - beta_2_power), p_t)

        r_t = tf.sqrt(
            (sma_t - 4.0) / (sma_inf - 4.0) * 
            (sma_t - 2.0) / (sma_inf - 2.0) * 
            sma_inf / sma_t
        )

        sma_threshold = self._get_hyper("sma_threshold", var_dtype)
        var_t = tf.where(
            sma_t >= sma_threshold, r_t * m_corr_t / (v_corr_t + epsilon_t), m_corr_t
        )

        if self._has_weight_decay:
            var_t += wd_t * var

        var_update = var.assign_sub(lr_t * var_t, use_locking=self._use_locking)

        updates = [var_update, m_t, v_t]
        return tf.group(*updates)

    def _resource_apply_sparse(self, grad, var, indices):
        var_dtype = var.dtype.base_dtype
        lr_t = self._decayed_lr(var_dtype)
        wd_t = self._decayed_wd(var_dtype)
        beta_1_t = self._decayed_beta1(var_dtype)
        beta_2_t = self._get_hyper("beta_2", var_dtype)
        epsilon_t = tf.convert_to_tensor(self.epsilon, var_dtype)
        p_t = tf.convert_to_tensor(self.p, var_dtype)
        local_step = tf.cast(self.iterations + 1, var_dtype)
        beta_1_power = tf.pow(beta_1_t, local_step)
        beta_2_power = tf.pow(beta_2_t, local_step)

        sma_inf = 2.0 / (1.0 - beta_2_t) - 1.0
        sma_t = sma_inf - 2.0 * local_step * beta_2_power / (1.0 - beta_2_power)

        m = self.get_slot(var, "m")
        m_scaled_g_values = grad * (1 - beta_1_t)
        m_t = m.assign(m * beta_1_t, use_locking=self._use_locking)
        with tf.control_dependencies([m_t]):
            m_t = self._resource_scatter_add(m, indices, m_scaled_g_values)
        m_corr_t = m_t / (1.0 - beta_1_power)

        v = self.get_slot(var, "v")
        v_scaled_g_values = (grad * grad) * (1 - beta_2_t)
        v_t = v.assign(v * beta_2_t, use_locking=self._use_locking)
        with tf.control_dependencies([v_t]):
            v_t = self._resource_scatter_add(v, indices, v_scaled_g_values)

        v_corr_t = tf.pow(v_t / (1.0 - beta_2_power), p_t)

        r_t = tf.sqrt(
            (sma_t - 4.0) / (sma_inf - 4.0) * 
            (sma_t - 2.0) / (sma_inf - 2.0) * 
            sma_inf / sma_t
        )

        sma_threshold = self._get_hyper("sma_threshold", var_dtype)
        var_t = tf.where(
            sma_t >= sma_threshold, r_t * m_corr_t / (v_corr_t + epsilon_t), m_corr_t
        )

        if self._has_weight_decay:
            var_t += wd_t * var

        with tf.control_dependencies([var_t]):
            var_update = self._resource_scatter_add(
                var, indices, tf.gather(-lr_t * var_t, indices)
            )

        updates = [var_update, m_t, v_t]
        return tf.group(*updates)

    def get_config(self):
        config = super().get_config()
        config.update(
            {
                "learning_rate": self._serialize_hyperparameter("learning_rate"),
                "beta_1": self._serialize_hyperparameter("beta_1"),
                "beta_1": self._serialize_hyperparameter("beta_1_final"),
                "beta_2": self._serialize_hyperparameter("beta_2"),
                "decay": self._serialize_hyperparameter("decay"),
                "weight_decay": self._serialize_hyperparameter("weight_decay"),
                "sma_threshold": self._serialize_hyperparameter("sma_threshold"),
                "total_steps": int(self._serialize_hyperparameter("total_steps")),
                "decay_betas": self.decay_betas,
                "p": self.p,
                "epsilon": self.epsilon
            }
        )
        return config
