#include "PythonCommunication.h"

PythonCommunication *PythonCommunication::instance = nullptr;

PythonCommunication::PythonCommunication()
{
}

PythonCommunication::~PythonCommunication()
{
    if (module != nullptr)
    {
        Py_CLEAR(module);
    }
    if (pPythonParamModule != nullptr)
    {
        Py_CLEAR(pPythonParamModule);
    }
    if (PythonParamConstructor != nullptr)
    {
        Py_CLEAR(PythonParamConstructor);
    }
    for (auto it = functions.begin(); it != functions.end(); it++)
    {
        Py_CLEAR(it->second);
        functions.erase(it);
    }
    Py_Finalize();
}

PythonCommunication *PythonCommunication::getInstance()
{
    if (instance == nullptr)
    {
        instance = new PythonCommunication();
        Py_Initialize();
        if (!Py_IsInitialized())
        {
            return nullptr;
        }
        PyRun_SimpleString("import sys");
        PyRun_SimpleString("sys.path.append('./')");
        instance->pPythonParamModule = PyImport_ImportModule("PythonParam");
        if (instance->pPythonParamModule == NULL)
        {
            std::cout << "can not find module <PythonParam>" << std::endl;
            throw "can not find module <PythonParam>";
        }
        PyObject *pClass = PyObject_GetAttrString(instance->pPythonParamModule, "PythonParam");
        if (pClass == NULL)
        {
            std::cout << "can not find class <PythonParam>" << std::endl;
            throw "can not find class <PythonParam>";
        }
        instance->PythonParamConstructor = PyInstanceMethod_New(pClass);
        if (instance->PythonParamConstructor == NULL) {
            std::cout << "can not find constructor of <PythonParam>" << std::endl;
            throw "can not find constructor of <PythonParam>";
        }
    }
    return instance;
}

void PythonCommunication::release()
{
    if (instance != nullptr) {
        Py_Finalize();
        instance = nullptr;
    }
}

void PythonCommunication::loadMoudle(const char *moduleName)
{
    std::cout << "LoadModule: " << moduleName << " is " << (module == nullptr ? "unload" : "loaded") << std::endl;
    if (module != nullptr)
    {
        if (strcmp(moduleName, this->moduleName) == 0)
            return;
        Py_CLEAR(module);
        for (auto it = functions.begin(); it != functions.end(); it++)
        {
            Py_CLEAR(it->second);
            functions.erase(it);
        }
    }
    module = PyImport_ImportModule(moduleName);
    if (module == NULL)
    {
        throw "can not find module";
    }
    this->moduleName = moduleName;
}

PythonCommunication::PythonParam *PythonCommunication::call(const char *funcName, PythonParam *args)
{
    PyObject *pp = PyObject_CallObject(PythonParamConstructor, nullptr);
    if (args != nullptr)
    {
        args->buildPythonParam(pp);
    }
    if (module == NULL)
    {
        throw "load module first";
    }
    // std::cout << "build param done" << std::endl;
    auto function_it = functions.find(funcName);
    PyObject *func = nullptr;
    if (function_it == functions.end())
    {
        func = PyObject_GetAttrString(module, funcName);
        if (func == NULL)
        {
            std::cout << "can not find function1 " << funcName << std::endl;
            throw "can not find function";
        }
        functions[funcName] = func;
    }
    else
    {
        func = function_it->second;
    }
    if (func == NULL) {
        std::cout << "can not find function " << funcName << std::endl;
        throw "can not find function";
    }
    PyObject *pArgs = PyTuple_New(1);
    PyTuple_SetItem(pArgs, 0, pp);
    PyObject *pRes = PyObject_CallObject(func, pArgs);
    if (pRes == NULL)
        return nullptr;
    PythonParam *result = new PythonParam(pRes);
    Py_CLEAR(pArgs);
    Py_CLEAR(pRes);
    Py_CLEAR(pp);
    return result;
}

PythonCommunication::PythonParam::PythonParam()
{
    if (PythonCommunication::instance == nullptr)
    {
        throw "Python not initialize";
    }
    param = PyDict_New();
}

PythonCommunication::PythonParam::PythonParam(PyObject *param)
{
    std::cout << PyBytes_AsString(PyUnicode_AsUTF8String(PyObject_Str(PyObject_GetAttrString(param, "param")))) << std::endl;
    this->param = PyDict_Copy(PyObject_GetAttrString(param, "param"));
}

PythonCommunication::PythonParam::~PythonParam()
{
    Py_CLEAR(param);
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
