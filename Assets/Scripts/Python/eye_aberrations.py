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
class EyeAberrationsDataGenerator(DataGenerator):
    # init
    def __init__(self, framework):
        super().__init__(framework)

    # get training vars in a file name format
    def get_file_name_prefix(self):
        return "ns[{ns}]_md[{md}]_nr[{nr}]_gs[{gs}]_sd[{sd}]".format(
            ns=self.config.data_generator.num_samples, 
            md=self.config.data_generator.max_degree, 
            nr=self.config.data_generator.num_rays,
            gs=self.config.data_generator.grid_shape,
            sd=self.config.data_generator.random_seed)

    # generate eye parameter samples
    @DataGenerator.GeneratorCallback(dataset='eye', whole_dataset=True, uses_matlab=False, depends=[])
    def generate_eye_dataset(self):
        np.random.seed(self.config.data_generator.random_seed)
        param_samples_normalized = np.random.uniform(low=0.0, high=1.0, size=(self.config.data_generator.num_samples, self.param_info.num_eye_params))
        param_samples = self.param_info.denormalize_eye_params(param_samples_normalized)
        return param_samples

    # generates aberration samples
    @DataGenerator.GeneratorCallback(dataset='aberration', whole_dataset=False, uses_matlab=True, threaded=True, depends=['eye'])
    def generate_aberration_dataset(self, environment, sample_id, batch_sample_id):
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
            param_names = [ param.name for param in param_info.param_list if param.domain == 'eye' ]
            param_values = [ params[param.col_id] for param in param_info.param_list if param.domain == 'eye' ]
            
            eye = mlh.call_fn(
                environment.matlab_instance.SetEyeParameters, 
                eye, 
                param_names, 
                param_values)

            # compute the eye parameters for the eye
            stage = 'Make eye elements'
            eye = mlh.call_fn(environment.matlab_instance.MakeElements, eye)

            # compute the corresponding aberrations
            stage = 'Compute eye aberrations'
            mlh.call_fn(
                environment.matlab_instance.ComputeAberrations,
                eye, 
                'NumRays', float(config.num_rays),
                'MaxDegree', config.max_degree,
                'TraceVectors', 'chief', 
                'TraceVectorsEye', 'input', 
                'TraceVectorsRays', float(100),
                'TraceVectorsTol', 1e-6,
                'IgnoreMissed', True, 
                'IgnoreBlocked', True, 
                'IgnoreTIR', True,
                'GridShape', config.grid_shape,
                'GridSpread', 'trace', 
                'GridFitPasses', 3,
                'CaptureDistance', 1e-1,
                'CaptureSize', 1e6,
                'RadiusThreshold', 1.0,
                'ProjectionMethod', 'parallel',
                'CircumscribeRays', 'expected',
                'CircumscribeShape', 'ellipse',
                'CircumscribeExtension', 'mirror',
                'EllipsePrecision', 2e-4,
                'Centering', 'chief',
                'Stretching', 'ellipse2circle',
                'PupilRounding', 0.001,
                'FitMethod', 'lsq',
                nargout=0)

            # extract the aberrations
            stage = 'Extract aberrations'
            aberrations = mlh.call_fn(
                environment.matlab_instance.GetAberrations,
                eye)

            # write out the results
            stage = 'Storing results'
            result[:] = np.array(aberrations._data, copy=True)

            # verification
            stage = 'Verification (Python)'
            verification_result = mlh.call_fn(
                environment.matlab_instance.GetEyeParameter,
                eye,
                'Alpha')
                
            if not np.array_equal(np.array(aberrations._data, copy=False), np.array(verification_result._data, copy=True)):
                raise RuntimeError('Sample failed verification.')

            stage = 'Verification (Matlab)'
            verification_result = mlh.call_fn(
                environment.matlab_instance.ValidateAberrations,
                eye,
                matlab.double(result.tolist()))

            stage = 'Delete the eye instance'
            mlh.call_fn(
                environment.matlab_instance.DeleteSelf,
                eye, 
                nargout=0)

        except Exception as exc: # log the wrong eye parameters
            self._handle_matlab_error(exc, environment, sample_id, stage, param_info, eye_params)

        # return the result
        return result

# object for wrapping the handling of model training
class EyeAberrationsModel(Model):
    # init
    def __init__(self, framework):
        super().__init__(framework)

if __name__ == '__main__':
    the_framework = TrainingFramework(
        data_generator_cls=EyeAberrationsDataGenerator,
        model_cls=EyeAberrationsModel)
    the_framework.run()