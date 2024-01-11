
# core packages
import time

# local packages
from framework.helpers import config_helper as ch
from framework.helpers import tensorflow_helper as tfh
from framework.helpers import logging_helper as lh

# create a new, module-level logger
logger = lh.get_main_module_logger()

def main():
    # init the necessary system modules    
    lh.setup_module_logger()
    tfh.init_tensorflow()
    config = ch.load_config('tensorboard')

    # launch TensorBoard
    for network in config:
        tfh.launch_tensorboard(
            module_name=network.module, 
            network_type=network.network_type, 
            model_type=network.model_type, 
            port=network.port,
            as_daemon=True)

    while True:
        time.sleep(1.0)

if __name__ == '__main__':
    main()