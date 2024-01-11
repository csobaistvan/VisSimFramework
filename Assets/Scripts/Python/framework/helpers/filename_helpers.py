import base64, uuid

from . import modules_helper as mh

def hash_filename(filename):
    # get the base64 encoduing of the filename
    filename_encoded = base64.urlsafe_b64encode(filename.encode()).decode()

    # create a unique namespace for the filename
    namespace_uuid = uuid.uuid3(uuid.NAMESPACE_DNS, 'train.{module}'.format(module=mh.get_main_module()))

    # construct the unique uuid
    return str(uuid.uuid3(namespace_uuid, filename_encoded))