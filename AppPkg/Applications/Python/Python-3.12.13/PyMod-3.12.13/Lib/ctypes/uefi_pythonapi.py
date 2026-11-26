# dummy pythonapi to make some dependent stuff happy
import ctypes

class DummyCFunc:
    def __init__(self):
        self.restype = None
        self.argtypes = None

    def __call__(self, *args, **kwargs):
        return None
    
PyOS_getsig = DummyCFunc()
PyOS_setsig = DummyCFunc()
