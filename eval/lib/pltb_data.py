from enum import Enum

GTR_MODEL = "012345"
GTR_LABEL = "GTR"
GTR_SELECTOR_LABEL = 'extra'

class OrderedEnum(Enum):
    def __ge__(self, other):
        if self.__class__ is other.__class__:
            return self.value >= other.value
        return NotImplemented
    def __gt__(self, other):
        if self.__class__ is other.__class__:
            return self.value > other.value
        return NotImplemented
    def __le__(self, other):
        if self.__class__ is other.__class__:
            return self.value <= other.value
        return NotImplemented
    def __lt__(self, other):
        if self.__class__ is other.__class__:
            return self.value < other.value
        return NotImplemented

class Selector(OrderedEnum):
    AIC = 'AIC'
    AICS = 'AICc-S'
    AICM = 'AICc-M'
    BICS = 'BIC-S'
    BICM = 'BIC-M'
    GTR = GTR_SELECTOR_LABEL
    def serialize(self):
        return self.value.replace(' ', '').replace('-', '').lower()
    def __str__(self):
        return self.value



class TreeEntry:
    def __init__(self, model, ics, newick_tree):
        self.model = model
        self.ics = ics
        self.newick_tree = newick_tree
