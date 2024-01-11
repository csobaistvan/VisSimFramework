
from enum import Enum

# describes the type of training to be performed
class TrainingType(Enum):
    Train = 1
    Tune = 2
    Test = 3