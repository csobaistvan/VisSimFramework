# custom packages
from framework.helpers import logging_helper as lh
from framework.helpers.dotdict import dotdict

# core packages
import functools

# data & machine learning packages
import numpy as np
import pandas as pd

# create a new, module-level logger
logger = lh.get_main_module_logger()

# Parameter info helper object
class ParamInfo(object):
    # common methods
    def __init__(self, expected_domains=[]):
        self.params = {} # parameter values assigned to their names
        self.param_list = [] # raw list of the same parameter objects
        self.num_params = 0 # number of parameters (for quick access)

        # workaround for Pylint; init the expected domains with None's
        for domain in expected_domains:
            self._register_domain(domain, [])

    def __str__(self):
        return self.params.__str__()

    def __repr__(self):
        return self.params.__repr__()

    # extracts the parameter value corresponding to 'name' from the input set of parameters
    def get_param(self, name, params):
        return params[self.params[name].col_id]

    # adds a new parameter
    def add_param(self, **kwargs):
        self.params[kwargs['name']] = dotdict(kwargs)
        self.params[kwargs['name']]['col_id'] = self.num_params
        self.param_list.append(self.params[kwargs['name']])
        self.num_params = self.num_params + 1

    def _register_domain(self, domain, param_list):
        param_list_name = '{domain}_param_list'.format(domain=domain)
        setattr(self, param_list_name, param_list)

        num_params_name = 'num_{domain}_params'.format(domain=domain)
        num_params = len(param_list)
        setattr(self, num_params_name, num_params)
    
        denormalize_fn_name = 'denormalize_{domain}_params'.format(domain=domain)
        denormalize_fn = functools.partial(self.denormalize_params_by_domain, param_domain=domain)
        setattr(self, denormalize_fn_name, denormalize_fn)

        normalize_fn_name = 'normalize_{domain}_params'.format(domain=domain)
        normalize_fn = functools.partial(self.normalize_params_by_domain, param_domain=domain)
        setattr(self, normalize_fn_name, normalize_fn)

    # builds the internal helper variables
    def build(self):
        # build a list of parameter minimums and parameter maximums
        self.min_values = np.asarray([var.min for var in self.param_list], dtype=float)
        self.max_values = np.asarray([var.max for var in self.param_list], dtype=float)

        # also build a list with the parameter extents
        self.extents = self.max_values - self.min_values

        # build a list of all the domains available
        self.domains = list(set([ param.domain for param in self.param_list ]))

        # build unique lists for all the domains
        for domain in self.domains:
            param_list = [ param for param in self.param_list if param.domain == domain ]
            self._register_domain(domain, param_list)

    # transforms a set of normalized parameters to their true values
    def denormalize_params_by_name(self, params_normalized, param_names):
        # extract the corresponding values
        min_values = [ self.min_values[i] for i, param in enumerate(self.param_list) if param.name in param_names ]
        extents = [ self.extents[i] for i, param in enumerate(self.param_list) if param.name in param_names ]

        # fall back to the single parameter set case if we only get a single array
        if len(params_normalized.shape) == 1:
            return min_values + params_normalized * extents

        # init the resulting structure
        params_denormalized = np.copy(params_normalized)
        for row_id in range(params_normalized.shape[0]):
            params_denormalized[row_id] = min_values + params_normalized[row_id] * extents

        # return the transformed parameters
        return params_denormalized

    def denormalize_params_by_domain(self, params_normalized, param_domain):
        # extract the corresponding values
        min_values = [ self.min_values[i] for i, param in enumerate(self.param_list) if param.domain == param_domain ]
        extents = [ self.extents[i] for i, param in enumerate(self.param_list) if param.domain == param_domain ]

        # fall back to the single parameter set case if we only get a single array
        if len(params_normalized.shape) == 1:
            return min_values + params_normalized * extents

        # init the resulting structure
        params_denormalized = np.copy(params_normalized)
        for row_id in range(params_normalized.shape[0]):
            params_denormalized[row_id] = min_values + params_normalized[row_id] * extents

        # return the transformed parameters
        return params_denormalized

    def denormalize_params_dataframe(self, params_normalized):
        return pd.DataFrame(
            self.denormalize_params_by_name(params_normalized.values, params_normalized.keys()), 
            columns=params_normalized.keys())
        
    def denormalize_params(self, params_normalized, param_names=None, param_domain=None):
        if type(params_normalized) is pd.DataFrame:
            return self.denormalize_params_dataframe(params_normalized)

        elif type(params_normalized) is np.ndarray:
            if not param_names is None:
                return self.denormalize_params_by_name(params_normalized, param_names)
            elif not param_domain is None:
                return self.denormalize_params_by_domain(params_normalized, param_domain)

        raise ValueError('Invalid combination of parameter data type and other parameters.')

    def normalize_params_by_name(self, params_unnormalized, param_names):
        # extract the corresponding values
        min_values = [ self.min_values[i] for i, param in enumerate(self.param_list) if param.name in param_names ]
        extents = [ self.extents[i] for i, param in enumerate(self.param_list) if param.name in param_names ]

        # fall back to the single parameter set case if we only get a single array
        if len(params_unnormalized.shape) == 1:
            return (params_unnormalized - min_values) / extents

        # init the resulting structure
        params_normalized = np.copy(params_unnormalized)
        for row_id in range(params_unnormalized.shape[0]):
            params_normalized[row_id] = (params_unnormalized[row_id] - min_values) / extents

        # return the transformed parameters
        return params_normalized

    def normalize_params_by_domain(self, params_unnormalized, param_domain):
        # extract the corresponding values
        min_values = [ self.min_values[i] for i, param in enumerate(self.param_list) if param.domain == param_domain ]
        extents = [ self.extents[i] for i, param in enumerate(self.param_list) if param.domain == param_domain ]

        # fall back to the single parameter set case if we only get a single array
        if len(params_unnormalized.shape) == 1:
            return (params_unnormalized - min_values) / extents

        # init the resulting structure
        params_normalized = np.copy(params_unnormalized)
        for row_id in range(params_unnormalized.shape[0]):
            params_normalized[row_id] = (params_unnormalized[row_id] - min_values) / extents

        # return the transformed parameters
        return params_normalized

    def normalize_params_dataframe(self, params_unnormalized):
        return pd.DataFrame(
            self.normalize_params_by_name(params_unnormalized.values, params_unnormalized.keys()), 
            columns=params_unnormalized.keys())
        
    def normalize_params(self, params_unnormalized, param_names=None, param_domain=None):
        if type(params_unnormalized) is pd.DataFrame:
            return self.normalize_params_dataframe(params_unnormalized)

        elif type(params_unnormalized) is np.ndarray:
            if not param_names is None:
                return self.normalize_params_by_name(params_unnormalized, param_names)
            elif not param_domain is None:
                return self.normalize_params_by_domain(params_unnormalized, param_domain)

        raise ValueError('Invalid combination of parameter data type and other parameters.')

    def filter_param_list(self, network_type, role, extract_attrib=None, transform_fn=None):
        if transform_fn is None:
            if extract_attrib is None:
                transform_fn = lambda x: x
            else:
                transform_fn = lambda x: getattr(x, extract_attrib)
        return [ transform_fn(param) for param in self.param_list if param.role[network_type] == role ]

    def extract_columns_from_dataset(self, dataset, network_type, param_role):
        all_column_names = map(lambda param: param.name, self.filter_param_list(network_type, param_role))
        colum_names_in_dataset = [ name for name in all_column_names if name in dataset.keys() ]
        return dataset[ colum_names_in_dataset ]

    def extract_columns_from_datasets(self, datasets, network_type, param_role):
        columns_from_datasets = list(map(lambda dataset: self.extract_columns_from_dataset(dataset, network_type, param_role), datasets))
        return columns_from_datasets[0].join(columns_from_datasets[1:])

    def extract_columns(self, dataset, network_type, param_role):
        if type(dataset) is list:
            return self.extract_columns_from_datasets(dataset, network_type, param_role) 
        elif type(dataset) is dict or type(dataset) is dotdict:
            return self.extract_columns_from_datasets(dataset.values(), network_type, param_role) 
        return self.extract_columns_from_dataset(dataset, network_type, param_role) 

    def extract_target_weights(self, network_type):
        return [ param.weight[network_type] for param in self.param_list if param.role[network_type] == 'target']
