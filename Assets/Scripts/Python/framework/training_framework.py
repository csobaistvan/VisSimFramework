# custom packages
from framework.helpers import config_helper as ch
from framework.helpers import tensorflow_helper as tfh
from framework.helpers import logging_helper as lh
from framework.helpers import slack_helper as sh
from framework.helpers import datetime_helper as dth
from framework.helpers import modules_helper as mh

# core packages
import os, traceback
import slackblocks

# create a new, module-level logger
logger = lh.get_main_module_logger()

# object wrapping all the available callbacks
class Callbacks(object):
    # init the callbacks object, storing the current config
    def __init__(self, config, data_generator, model):
        self.config = config
        self.data_generator = data_generator
        self.model = model

    def run(self, callback_name):
        callback = getattr(self, callback_name)
        return callback()

    # perform various analyzations
    def analyze(self):
        callbacks = {
            'datasize': self.model.data_generator.estimate_data_size,
            'data': self.model.analyze_training_data,
            'network': self.model.test_network
        }
        callbacks[self.config.analyze.mode]()

    # generates training data using the input parameters
    def generate(self):
        self.data_generator.generate_training_data()

    # performs the training step, using a custom keras model
    def train(self):
        self.model.train_network()

    # performs hparam training
    def tune(self):
        self.model.tune_hparams()

    # exports a trained network
    def export(self):
        self.model.export_network()

# Main class for handling the entire training process
class TrainingFramework(object):
    def __init__(self, data_generator_cls, model_cls):
        self.data_generator_cls = data_generator_cls
        self.model_cls = model_cls

        self.config = None
        self.slack_log = []
        self.finished = False
        self.successful = False
        self.error_message = None
        self.error_cls = None

    def _startup(self):
        # initialize the necessary modules
        lh.setup_module_logger()
        tfh.init_tensorflow()

        # parse the config file
        self.config = ch.load_config()

    def _send_startup_slack_message(self):
        # build a block with the current status
        status = {
            'Module': mh.get_main_module(),
            'Start time': dth.get_startup_time(),
        }

        # send a Slack message with a summary of the start
        sh.send_message([ 
            slackblocks.HeaderBlock('Run started:'), 
            sh.dict_block(status),
            slackblocks.DividerBlock(),
            slackblocks.HeaderBlock('Configuration:'), 
            sh.dict_block(self.config) ])
    
    def _send_final_slack_message(self):
        # construct the run history text
        status = {
            'Status': 'Success ✅' if self.successful else 'Failed ❌',
            'Module': mh.get_main_module(),
            'Run Mode': self.config.run_mode,
            'Start time': dth.get_startup_time(),
            'Finish time': dth.get_current_time(),
            'Total running time': dth.get_elapsed_time(),
        }
        if self.successful == False:
            status['Error message'] = self.error_message

        # send a Slack message with the summary, and another with the accummulated Slack log
        sh.send_message([ slackblocks.HeaderBlock('Run completed:'), sh.dict_block(status), slackblocks.DividerBlock() ])
        if self.slack_log:
            sh.send_message(self.slack_log)

    def _system_sleep(self):
        logger.info('Execution finished, entering sleep mode...')
        os.system("rundll32.exe powrprof.dll,SetSuspendState 0,1,0")

    def _run_main_loop(self):
        # work in an infinite loop, to automatically continue our progress if an error occurs
        while not self.finished:
            # attempt to execute the process that was requested by the user
            try:
                # run the callback using the callback object
                self.callbacks.run(self.config.run_mode)
                
                # necessary bookkeeping
                self.finished = True # mark that we finished
                self.successful = True # we also finished successfully

            # a keyboard interrupt can also get us out
            except KeyboardInterrupt as e:
                self.error_message = '{name}: {msg}'.format(name=type(e).__name__, msg=str(e))
                logger.info(self.error_message)
                self.finished = True # exit the infinite loop
                self.error_cls = KeyboardInterrupt

            # in case of any other error, report the error and restart the loop
            except Exception as e:
                self.error_message = '{name}: {msg}'.format(name=type(e).__name__, msg=str(e))
                logger.error("Encountered an error while executing '{}'; cause: {}".format(self.config.run_mode, self.error_message))
                traceback.print_exc()
                self.error_cls = type(e)

            # turn off the infinite loop behavior, for now
            self.finished = True

    def run(self):
        try:
            # initialize the main logger
            self._startup()

            # send the initial slack message
            if self.config.slack_notifications:
                self._send_startup_slack_message()

            # construct the model and data generator objects
            self.data_generator = self.data_generator_cls(self)
            self.model = self.model_cls(self)

            # construct the callback object
            self.callbacks = Callbacks(
                config=self.config,
                data_generator=self.data_generator,
                model=self.model)

            # run the execution loop
            self._run_main_loop()

        except Exception as e:
            self.error_message = str(e)
            logger.error("Encountered an error while executing '{}'; cause: {}".format(self.config.run_mode, self.error_message))
            traceback.print_exc()
            
        if self.config.slack_notifications:
            self._send_final_slack_message()

        # sleep when done, if requested
        if self.config.sleep_on_finish and self.error_cls != KeyboardInterrupt:
            self._system_sleep()
