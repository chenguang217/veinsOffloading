from PythonParam import PythonParam

# type1
def sumTest(param: PythonParam):
    sumResult = param.getInt("add1") + param.getInt("add2")
    res = PythonParam()
    res.set("sumResult", sumResult)
    return res