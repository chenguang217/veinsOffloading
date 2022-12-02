from typing import List


class PythonParam:
    def __init__(self, param = dict()):
        self.param = param

    def set(self, key: str, v: int or float or str or List[str] or List[int] or List[float] or bool):
        if isinstance(v, bool):
            self.param[key] = 1 if v else 0
        self.param[key] = v

    def getInt(self, key: str) -> int:
        return int(self.param[key])
    
    def getLong(self, key: str) -> int:
        return int(self.param[key])
    
    def getDouble(self, key: str) -> float:
        return float(self.param[key])
    
    def getString(self, key: str) -> str:
        return str(self.param[key])
    
    def getStringArray(self, key: str) -> List[str]:
        return self.param[key]
    
    def getIntArray(self, key: str) -> List[int]:
        return self.param[key]
    
    def getDoubleArray(self, key: str) -> List[float]:
        return self.param[key]
    
    def getBool(self, key: str) -> bool:
        return int(self.param[key]) == 1
    
    def get(self, key: str) -> 'PythonParam':
        return PythonParam(self.param[key])
    
    def has(self, key: str) -> bool:
        return key in self.param

