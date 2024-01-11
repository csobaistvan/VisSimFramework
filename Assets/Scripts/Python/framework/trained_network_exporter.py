# custom packages
from framework.helpers import tensorflow_helper as tfh
from framework.helpers import logging_helper as lh
from framework.helpers import macro_helper as mch

# core packages
import json
from pathlib import Path

# data & machine learning packages
import numpy as np

# local packages
from framework.enums import TrainingType

# create a new, module-level logger
logger = lh.get_main_module_logger()

# Exports a trained network
class TrainedNetworkExporter(object):
    def __init__(self, config, data_generator, model_builder_class):
        self.config = config
        self.network_params = self.config.network
        self.data_generator = data_generator
        self.model_builder_class = model_builder_class

    def _export_metadata(self, test_data, out_filepath):
        metadata = {
            "network_config": test_data.network_params,
            "data_config": self.config.data_generator,
            "network_name": test_data.model_name,
            "features": {
                "names": test_data.feature_names,
                "normalization": {
                    "bias": np.array(test_data.x_train_normalization_terms.normalization_bias).tolist(),
                    "scale": np.array(test_data.x_train_normalization_terms.normalization_scale).tolist()
                }
            },
            "targets": {
                "names": test_data.target_names,
                "normalization": {
                    "bias": np.array(test_data.y_train_normalization_terms.normalization_bias).tolist(),
                    "scale": np.array(test_data.y_train_normalization_terms.normalization_scale).tolist()
                }
            }
        }

        with open(out_filepath, 'w') as out_file:
            out_file.write(json.dumps(metadata, indent=4))

    def _export_network_tf(self, test_data, out_folder, out_name):
        # export the trained model
        out_path = out_folder + out_name
        test_data.model.save(filepath=out_path, overwrite=True, include_optimizer=False, save_format='tf')

        # export the corresponding model metadata
        if self.config.export.export_metadata:
            metadata_filepath = "{folder}{name}/metadata.json".format(folder=out_folder, name=out_name)
            self._export_metadata(test_data, metadata_filepath)
    
    def _export_network_hdf(self, test_data, out_folder, out_name):
        # export the trained model
        out_path = out_folder + out_name + ".h5"
        test_data.model.save(filepath=out_path, overwrite=True, include_optimizer=False, save_format='h5')
        
        # export the corresponding model metadata
        if self.config.export.export_metadata:
            metadata_filepath = "{folder}{name}_metadata.json".format(folder=out_folder, name=out_name)
            self._export_metadata(test_data, metadata_filepath)

    # exports the network
    def export(self):
        # instantiate the model builder
        model = self.model_builder_class(
            config=self.config, 
            data_generator=self.data_generator, 
            network_params=self.network_params)

        # prepare the test data
        test_data = model.prepare_training_data(
            training_type=TrainingType.Test)

        # list of export formats available
        format_saver_callbacks = [
            ([ 'saved_model', 'tf' ], self._export_network_tf),
            ([ 'hdf5', 'h5' ], self._export_network_hdf)
        ]

        with mch.ScopedMacros({
            "network_type": test_data.network_params.type,
            "network_name": test_data.model_name,
            "model_type": test_data.network_params.model_type,
        }):
            # save the model in each export format
            for format_keys, save_callback in format_saver_callbacks:
                # skip the format if it is not on the list of formats requested
                if len(set(self.config.export.export_formats).intersection(set(format_keys))) == 0:
                    continue

                # construct the output path
                out_folder = mch.resolve_macros(self.config.export.target_folder)
                out_name = mch.resolve_macros(self.config.export.name_format)

                # make sure the output folder exists
                Path(out_folder).mkdir(parents=True, exist_ok=True)
        
                logger.info('Saving trained model to {}, with name: {}', out_folder, out_name)

                # invoke the appropriate export function
                save_callback(test_data, out_folder, out_name)
