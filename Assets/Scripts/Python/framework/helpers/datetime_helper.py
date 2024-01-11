import os, psutil, time
import humanize

_formats = { 
    'filename': "%Y-%m-%d %H-%M-%S",
    'display': "%Y. %m. %d. %H:%M:%S" }

def _startup_time():
    p = psutil.Process(os.getpid())
    return p.create_time()

def format_time(tm, fmt="display"):
    if fmt in _formats:
        fmt = _formats[fmt]
    return time.strftime(fmt, time.localtime(tm))

def format_time_delta(start_time, end_time):
    return humanize.precisedelta(end_time - start_time)

# returns the startup date and time in the parameter format
def get_startup_time(fmt="display"):
    return format_time(_startup_time(), fmt)
    
# returns the current date and time in the parameter format
def get_current_time(fmt="display"):
    return format_time(time.time(), fmt)

# returns the elapsed time since the startup, in a human readable format
def get_elapsed_time(start_time=None):
    # default to system start
    if start_time is None:
        start_time = _startup_time()

    # convert the results to a human readable format
    return format_time_delta(start_time=start_time, end_time=time.time())

if __name__ == '__main__':
    time.sleep(1.5)
    print('Startup time:', get_startup_time())
    print('Current time:', get_current_time())
    print('Elapsed:', get_elapsed_time())