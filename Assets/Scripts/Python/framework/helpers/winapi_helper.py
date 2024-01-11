import win32api, win32con, win32process
import functools, enum

def set_process_affinity(pid, mask):
    mask = functools.reduce(lambda x, y: x | y, [ 1 << id for id in mask ])
    handle = win32api.OpenProcess(win32con.PROCESS_ALL_ACCESS, True, pid)
    win32process.SetProcessAffinityMask(handle, mask)

class ProcessPriority(enum.Enum):
    IDLE = 0
    NORMAL = 1
    HIGH = 2
    REALTIME = 3

_priority_classes = {
    ProcessPriority.IDLE: win32con.IDLE_PRIORITY_CLASS,
    ProcessPriority.NORMAL: win32con.NORMAL_PRIORITY_CLASS,
    ProcessPriority.HIGH: win32con.HIGH_PRIORITY_CLASS,
    ProcessPriority.REALTIME: win32con.REALTIME_PRIORITY_CLASS
}

def set_process_priority(pid, priority):
    handle = win32api.OpenProcess(win32con.PROCESS_ALL_ACCESS, True, pid)
    win32process.SetPriorityClass(handle, _priority_classes[priority])