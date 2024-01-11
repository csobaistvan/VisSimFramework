# custom packages
from framework.helpers import tensorflow_helper as tfh
from framework.helpers import matlab_helper as mlh
from framework.helpers import modules_helper as mh
from framework.helpers import logging_helper as lh
from framework.helpers import filename_helpers as fnh
from framework.helpers import slack_helper as sh
from framework.helpers import datetime_helper as dth
from framework.helpers import winapi_helper as wah
from framework.helpers.dotdict import dotdict

# core packages
import psutil
import time
import humanize, slackblocks
import threading
import csv, json
from pathlib import Path

# data & machine learning packages
import numpy as np
import pandas as pd

# local packages
from framework.param_info import ParamInfo

# create a new, module-level logger
logger = lh.get_main_module_logger()

# object for wrapping the handling of data generation
class DataGenerator(object):
    # Decorator for a data generator callback
    class GeneratorCallback(object):
        def __init__(self, dataset, whole_dataset=True, uses_matlab=False, threaded=True, depends=[], **kwargs):
            self.fn = None
            self.dataset = dataset
            self.whole_dataset = whole_dataset
            self.uses_matlab = uses_matlab
            self.threaded = threaded
            self.depends = depends
            self.__dict__.update(kwargs)

        def __call__(self, *args, **kwargs):
            if len(args) == 1 and len(kwargs) == 0 and self.fn == None:
                self.fn = args[0]
                return self
            return self.fn(*args, **kwargs)

    # init
    def __init__(self, framework):
        # cache the input framework and its config
        self.framework = framework
        self.config = framework.config

        # read the train parameters from disk
        self.param_info = self.read_train_parameters()

        # make a cache of all the loaded and generated data
        self.cache = None

        # build a list of datasets to generate
        self.datasets = self.get_datasets()

    # get training vars in a file name format
    def get_file_name_prefix(self):
        return fnh.hash_filename(str(self.config.data_generator))
    
    # generates the file name for the training data
    def get_data_file_name(self, suffix, fmt='pkl'):
        return "Data/Train/{module}/{suffix}_{params}.{fmt}".format(
            module=mh.get_main_module(),
            params=self.get_file_name_prefix(),
            suffix=suffix,
            fmt=fmt)

    # fills a dict with all the attributes of the parameter generator
    def _process_generator(self, generator):
        result = dotdict()
        result.callback = generator
        result.columns = getattr(self.param_info, '{dataset}_param_list'.format(dataset=generator.dataset))
        result.whole_dataset = generator.whole_dataset
        result.depends = generator.depends
        result.uses_matlab = generator.uses_matlab
        result.threaded = generator.threaded
        result.dataframe = None
        return result

    # returns a dict with all the generator functions
    def _get_generators(self):
        return { 
            k: getattr(self, k) 
            for k in dir(self) 
            if type(getattr(self, k)) is DataGenerator.GeneratorCallback
        }

    # returns a dict with all the datasets that we need to generate
    def get_datasets(self):
        return dotdict({ 
            generator.dataset: self._process_generator(generator) 
            for _, generator in self._get_generators().items() 
        })

    # reads back the train parameters from disk
    def read_train_parameters(self):
        logger.info('Reading training parameters...')

        # read the CSV file
        eye_params_path = 'Data/GeneratorParameters/{filename}'.format(filename=self.config.data_generator.data_parameters)
        with open(eye_params_path, 'r') as eye_params_file:
            reader = csv.reader(eye_params_file, delimiter=';')
            param_attribs = self.get_train_parameters(reader)  # invoke the parameter extractor callback
        
        # init the param attrib object's internal state
        param_attribs.build()

        logger.debug('Training parameters: {}', json.dumps(param_attribs.params, indent=4))

        # return the results
        return param_attribs

    def extract_per_network_params(self, entry_dict, headers, key_name):
        prefix = key_name + '_'
        key_names = filter(lambda header: (prefix in header), headers)
        return { kname.replace(prefix, ''): entry_dict[kname] for kname in key_names }

    # processes a single row of the parameter specifications file
    def get_train_parameters_entry(self, headers, entry):
        # create a dict from all the entries
        entry_dict = { headers[i]: val for i, val in enumerate(entry) }

        # extract the list of role names and put them in a single dict named 'role'
        entry_dict['role'] = self.extract_per_network_params(entry_dict, headers, 'role')
        entry_dict['weight'] = self.extract_per_network_params(entry_dict, headers, 'weight')

        # return the final entry dict
        return entry_dict

    # obtains train parameters from the parameter csv contents
    def get_train_parameters(self, csv):
        # get the column names
        headers = csv.__next__()

        # transform each line to the corresponding param object
        param_attribs = ParamInfo() # resulting parameter info object
        for row in csv:
            param_attribs.add_param(**self.get_train_parameters_entry(headers, row))
        
        # return the results
        return param_attribs

    # estimates the size of the generated data
    def estimate_data_size(self):
        # size of a single float
        float_info = np.finfo(np.float)
        float_size = np.dtype(np.float).itemsize

        # print out the number of samples
        logger.info('=' * 80)
        logger.info('Number of sample points: {ns:,}'.format(ns=self.config.data_generator.num_samples))
        for dataset, dataset_params in self.datasets.items():
            num_columns = len(dataset_params.columns)
            logger.info('Number of {dataset} parameters per sample: {np}'.format(
                dataset=dataset, np=num_columns))
        logger.info('Underlying data type is {dtype} ({size})'.format(
            dtype=float_info.dtype, size=humanize.naturalsize(float_size, binary=True)))
        logger.info('-' * 80)

        # print out the corresponding data size
        total_data_size = 0
        for dataset, dataset_params in self.datasets.items():
            num_columns = len(dataset_params.columns)
            dataset_size_entry = num_columns * float_size
            dataset_size_total = self.config.data_generator.num_samples * dataset_size_entry
            total_data_size = total_data_size + dataset_size_total

            logger.info('Total data size of the {dataset} parameters: {total}; {entry} per entry.'.format(
                dataset=dataset,
                total=humanize.naturalsize(dataset_size_entry, binary=True), 
                entry=humanize.naturalsize(dataset_size_total, binary=True)))
            
        logger.info('-' * 80)
        logger.info('Total generated data size: {total}.'.format(total=humanize.naturalsize(total_data_size, binary=True)))
        logger.info('=' * 80)

    # try to load back a single training data file
    def _load_training_data_file(self, suffix):
        # construct the target filename
        target_filename = self.get_data_file_name(suffix)
        target_filepath = Path(target_filename)

        logger.debug("Attempting to load '{}'", target_filename)

        # make sure it exists
        if not target_filepath.exists():
            return None

        # read back the data and return it
        return pd.read_pickle(target_filename, compression='zip')

    # try to load back the parameter training data
    def _load_training_data(self):
        result = {}
        for dataset_name in self.datasets:
            dataset = self._load_training_data_file(dataset_name)
            if dataset is None:
                return None
            #print(dataset.shape)
            if dataset.shape[0] < self.config.data_generator.num_samples:
                dataset = dataset[0:self.config.data_generator.num_samples]
            result[dataset_name] = dataset

        # remove NaNs/Infs from the dataset
        num_samples = self.config.data_generator.num_samples
        for dataset_name, dataset in result.items():
            notnulls = dataset.notnull().all(axis=1)
            #print(notnulls.shape)
            for dataset_name2, dataset2 in result.items():
                result[dataset_name2] = dataset2.loc[notnulls]
                num_samples_left = dataset2.shape[0]
        num_removed = num_samples - num_samples_left
        if num_removed > 0:
            logger.info("{} rows removed for containing infs/nans", num_removed)

        """
        # remove 0s from the dataset
        num_samples = num_samples_left
        for dataset_name, dataset in result.items():
            nonzeros = (dataset != 0).any(axis=1)
            print(nonzeros.shape)
            for dataset_name2, dataset2 in result.items():
                result[dataset_name2] = dataset2.loc[nonzeros]
                num_samples_left = dataset2.shape[0]
        num_removed = num_samples - num_samples_left
        if num_removed > 0:
            logger.info("{} rows removed for containing only zeros", num_removed)
        """

        return result

    # saves a single data file
    def _save_training_data_file(self, data, suffix):
        # get the output file name
        target_filename = self.get_data_file_name(suffix)
        target_filepath = Path(target_filename)

        # make sure the parent folder exists
        target_folder = Path(target_filepath.parent)
        target_folder.mkdir(parents=True, exist_ok=True)

        # save the data frame
        data.to_pickle(target_filename, compression='zip')

    # save the generated data
    def _save_training_data(self, datasets):
        # write out the datasets
        for dataset_name, dataset in datasets.items():
            self._save_training_data_file(dataset.dataframe, dataset_name)

        # write out the data parameters
        params_fname = self.get_data_file_name("params", "txt")
        with open(params_fname, 'w') as params_file:
            params_file.write(json.dumps(self.config.data_generator, indent=4))
    
    # saves a checkpoint
    def _save_checkpoint(self):
        # TODO: implement
        pass

    def _handle_matlab_error(self, exception, environment, sample_id, stage, param_info, input_params):
        # increment the error counter
        environment.error_count = environment.error_count + 1

        # show the exception information
        logger.warning('Error during computation at stage {} (sample #{:,}); see [DEBUG] for the input parameters.', stage, sample_id, exc_info=True)

        # log the MATLAB exception info
        environment.matlab_instance.eval('exception = MException.last;', nargout=0)
        error_info = environment.matlab_instance.eval('getReport(exception)')
        logger.debug('Matlab exception info: {}', error_info)
        
        # log the eye params that generated the error (might be relevant)
        params = input_params[sample_id].tolist()
        params_dict = { param_info.param_list[i].name: params[i] for i in range(len(params)) }
        logger.debug('Eye parameters: {}', json.dumps(params_dict, indent=4))

        # TODO: send immediate slack message about the error

    def _start_matlab(self, thread_data):
        return mlh.start_matlab(
            thread_id=thread_data.thread_id,
            set_processor_affinity=self.config.matlab.set_affinity,
            process_priority=wah.ProcessPriority[self.config.matlab.priority.upper()])

    # helper for generating data in a threaded manner
    def _generate_dataset_threaded(self, dataset_name, sample_callback):
        # extract the dataset parameters for the dataset
        dataset_params = self.datasets[dataset_name]

        # extract the number of threads
        num_threads = min(self.config.data_generator.num_threads, psutil.cpu_count())

        # total number of samples
        num_samples = self.config.data_generator.num_samples

        # number of samples per thread
        samples_per_thread = num_samples // num_threads

        # per-thread data           
        threads = [dotdict() for i in range(num_threads)]
        
        # main entry point for the workers
        def thread_callback(thread_data):
            # start MATLAB, if needed
            if thread_data.dataset_params.uses_matlab:
                thread_data.matlab_instance = self._start_matlab(thread_data)

            # number of individual samples
            num_samples = thread_data.sample_range[1] - thread_data.sample_range[0]

            # init the thread's output array
            thread_data.dataset = np.zeros((num_samples, thread_data.num_outputs), dtype=float)

            # signal that the thread has been properly initialized
            thread_data.initialized = True

            # loop over the parameter samples
            sample_id_start, sample_id_end = thread_data.sample_range
            sample_id = sample_id_start
            last_error_id = -1
            while sample_id < sample_id_end:
                # restart MATLAB, if needed
                if thread_data.dataset_params.uses_matlab and not mlh.check_matlab_instance(thread_data.matlab_instance):
                    logger.warning('MATLAB instance not running; restarting...')
                    thread_data.matlab_instance = self._start_matlab(thread_data)

                # start time for the sample processing
                sample_start_time = time.time()

                # relative sample id (current batch)
                batch_sample_id = sample_id - thread_data.sample_range[0]
                
                # try to invoke the sample generator callback
                try:
                    sample_data = thread_data.sample_callback(self, thread_data, sample_id, batch_sample_id)
                    thread_data.dataset[batch_sample_id] = sample_data.reshape((1, thread_data.num_outputs))
                except Exception as e:
                    # fill the row with Nans
                    sample_data = np.empty((1, thread_data.num_outputs), dtype=float)
                    sample_data[:] = np.NaN
                    thread_data.dataset[batch_sample_id] = sample_data
                    # log the error
                    logger.warning('Encountered error in sample #{:}', sample_id, exc_info=True)
                    if last_error_id != sample_id:
                        last_error_id = sample_id
                    else:
                        logger.error('Unable to continue execution in thread #{:}; erroneous sample: #{:}', thread_data.thread_id, sample_id)
                        break

                # increment the perf. counters
                sample_end_time = time.time()
                thread_data.perf.samples = thread_data.perf.samples + (sample_end_time - sample_start_time)

                # mark our progress
                thread_data.progress = batch_sample_id + 1
                sample_id += 1

            logger.info('Thread finished; error count: {:,}', thread_data.error_count)

            # stop the MATLAB instance
            if thread_data.dataset_params.uses_matlab:
                thread_data.matlab_instance.quit()

        logger.info('Starting workers...')

        # init the thread's payload
        for thread_id in range(num_threads):
            # compute the start and end sample ids for this thread
            start_id = thread_id * samples_per_thread
            end_id = min((thread_id + 1) * samples_per_thread, num_samples)

            # init the thread attributes
            threads[thread_id].thread_id = thread_id
            threads[thread_id].data_generator = self
            threads[thread_id].sample_callback = sample_callback
            threads[thread_id].dataset_params = dataset_params
            threads[thread_id].num_outputs = len(dataset_params.columns)
            threads[thread_id].sample_range = (start_id, end_id)
            threads[thread_id].progress = 0
            threads[thread_id].error_count = 0
            threads[thread_id].perf = dotdict({ 'samples': 0.0 })
            threads[thread_id].initialized = False

            # create the corresponding thread
            threads[thread_id].thread = threading.Thread(
                daemon=True,
                target=thread_callback,
                kwargs={ 'thread_data': threads[thread_id] })

        # start each thread
        for thread_id in range(num_threads):
            threads[thread_id].thread.start()
        
        # work in an endless loop
        start_time = None
        total_progress = 0
        while (True):
            # count the number of threads alive, and the total progress
            progress_delta = -total_progress
            total_progress = 0
            total_errors = 0
            best_progress = 0
            worst_progress = samples_per_thread
            num_alive = 0
            num_initialized = 0
            samples_left = num_samples
            
            # go through each thread and examine its state
            for thread_id in range(num_threads):
                # cache this thread's progress
                this_progress = threads[thread_id].progress
                this_errors = threads[thread_id].error_count

                # log the progress
                if num_initialized == num_threads:
                    logger.debug('Thread #{} progress: {:,}/{:,}', thread_id, this_progress, samples_per_thread)

                # compute the total progress and the berst/worst progresses
                total_progress = total_progress + this_progress
                samples_left = samples_left - this_progress
                total_errors = total_errors + this_errors
                progress_delta = progress_delta + this_progress
                best_progress = max(best_progress, this_progress)
                worst_progress = min(worst_progress, this_progress)

                # count the alive threads
                if threads[thread_id].thread.is_alive():
                    num_alive = num_alive + 1

                # count the initialized threads
                if threads[thread_id].initialized:
                    num_initialized = num_initialized + 1

            # wait for all threads to properly init
            if num_initialized < num_threads:
                time.sleep(1.0)
                continue

            # record the start time
            if start_time is None:
                start_time = time.time()
                prev_slack_report_time = start_time
                prev_checkpoint_time = start_time

            # compute the elapsed time
            current_time = time.time()
            elapsed = current_time - start_time

            # converts the estimated time left to a human readable format
            def time_left(samples, delta, interval):
                return ((samples + delta - 1) // delta) * interval if delta > 0 else 0.0

            def time_left_str(estimated_time_left):
                if estimated_time_left < 1.1:
                    return 'unknown'

                suppress = ['seconds', 'minutes']
                if estimated_time_left < 60:
                    suppress = []
                elif estimated_time_left < 60 * 60:
                    suppress = ['seconds']
                return humanize.precisedelta(estimated_time_left, suppress=suppress)

            def finish_time_str(estimated_time_left):
                if estimated_time_left < 1.1:
                    return 'unknown'
                return dth.format_time(time.time() + estimated_time_left)

            # try to estimate how much time is left, based on the worst performing thread and the total elapsed time
            avg_time_per_sample = 0.0 if worst_progress == 0 else elapsed / worst_progress
            num_samples_left = samples_per_thread - worst_progress
            estimated_time_left = num_samples_left * avg_time_per_sample
            #estimated_time_left = time_left(samples_left, progress_delta, self.config.data_generator.progress_report_interval)
            estimated_time_left_str = time_left_str(estimated_time_left)
            estimated_finish_time_str = finish_time_str(estimated_time_left)

            # display the elapsed time
            logger.info("[{}]: {:,}/{:,} samples finished. Delta: {:,}. Threads running: {}/{}. Errors: {}. Est. finish time: {} ({})", 
                dataset_name, total_progress, num_samples, progress_delta, 
                num_alive, num_threads, 
                total_errors, estimated_finish_time_str, estimated_time_left_str)

            # send a Slack notification
            if self.config.slack_notifications and current_time - prev_slack_report_time > self.config.data_generator.slack_progress_report_interval:
                prev_slack_report_time = current_time
                generate_status = {
                    'Dataset': dataset_name,
                    'Samples': '{samples:,}/{total:,}'.format(samples=total_progress, total=num_samples),
                    'Alive workers': '{alive:,}/{total:,}'.format(alive=num_alive, total=num_threads),
                    'Errors': '{:,}'.format(total_errors),
                    'Start time': dth.format_time(start_time),
                    'Current time': dth.format_time(current_time),
                    'Total elapsed time': humanize.precisedelta(current_time - start_time),
                    'Estimated time left': estimated_time_left_str
                }
                slack_msg = [ slackblocks.HeaderBlock('Data generation progress:'), sh.dict_block(generate_status) ]
                sh.send_message(slack_msg)

            # stop when no more threads are running
            if num_alive == 0:
                break

            # save checkpoints
            if self.config.data_generator.checkpoint_save_interval > 0 and current_time - prev_checkpoint_time > self.config.data_generator.checkpoint_save_interval:
                prev_checkpoint_time = current_time
                # TODO: implement

            # sleep for a while
            sleep_duration = min(max(estimated_time_left, 60.0), self.config.data_generator.progress_report_interval)
            time.sleep(self.config.data_generator.progress_report_interval)

        # get the number of total errors
        total_errors = 0
        for thread_id in range(num_threads):
            total_errors = total_errors + threads[thread_id].error_count

        logger.info('Data generation finished; total errors: {}', total_errors)
        logger.info('Combining per-thread results...')

        # create the collective np array
        combined_samples = np.zeros((num_samples, threads[0].num_outputs), dtype=tfh.float_np)
        perf = { name: 0.0 for name in threads[0].perf.keys() }

        # combine the dataset to one
        for thread_id in range(num_threads):
            # append the samples to the sample array
            data_range = threads[thread_id].sample_range
            combined_samples[data_range[0]:data_range[1]] = threads[thread_id].dataset

            # increment perf stats
            for stat_name, stat_val in threads[thread_id].perf.items():
                perf[stat_name] = perf[stat_name] + stat_val

        # derive other perf-related stats
        perf.update({ name + '_avg': val / num_samples  for name, val in perf.items() })

        logger.debug('Perf statistics: {}', json.dumps(perf, indent=4))

        return combined_samples

    # generates (or loads back) training data with the corresponding number of samples
    def generate_training_data(self):
        # simply return the cached data
        if self.cache is not None:
            logger.debug('Training data already in cache; returning it...')
            return self.cache

        # first, estimate the size of the data (so that we can sanity check)
        self.estimate_data_size()

        # early out if the file exists and we are not required to overwrite it
        if self.config.data_generator.overwrite_existing==False:
            logger.debug('Looking for existing training data')

            # read back the raw data
            training_data = self._load_training_data()

            # if it exists, return it
            if training_data is not None:
                logger.info('Training data with the input parameters already exists; loading back generated data...')
                for dataset_name in self.datasets.keys():
                    self.datasets[dataset_name].dataframe = training_data[dataset_name]
                    #print(training_data[dataset_name].shape)
                self.cache = training_data
                return self.cache

        logger.info('Generating new training data...')

        # clear any existing data
        for dataset_name in self.datasets.keys():
            self.datasets[dataset_name].dataframe = None

        # generate the individual datasets
        while any(dataset.dataframe is None for dataset in self.datasets.values()):
            for dataset_name, dataset_params in self.datasets.items():
                # skip already finished datasets
                if self.datasets[dataset_name].dataframe is not None:
                    continue

                # skip the dataset for now if not all of its dependencies are satisfied
                if any(self.datasets[dependency].dataframe is None for dependency in dataset_params.depends):
                    continue

                logger.info("Generating dataset '{}'...", dataset_name)

                # generate the dataset
                if dataset_params.whole_dataset:  # normal, single-threaded generator
                    self.datasets[dataset_name].dataframe = dataset_params.callback(self)                
                else: # threaded, per-sample generator
                    self.datasets[dataset_name].dataframe = self._generate_dataset_threaded(dataset_name, dataset_params.callback)

        # wrap the generated data in a pd DataFrames
        for dataset_name, dataset_params in self.datasets.items():
            column_names = [ p.name for p in dataset_params.columns ]
            self.datasets[dataset_name].dataframe = pd.DataFrame(self.datasets[dataset_name].dataframe, columns=column_names)

        logger.info('Saving training data to disk...')

        # save the resulting data
        self._save_training_data(self.datasets)

        # also cache it
        self.cache = { dataset_name: dataset for dataset_name, dataset in self.datasets.items() }

        # return the generated data
        return self.cache

    # validates the training data
    def validate_training_data(self):
        # TODO: implement
        pass
