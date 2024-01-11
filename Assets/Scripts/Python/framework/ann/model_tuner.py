# custom packages
from framework.helpers.dotdict import dotdict
from framework.helpers import logging_helper as lh
from framework.helpers import tensorflow_helper as tfh
from framework.helpers import modules_helper as mh

# core packages
import time
from tabulate import tabulate

# data & machine learning packages
import keras_tuner as kt
from keras_tuner.engine import tuner_utils
from tensorboard.plugins.hparams import api as hparams_api

# local packages
from framework.enums import TrainingType
from framework.base.model_tuner import ModelTuner
from framework.ann.model_builder import ANNModelBuilder

# create a new, module-level logger
logger = lh.get_main_module_logger()

# custom hypermodel object
class ANNHypermodel(kt.HyperModel):
    def __init__(self, name, tunable, config, data_generator, network_params, tune_params):
        super().__init__(name=name, tunable=tunable)

        self.config = config
        self.data_generator = data_generator
        self.network_params = network_params
        self.tune_params = tune_params

    def _extract_hparam(self, hp, hparam_name, hparam_def):
        # arguments to build the hparam definition
        kwargs = {
            'name': hparam_name,
            'parent_name': hparam_def.condition.name if 'condition' in hparam_def else None,
            'parent_values': hparam_def.condition.values if 'condition' in hparam_def and 'values' in hparam_def.condition else None,
        }
        hparam_class = None

        # Boolean hyperparameters
        if 'default' in hparam_def and type(hparam_def.default) is bool:
            hparam_class = hp.Boolean
            kwargs.update({
                'default': hparam_def.default,
            })

        # Fixed hyperparameters
        elif 'fixed' in hparam_def:
            hparam_class = hp.Fixed
            kwargs.update({
                'value': hparam_def.fixed,
            })

        # Choice hyperparameters
        elif 'choices' in hparam_def:
            hparam_class = hp.Choice
            kwargs.update({
                'values': hparam_def.choices,
            })

        # Int / float hyperparameters
        if 'min' in hparam_def and 'max' in hparam_def:
            if type(hparam_def.min) is int:
                hparam_class = hp.Int
            elif type(hparam_def.min) is float:
                hparam_class = hp.Float
            kwargs.update({
                'min_value': hparam_def.min,
                'max_value': hparam_def.max,
                'step': hparam_def.step if 'step' in hparam_def else None,
                'sampling': hparam_def.sampling if 'sampling' in hparam_def else None,
                'default': hparam_def.default if 'default' in hparam_def else None,
            })

        # unable to interpret hyperparam definition
        if hparam_class is None:
            raise RuntimeError("Unable to interpret hyperparameter definition for hyperparam '{}'", hparam_name)

        # clean the unset kwargs and instantiate the hparam class
        kwargs = { k: v for k, v in kwargs.items() if v is not None }
        return hparam_class(**kwargs)

    def _set_hparam(self, network_params, hparam_variable, hparam_value):
        parent = network_params
        keys_split = hparam_variable.split('.')
        for key in keys_split[:-1]:
            parent = parent[key]
        parent[keys_split[-1]] = hparam_value

    def create_model_builder(self, hp):
        # start with a fresh copy of the network parameters
        network_params = self.network_params

        # iterate over all the hyperparameters
        for hparam_name, hparam_def in self.tune_params.hyperparameters.items():
            # extract the value of the hyperparameter
            hparam_value = self._extract_hparam(hp, hparam_name, hparam_def)
            
            # set the hparam on the network parameters
            if isinstance(hparam_def.variable, list):
                for hparam_variable in hparam_def.variable:
                    self._set_hparam(network_params, hparam_variable, hparam_value)
            else:
                self._set_hparam(network_params, hparam_def.variable, hparam_value)
            
        # construct the model builder object
        model_builder = ANNModelBuilder(
            config=self.config, 
            data_generator=self.data_generator, 
            network_params=network_params)

        # prepare the training data for tuning
        training_data = model_builder.prepare_training_data(training_type=TrainingType.Tune)

        # compile the model too
        training_data.model.compile(**training_data.model_compile_args)
        
        # handle custom epochs coming from the Hyperband oracle
        if "tuner/epochs" in hp.values:
            training_data.model_train_args["epochs"] = hp.values["tuner/epochs"]
            training_data.model_train_args["initial_epoch"] = hp.values["tuner/initial_epoch"]

        # store the elements in the hypermodel
        return dotdict({
            "model_builder": model_builder,
            "training_data": training_data
        })

    def build(self, hp):
        return self.create_model_builder(hp).training_data.model

# custom HyperParam tuner
class ANNTuner(kt.Tuner):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        # get rid of the wrapped hypermodel
        self.hypermodel = kwargs["hypermodel"]

        # decrease the number of stored checkpoints
        self._save_n_checkpoints = 5

        # search start and end times
        self.search_start_time = None
        self.search_end_time = None
        
    def save_model(self, trial_id, model, step=0):
        logger.info('Saving model for trial: {}, epoch: {}', trial_id, step)

        # save the whole model
        tfh.export_keras_model(model, self._get_checkpoint_dir(trial_id, step), save_formats=['tf'])

        # delete unneeded checkpoints
        if self.oracle.get_trial(trial_id).metrics.exists(self.oracle.objective.name):
            epoch_to_delete = step - self._save_n_checkpoints
            best_epoch = self.oracle.get_trial(trial_id).metrics.get_best_step(self.oracle.objective.name)
            if step > self._save_n_checkpoints and epoch_to_delete != best_epoch:
                self._delete_checkpoint(trial_id, epoch_to_delete)

    def load_model(self, trial):
        logger.info('Loading model for trial: {}', trial.trial_id)
        return tfh.restore_trained_keras_model(self._get_checkpoint_dir(trial.trial_id, trial.best_step), save_format='tf')

    def run_trial(self, trial):
        # extract the wrapped hypermodel and the hyperparameters
        hp = trial.hyperparameters

        # create a custom model builder and the corresponding training data
        train_elements = self.hypermodel.create_model_builder(hp)
        
        # inject the tuner callback
        train_elements.training_data.model_train_args['callbacks'].append(tfh.KerasTunerCallback(self, trial))
        
        # Patch TensorBoard log_dir and convert the HParams to TensorFlow's hparams
        for callback in train_elements.training_data.model_train_args['callbacks']:
            if callback.__class__.__name__ == "TensorBoard":
                logdir = self.get_trial_dir(trial.trial_id)
                callback.log_dir = logdir
                hparams = tuner_utils.convert_hyperparams_to_hparams(trial.hyperparameters)
                hparams_callback = hparams_api.KerasCallback(writer=logdir, hparams=hparams, trial_id=trial.trial_id)
                train_elements.training_data.model_train_args['callbacks'].append(hparams_callback)

        # load the best checkpoint when continuing a previous trial
        if "tuner/trial_id" in hp.values:
            history_trial = self.oracle.get_trial(hp.values["tuner/trial_id"])
            train_elements.training_data.model = self.load_model(history_trial)
            train_elements.training_data.model.compile(**train_elements.training_data.model_compile_args) # recompile the model
        
        # invoke fit and return the result
        return train_elements.training_data.model.fit(**train_elements.training_data.model_train_args)

    def search(self, *args, **kwargs):
        self.search_start_time = time.time()
        super().search(*args, **kwargs)
        self.search_end_time = time.time()

# helper class for building the results table
class Tabulator(dict):
    def __init__(self):
        self.values = {}

    def append(self, name, value):
        if name not in self.values:
            self.values[name] = []
        self.values[name].append(value)

    def tabulate(self, **kwargs):
        return tabulate(tabular_data=self.values, **kwargs)

# helper class for building training datasets
class ANNModelTuner(ModelTuner):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
     
    def prepare_elements(self):
        result = dotdict()

        # define the objective for the oracle
        result.objective = kt.Objective(self.tune_params.objective, 'min')

        # construct the tuner oracle
        common_oracle_params = { 
            'objective': result.objective,
            'seed': self.tune_params.random_seed }
        oracles = {
            "Random": (kt.oracles.RandomSearchOracle, { **common_oracle_params }),
            "Bayesian": (kt.oracles.BayesianOptimizationOracle, { **common_oracle_params }),
            "Hyperband": (kt.oracles.HyperbandOracle, { **common_oracle_params })
        }
        oracle_class, oracle_params = oracles[self.tune_params.algorithm]
        if 'algorithm_params' in self.tune_params and self.tune_params.algorithm in self.tune_params.algorithm_params:
            oracle_params.update(self.tune_params.algorithm_params[self.tune_params.algorithm])
        result.oracle = oracle_class(**oracle_params)

        # generate various output names and paths
        result.project_name = tfh.get_tuner_name(
            network_params=self.network_params, 
            tune_params=self.tune_params)
        result.out_folder = tfh.get_network_type_folder(
            module_name=mh.get_main_module(),
            network_type=self.network_params.type, 
            model_type=self.network_params.model_type) + '/_tuning/'
        result.hparam_log_dir = result.out_folder + result.project_name + '/hparams/'

        # create the hypermodel object
        result.hypermodel = ANNHypermodel(
            name='HyperParamTuningModel', 
            tunable=True,
            config=self.config,
            data_generator=self.data_generator,
            network_params=self.network_params,
            tune_params=self.tune_params
        )

        # construct the tuner itself
        result.tuner = ANNTuner(
            oracle=result.oracle,
            hypermodel=result.hypermodel,
            directory=result.out_folder,
            project_name=result.project_name
        )

        return result

    def tabulate_results(self, tuner_elements):
        best_trials = tuner_elements.oracle.get_best_trials(num_trials=10)
        results_table = Tabulator()
        for trial in best_trials:
            results_table.append("trial", trial.trial_id)
            results_table.append("score", trial.score)
            for hp_name, hp_val in trial.hyperparameters.values.items():
                results_table.append(hp_name, hp_val)
        return results_table

    def show_summary(self, tuner_elements):
        # show the elapsed time
        elapsed_time = tuner_elements.tuner.search_end_time - tuner_elements.tuner.search_start_time
        elapsed_time_str = time.strftime("%Hh %Mm %Ss", time.gmtime(elapsed_time))
        print('Search finished in:', elapsed_time_str)

        # show the best results
        results_table = self.tabulate_results(tuner_elements)
        print(results_table.tabulate(headers='keys', tablefmt='psql'))

    # performs hyperparameter tuning
    def tune(self):
        logger.info('Initiating hyperparameter tuning...')

        # prepare the tuner elements
        tuner_elements = self.prepare_elements()

        # perform the search
        logger.info('Performing tuning step...')
        tuner_elements.tuner.search()

        # finish the tuning process
        logger.info('Finalizing hyperparameter tuning process...')        
        self.show_summary(tuner_elements)

        # return the best hyperparameters
        return tuner_elements.tuner.get_best_hyperparameters(num_trials=1)