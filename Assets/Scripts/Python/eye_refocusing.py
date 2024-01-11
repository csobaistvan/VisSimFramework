# core packages
import json
import numpy as np
import matlab.engine

# training framework
from framework.helpers import matlab_helper as mlh
from framework.helpers import logging_helper as lh

from framework.model import Model
from framework.data_generator import DataGenerator
from framework.training_framework import TrainingFramework

# create a new, module-level logger
logger = lh.get_main_module_logger()

# global TrainingFramework instance
the_framework = None

# object for wrapping the handling of data generation
class EyeRefocusingDataGenerator(DataGenerator):
    # init
    def __init__(self, framework):
        super().__init__(framework)

    # get training vars in a file name format
    def get_file_name_prefix(self):
        return "ns[{ns}]_nf[{nf}]_np[{np}]_nsd[{nsd}]_nr[{nr}]".format(
            ns=self.config.data_generator.num_samples, 
            nf=self.config.data_generator.num_focus_steps,
            np=self.config.data_generator.num_passes,
            nsd=self.config.data_generator.num_subdivisions,
            nr=self.config.data_generator.num_rays)

    # generate eye parameter samples
    @DataGenerator.GeneratorCallback(dataset='eye', whole_dataset=True, uses_matlab=False, depends=[])
    def generate_eye_dataset(self):
        num_focus_steps = self.config.data_generator.num_focus_steps
        num_samples_per_focus = self.config.data_generator.num_samples // num_focus_steps
        focus_distance_id = self.param_info.params['FocusDioptres'].col_id

        np.random.seed(self.config.data_generator.random_seed)
        focus_samples_normalized = np.linspace(0.0, 1.0, num_focus_steps)
        focus_samples_normalized = np.repeat(focus_samples_normalized, num_samples_per_focus)
        param_samples_normalized = np.random.uniform(low=0.0, high=1.0, size=(num_samples_per_focus, self.param_info.num_eye_params))
        param_samples_normalized = np.tile(param_samples_normalized, (num_focus_steps, 1))
        param_samples_normalized[:, focus_distance_id] = focus_samples_normalized
        param_samples = self.param_info.denormalize_eye_params(param_samples_normalized)

        return param_samples

    # generates aberration samples
    @DataGenerator.GeneratorCallback(dataset='refocus', whole_dataset=False, uses_matlab=True, threaded=True, depends=['eye'])
    def generate_refocus_dataset(self, environment, sample_id, batch_sample_id):
        # data generation parameters
        param_info = environment.data_generator.param_info
        eye_params = environment.data_generator.datasets['eye'].dataframe
        config = environment.data_generator.config.data_generator

        # init the result to NaN
        result = np.empty((1, environment.num_outputs), dtype=float)
        result[:] = np.NaN

        # compute and store the aberrations
        try: # attempt to compute the aberrations through MATLAB
            # construct a new eye instance
            stage = 'Eye construction'
            eye = mlh.call_fn(environment.matlab_instance.EyeParametric)

            # set the params for our eye instance
            params = eye_params[sample_id].tolist()
            eye_properties = set(mlh.call_fn(environment.matlab_instance.properties, eye))
            for param in param_info.param_list:
                if param.domain == 'eye' and param.name in eye_properties:
                    stage = 'Eye parameter: ' + param.name
                    eye = mlh.call_fn(
                        environment.matlab_instance.SetEyeParameter,
                        eye, param.name, 
                        params[param.col_id])

            # compute the eye parameters for the eye
            stage = 'Make eye elements'
            eye = mlh.call_fn(environment.matlab_instance.MakeElements, eye)

            # extract the parameters for refocusing
            focus_distance = 1.0 / param_info.get_param('FocusDioptres', params)
            lens_diameter = param_info.get_param('LensD', params)
            
            # compute the refocused parameters
            stage = 'Eye refocusing'
            refocused_eye, *_ = mlh.call_fn(
                environment.matlab_instance.FocusAt,
                eye,
                focus_distance,
                587.56, 
                config.num_passes, 
                float(config.num_rays), 
                config.num_subdivisions, 
                matlab.double([lens_diameter * 0.91, lens_diameter]),
                nargout=5)

            # extract the original lens diameter
            stage = 'Get original lens diameter'
            original_ld = mlh.call_fn(
                environment.matlab_instance.GetEyeParameter, 
                eye, 
                'LensD')

            # extract the original aqueous thickness
            stage = 'Get original ACD'
            original_acd = mlh.call_fn(
                environment.matlab_instance.GetEyeParameter, 
                eye, 
                'AqueousT')

            # extract the refocused lens diameter
            stage = 'Get refocused lens diameter'
            refocused_ld = mlh.call_fn(
                environment.matlab_instance.GetEyeParameter, 
                refocused_eye, 
                'LensD')

            # extract the refocused aqueous thickness
            stage = 'Get refocused ACD'
            refocused_acd = mlh.call_fn(
                environment.matlab_instance.GetEyeParameter, 
                refocused_eye, 
                'AqueousT')
            
            # write out the computed results
            stage = 'Storing results'
            result[0, 0] = refocused_ld - original_ld
            result[0, 1] = refocused_acd - original_acd

            stage = 'Delete the eye instance'
            mlh.call_fn(
                environment.matlab_instance.DeleteSelf,
                eye, 
                nargout=0)

            stage = 'Delete the refocused eye instance'
            mlh.call_fn(
                environment.matlab_instance.DeleteSelf,
                refocused_eye, 
                nargout=0)

        except Exception as exc: # log the wrong eye parameters
            self._handle_matlab_error(exc, environment, sample_id, stage, param_info, eye_params)

        # return the result
        return result

# object for wrapping the handling of model training
class EyeRefocusingModel(Model):
    # init
    def __init__(self, framework):
        super().__init__(framework)

if __name__ == '__main__':
    the_framework = TrainingFramework(
        data_generator_cls=EyeRefocusingDataGenerator,
        model_cls=EyeRefocusingModel)
    the_framework.run()