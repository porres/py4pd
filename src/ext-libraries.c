#include "py4pd.h"

#define NPY_NO_DEPRECATED_API NPY_1_25_API_VERSION
#include <numpy/arrayobject.h>

static t_class *py4pdInlets_proxy_class;

void Py4pdLib_Bang(t_py *x);


// =====================================
void Py4pdLib_Py4pdObjPicSave(t_gobj *z, t_binbuf *b){ 
    t_py *x = (t_py *)z;
    if (x->visMode){
        binbuf_addv(b, "ssii", gensym("#X"), gensym("obj"), x->obj.te_xpix, x->obj.te_ypix);
        binbuf_addbinbuf(b, ((t_py *)x)->obj.te_binbuf);
        int objAtomsCount = binbuf_getnatom(((t_py *)x)->obj.te_binbuf);
        if (objAtomsCount == 1){
            binbuf_addv(b, "ii", x->x_width, x->x_height);
        }
        binbuf_addsemi(b);
    }
    return;
}

// ====================================================
void Py4pdLib_Click(t_py *x) {
    PyCodeObject *code = (PyCodeObject *)PyFunction_GetCode(x->function);
    int line = PyCode_Addr2Line(code, 0);
    char command[MAXPDSTRING];
    Py4pdUtils_GetEditorCommand(x, command, line);
    Py4pdUtils_ExecuteSystemCommand(command);
    return;
}

// =====================================
void Py4pdLib_SetKwargs(t_py *x, t_symbol *s, int ac, t_atom *av){
    (void)s;
    t_symbol *key;

    if (av[0].a_type != A_SYMBOL){
        pd_error(x, "The first argument of the message 'kwargs' must be a symbol");
        return;
    }

    if (ac < 2){
        pd_error(x, "You need to specify a value for the key");
        return;
    }


    key = av[0].a_w.w_symbol;

    if (x->kwargsDict == NULL)
        x->kwargsDict = PyDict_New();

    if (ac == 2){
        if (av[1].a_type == A_FLOAT){
            int isInt = (int)av[0].a_w.w_float == av[0].a_w.w_float;
            if (isInt)
                PyDict_SetItemString(x->kwargsDict, key->s_name, PyLong_FromLong(av[1].a_w.w_float));
            else
                PyDict_SetItemString(x->kwargsDict, key->s_name, PyFloat_FromDouble(av[1].a_w.w_float));
            
        }
        else if (av[1].a_type == A_SYMBOL)
            PyDict_SetItemString(x->kwargsDict, key->s_name, PyUnicode_FromString(av[1].a_w.w_symbol->s_name));
        else{
            pd_error(x, "The third argument of the message 'kwargs' must be a symbol or a float");
            return;
        }
    }
    else if (ac > 2){
        PyObject *pyInletValue = PyList_New(ac - 1);
        for (int i = 1; i < ac; i++){
            if (av[i].a_type == A_FLOAT){ 
                int isInt = (int)av[i].a_w.w_float == av[i].a_w.w_float;
                if (isInt)
                    PyList_SetItem(pyInletValue, i - 1, PyLong_FromLong(av[i].a_w.w_float));
                else
                    PyList_SetItem(pyInletValue, i - 1, PyFloat_FromDouble(av[i].a_w.w_float));
            }
            else if (av[i].a_type == A_SYMBOL)
                PyList_SetItem(pyInletValue, i - 1, PyUnicode_FromString(av[i].a_w.w_symbol->s_name));
        }
        PyDict_SetItemString(x->kwargsDict, key->s_name, pyInletValue);
    }
    return;
}

// ====================================================
int Py4pdLib_CreateObjInlets(PyObject *function, t_py *x, int argc, t_atom *argv) {
    (void)function;
    t_pd **py4pdInlet_proxies;
    int i;
    int pyFuncArgs = x->py_arg_numbers - 1;
    
    PyObject *defaults = PyObject_GetAttrString(function, "__defaults__"); // TODO:, HOW CLEAR THIS?
    int defaultsCount = PyTuple_Size(defaults);

    if (x->use_pArgs && defaultsCount > 0){
        pd_error(x, "[py4pd] You can't use *args and defaults at the same time");
        return -1;
    }
    int indexWhereStartDefaults = x->py_arg_numbers - defaultsCount;
    if (indexWhereStartDefaults == 0){
        t_py4pd_pValue *PyPtrValue = (t_py4pd_pValue *)malloc(sizeof(t_py4pd_pValue));
        PyPtrValue->objectsUsing = 0;
        PyPtrValue->pdout = 0;
        PyPtrValue->objOwner = x->objectName;
        PyPtrValue->pValue = PyTuple_GetItem(defaults, 0);
        x->pyObjArgs[0] = PyPtrValue;
    }

    if (pyFuncArgs != 0){
        py4pdInlet_proxies = (t_pd **)getbytes((pyFuncArgs + 1) * sizeof(*py4pdInlet_proxies));
        for (i = 0; i < pyFuncArgs; i++){
            py4pdInlet_proxies[i] = pd_new(py4pdInlets_proxy_class);
            t_py4pdInlet_proxy *y = (t_py4pdInlet_proxy *)py4pdInlet_proxies[i];
            y->p_master = x;
            y->inletIndex = i + 1;
            inlet_new((t_object *)x, (t_pd *)y, 0, 0);
            t_py4pd_pValue *PyPtrValue = (t_py4pd_pValue *)malloc(sizeof(t_py4pd_pValue));
            PyPtrValue->objectsUsing = 0;
            PyPtrValue->pdout = 0;
            PyPtrValue->objOwner = x->objectName;
            if (i + 1 >= indexWhereStartDefaults){
                PyPtrValue->pValue = PyTuple_GetItem(defaults, (i + 1) - indexWhereStartDefaults);
            }
            else{
                PyPtrValue->pValue = Py_None;
                Py_INCREF(Py_None);
            }
            x->pyObjArgs[i + 1] = PyPtrValue;
        }
        int argNumbers = x->py_arg_numbers;

        for (i = 0; i < argNumbers; i++) {
            if (i <= argc) {
                if (argv[i].a_type == A_FLOAT) {
                    int isInt = (int)argv[i].a_w.w_float == argv[i].a_w.w_float;
                    if (isInt){
                        x->pyObjArgs[i]->pValue = PyLong_FromLong(argv[i].a_w.w_float);
                    }
                    else{
                        x->pyObjArgs[i]->pValue = PyFloat_FromDouble(argv[i].a_w.w_float);
                    }
                }

                else if (argv[i].a_type == A_SYMBOL) {
                    if (strcmp(argv[i].a_w.w_symbol->s_name, "None") == 0){
                        Py_INCREF(Py_None);
                        x->pyObjArgs[i]->pValue = Py_None;
                    }
                    else{ 
                        x->pyObjArgs[i]->pValue = PyUnicode_FromString(argv[i].a_w.w_symbol->s_name);
                    }
                }
                else if(x->pyObjArgs[i]->pValue == NULL){
                    Py_INCREF(Py_None);
                    x->pyObjArgs[i]->pValue = Py_None;
                }
            }
            else if(x->pyObjArgs[i]->pValue == NULL){
                Py_INCREF(Py_None);
                x->pyObjArgs[i]->pValue = Py_None;
            }
        }
    }
    else{
        if (indexWhereStartDefaults == 0)
            return 0;
        else{
            t_py4pd_pValue *PyPtrValue = (t_py4pd_pValue *)malloc(sizeof(t_py4pd_pValue));
            PyPtrValue->objectsUsing = 0;
            PyPtrValue->pdout = 0;
            PyPtrValue->objOwner = x->objectName;
            Py_INCREF(Py_None);
            PyPtrValue->pValue = Py_None;
            x->pyObjArgs[0] = PyPtrValue;
        }
    }
    return 0;
}

// =====================================
void Py4pdLib_ProxyPointer(t_py4pdInlet_proxy *x, t_symbol *s, t_gpointer *gp){
    (void)s;
    t_py *py4pd = (t_py *)x->p_master;
    t_py4pd_pValue *pArg;
    pArg = (t_py4pd_pValue *)gp;
    if (!pArg->pdout)
        Py_DECREF(py4pd->pyObjArgs[x->inletIndex]->pValue);

    Py4pdUtils_CopyPy4pdValueStruct(pArg, py4pd->pyObjArgs[x->inletIndex]);
    if (!pArg->pdout)
        Py_INCREF(py4pd->pyObjArgs[x->inletIndex]->pValue); 
    
    return;
}

// =============================================
void Py4pdLib_Pointer(t_py *x, t_symbol *s, t_gpointer *gp){
    (void)s;

    t_py4pd_pValue *pArg;
    pArg = (t_py4pd_pValue *)gp;
    pArg->objectsUsing++;

    PyObject* pArgs = PyTuple_New(x->py_arg_numbers);
    PyTuple_SetItem(pArgs, 0, pArg->pValue);
    Py_INCREF(pArg->pValue);

    for (int i = 1; i < x->py_arg_numbers; i++){
        PyTuple_SetItem(pArgs, i, x->pyObjArgs[i]->pValue);
        Py_INCREF(x->pyObjArgs[i]->pValue); 
    }

    if (x->audioOutput)
        return; 
    

    Py4pdUtils_RunPy(x, pArgs, x->kwargsDict);
    Py_DECREF(pArgs);
    
    return;
}

// =====================================
void Py4pdLib_ProxyAnything(t_py4pdInlet_proxy *x, t_symbol *s, int ac, t_atom *av){
    (void)s;
    t_py *py4pd = (t_py *)x->p_master;
    PyObject *pyInletValue = NULL;

    if (ac == 0)
        pyInletValue = PyUnicode_FromString(s->s_name);
    else if ((s == gensym("list") || s == gensym("anything")) && ac > 1){
        pyInletValue = PyList_New(ac);
        for (int i = 0; i < ac; i++){
            if (av[i].a_type == A_FLOAT){ 
                int isInt = (int)av[i].a_w.w_float == av[i].a_w.w_float;
                if (isInt)
                    PyList_SetItem(pyInletValue, i, PyLong_FromLong(av[i].a_w.w_float));
                else
                    PyList_SetItem(pyInletValue, i, PyFloat_FromDouble(av[i].a_w.w_float));
            }
            else if (av[i].a_type == A_SYMBOL)
                PyList_SetItem(pyInletValue, i, PyUnicode_FromString(av[i].a_w.w_symbol->s_name));
        }
    }
    else if ((s == gensym("float") || s == gensym("symbol")) && ac == 1){
        if (av[0].a_type == A_FLOAT){ 
            int isInt = (int)av[0].a_w.w_float == av[0].a_w.w_float;
            if (isInt)
                pyInletValue = PyLong_FromLong(av[0].a_w.w_float);
            else
                pyInletValue = PyFloat_FromDouble(av[0].a_w.w_float);
        }
        else if (av[0].a_type == A_SYMBOL)
            pyInletValue = PyUnicode_FromString(av[0].a_w.w_symbol->s_name);
    }
    else{
        pyInletValue = PyList_New(ac + 1);
        PyList_SetItem(pyInletValue, 0, PyUnicode_FromString(s->s_name));
        for (int i = 0; i < ac; i++){
            if (av[i].a_type == A_FLOAT){ 
                int isInt = (int)av[i].a_w.w_float == av[i].a_w.w_float;
                if (isInt)
                    PyList_SetItem(pyInletValue, i + 1, PyLong_FromLong(av[i].a_w.w_float));
                else
                    PyList_SetItem(pyInletValue, i + 1, PyFloat_FromDouble(av[i].a_w.w_float));
            }
            else if (av[i].a_type == A_SYMBOL)
                PyList_SetItem(pyInletValue, i + 1, PyUnicode_FromString(av[i].a_w.w_symbol->s_name));
        }
    }
    if (!py4pd->pyObjArgs[x->inletIndex]->pdout)
        Py_DECREF(py4pd->pyObjArgs[x->inletIndex]->pValue);

    py4pd->pyObjArgs[x->inletIndex]->objectsUsing = 0;
    py4pd->pyObjArgs[x->inletIndex]->pdout = 0;
    py4pd->pyObjArgs[x->inletIndex]->objOwner = py4pd->objectName;
    py4pd->pyObjArgs[x->inletIndex]->pValue = pyInletValue;
    return;
}

// =====================================
void Py4pdLib_Anything(t_py *x, t_symbol *s, int ac, t_atom *av){

    if (x->function == NULL){
        pd_error(x, "[py4pd] Function not defined");
        return;
    }
    if (s == gensym("bang")){
        Py4pdLib_Bang(x);
        return;
    }
    
    PyObject *pyInletValue = NULL;
    if (ac == 0)
        pyInletValue = PyUnicode_FromString(s->s_name);
    else if ((s == gensym("list") || s == gensym("anything")) && ac > 1){
        pyInletValue = PyList_New(ac);
        for (int i = 0; i < ac; i++){
            if (av[i].a_type == A_FLOAT){ 
                int isInt = (int)av[i].a_w.w_float == av[i].a_w.w_float;
                if (isInt)
                    PyList_SetItem(pyInletValue, i, PyLong_FromLong(av[i].a_w.w_float));
                else
                    PyList_SetItem(pyInletValue, i, PyFloat_FromDouble(av[i].a_w.w_float));
            }
            else if (av[i].a_type == A_SYMBOL)
                PyList_SetItem(pyInletValue, i, PyUnicode_FromString(av[i].a_w.w_symbol->s_name));
        }
    }
    else if ((s == gensym("float") || s == gensym("symbol")) && ac == 1){
        if (av[0].a_type == A_FLOAT){ 
            int isInt = (int)av[0].a_w.w_float == av[0].a_w.w_float;
            if (isInt)
                pyInletValue = PyLong_FromLong(av[0].a_w.w_float);
            else
                pyInletValue = PyFloat_FromDouble(av[0].a_w.w_float);
        }
        else if (av[0].a_type == A_SYMBOL)
            pyInletValue = PyUnicode_FromString(av[0].a_w.w_symbol->s_name);
    }
    else{
        pyInletValue = PyList_New(ac + 1);
        PyList_SetItem(pyInletValue, 0, PyUnicode_FromString(s->s_name));
        for (int i = 0; i < ac; i++){
            if (av[i].a_type == A_FLOAT){ 
                int isInt = (int)av[i].a_w.w_float == av[i].a_w.w_float;
                if (isInt)
                    PyList_SetItem(pyInletValue, i + 1, PyLong_FromLong(av[i].a_w.w_float));
                else
                    PyList_SetItem(pyInletValue, i + 1, PyFloat_FromDouble(av[i].a_w.w_float));
            }
            else if (av[i].a_type == A_SYMBOL)
                PyList_SetItem(pyInletValue, i + 1, PyUnicode_FromString(av[i].a_w.w_symbol->s_name));
        }
    }

    if (x->audioOutput)
        return; 
    
    PyObject* pArgs = PyTuple_New(x->py_arg_numbers);
    PyTuple_SetItem(pArgs, 0, pyInletValue);

    for (int i = 1; i < x->py_arg_numbers; i++){
        PyTuple_SetItem(pArgs, i, x->pyObjArgs[i]->pValue);
        Py_INCREF(x->pyObjArgs[i]->pValue); // This keep the reference.
    }


    Py4pdUtils_RunPy(x, pArgs, x->kwargsDict);

    Py4pdUtils_DECREF(pArgs);
    return;
}

// =====================================
void Py4pdLib_Bang(t_py *x){
    if (x->py_arg_numbers != 0){
        post("Bang can be used only with no arguments Function");
        return;
    }
    if (x->function == NULL){
        pd_error(x, "[py4pd] Function not defined");
        return;
    }
    if (x->audioOutput){
        return; 
    }
    PyObject* pArgs = PyTuple_New(0);
    Py4pdUtils_RunPy(x, pArgs, x->kwargsDict);
    Py_DECREF(pArgs);
}


// =====================================
void Py4pdLib_ReloadObject(t_py *x){
    char *script_filename = strdup(x->script_name->s_name);

    PyObject *ScriptFolder = PyUnicode_FromString(Py4pdUtils_GetFolderName(script_filename));
    PyObject *sys_path = PySys_GetObject("path");
    PyList_Insert(sys_path, 0, ScriptFolder);

    const char *ScriptFileName = Py4pdUtils_GetFilename(script_filename);
    
    PyObject *pModule = PyImport_ImportModule(ScriptFileName);
    if (pModule == NULL) {
        PyObject *ptype, *pvalue, *ptraceback;
        PyErr_Fetch(&ptype, &pvalue, &ptraceback);
        PyErr_NormalizeException(&ptype, &pvalue, &ptraceback);
        PyObject *pstr = PyObject_Str(pvalue);
        pd_error(x, "[Python] %s", PyUnicode_AsUTF8(pstr));
        Py_DECREF(pstr);
        Py_XDECREF(ptype);
        Py_XDECREF(pvalue);
        Py_XDECREF(ptraceback);
        PyErr_Clear();
        return;
    }
    PyObject *pModuleReloaded = PyImport_ReloadModule(pModule);
    if (pModuleReloaded == NULL) {
        pd_error(x, "[Python] Failed to reload module");
        return;
    }
    x->function = PyObject_GetAttrString(pModuleReloaded, x->function_name->s_name);
    if (x->function == NULL) {
        pd_error(x, "[Python] Failed to get function");
        return;
    }
    else {
        post("[Python] Function reloaded");
    }
    return;
}


// =====================================
// ================ AUDIO ==============
// =====================================
static void Py4pdLib_Audio2PdAudio(t_py *x, PyObject *pValue, t_sample *audioOut, int numChannels, int n){ 
    if (pValue != NULL) {
        if (PyArray_Check(pValue)) {
            PyArrayObject *pArray = PyArray_GETCONTIGUOUS((PyArrayObject *)pValue); // this is the output vector
            PyArray_Descr *pArrayType = PyArray_DESCR(pArray); // this is the type of the output vector
            int arrayLength = PyArray_SIZE(pArray); // this is the length of the output vector
            if (arrayLength <= n){
                if (pArrayType->type_num == NPY_FLOAT) {
                    for (int i = 0; i < numChannels; i++) {
                        float *audioFloat = (float*)PyArray_GETPTR2(pArray, i, 0);
                        for (int j = 0; j < x->vectorSize; j++) {
                            audioOut[i * x->vectorSize + j] = (t_sample)audioFloat[j];
                        }
                    }
                }
                else if (pArrayType->type_num == NPY_DOUBLE) {
                    for (int i = 0; i < numChannels; i++) {
                        double *audioFloat = (double *)PyArray_GETPTR2(pArray, i, 0);
                        for (int j = 0; j < x->vectorSize; j++) {
                            audioOut[i * x->vectorSize + j] = (t_sample)audioFloat[j];
                        }
                    }
                }
                else {
                    pd_error(x, "[py4pd] The numpy array must be float or double, returned %d", pArrayType->type_num);
                }
                Py_DECREF(pArrayType);
                Py_DECREF(pArray);
            
            }
            else {
                pd_error(x, "[py4pd] The numpy array return more channels that the object was created. It has %d channels returned %d channels", numChannels, (int)arrayLength / (int)x->vectorSize);
                Py_DECREF(pArrayType);
                Py_DECREF(pArray);
            }
        } 
        else{
            pd_error(x, "[Python] Python function must return a numpy array");
        }
    }
    else {                             
        PyObject *ptype, *pvalue, *ptraceback;
        PyErr_Fetch(&ptype, &pvalue, &ptraceback);
        PyErr_NormalizeException(&ptype, &pvalue, &ptraceback);
        PyObject *pstr = PyObject_Str(pvalue);
        pd_error(x, "[py4pd] Call failed: %s", PyUnicode_AsUTF8(pstr));
        Py_DECREF(pstr);
        Py_XDECREF(ptype);
        Py_XDECREF(pvalue);
        Py_XDECREF(ptraceback);
        PyErr_Clear();
    }

}

// =====================================
t_int *Py4pdLib_AudioINPerform(t_int *w) {
    t_py *x = (t_py *)(w[1]);  // this is the object itself
    t_sample *in = (t_sample *)(w[2]);  // this is the input vector (the sound)
    int n = (int)(w[3]);
    x->vectorSize = n;

    int numChannels = n / x->vectorSize;
    npy_intp dims[] = {numChannels, x->vectorSize};
    PyObject *pAudio = PyArray_SimpleNewFromData(2, dims, NPY_FLOAT, in);
    PyTuple_SetItem(x->argsDict, 0, pAudio);

    Py4pdUtils_RunPy(x, x->argsDict, x->kwargsDict);

    return (w + 4);
}

// =====================================
t_int *Py4pdLib_AudioOUTPerform(t_int *w) {
    t_py *x = (t_py *)(w[1]);  // this is the object itself
    t_sample *audioOut = (t_sample *)(w[2]);
    int n = (int)(w[3]);
    PyObject *pValue; 
    int numChannels = x->n_channels;
    pValue = Py4pdUtils_RunPy(x, x->argsDict, x->kwargsDict);
    if (pValue != NULL){
        Py4pdLib_Audio2PdAudio(x, pValue, audioOut, numChannels, n);
        Py_DECREF(pValue);
    }
    return (w + 4);
}

// =====================================
t_int *Py4pdLib_AudioPerform(t_int *w){
    t_py *x = (t_py *)(w[1]);  // this is the object itself
    t_sample *in = (t_sample *)(w[2]);
    t_sample *audioOut = (t_sample *)(w[3]);
    int n = (int)(w[4]);
    int numChannels = n / x->vectorSize;
    npy_intp dims[] = {numChannels, x->vectorSize};
    PyObject *pAudio = PyArray_SimpleNewFromData(2, dims, NPY_FLOAT, in);
    PyTuple_SetItem(x->argsDict, 0, pAudio);
    PyObject *pValue = Py4pdUtils_RunPy(x, x->argsDict, x->kwargsDict);
    if (pValue != NULL){
        Py4pdLib_Audio2PdAudio(x, pValue, audioOut, numChannels, n);
        Py_DECREF(pValue);
    }
    return (w + 5);  // BUG: Note work
}

// =====================================
static void Py4pdLib_Dsp(t_py *x, t_signal **sp) {
    if (x->audioOutput == 0) {
        dsp_add(Py4pdLib_AudioINPerform, 4, x, sp[0]->s_vec, sp[1]->s_vec, PY4PDSIGTOTAL(sp[0])); 
    } 
    else if ((x->audioInput == 0) && (x->audioOutput == 1)){
        x->vectorSize = sp[0]->s_n;
        signal_setmultiout(&sp[0], x->n_channels);
        dsp_add(Py4pdLib_AudioOUTPerform, 3, x, sp[0]->s_vec, PY4PDSIGTOTAL(sp[0]));
    }
    else if ((x->audioInput == 1) && (x->audioOutput == 1)){
        x->vectorSize = sp[0]->s_n;
        signal_setmultiout(&sp[1], sp[0]->s_nchans);
        dsp_add(Py4pdLib_AudioPerform, 4, x, sp[0]->s_vec, sp[1]->s_vec, PY4PDSIGTOTAL(sp[0]));
    }
}

// =====================================
static void *Py4pdLib_NewObj(t_symbol *s, int argc, t_atom *argv) {
    const char *objectName = s->s_name;
    char py4pd_objectName[MAXPDSTRING];
    sprintf(py4pd_objectName, "py4pd_ObjectDict_%s", objectName);

    PyObject *pd_module = PyImport_ImportModule("pd");
    PyObject *py4pd_capsule = PyObject_GetAttrString(pd_module, py4pd_objectName);
    PyObject *PdDictCapsule = PyCapsule_GetPointer(py4pd_capsule, objectName);
    if (PdDictCapsule == NULL) {
        pd_error(NULL, "Error: PdDictCapsule is NULL, please report!");
        Py_DECREF(pd_module);
        Py_DECREF(py4pd_capsule);
        return NULL;
    }
    PyObject *PdDict = PyDict_GetItemString(PdDictCapsule, objectName);
    if (PdDict == NULL) {
        pd_error(NULL, "Error: PdDict is NULL");
        Py_DECREF(pd_module);
        Py_DECREF(py4pd_capsule);
        return NULL;
    }
    
    PyObject *PY_objectClass = PyDict_GetItemString(PdDict, "py4pdOBJ_CLASS");
    if (PY_objectClass == NULL) {
        pd_error(NULL, "Error: object Class is NULL");
        Py_DECREF(pd_module);
        Py_DECREF(py4pd_capsule);
        return NULL;
    }

    PyObject *pyFunction = PyDict_GetItemString(PdDict, "py4pdOBJFunction");
    PyObject *ignoreOnNone = PyDict_GetItemString(PdDict, "py4pdOBJIgnoreNone");
    PyObject *playable = PyDict_GetItemString(PdDict, "py4pdOBJPlayable");
    PyObject *pyOUT = PyDict_GetItemString(PdDict, "py4pdOBJpyout");
    PyObject *nooutlet = PyDict_GetItemString(PdDict, "py4pdOBJnooutlet");
    PyObject *AuxOutletPy = PyDict_GetItemString(PdDict, "py4pdAuxOutlets");
    PyObject *RequireUserToSetOutletNumbers = PyDict_GetItemString(PdDict, "py4pdOBJrequireoutletn");
    PyObject *Py_ObjType = PyDict_GetItemString(PdDict, "py4pdOBJType");

    int AuxOutlet = PyLong_AsLong(AuxOutletPy);
    int requireNofOutlets = PyLong_AsLong(RequireUserToSetOutletNumbers);
    PyCodeObject *code = (PyCodeObject*)PyFunction_GetCode(pyFunction);
    t_class *object_PY4PD_Class = (t_class *)PyLong_AsVoidPtr(PY_objectClass);
    t_py *x = (t_py *)pd_new(object_PY4PD_Class);

    x->visMode  = 0;
    x->pyObject = 1;
    x->objArgsCount = argc; // NOTE: NEW
    x->stackLimit = 100; // NOTE: NEW
    x->canvas = canvas_getcurrent();       
    t_canvas *c = x->canvas;
    t_symbol *patch_dir = canvas_getdir(c);  
    x->objectName = gensym(objectName);
    x->ignoreOnNone = PyLong_AsLong(ignoreOnNone);
    x->outPyPointer = PyLong_AsLong(pyOUT);
    x->function_called = 1;
    x->function = pyFunction;
    x->pdPatchFolder = patch_dir;         // set name of the home path
    x->pkgPath = patch_dir;     // set name of the packages path
    x->playable = PyLong_AsLong(playable);
    x->function_name = gensym(PyUnicode_AsUTF8(code->co_name));
    x->script_name = gensym(PyUnicode_AsUTF8(code->co_filename));
    x->objType = PyLong_AsLong(Py_ObjType);
    Py4pdUtils_SetObjConfig(x);  

    if (x->objType == PY4PD_VISOBJ)
        Py4pdUtils_CreatePicObj(x, PdDict, object_PY4PD_Class, argc, argv); 

    // Args, Inlets, and Outlets
    x->py_arg_numbers = 0;
    int parseArgsRight = Py4pdUtils_ParseLibraryArguments(x, code, &argc, &argv); 
    if (parseArgsRight == 0) {
        pd_error(NULL, "[%s] Error to parse arguments.", objectName);
        Py_DECREF(pd_module);
        Py_DECREF(py4pd_capsule);
        return NULL;
    }
    x->pdObjArgs = malloc(sizeof(t_atom) * argc); // TODO: FREE
    for (int i = 0; i < argc; i++) {
        x->pdObjArgs[i] = argv[i]; // TODO: FREE
    }
    x->objArgsCount = argc;
    if (x->pyObjArgs == NULL){
        x->pyObjArgs = malloc(sizeof(t_py4pd_pValue *) * x->py_arg_numbers);
    }
    int inlets = Py4pdLib_CreateObjInlets(pyFunction, x, argc, argv);
    if (inlets != 0) {
        free(x->pdObjArgs);
        free(x->pyObjArgs);
        return NULL;
    }

    
    if (x->objType > 2){
        x->argsDict = PyTuple_New(x->py_arg_numbers);
    }

    if (!PyLong_AsLong(nooutlet)){
        if (x->objType != PY4PD_AUDIOOUTOBJ && x->objType != PY4PD_AUDIOOBJ)
            x->out1 = outlet_new(&x->obj, 0);
        else{
            x->out1 = outlet_new(&x->obj, &s_signal);
            x->x_numOutlets = 1;
        }
    }
    if (requireNofOutlets){
        if (x->x_numOutlets == -1){
            pd_error(NULL, "[%s]: This function require that you set the number of outlets "
                     "using -outn {number_of_outlets} flag", objectName);
            Py_DECREF(pd_module);
            Py_DECREF(py4pd_capsule);
            return NULL;
        }
        else
            AuxOutlet = x->x_numOutlets;
    }
    x->outAUX = (t_py4pd_Outlets *)getbytes(AuxOutlet * sizeof(*x->outAUX));
    x->outAUX->u_outletNumber = AuxOutlet;
    t_atom defarg[AuxOutlet], *ap;
    t_py4pd_Outlets *u;
    int i;

    if (x->objType > 1){
        int numpyArrayImported = _import_array();
        if (numpyArrayImported == 0) {
            x->numpyImported = 1;
        }
        else{
            x->numpyImported = 0;
            pd_error(x, "[%s] Not possible to import numpy array", objectName);
            Py_DECREF(pd_module);
            Py_DECREF(py4pd_capsule);
            return NULL;
        }
    }

    for (i = 0, u = x->outAUX, ap = defarg; i < AuxOutlet; i++, u++, ap++) {
        u->u_outlet = outlet_new(&x->obj, &s_anything);
    }
    object_count++;

    Py_DECREF(pd_module);
    Py_DECREF(py4pd_capsule);

    return (x);
}

// =====================================
/**
 * @brief add new Python Object to PureData
 * @param x
 * @param argc
 * @param argv
 * @return
 */
PyObject *Py4pdLib_AddObj(PyObject *self, PyObject *args, PyObject *keywords) {
    (void)self;
    char *objectName;
    const char *helpPatch;
    PyObject *Function; // *showFunction;
    int w = 250, h = 250;
    int objpyout = 0;
    int nooutlet = 0;
    int added2pd_info = 0;
    int personalisedHelp = 0;
    int ignoreNoneReturn = 0;
    const char *gifImage = NULL;
    int auxOutlets = 0;
    int require_outlet_n = 0;
    int playableInt = 0;

    // get file folder where this function is called from self
    t_py *py4pd = Py4pdUtils_GetObject();

    if (py4pd->libraryFolder == NULL) {
        pd_error(py4pd, "[py4pd] Library Folder is NULL, some help patches may not be found");
    } 

    const char *helpFolder = "/help/";

    size_t totalLength = strlen(py4pd->libraryFolder->s_name) + strlen(helpFolder) + 1;
    char *helpFolderCHAR = (char *)malloc(totalLength * sizeof(char));
    if (helpFolderCHAR == NULL) {
        pd_error(py4pd, "[py4pd] Error allocating memory (code 001)"); 
        return NULL;
    }
    strcpy(helpFolderCHAR, py4pd->libraryFolder->s_name);
    strcat(helpFolderCHAR, helpFolder);
    
    if (!PyArg_ParseTuple(args, "Os", &Function, &objectName)) { 
        post("[Python]: Error parsing arguments");
        return NULL;
    }
    int objectType = 0;
    if (keywords != NULL) {
        if (PyDict_Contains(keywords, PyUnicode_FromString("objtype"))) {
            PyObject *type = PyDict_GetItemString(keywords, "objtype");
            objectType = PyLong_AsLong(type);
        }
        if(PyDict_Contains(keywords, PyUnicode_FromString("figsize"))) {
            PyObject *figsize = PyDict_GetItemString(keywords, "figsize"); // this is defined in python using figsize=(w,h)
            PyObject *width = PyTuple_GetItem(figsize, 0);
            PyObject *height = PyTuple_GetItem(figsize, 1);
            w = PyLong_AsLong(width);
            h = PyLong_AsLong(height);
        }
        if (PyDict_Contains(keywords, PyUnicode_FromString("pyout"))) {
            PyObject *output = PyDict_GetItemString(keywords, "pyout"); // it gets the data type output
            if (output == Py_True) {
                objpyout = 1;
            }
        }
        if (PyDict_Contains(keywords, PyUnicode_FromString("no_outlet"))) {
            PyObject *output = PyDict_GetItemString(keywords, "no_outlet"); // it gets the data type output
            if (output == Py_True) {
                nooutlet = 1;
            }
        }
        if (PyDict_Contains(keywords, PyUnicode_FromString("require_outlet_n"))) {
            PyObject *output = PyDict_GetItemString(keywords, "require_outlet_n"); // it gets the data type output
            if (output == Py_True) {
                require_outlet_n = 1;
            }
        }
        if (PyDict_Contains(keywords, PyUnicode_FromString("added2pd_info"))) {
            PyObject *output = PyDict_GetItemString(keywords, "added2pd_info"); // it gets the data type output
            if (output == Py_True) {
                added2pd_info = 1;
            }
        }
        if (PyDict_Contains(keywords, PyUnicode_FromString("helppatch"))) {
            PyObject *helpname = PyDict_GetItemString(keywords, "helppatch"); // it gets the data type output
            helpPatch = PyUnicode_AsUTF8(helpname);
            personalisedHelp = 1;
        }
        if (PyDict_Contains(keywords, PyUnicode_FromString("ignore_none_return"))) {
            PyObject *noneReturn = PyDict_GetItemString(keywords, "ignore_none_return"); // it gets the data type output
            if (noneReturn == Py_True) {
                ignoreNoneReturn = 1;
            }
        }
        if (PyDict_Contains(keywords, PyUnicode_FromString("objimage"))) {
            PyObject *type = PyDict_GetItemString(keywords, "objimage");
            gifImage = PyUnicode_AsUTF8(type);
        }
        if (PyDict_Contains(keywords, PyUnicode_FromString("num_aux_outlets"))) {
            PyObject *type = PyDict_GetItemString(keywords, "num_aux_outlets");
            auxOutlets = PyLong_AsLong(type);
        }
        if (PyDict_Contains(keywords, PyUnicode_FromString("playable"))) {
            PyObject *playable = PyDict_GetItemString(keywords, "playable");
            if (playable == Py_True) {
                playableInt = 1;
            }
        }
    }

    class_set_extern_dir(gensym(helpFolderCHAR));
    t_class *localClass;

    if (objectType == PY4PD_NORMALOBJ) {
        localClass = class_new(gensym(objectName), (t_newmethod)Py4pdLib_NewObj,  
                            (t_method)Py4pdLib_FreeObj, sizeof(t_py), CLASS_DEFAULT, A_GIMME, 0);
    }
    else if (objectType == PY4PD_VISOBJ) {
        localClass = class_new(gensym(objectName), (t_newmethod)Py4pdLib_NewObj, 
                            (t_method)Py4pdLib_FreeObj, sizeof(t_py), CLASS_DEFAULT, A_GIMME, 0);
    }
    else if (objectType == PY4PD_AUDIOINOBJ) {
        localClass = class_new(gensym(objectName), (t_newmethod)Py4pdLib_NewObj, 
                            (t_method)Py4pdLib_FreeObj, sizeof(t_py), CLASS_MULTICHANNEL, A_GIMME, 0);
    }
    else if (objectType == PY4PD_AUDIOOUTOBJ) {
        localClass = class_new(gensym(objectName), (t_newmethod)Py4pdLib_NewObj, 
                            (t_method)Py4pdLib_FreeObj, sizeof(t_py), CLASS_MULTICHANNEL, A_GIMME, 0);
    }
    else if (objectType == PY4PD_AUDIOOBJ) {
        localClass = class_new(gensym(objectName), (t_newmethod)Py4pdLib_NewObj, 
                            (t_method)Py4pdLib_FreeObj, sizeof(t_py), CLASS_MULTICHANNEL, A_GIMME, 0);
    }
    else{
        PyErr_SetString(PyExc_TypeError, "Object type not supported, check the spelling");
        return NULL;
    }

    // Add configs to the object
    PyObject *nestedDict = PyDict_New(); // New

    PyDict_SetItemString(nestedDict, "py4pdOBJFunction", Function);


    PyObject* Py_ObjType = PyLong_FromLong(objectType);
    PyDict_SetItemString(nestedDict, "py4pdOBJType", Py_ObjType);
    Py_DECREF(Py_ObjType);

    PyObject* Py_LibraryFolder = PyUnicode_FromString(py4pd->libraryFolder->s_name);
    PyDict_SetItemString(nestedDict, "py4pdOBJLibraryFolder", Py_LibraryFolder);
    Py_DECREF(Py_LibraryFolder);

    PyObject* Py_ClassLocal = PyLong_FromVoidPtr(localClass);
    PyDict_SetItemString(nestedDict, "py4pdOBJ_CLASS", Py_ClassLocal);
    Py_DECREF(Py_ClassLocal);

    PyObject* Py_Width = PyLong_FromLong(w);
    PyDict_SetItemString(nestedDict, "py4pdOBJwidth", Py_Width);
    Py_DECREF(Py_Width);

    PyObject* Py_Height = PyLong_FromLong(h);
    PyDict_SetItemString(nestedDict, "py4pdOBJheight", Py_Height);
    Py_DECREF(Py_Height);

    PyObject* Py_Playable = PyLong_FromLong(playableInt);
    PyDict_SetItemString(nestedDict, "py4pdOBJPlayable", Py_Playable);
    Py_DECREF(Py_Playable);
    
    if (gifImage != NULL) {
        PyObject* Py_GifImage = PyUnicode_FromString(gifImage);
        PyDict_SetItemString(nestedDict, "py4pdOBJGif", Py_GifImage);
        Py_DECREF(Py_GifImage);
    }

    PyObject* Py_ObjOuts = PyLong_FromLong(objpyout);
    PyDict_SetItemString(nestedDict, "py4pdOBJpyout", PyLong_FromLong(objpyout));
    Py_DECREF(Py_ObjOuts);

    PyObject* Py_NoOutlet = PyLong_FromLong(nooutlet);
    PyDict_SetItemString(nestedDict, "py4pdOBJnooutlet", Py_NoOutlet);
    Py_DECREF(Py_NoOutlet);

    PyObject* Py_RequireOutletN = PyLong_FromLong(require_outlet_n);
    PyDict_SetItemString(nestedDict, "py4pdOBJrequireoutletn", Py_RequireOutletN);
    Py_DECREF(Py_RequireOutletN);

    PyObject* Py_auxOutlets = PyLong_FromLong(auxOutlets);
    PyDict_SetItemString(nestedDict, "py4pdAuxOutlets", Py_auxOutlets);
    Py_DECREF(Py_auxOutlets);

    PyObject* Py_ObjName = PyUnicode_FromString(objectName);
    PyDict_SetItemString(nestedDict, "py4pdOBJname", Py_ObjName);
    Py_DECREF(Py_ObjName);

    PyObject* Py_IgnoreNoneReturn = PyLong_FromLong(ignoreNoneReturn);
    PyDict_SetItemString(nestedDict, "py4pdOBJIgnoreNone", Py_IgnoreNoneReturn);
    Py_DECREF(Py_IgnoreNoneReturn);


    PyObject *objectDict = PyDict_New();
    PyDict_SetItemString(objectDict, objectName, nestedDict);
    PyObject *py4pd_capsule = PyCapsule_New(objectDict, objectName, NULL);
    char py4pd_objectName[MAXPDSTRING];
    sprintf(py4pd_objectName, "py4pd_ObjectDict_%s", objectName);
    
    PyObject* pdModule = PyImport_ImportModule("pd");
    PyModule_AddObject(pdModule, py4pd_objectName, py4pd_capsule);
    
    Py_DECREF(pdModule);


    // =====================================
    PyCodeObject* code = (PyCodeObject*)PyFunction_GetCode(Function);
    int py_args = code->co_argcount;

    // special methods
    if (objectType == PY4PD_NORMALOBJ){
        class_addmethod(localClass, (t_method)Py4pdLib_Click, gensym("click"), 0, 0);
        if (playableInt == 1){
            class_addmethod(localClass, (t_method)Py4pdLib_Play, gensym("play"), A_GIMME, 0);
            class_addmethod(localClass, (t_method)Py4pdLib_Stop, gensym("stop"), 0, 0);
            class_addmethod(localClass, (t_method)Py4pdLib_Clear, gensym("clear"), 0, 0);
        }
    }
    else if (objectType == PY4PD_VISOBJ){
        if (playableInt == 1){
            class_addmethod(localClass, (t_method)Py4pdLib_Play, gensym("play"), A_GIMME, 0);
            class_addmethod(localClass, (t_method)Py4pdLib_Stop, gensym("stop"), 0, 0); 
            class_addmethod(localClass, (t_method)Py4pdLib_Clear, gensym("clear"), 0, 0);
        }
        class_addmethod(localClass, (t_method)Py4pdPic_Zoom, gensym("zoom"), A_CANT, 0);
        class_addmethod(localClass, (t_method)Py4pd_SetPythonPointersUsage, gensym("pointers"), A_FLOAT, 0);
        class_setsavefn(localClass, &Py4pdLib_Py4pdObjPicSave);
    }
    // AUDIOIN
    else if (objectType == PY4PD_AUDIOINOBJ){
        class_addmethod(localClass, (t_method)Py4pdLib_Click, gensym("click"), 0, 0);
        class_addmethod(localClass, (t_method)Py4pdLib_Dsp, gensym("dsp"), A_CANT, 0);  // add a method to a class
        CLASS_MAINSIGNALIN(localClass, t_py, py4pdAudio);
    }
    // AUDIOOUT
    else if (objectType == PY4PD_AUDIOOUTOBJ){
        class_addmethod(localClass, (t_method)Py4pdLib_Click, gensym("click"), 0, 0);
        class_addmethod(localClass, (t_method)Py4pdLib_Dsp, gensym("dsp"), A_CANT, 0);  // add a method to a class

    }
    // AUDIO
    else if (objectType == PY4PD_AUDIOOBJ){
        class_addmethod(localClass, (t_method)Py4pdLib_Click, gensym("click"), 0, 0);
        class_addmethod(localClass, (t_method)Py4pdLib_Dsp, gensym("dsp"), A_CANT, 0);  // add a method to a class
        CLASS_MAINSIGNALIN(localClass, t_py, py4pdAudio);
    }
    else{
        PyErr_SetString(PyExc_TypeError, "Object type not supported, check the spelling");
        return NULL;
    }
    // add methods to the class
    class_addanything(localClass, Py4pdLib_Anything);
    class_addmethod(localClass, (t_method)Py4pdLib_Pointer, gensym("PyObject"), A_SYMBOL, A_POINTER, 0);
    class_addmethod(localClass, (t_method)Py4pd_PrintDocs, gensym("doc"), 0, 0);
    class_addmethod(localClass, (t_method)Py4pdLib_SetKwargs, gensym("kwargs"), A_GIMME, 0);
    class_addmethod(localClass, (t_method)Py4pd_SetPythonPointersUsage, gensym("pointers"), A_FLOAT, 0);
    class_addmethod(localClass, (t_method)Py4pdLib_ReloadObject, gensym("reload"), 0, 0);
    
    // add help patch
    if (personalisedHelp == 1){
        class_sethelpsymbol(localClass, gensym(helpPatch));
    }
    else{
        class_sethelpsymbol(localClass, gensym(objectName));
    }
    free(helpFolderCHAR);

    if (py_args != 0){
        py4pdInlets_proxy_class = class_new(gensym("_py4pdInlets_proxy"), 0, 0, sizeof(t_py4pdInlet_proxy), CLASS_DEFAULT, 0);
        class_addanything(py4pdInlets_proxy_class, Py4pdLib_ProxyAnything);
        class_addmethod(py4pdInlets_proxy_class, (t_method)Py4pdLib_ProxyPointer, gensym("PyObject"), A_SYMBOL, A_POINTER, 0);
    }
    if (added2pd_info == 1){
        post("[py4pd]: Object {%s} added to PureData", objectName);
    }
    class_set_extern_dir(&s_);
    Py_RETURN_TRUE;
}

