#include "PythonCommunication.h"

PythonCommunication *PythonCommunication::instance = nullptr;
bool PythonCommunication::printInfomation = true;

PythonCommunication::PythonCommunication()
{
}

PythonCommunication::~PythonCommunication()
{
    Py_Finalize();
}

PythonCommunication *PythonCommunication::getInstance(const wchar_t *pythonHomePath)
{
    if (instance == nullptr)
    {
        if (pythonHomePath == nullptr)
        {
            std::cout << "first initialize python, please set the pythonHomePath" << std::endl;
        }
        instance = new PythonCommunication();
        Py_SetPythonHome(pythonHomePath);
        Py_Initialize();
        if (!Py_IsInitialized())
        {
            return nullptr;
        }
        PyRun_SimpleString("import sys");
        PyRun_SimpleString("sys.path.append('./')");
    }
    return instance;
}

void PythonCommunication::release()
{
    if (instance != nullptr)
    {
        Py_Finalize();
        instance = nullptr;
    }
}

void PythonCommunication::setIfPrintInfomation(bool printInfomation)
{
    PythonCommunication::printInfomation = printInfomation;
}

void PythonCommunication::setModuleName(const char *moduleName)
{
    this->moduleName = moduleName;
}

PyObject *PythonCommunication::getFunction(const char *funcName)
{
    PyObject *module = PyImport_ImportModule(moduleName);
    if (module == NULL)
    {
        std::cout << "can not find module" << moduleName << std::endl;
        return nullptr;
    }
    PyObject *func = PyObject_GetAttrString(module, funcName);
    if (func == NULL)
    {
        std::cout << "can not find function " << funcName << std::endl;
        return nullptr;
    }
    return func;
}

PyObject *PythonCommunication::buildParam(PythonParam *pp) {
    PyObject *pPythonParamModule = PyImport_ImportModule("PythonParam");
    if (pPythonParamModule == NULL)
    {
        std::cout << "can not find module <PythonParam>" << std::endl;
        return nullptr;
    }

    PyObject *pClass = PyObject_GetAttrString(pPythonParamModule, "PythonParam");
    Py_DECREF(pPythonParamModule);
    if (pClass == NULL)
    {
        std::cout << "can not find class <PythonParam>" << std::endl;
        return nullptr;
    }

    PyObject *constructor = PyInstanceMethod_New(pClass);
    Py_DECREF(pClass);
    if (constructor == NULL)
    {
        std::cout << "can not find constructor of <PythonParam>" << std::endl;
        return nullptr;
    }

    PyObject *ppp = PyObject_CallObject(constructor, nullptr);
    Py_DECREF(constructor);
    if (ppp == nullptr)
    {
        PyErr_Print();
        std::cout << "param constructor failed" << std::endl;
        return nullptr;
    }

    if (pp != nullptr)
    {
        pp->buildPythonParam(ppp);
        if (printInfomation)
        {
            std::cout << "param build" << std::endl;
        }
    }
    return ppp;
}

PythonCommunication::PythonParam *PythonCommunication::call(const char *funcName, PythonParam *args)
{
    PyObject *pp = nullptr;
    if ((pp = buildParam(args)) == nullptr) {
        return nullptr;
    }

    PyObject *func = getFunction(funcName);
    if (func == nullptr)
    {
        std::cout << "can not find function " << funcName << std::endl;
        return nullptr;
    }

    PyObject *pArgs = PyTuple_New(1);
    PyTuple_SetItem(pArgs, 0, pp);
    if (printInfomation)
    {
        std::cout << "call " << funcName << std::endl;
    }

    PyObject *pRes = PyObject_CallObject(func, pArgs);
    Py_DECREF(pp);
    Py_DECREF(func);
    Py_DECREF(pArgs);

    if (printInfomation)
    {
        std::cout << "call " << funcName << " finish" << std::endl;
    }

    if (pRes == NULL)
        return nullptr;
    
    PythonParam *result = new PythonParam(pRes);
    Py_DECREF(pRes);

    return result;
}

PythonCommunication::PythonParam::PythonParam()
{
    if (PythonCommunication::instance == nullptr)
    {
        std::cout << "Python not initialize" << std::endl;
        throw "Python not initialize";
    }
    param = PyDict_New();
}

PythonCommunication::PythonParam::PythonParam(PyObject *param)
{
    if (PythonCommunication::printInfomation)
    {
        std::cout << "param build--" << std::endl;
        std::cout << PyBytes_AsString(PyUnicode_AsUTF8String(PyObject_Str(PyObject_GetAttrString(param, "param")))) << std::endl;
    }
    this->param = PyDict_Copy(PyObject_GetAttrString(param, "param"));
}

PythonCommunication::PythonParam::~PythonParam()
{
    Py_DECREF(param);
}

void PythonCommunication::PythonParam::set(const char *name, int v)
{
    PyDict_SetItemString(param, name, Py_BuildValue("i", v));
}

void PythonCommunication::PythonParam::set(const char *name, long v)
{
    PyDict_SetItemString(param, name, PyLong_FromLong(v));
}

void PythonCommunication::PythonParam::set(const char *name, double v)
{
    PyDict_SetItemString(param, name, PyFloat_FromDouble(v));
}

void PythonCommunication::PythonParam::set(const char *name, std::string v)
{
    PyDict_SetItemString(param, name, Py_BuildValue("s", v.c_str()));
}

void PythonCommunication::PythonParam::set(const char *name, std::string v[], size_t len)
{
    PyObject *list = PyList_New(len);
    for (int i = 0; i < len; i++)
    {
        PyList_SetItem(list, i, Py_BuildValue("s", v[i].c_str()));
    }
    PyDict_SetItemString(param, name, list);
}

void PythonCommunication::PythonParam::set(const char *name, std::vector<std::string> v)
{
    PyObject *list = PyList_New(0);
    for (auto it = v.begin(); it != v.end(); it++)
    {
        PyList_Append(list, Py_BuildValue("s", (*it).c_str()));
    }
    PyDict_SetItemString(param, name, list);
}

void PythonCommunication::PythonParam::set(const char *name, int v[], size_t len)
{
    PyObject *list = PyList_New(len);
    for (int i = 0; i < len; i++)
    {
        PyList_SetItem(list, i, Py_BuildValue("i", v[i]));
    }
    PyDict_SetItemString(param, name, list);
}

void PythonCommunication::PythonParam::set(const char *name, std::vector<int> v)
{
    PyObject *list = PyList_New(0);
    for (auto it = v.begin(); it != v.end(); it++)
    {
        PyList_Append(list, Py_BuildValue("i", *it));
    }
    PyDict_SetItemString(param, name, list);
}

void PythonCommunication::PythonParam::set(const char *name, double v[], size_t len)
{
    PyObject *list = PyList_New(len);
    for (int i = 0; i < len; i++)
    {
        PyList_SetItem(list, i, Py_BuildValue("d", v[i]));
    }
    PyDict_SetItemString(param, name, list);
}

void PythonCommunication::PythonParam::set(const char *name, std::vector<double> v)
{
    PyObject *list = PyList_New(0);
    for (auto it = v.begin(); it != v.end(); it++)
    {
        PyList_Append(list, Py_BuildValue("b", *it));
    }
    PyDict_SetItemString(param, name, list);
}

void PythonCommunication::PythonParam::set(const char *name, bool v)
{
    PyDict_SetItemString(param, name, Py_BuildValue("i", v ? 1 : 0));
}

void PythonCommunication::PythonParam::set(const char *name, PythonParam v)
{
    PyDict_SetItemString(param, name, PyDict_Copy(v.param));
}

int PythonCommunication::PythonParam::getInt(const char *name)
{
    int res;
    PyArg_Parse(PyDict_GetItemString(param, name), "i", &res);
    return res;
}

long PythonCommunication::PythonParam::getLong(const char *name)
{
    return PyLong_AsLong(PyDict_GetItemString(param, name));
}

double PythonCommunication::PythonParam::getDouble(const char *name)
{
    return PyFloat_AsDouble(PyDict_GetItemString(param, name));
}

std::string PythonCommunication::PythonParam::getString(const char *name)
{
    PyObject *pStr = PyDict_GetItemString(param, name);
    char *res;
    PyArg_Parse(pStr, "s", &res);
    std::string result(res);
    return result;
}

std::vector<std::string> PythonCommunication::PythonParam::getStringArray(const char *name)
{
    PyObject *pList = PyDict_GetItemString(param, name);
    int list_len = PyList_Size(pList);
    std::vector<std::string> result;
    char *res;
    for (int i = 0; i < list_len; i++)
    {
        PyObject *pStr = PyList_GetItem(pList, i);
        Py_ssize_t len = PySequence_Length(pStr);
        PyArg_Parse(pStr, "s", &res);
        result.push_back(std::string(res));
    }
    return result;
}

std::vector<int> PythonCommunication::PythonParam::getIntArray(const char *name)
{
    PyObject *pList = PyDict_GetItemString(param, name);
    int list_len = PyList_Size(pList);
    std::vector<int> result;
    int num;
    for (int i = 0; i < list_len; i++)
    {
        PyObject *pItem = PyList_GetItem(pList, i);
        PyArg_Parse(pItem, "i", &num);
        result.push_back(num);
    }
    return result;
}

std::vector<double> PythonCommunication::PythonParam::getDoubleArray(const char *name)
{
    PyObject *pList = PyDict_GetItemString(param, name);
    int list_len = PyList_Size(pList);
    std::vector<double> result;
    double num;
    for (int i = 0; i < list_len; i++)
    {
        PyObject *pItem = PyList_GetItem(pList, i);
        PyArg_Parse(pItem, "d", &num);
        result.push_back(num);
    }
    return result;
}

bool PythonCommunication::PythonParam::getBool(const char *name)
{
    int num;
    PyArg_Parse(PyDict_GetItemString(param, name), "i", &num);
    return num == 1 ? true : false;
}

PythonCommunication::PythonParam PythonCommunication::PythonParam::get(const char *name)
{
    PyObject *o = PyDict_GetItemString(param, name);
    return PythonParam(o);
}

bool PythonCommunication::PythonParam::has(const char *name)
{
    return PyDict_GetItemString(param, name) == NULL;
}

void PythonCommunication::PythonParam::buildPythonParam(PyObject *pp)
{
    PyObject_SetAttrString(pp, "param", param);
}
