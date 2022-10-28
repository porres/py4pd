// =================================
// https://github.com/pure-data/pure-data/src/x_gui.c 
#include <m_pd.h>
#include <g_canvas.h>
#include <stdio.h>
#include <string.h>
#include <complex.h>
#ifdef _WIN64 // If windows 64bits include 
#include <windows.h>
#else 
#include <pthread.h>
#endif
#ifdef HAVE_UNISTD_H // from Pure Data source code
#include <unistd.h>
#endif

// Python include
#define PY_SSIZE_T_CLEAN // Good practice to use this
#include <Python.h>

// Include NPY_FLOAT to use with numpy and audio processing
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>

/* 
TODO: Way to set global variables, I think that will be important for things like general path (lilypond, etc)

TODO: Reset the function (like panic for sfont~), In some calls seems that the *function become NULL? 

TODO: make function home work with spaces, mainly for Windows OS where the use of lilypond in python need to be specified with spaces

TODO: Add some way to run list how arguments
    FORMULA I: Work with [1 2 3 4 5] and transform this to a list for Python
    FORMULA II: MAKE SOME SPECIAL OBJECT TO WORK WITH LISTS 
    
TODO: If the run method set before the end of the thread, there is an error, that close all PureData.

TODO: The set function need to run after the load of the object, but before the start of the thread???
        It does not load external modules in the pd_new.

*/

static int object_count = 1;
static int thread_status[100];
static t_class *py4pd_class; // 


// define global pointer for py_object

// =====================================
typedef struct _py_tilde { // It seems that all the objects are some kind of class.

    t_object        x_obj; // convensao no puredata source code
    t_canvas        *x_canvas; // pointer to the canvas
    PyObject        *module; // python object
    PyObject        *function; // function name  
    int             *state; // thread state
    int             *object_number; // object number
    int             *audio; // audio flag
    t_inlet         *in1;
    t_float         x_f;
    t_float         *thread; // arguments
    t_float         *function_called; // flag to check if the set function was called
    t_float         *audio_warning; // function name
    t_symbol        *packages_path; // packages path 
    t_symbol        *home_path; // home path this always is the path folder (?)
    t_symbol        *object_path;
    t_symbol        *function_name; // function name
    t_symbol        *script_name; // script name
    int             *py_arg_numbers; // number of arguments
    t_outlet        *out_A; // outlet 1.
    t_outlet        *out_B; // outlet 2. FOR AUDIO
}t_py_tilde;


// PD GLOBAL OBJECT
// // ============================================

static t_py_tilde *py4pd_object;

// ======================================
// ======== PD Module for Python ========
// ======================================

static PyObject *pdout(PyObject *self, PyObject *args){
    // self is void
    (void)self;
    
    float f;
    char *string;

    if (PyArg_ParseTuple(args, "f", &f)){
        outlet_float(py4pd_object->out_A, f);
        PyErr_Clear();
    } else if (PyArg_ParseTuple(args, "s", &string)){
        // pd string
        char *pd_string = string;
        t_symbol *pd_symbol = gensym(pd_string);
        outlet_symbol(py4pd_object->out_A, pd_symbol);
        PyErr_Clear();
    } else if (PyArg_ParseTuple(args, "O", &args)){
        int list_size = PyList_Size(args);
        t_atom *list_array = (t_atom *) malloc(list_size * sizeof(t_atom));  
        int i;          
        for (i = 0; i < list_size; ++i) {
            PyObject *pValue_i = PyList_GetItem(args, i);
            if (PyLong_Check(pValue_i)) {            // DOC: If the function return a list of integers
                long result = PyLong_AsLong(pValue_i);
                float result_float = (float)result;
                list_array[i].a_type = A_FLOAT;
                list_array[i].a_w.w_float = result_float;

            } else if (PyFloat_Check(pValue_i)) {    // DOC: If the function return a list of floats
                double result = PyFloat_AsDouble(pValue_i);
                float result_float = (float)result;
                list_array[i].a_type = A_FLOAT;
                list_array[i].a_w.w_float = result_float;
            } else if (PyUnicode_Check(pValue_i)) {  // DOC: If the function return a list of strings
                const char *result = PyUnicode_AsUTF8(pValue_i); 
                list_array[i].a_type = A_SYMBOL;
                list_array[i].a_w.w_symbol = gensym(result);
            } else if (Py_IsNone(pValue_i)) {        // DOC: If the function return a list of None
                    // post("None");
            } else {
                pd_error(py4pd_object, "[py4pd~] py4pd just convert int, float and string!\n");
                pd_error(py4pd_object, "INFO [!] The value received is of type %s", Py_TYPE(pValue_i)->tp_name);
                Py_DECREF(pValue_i);
                Py_DECREF(args);
                return NULL;
            }
        }
        outlet_list(py4pd_object->out_A, 0, list_size, list_array); 
        PyErr_Clear();
    } else {
        PyErr_SetString(PyExc_TypeError, "pdout: argument must be a float or a string"); // Colocar melhor descrição do erro
        return NULL;
    }
    return PyLong_FromLong(0);
}

// =================================
static PyObject *pdmessage(PyObject *self, PyObject *args){
    (void)self;
    char *string;
    if (PyArg_ParseTuple(args, "s", &string)){
        post("[py]: %s", string);
        PyErr_Clear();
    } else {
        PyErr_SetString(PyExc_TypeError, "pdmessage: argument must be a string"); // Colocar melhor descrição do erro
        return NULL;
    }
    return PyLong_FromLong(0);
}

// =================================
static PyObject *pderror(PyObject *self, PyObject *args){
    (void)self;
    char *string;
    if (PyArg_ParseTuple(args, "s", &string)){
        post("Not working yet");
        pd_error(py4pd_object, "Ocorreu um erro");
        PyErr_Clear();
    } else {
        PyErr_SetString(PyExc_TypeError, "pdmessage: argument must be a string"); // Colocar melhor descrição do erro
        return NULL;
    }
    return PyLong_FromLong(0);
} // WARNING: This function is not working yet.
 

// =================================
static PyMethodDef PdMethods[] = { // here we define the function spam_system
    {"out", pdout, METH_VARARGS, "Output in out0 from PureData"}, // one function for now
    {"message", pdmessage, METH_VARARGS, "Print informations in PureData Console"}, // one function for now
    {"error", pderror, METH_VARARGS, "Print error in PureData"}, // one function for now
    {NULL, NULL, 0, NULL}        
};

// =================================

static struct PyModuleDef pdmodule = {
    PyModuleDef_HEAD_INIT,
    "pd",   /* name of module */
    NULL, /* module documentation, may be NULL */
    -1,       /* size of per-interpreter state of the module,
                 or -1 if the module keeps state in global variables. */
    PdMethods,
    NULL,
    NULL,
    NULL,
    NULL,
};

// =================================

static PyObject *pdmoduleError;

PyMODINIT_FUNC PyInit_pd(void){
    PyObject *m;
    m = PyModule_Create(&pdmodule);
    if (m == NULL)
        return NULL;
    pdmoduleError = PyErr_NewException("spam.error", NULL, NULL);
    Py_XINCREF(pdmoduleError);
    if (PyModule_AddObject(m, "error", pdmoduleError) < 0) {
        Py_XDECREF(pdmoduleError);
        Py_CLEAR(pdmoduleError);
        Py_DECREF(m);
        return NULL;
    }
    return m;
}

// =================================
// ============ Pd Object code  ====
// =================================


static void home_tilde(t_py_tilde *x, t_symbol *s, int argc, t_atom *argv) {
    (void)s; // unused but required by pd
    if (argc < 1) {
        post("[py4pd~] The home path is: %s", x->home_path->s_name);
        return; 
    } else {
        x->home_path = atom_getsymbol(argv);
        post("[py4pd~] The home path set to: %s", x->home_path->s_name);
    }
}

// // ============================================
// // ============================================
// // ============================================

static void packages(t_py_tilde *x, t_symbol *s, int argc, t_atom *argv) {
    (void)s; 
    if (argc < 1) {
        post("[py4pd~] The packages path is: %s", x->packages_path->s_name);
        return; // is this necessary?
    }
    else {
        if (argc < 2 && argc > 0){
            x->packages_path = atom_getsymbol(argv);
            // probe if the path is valid
            if (access(x->packages_path->s_name, F_OK) != -1) {
                // do nothing
            } else {
                pd_error(x, "The packages path is not valid");
            }
        }   
        else{
            pd_error(x, "It seems that your package folder has |spaces|.");
            return;
        }    
    }
}

// ====================================
// ====================================
// ====================================

static void documentation(t_py_tilde *x){
    PyObject *pFunc;
    if (x->function_called == 0) { // if the set method was not called, then we can not run the function :)
        pd_error(x, "[py4pd~] To see the documentaion you need to set the function first!");
        return;
    }

    pFunc = x->function;
    if (pFunc && PyCallable_Check(pFunc)){ // Check if the function exists and is callable
        PyObject *pDoc = PyObject_GetAttrString(pFunc, "__doc__"); // Get the documentation of the function
        if (pDoc != NULL){
            const char *Doc = PyUnicode_AsUTF8(pDoc); 
            if (Doc != NULL){
                post("");
                post("[+][+] %s [+][+]", x->function_name->s_name);
                post("");
                post("%s", Doc);
                post("");
            }
            else{

                post("");
                pd_error(x, "[py4pd~] No documentation found!");
                post("");
            }
        }
        else{
            post("");
            pd_error(x, "[py4pd~] No documentation found!");
            post("");
        }
    }
}

// ====================================
// ====================================
// ====================================

void pd4py_system_func (const char *command){
    int result = system(command);
    if (result == -1){
        post("[py4pd~] %s", command);
    }
}

// ============================================
// ============================================

static void create(t_py_tilde *x, t_symbol *s, int argc, t_atom *argv){
    // If Windows OS run, if not then warn the user
    (void)s;
    (void)argc;
    
    const char *script_name = argv[0].a_w.w_symbol->s_name;
    post("[py4pd~] Opening vscode...");
    #ifdef _WIN64 // ERROR: the endif is missing directive _WIN64

    char *command = malloc(strlen(x->home_path->s_name) + strlen(script_name) + 20);
    sprintf(command, "/c code %s/%s.py", x->home_path->s_name, script_name);
    SHELLEXECUTEINFO sei = {0};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    // sei.lpVerb = "open";
    
    sei.lpFile = "cmd.exe ";
    sei.lpParameters = command;
    sei.nShow = SW_HIDE;
    ShellExecuteEx(&sei);
    CloseHandle(sei.hProcess);
    free(command);
    return;

    // Not Windows OS

    #else // if not windows 64bits
    char *command = malloc(strlen(x->home_path->s_name) + strlen(script_name) + 20);
    sprintf(command, "code %s/%s.py", x->home_path->s_name, script_name);

    pd4py_system_func(command);
    return;
    #endif
}

// ====================================
// ====================================
// ====================================

static void vscode(t_py_tilde *x){
    // If Windows OS run, if not then warn the user
       
    if (x->function_called == 0) { // if the set method was not called, then we can not run the function :)
        pd_error(x, "[py4pd~] To open vscode you need to set the function first!");
        return;
    }
    post("[py4pd~] Opening vscode...");
    
    #ifdef _WIN64 // ERROR: the endif is missing directive _WIN64
    char *command = malloc(strlen(x->home_path->s_name) + strlen(x->script_name->s_name) + 20);
    sprintf(command, "/c code %s/%s.py", x->home_path->s_name, x->script_name->s_name);
    SHELLEXECUTEINFO sei = {0};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    // sei.lpVerb = "open";
    sei.lpFile = "cmd.exe ";
    sei.lpParameters = command;
    sei.nShow = SW_HIDE;
    ShellExecuteEx(&sei);
    CloseHandle(sei.hProcess);

    return;

    // Not Windows OS

    #else // if not windows 64bits
    char *command = malloc(strlen(x->home_path->s_name) + strlen(x->script_name->s_name) + 20);
    sprintf(command, "code %s/%s.py", x->home_path->s_name, x->script_name->s_name);
    pd4py_system_func(command);
    pd_error(x, "Not tested in your Platform, please send me a bug report!");
    return;
    #endif
}

// ====================================
// ====================================
// ====================================

static void reload(t_py_tilde *x){
    PyObject *pName, *pFunc, *pModule, *pReload;
    if (x->function_called == 0) { // if the set method was not called, then we can not run the function :)
        pd_error(x, "To reload the script you need to set the function first!");
        return;
    }
    pFunc = x->function;
    pModule = x->module;

    // reload the module
    pName = PyUnicode_DecodeFSDefault(x->script_name->s_name); // Name of script file
    pModule = PyImport_Import(pName);
    pReload = PyImport_ReloadModule(pModule);
    if (pReload == NULL) {
        pd_error(x, "Error reloading the module!");
        x->function_called = 0;
        Py_DECREF(x->function);
        Py_DECREF(x->module);
        return;
    } else{
        Py_XDECREF(x->module);
        pFunc = PyObject_GetAttrString(pModule, x->function_name->s_name); // Function name inside the script file
        Py_DECREF(pName); // 
        if (pFunc && PyCallable_Check(pFunc)){ // Check if the function exists and is callable 
            x->function = pFunc;
            x->module = pModule;
            x->script_name = x->script_name;
            x->function_name = x->function_name; // why 
            x->function_called = malloc(sizeof(int)); 
            *(x->function_called) = 1; // 
            post("The module was reloaded!");
            return; 
        }
        else{
            pd_error(x, "Error reloading the module!");
            x->function_called = 0;
            Py_DECREF(x->function);
            Py_DECREF(x->module);
            return;
        }
    } 
}

// ====================================
// ====================================
// ====================================


static void set_function(t_py_tilde *x, t_symbol *s, int argc, t_atom *argv){
    (void)s;

    t_symbol *script_file_name = atom_gensym(argv+0);
    t_symbol *function_name = atom_gensym(argv+1);

    // Check if the already was set
    if (x->function_name != NULL){
        int function_is_equal = strcmp(function_name->s_name, x->function_name->s_name);
        if (function_is_equal == 0){    // If the user wants to call the same function again! This is not necessary at first glance. 
            pd_error(x, "[py4pd~] The function was already called!");
            Py_XDECREF(x->function);
            Py_XDECREF(x->module);
            x->function = NULL;
            x->module = NULL;
            x->function_name = NULL;
        }
        else{ // DOC: If the function is different, then we need to delete the old function and create a new one.
            Py_XDECREF(x->function);
            Py_XDECREF(x->module);
            x->function = NULL;
            x->module = NULL;
            x->function_name = NULL;
        }      
    }

    // Infos
    post("[py4pd~] Script file: %s.py", script_file_name->s_name);
    post("[py4pd~] Function name: %s", function_name->s_name);

    // Erro handling
    // Check if script has .py extension
    char *extension = strrchr(script_file_name->s_name, '.');
    if (extension != NULL) {
        pd_error(x, "[py4pd~] Don't use extensions in the script file name!");
        Py_XDECREF(x->function);
        Py_XDECREF(x->module);
        return;
    }
    
    // check if script file exists
    char script_file_path[MAXPDSTRING];
    snprintf(script_file_path, MAXPDSTRING, "%s/%s.py", x->home_path->s_name, script_file_name->s_name);
    if (access(script_file_path, F_OK) == -1) {
        pd_error(x, "The script file %s does not exist!", script_file_path);
        Py_XDECREF(x->function);
        Py_XDECREF(x->module);
        return;
    }
    
    // =====================
    // DOC: check number of arguments
    if (argc < 2) { // check is the number of arguments is correct | set "function_script" "function_name"
        pd_error(x,"[py4pd~] {set} message needs two arguments! The 'Script name' and the 'function name'!");
        return;
    }
    
    // =====================
    PyObject *pName, *pModule, *pFunc; // DOC: Create the variables of the python objects
    const wchar_t *py_name_ptr; // DOC: Program name pointer
    char *random_name = malloc(sizeof(char) * 10); // DOC: Random name for the program name
    sprintf(random_name, "py4pd_%d", *(x->object_number)); // DOC: Set
    post("[py4pd~] Random name: %s", random_name); // DOC: Print the random name
    py_name_ptr = Py_DecodeLocale(random_name, NULL); // DOC: Decode the random name

    // =============== DOC: add pd Module for Python =================================
    if (PyImport_AppendInittab("pd", PyInit_pd) == -1) {
        pd_error(x, "[py4pd~] Could not load py4pd module! Please report, this shouldn't happen!\n");
        post("You can not use 'import pd' in your script!");
    } 

    // =============== DOC: add spam Module for Python =================================

    Py_SetProgramName(py_name_ptr); // set program name
    Py_Initialize(); // initialize python
    return;
    // =====================
    // DOC: Set the packages path
    const char *site_path_str = x->packages_path->s_name;
    char *add_packages = malloc(strlen(site_path_str) + strlen("sys.path.append('") + strlen("')") + 1);
    sprintf(add_packages, "sys.path.append('%s')", site_path_str);
    PyRun_SimpleString("import sys");
    // print add_add_packages
    // post("[py4pd~] %s", add_packages);
    PyRun_SimpleString(add_packages);
    free(add_packages);

    // =====================
    const char *home_path_str = x->home_path->s_name;
    char *sys_path_str = malloc(strlen(home_path_str) + strlen("sys.path.append('") + strlen("')") + 1);
    sprintf(sys_path_str, "sys.path.append('%s')", home_path_str);
    PyRun_SimpleString(sys_path_str);
    free(sys_path_str); // free the memory allocated for the string, check if this is necessary
    
    // =====================
    pName = PyUnicode_DecodeFSDefault(script_file_name->s_name); // Name of script file
    pModule = PyImport_Import(pName);
    // =====================

    // check if the module was loaded
    if (pModule == NULL) {
        PyObject *ptype, *pvalue, *ptraceback;
        PyErr_Fetch(&ptype, &pvalue, &ptraceback);
        PyErr_NormalizeException(&ptype, &pvalue, &ptraceback);
        PyObject *pstr = PyObject_Str(pvalue);
        pd_error(x, "[py4pd~] Call failed: %s", PyUnicode_AsUTF8(pstr));
        Py_XDECREF(pstr);
        Py_XDECREF(pModule);
        Py_XDECREF(pName);
        return;
    }
 
    pFunc = PyObject_GetAttrString(pModule, function_name->s_name); // Function name inside the script file
    Py_DECREF(pName); // DOC: Py_DECREF(pName) is not necessary! 
    if (pFunc && PyCallable_Check(pFunc)){ // Check if the function exists and is callable   
        PyObject *inspect=NULL, *getargspec=NULL, *argspec=NULL, *args=NULL;
        inspect = PyImport_ImportModule("inspect");
        getargspec = PyObject_GetAttrString(inspect, "getargspec");
        argspec = PyObject_CallFunctionObjArgs(getargspec, pFunc, NULL);
        args = PyObject_GetAttrString(argspec, "args");
        int py_args = PyObject_Size(args);
        post("[py4pd~] Function loaded!");
        post("[py4pd~] It has %i arguments!", py_args);
        // x->py_arg_numbers = py_args;
        // warning: assignment to 'int *' from 'int' makes pointer from integer without a cast [-Wint-conversion] in x->py_arg_numbers = py_args;
        // The solution is to use a pointer to the int variable
        x->py_arg_numbers = py_args;
        x->function = pFunc;
        x->module = pModule;
        x->script_name = script_file_name;
        x->function_name = function_name; 
        x->function_called = malloc(sizeof(unsigned int));
        *(x->function_called) = 1; // 
        py4pd_object = x;
        Py_XDECREF(inspect);
        Py_XDECREF(getargspec);
        Py_XDECREF(argspec);
        Py_XDECREF(args);
        return;

    } else {
        // post PyErr_Print() in pd
        pd_error(x, "Function %s not loaded!", function_name->s_name);
        x->function_called = 0; // set the flag to 0 because it crash Pd if user try to use args method
        x->function_name = NULL;
        PyObject *ptype, *pvalue, *ptraceback;
        PyErr_Fetch(&ptype, &pvalue, &ptraceback);
        PyErr_NormalizeException(&ptype, &pvalue, &ptraceback);
        PyObject *pstr = PyObject_Str(pvalue);
        pd_error(x, "Call failed:\n %s", PyUnicode_AsUTF8(pstr));
        Py_DECREF(pstr);
        Py_XDECREF(pModule);
        Py_XDECREF(pFunc);
        Py_XDECREF(pName);
        Py_XDECREF(ptype);
        Py_XDECREF(pvalue);
        Py_XDECREF(ptraceback);
        return;
    }
}


// ============================================
// ============================================
// ============================================

static void run_function(t_py_tilde *x, t_symbol *s, int argc, t_atom *argv){
    (void)s;


    if (argc != x->py_arg_numbers){
        pd_error(x, "[py4pd~] Number of arguments is not correct!");
        return;
    }
    
    // int running_on_thread = *(x->function_called);
    // PyGILState_STATE gstate = PyGILState_Ensure();
    if (x->function_called == 0) { // if the set method was not called, then we can not run the function :)
        pd_error(x, "[py4pd~] The message need to be formatted like 'set {script_name} {function_name}'!");
        return;
    }
    PyObject *pFunc, *pArgs, *pValue; // pDict, *pModule,
    pFunc = x->function;
    pArgs = PyTuple_New(argc);
    int i;
    // DOC: CONVERTION TO PYTHON OBJECTS
    for (i = 0; i < argc; ++i) {
        // NUMBERS 
        if (argv[i].a_type == A_FLOAT) { 
            float arg_float = atom_getfloat(argv+i);
            if (arg_float == (int)arg_float){ // DOC: If the float is an integer, then convert to int
                int arg_int = (int)arg_float;
                pValue = PyLong_FromLong(arg_int);
            }
            else{ // If the int is an integer, then convert to int
                pValue = PyFloat_FromDouble(arg_float);
            }

        // STRINGS
        } else if (argv[i].a_type == A_SYMBOL) {
            pValue = PyUnicode_DecodeFSDefault(argv[i].a_w.w_symbol->s_name); // convert to python string
        } else {
            pValue = Py_None;
            Py_INCREF(Py_None);
        }
        // ERROR IF THE ARGUMENT IS NOT A NUMBER OR A STRING       
        if (!pValue) {
            pd_error(x, "Cannot convert argument\n");
            //PyGILState_Release(gstate);
            return;
        }
        PyTuple_SetItem(pArgs, i, pValue); // DOC: Set the argument in the tuple
    }

    pValue = PyObject_CallObject(pFunc, pArgs);
    if (pValue != NULL) {                                // DOC: if the function returns a value   
        if (PyList_Check(pValue)){                       // DOC: If the function return a list list
            int list_size = PyList_Size(pValue);
            t_atom *list_array = (t_atom *) malloc(list_size * sizeof(t_atom));            
            for (i = 0; i < list_size; ++i) {
                PyObject *pValue_i = PyList_GetItem(pValue, i);
                if (PyLong_Check(pValue_i)) {            // DOC: If the function return a list of integers
                    long result = PyLong_AsLong(pValue_i);
                    float result_float = (float)result;
                    list_array[i].a_type = A_FLOAT;
                    list_array[i].a_w.w_float = result_float;

                } else if (PyFloat_Check(pValue_i)) {    // DOC: If the function return a list of floats
                    double result = PyFloat_AsDouble(pValue_i);
                    float result_float = (float)result;
                    list_array[i].a_type = A_FLOAT;
                    list_array[i].a_w.w_float = result_float;
                } else if (PyUnicode_Check(pValue_i)) {  // DOC: If the function return a list of strings
                    const char *result = PyUnicode_AsUTF8(pValue_i); 
                    list_array[i].a_type = A_SYMBOL;
                    list_array[i].a_w.w_symbol = gensym(result);
                } else if (Py_IsNone(pValue_i)) {        // DOC: If the function return a list of None
                     // post("None");
                
                } else {
                    pd_error(x, "[py4pd~] py4pd just convert int, float and string!\n");
                    pd_error(x, "INFO  [!] The value received is of type %s", Py_TYPE(pValue_i)->tp_name);
                    Py_DECREF(pValue_i);
                    Py_DECREF(pArgs);
                    //PyGILState_Release(gstate);
                    return;
                }
            }
            outlet_list(x->out_A, 0, list_size, list_array); // TODO: possible do in other way? Seems slow!
            return;
        } else {
            if (PyLong_Check(pValue)) {
                long result = PyLong_AsLong(pValue); // DOC: If the function return a integer
                outlet_float(x->out_A, result);
                //PyGILState_Release(gstate);
                return;
            } else if (PyFloat_Check(pValue)) {
                double result = PyFloat_AsDouble(pValue); // DOC: If the function return a float
                float result_float = (float)result;
                outlet_float(x->out_A, result_float);
                //PyGILState_Release(gstate);
                return;
                // outlet_float(x->out_A, result);
            } else if (PyUnicode_Check(pValue)) {
                const char *result = PyUnicode_AsUTF8(pValue); // DOC: If the function return a string
                outlet_symbol(x->out_A, gensym(result)); 
                //PyGILState_Release(gstate);
                return;
            // now check if it return None
            } else if (Py_IsNone(pValue)) {
                //post("None");
            } else {
                pd_error(x, "[py4pd~] py4pd just convert int, float and string!\n");
                pd_error(x, "INFO  [!!!!] The value received is of type %s", Py_TYPE(pValue)->tp_name);
                Py_DECREF(pValue);
                Py_DECREF(pArgs);
                return;
            }
        }
        Py_DECREF(pValue);
    }
    else { // DOC: if the function returns a error
        PyObject *ptype, *pvalue, *ptraceback;
        PyErr_Fetch(&ptype, &pvalue, &ptraceback);
        PyErr_NormalizeException(&ptype, &pvalue, &ptraceback);
        PyObject *pstr = PyObject_Str(pvalue);
        pd_error(x, "[py4pd~] Call failed: %s", PyUnicode_AsUTF8(pstr));
        Py_DECREF(pstr);
        Py_DECREF(pvalue);
        Py_DECREF(ptype);
        Py_DECREF(ptraceback);
        //PyGILState_Release(gstate);
        return;
    }
}

// ============================================
// ============================================
// ============================================
struct thread_arg_struct {
    t_py_tilde x;
    t_symbol s;
    int argc;
    t_atom *argv;
    PyGILState_STATE gil_state;
} thread_arg;

// ============================================
// ============================================
// ============================================

#ifdef _WIN64

static void env_install(t_py_tilde *x, t_symbol *s, int argc, t_atom *argv){
    // If Windows OS run, if not then warn the user
    (void)s;
    (void)argc;
    (void)argv;
    
    // concat venv_path with the name py4pd
    char *pip_install = malloc(strlen(x->home_path->s_name) + strlen("py4pd") + 20);
    sprintf(pip_install, "/c python -m venv %s/py4pd_packages", x->home_path->s_name);

    // path to venv, 
    char *pip = malloc(strlen(x->home_path->s_name) + strlen("/py4pd_packages/") + 40);
    sprintf(pip, "%s/py4pd_packages/Scripts/pip.exe", x->home_path->s_name);
    // check if pip_path exists
    if (access(pip, F_OK) == -1) {
        SHELLEXECUTEINFO sei = {0};
        sei.cbSize = sizeof(sei);
        sei.fMask = SEE_MASK_NOCLOSEPROCESS;
        sei.lpFile = "cmd.exe ";
        sei.lpParameters = pip_install;
        sei.nShow = SW_HIDE;
        ShellExecuteEx(&sei);
        CloseHandle(sei.hProcess);
        return;
    } else{
        pd_error(x, "The pip already installed!");
    }
}

// ====================================
// ====================================
// ====================================


static void pip_install(t_py_tilde *x, t_symbol *s, int argc, t_atom *argv){
    (void)s;
    (void)argc;
    
    char *package = malloc(strlen(atom_getsymbol(argv+0)->s_name) + 1);
    strcpy(package, atom_getsymbol(argv+0)->s_name);

    char *pip = malloc(strlen(x->home_path->s_name) + strlen("%s/py4pd_packages/Scripts/pip.exe") + 40);
    sprintf(pip, "%s/py4pd_packages/Scripts/pip.exe", x->home_path->s_name);
    if (access(pip, F_OK) == -1) {
        pd_error(x, "The pip path does not exist. Send a message {env_install} to install pip first!");
        return;
    } else{
        char *pip_cmd = malloc(strlen(x->packages_path->s_name) + strlen("py4pd") + 20);
        sprintf(pip_cmd, "/c %s install %s", pip, package);
        post("[py4pd~] Installing %s", package);
        SHELLEXECUTEINFO sei = {0};
        sei.cbSize = sizeof(sei);
        sei.fMask = SEE_MASK_NOCLOSEPROCESS;
        sei.lpFile = "cmd.exe ";
        sei.lpParameters = pip_cmd;
        sei.nShow = SW_HIDE;
        ShellExecuteEx(&sei);
        CloseHandle(sei.hProcess);
        post("[py4pd~] %s installed!", package);
        return;
    }
}

// ============================================
// =========== CREATE WIN THREAD ==============
// ============================================

DWORD WINAPI ThreadFunc(LPVOID lpParam) { // DOC: Thread function in Windows
    struct thread_arg_struct *arg = (struct thread_arg_struct *)lpParam;       
    t_py_tilde *x = &arg->x;
    int argc = arg->argc;
    t_symbol *s = &arg->s;
    t_atom *argv = arg->argv;
    int object_number = *(x->object_number);
    thread_status[object_number] = 1;
    run_function(x, s, argc, argv);
    thread_status[object_number] = 0;
    return 0;
}
// ============================================
// ============================================
// ============================================

static void create_thread(t_py_tilde *x, t_symbol *s, int argc, t_atom *argv){
    (void)s;
    DWORD threadID;
    HANDLE hThread;
    struct thread_arg_struct *arg = (struct thread_arg_struct *)malloc(sizeof(struct thread_arg_struct));

    arg->x = *x;
    arg->argc = argc;
    arg->argv = argv;
    int object_number = *(x->object_number);
    if (x->function_called == 0) {
        pd_error(x, "[py4pd~] You need to call a function before run!");
        free(arg);
        return;
        } 
    else {
        if (thread_status[object_number] == 0){
            hThread = CreateThread(NULL, 0, ThreadFunc, arg, 0, &threadID);
            x->state = malloc(sizeof(int)); // 
            *(x->state) = 1; // DOC: 1 = thread is running, not two threads can run at the same time
            if (hThread == NULL) {
                pd_error(x, "[py4pd~] CreateThread failed");
                free(arg);
                return;
            } else {
                return;
            }
        } else {
            pd_error(x, "[py4pd~] Just one thread can be running at a time!");
            free(arg);
            return;
        }
    }
}

// ============================================
// ============= UNIX =========================
// ============================================

// If OS is Linux or Mac OS then use this function
#else

// what is the linux equivalent for Lvoid Parameter(void *lpParameter)
static void *ThreadFunc(void *lpParameter) {
    
    struct thread_arg_struct *arg = (struct thread_arg_struct *)lpParameter;
    t_py_tilde *x = &arg->x; 
    t_symbol *s = &arg->s;
    int argc = arg->argc;
    t_atom *argv = arg->argv;
    int object_number = *(x->object_number);
    thread_status[object_number] = 1;
    run_function(x, s, argc, argv);
    thread_status[object_number] = 0;
    return NULL;
}

// create_thread in Linux
static void create_thread(t_py_tilde *x, t_symbol *s, int argc, t_atom *argv){
    (void)s;
    struct thread_arg_struct *arg = (struct thread_arg_struct *)malloc(sizeof(struct thread_arg_struct));
    arg->x = *x;
    arg->argc = argc;
    arg->argv = argv;
    int object_number = *(x->object_number);
    if (x->function_called == 0) {
        // Pd is crashing when I try to create a thread.
        pd_error(x, "[py4pd~] You need to call a function before run!");
        free(arg);
        return;
    } else {
        if (thread_status[object_number] == 0){
            pthread_t thread;
            pthread_t hThread;
            hThread = pthread_create(&thread, NULL, ThreadFunc, arg);
            // convert int * to int
            int state = 1;
            x->state = &state;
            // check the Thread was created
            if (hThread != 0) {
                pd_error(x, "[py4pd~] CreateThread failed");
                free(arg);
                return;
            } else {
                return;
            }
        } else {
            pd_error(x, "[py4pd~] Just one thread can be running at a time!");
            free(arg);
            return;
        }
    }
}
    
#endif

// ============================================
static void run(t_py_tilde *x, t_symbol *s, int argc, t_atom *argv){
    // convert pointer x->thread to a int
    int thread = *(x->thread);
    if (thread == 1) {
        create_thread(x, s, argc, argv);
    } else if (thread == 2) {
        run_function(x, s, argc, argv);
    } else {
        pd_error(x, "[py4pd~] Thread not created");
    }
}

// ============================================
// ============================================
// ============================================

static void thread(t_py_tilde *x, t_floatarg f){
    int thread = (int)f;
    if (thread == 1) {
        post("[py4pd~] Threading enabled");
        x->thread = malloc(sizeof(int)); 
        *(x->thread) = 1; // 
        return;
    } else if (thread == 0) {
        x->thread = malloc(sizeof(int)); 
        *(x->thread) = 2; // 
        post("[py4pd~] Threading disabled");
        return;
    } else {
        pd_error(x, "[py4pd~] Threading status must be 0 or 1");
    }
}

// ============================================
// ============= PY4PD AUDIO ==================
// ============================================

static t_int * py4pd_perform(t_int *w){
    t_float *py4pd = (t_float *) (w[1]); // the first element is a pointer to the dataspace of this object
    t_sample *in1 =      (t_sample *)(w[2]); // here is a pointer to the t_sample arrays that hold the 2 input signals
    t_py_tilde *x  = (t_py_tilde *) (w[3]);
    int py4pdBlocksize = (int)(w[4]); // the number of samples to process
    // transform in to a numpy array
    // check if the function is called
    if (py4pd_object->function_called == 0) {
    //     // post function_called
        post("function_called: %d", py4pd_object->function_called);
        if (py4pd_object->audio_warning == 0) { // TODO: Need to fix this!!!
            pd_error(py4pd_object, "[py4pd~] You need to call a function before run!");
            py4pd_object->audio_warning = malloc(sizeof(unsigned int));
            py4pd_object->audio_warning = 1;
        }
        return (w + 4);
    }
    PyObject *pArgs = PyTuple_New(1);
    PyObject *pValue;
    // convert in1 to a numpy array
    npy_intp dims[1] = {py4pdBlocksize};
    PyObject *in1_array = PyArray_SimpleNewFromData(1, dims, NPY_FLOAT, in1);

    // add in1_array to pArgs
    PyTuple_SetItem(pArgs, 0, in1_array);

    // run the function
    pValue = PyObject_CallObject(py4pd_object->function, pArgs);
    // call the function
    if (pValue != NULL) {
        // convert pValue to a numpy array
        PyObject *out_array = PyArray_FROM_OTF(pValue, NPY_FLOAT, NPY_ARRAY_IN_ARRAY);
        // convert out_array to a t_sample
        t_sample *out = (t_sample *)PyArray_DATA(out_array);
        // copy out to py4pd
        memcpy(py4pd, out, py4pdBlocksize * sizeof(t_sample));
        // free the memory
        Py_DECREF(pArgs);
        Py_DECREF(pValue);
        Py_DECREF(out_array);
        return (w + 4);
    } else {
        pd_error(py4pd_object, "[py4pd~] Function call failed");
        Py_DECREF(pArgs);
        if (PyErr_Occurred())
            PyErr_Print();
        return (w + 4);
    }
}


// ============================================
static void py4pd_dsp(t_py_tilde *x, t_signal **sp){
  dsp_add(py4pd_perform, 3, sp[0]->s_vec, sp[0]->s_n, x);
}

// create_audio_output ========================
static void create_audio_output(t_py_tilde *x){
    // create a new signal out in out_A
    outlet_new(&x->x_obj, gensym("signal"));
    
}

// ============================================
// =========== SETUP OF OBJECT ================
// ============================================


void *py4pd_tilde_new(t_symbol *s, int argc, t_atom *argv){ 
    t_py_tilde *x = (t_py_tilde *)pd_new(py4pd_class);
    // credits
    post("");
    post("[py4pd~] py4pd by Charles K. Neimog");
    post("[py4pd~] version 0.0.3       ");
    post("[py4pd~] based on Python 3.10.5  ");
    post("[py4pd~] inspired by the work of Thomas Grill and SOPI research group.");
    // Object count
    x->object_number = malloc(sizeof(int));
    *(x->object_number) = object_count;
    object_count++;
    x->out_A = outlet_new(&x->x_obj, &s_signal); // cria um outlet 
    x->x_canvas = canvas_getcurrent(); // pega o canvas atual
    t_canvas *c = x->x_canvas;  // p
    x->home_path = canvas_getdir(c);     // set name 

    // change \ to / in x->home_path | PlugData give the path with \ and Python need /
    char *p = x->home_path;
    while (*p) {
        if (*p == '\\') *p = '/';
        p++;
    }

    x->packages_path = canvas_getdir(c); // set name
    x->thread = malloc(sizeof(int)); 
    *(x->thread) = 2; // solution but it is weird, 2 is used as false because 0 gives error! 
    post("[py4pd~] Home folder is: %s", x->home_path->s_name);

    // audio_warning == 0
    x->audio_warning = 0;
    
    // check if in x->home_path there is a file py4pd.config
    char *config_path = (char *)malloc(sizeof(char) * (strlen(x->home_path->s_name) + strlen("/py4pd.cfg") + 1)); // 
    strcpy(config_path, x->home_path->s_name); // copy string one into the result.
    strcat(config_path, "/py4pd.cfg"); // append string two to the result.
    if (access(config_path, F_OK) != -1) { // check if file exists
        FILE* file = fopen(config_path, "r"); /* should check the result */
        char line[256]; // line buffer
        while (fgets(line, sizeof(line), file)) { // read a line
            if (strstr(line, "packages =") != NULL) { // check if line contains "packages ="
                char *packages_path = (char *)malloc(sizeof(char) * (strlen(line) - strlen("packages = ") + 1)); // 
                strcpy(packages_path, line + strlen("packages = ")); // copy string one into the result.
                packages_path[strlen(packages_path) - 1] = '\0'; // remove last character
                if (strlen(packages_path) > 0) { // check if path is not empty
                    x->packages_path = gensym(packages_path); // set name
                    post("[py4pd~] Packages path set to %s", packages_path); // print path
                }
                free(packages_path); // free memory
            }
        }
        fclose(file); // close file
        // free(line); // free memory
    } else {
        post("[py4pd~] Could not find py4pd.cfg in home directory"); // print path
    }
    free(config_path); // free memory
    if (argc > 1) { // check if there are two arguments
        // 
        set_function(x, s, argc, argv); // this not work with python submodules
    }
    // save the t_py_tilde *x in a global variable TODO: How send this using PyCObject?
    py4pd_object = x;
    return(x);
}

// ============================================
// =========== REMOVE OBJECT ==================
// ============================================

void py4pd_tilde_free(t_py_tilde *x){
    PyObject  *pModule, *pFunc; // pDict, *pName,
    pFunc = x->function;
    pModule = x->module;
    object_count--;
    // clear all struct
    if (pModule != NULL) {
        Py_DECREF(pModule);
    }
    if (pFunc != NULL) {
        Py_DECREF(pFunc);
    }
}

// ====================================================
void py4pd_tilde_setup(void){
    py4pd_class =     class_new(gensym("py4pd~"), // cria o objeto quando escrevemos py4pd
                        (t_newmethod)py4pd_tilde_new, // metodo de criação do objeto             
                        (t_method)py4pd_tilde_free, // quando voce deleta o objeto
                        sizeof(t_py_tilde), // quanta memoria precisamos para esse objeto
                        CLASS_DEFAULT, // nao há uma GUI especial para esse objeto???
                        A_GIMME, // o argumento é um símbolo
                        0); // todos os outros argumentos por exemplo um numero seria A_DEFFLOAT
    
    // add method for bang
    class_addbang(py4pd_class, run);
    class_addmethod(py4pd_class, (t_method)home_tilde, gensym("home"), A_GIMME, 0); // set home path
    class_addmethod(py4pd_class, (t_method)vscode, gensym("click"), 0, 0); // when click open vscode
    class_addmethod(py4pd_class, (t_method)packages, gensym("packages"), A_GIMME, 0); // set packages path
    #ifdef _WIN64
    class_addmethod(py4pd_class, (t_method)env_install, gensym("env_install"), 0, 0); // install enviroment
    class_addmethod(py4pd_class, (t_method)pip_install, gensym("pip"), 0, 0); // install packages with pip
    // Audio support
    class_addmethod(py4pd_class, (t_method)create_audio_output, gensym("audio_output"), 0, 0); // Create Audio Output
    class_addmethod(py4pd_class, (t_method)py4pd_dsp, gensym("dsp"), 0);
    CLASS_MAINSIGNALIN(py4pd_class, t_py_tilde, x_f);
    // end audio support
    #endif
    class_addmethod(py4pd_class, (t_method)vscode, gensym("vscode"), 0, 0); // open vscode
    class_addmethod(py4pd_class, (t_method)reload, gensym("reload"), 0, 0); // reload python script
    class_addmethod(py4pd_class, (t_method)create, gensym("create"), A_GIMME, 0); // create file or open it
    class_addmethod(py4pd_class, (t_method)documentation, gensym("doc"), 0, 0); // open documentation
    class_addmethod(py4pd_class, (t_method)set_function, gensym("set"), A_GIMME, 0); // set function to be called
    class_addmethod(py4pd_class, (t_method)run, gensym("run"), A_GIMME, 0);  // run function
    class_addmethod(py4pd_class, (t_method)thread, gensym("thread"), A_FLOAT, 0); // on/off threading
    }

// ==================== PD FUNCTIONS INSIDE PYTHON ====================
// ====================================================================
// ====================================================================

// dll export function
#ifdef _WIN64

__declspec(dllexport) void py4pd_tilde_setup(void); // when I add python module for some reson pd not see py4pd_setup

#endif
