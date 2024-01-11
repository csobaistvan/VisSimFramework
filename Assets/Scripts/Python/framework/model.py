# custom packages
from framework.helpers import logging_helper as lh

# local packages
from framework.training_data_analyzer import TrainingDataAnalyzer
from framework.model_trainer import ModelTrainer
from framework.model_tuner import ModelTuner
from framework.trained_network_analyzer import TrainedNetworkAnalyzer
from framework.trained_network_exporter import TrainedNetworkExporter

from framework.ann.model_builder import ANNModelBuilder
from framework.ann.model_tuner import ANNModelTuner
from framework.boosted_trees.model_builder import BoostedTreesModelBuilder
from framework.boosted_trees.model_tuner import BoostedTreesModelTuner

# create a new, module-level logger
logger = lh.get_main_module_logger()

# object for wrapping the handling of model training
class Model(object):
    # init
    def __init__(self, framework):
        # cache the input framework and its relevant elements
        self.framework = framework
        self.config = framework.config
        self.data_generator = framework.data_generator

        # create the appropriate model class
        self.model_classes = {
            "ann": {
                "model_builder": ANNModelBuilder,
                "model_tuner": ANNModelTuner
            },
            "boostedtrees": {
                "model_builder": BoostedTreesModelBuilder,
                "model_tuner": BoostedTreesModelTuner
            }
        }

    # gets the appropriate model builder class
    def get_model_classes(self, network_params):
        return self.model_classes[network_params.model_type.lower()]

    # gets the appropriate model builder class
    def get_model_builder_class(self, network_params):
        return self.get_model_classes(network_params)["model_builder"]

    # gets the appropriate model tuner class
    def get_model_tuner_class(self, network_params):
        return self.get_model_classes(network_params)["model_tuner"]
    
    # visualizes the training and evaluation data
    def analyze_training_data(self):
        TrainingDataAnalyzer(
            config=self.config,
            data_generator=self.data_generator,
            model_builder_class=self.get_model_builder_class(self.config.network)
        ).analyze()

    # trains the network
    def train_network(self):
        ModelTrainer(
            config=self.config,
            data_generator=self.data_generator,
            model_builder_class=self.get_model_builder_class(self.config.network)
        ).train()

    # tunes the network hyperparams
    def tune_hparams(self):
        ModelTuner(
            config=self.config,
            data_generator=self.data_generator,
            model_tuner_class=self.get_model_tuner_class(self.config.network)
        ).tune()

    # exports the network
    def export_network(self):
        TrainedNetworkExporter(
            config=self.config,
            data_generator=self.data_generator,
            model_builder_class=self.get_model_builder_class(self.config.network)
        ).export()

    # tests the performance of the trained estimator model
    def test_network(self):
        TrainedNetworkAnalyzer(
            config=self.config,
            data_generator=self.data_generator,
            model_builder_class=self.get_model_builder_class(self.config.network)
        ).analyze()
