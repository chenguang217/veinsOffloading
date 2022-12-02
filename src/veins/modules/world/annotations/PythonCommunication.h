#ifndef PYTHON_COMMUNICATION_H
#define PYTHON_COMMUNICATION_H

#include <Python.h>
#include <vector>
#include <string>
#include <map>
#include <iostream>

class PythonCommunication
{
private:
    PythonCommunication();

    const char *moduleName = nullptr;
    static PythonCommunication *instance;
    static bool printInfomation;

public:
    class PythonParam
    {
    private:
        PyObject *param = nullptr;

    public:
        PythonParam();
        PythonParam(PyObject *);
        ~PythonParam();

        void set(const char *, int v);
        void set(const char *, long v);
        void set(const char *, double v);
        void set(const char *,std::string v);
        void set(const char *, std::vector<std::string> v);
        void set(const char *, std::string v[], size_t len);
        void set(const char *, std::vector<int> v);
        void set(const char *,int v[], size_t len);
        void set(const char *, std::vector<double> v);
        void set(const char *, double v[], size_t len);
        void set(const char *, bool v);
        void set(const char *, PythonParam v);

        int getInt(const char *);
        long getLong(const char *);
        double getDouble(const char *);
        std::string getString(const char *);
        std::vector<std::string> getStringArray(const char *);
        std::vector<int> getIntArray(const char *);
        std::vector<double> getDoubleArray(const char *);
        bool getBool(const char *);
        PythonParam get(const char *);

        bool has(const char *);

        void buildPythonParam(PyObject *pp);
    };

    ~PythonCommunication();
    PyObject *buildParam(PythonParam *pp);
    static PythonCommunication *getInstance(const wchar_t *pythonHomePath = nullptr);
    static void release();
    static void setIfPrintInfomation(bool);
    void setModuleName(const char *moduleName);
    PyObject *getFunction(const char *funcName);
    PythonParam *call(const char *funcName, PythonParam *args);
};

#endif