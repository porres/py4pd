// clang-format off
#ifndef PY4PD_H
#define PY4PD_H

#include <m_pd.h>
#include <g_canvas.h>
#include <s_stuff.h> // get the search paths
#include <pthread.h>
#include <dirent.h>

#define PY_SSIZE_T_CLEAN // Remove deprecated functions
#include <Python.h>

#ifdef __linux__
    #define __USE_GNU
#endif

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_RESET "\x1b[0m"

#define PY4PD_DEBUG 0


#define PY4PD_NORMALOBJ 0
#define PY4PD_VISOBJ 1
#define PY4PD_AUDIOINOBJ 2
#define PY4PD_AUDIOOUTOBJ 3
#define PY4PD_AUDIOOBJ 4


#define PY4PD_MAJOR_VERSION 0
#define PY4PD_MINOR_VERSION 8
#define PY4PD_MICRO_VERSION 2


#define PYTHON_REQUIRED_VERSION(major, minor) ((major < PY_MAJOR_VERSION) || (major == PY_MAJOR_VERSION && minor <= PY_MINOR_VERSION))

#ifdef _WIN32
    #include <windows.h>
    #include <limits.h>
#endif

// DEFINE STANDARD IDE EDITOR
#ifndef PY4PD_EDITOR
    #ifdef _WIN32
        #define PY4PD_EDITOR "idle3.11"
    #else
        #define PY4PD_EDITOR "idle3.11"
    #endif
#endif

// ====================================
// ============== OUTLETS =============
// ====================================
/*
 * @brief Structure representing an array of all the auxiliar outlets of py4pd.
 */
typedef struct _py4pdOuts{
    t_atomtype u_type;
    t_outlet *u_outlet;
    int u_outletNumber;
} py4pdOuts;

// ====================================
// =============== PLAYER =============
// ====================================
/*
 * @brief Structure representing the values that are stored in the dictionary to be played for the player.
 */
typedef struct {
    int onset;
    int size;
    PyObject **values;
} KeyValuePair;

// ====================================
typedef struct {
    KeyValuePair* entries;
    int size;
    int lastOnset;
    int isSorted;
} Dictionary;

// ====================================
// ========== pValues Objs ============
// ====================================

typedef struct _py4pd_pValue{ 
    PyObject* pValue;
    int isPvalue;
    int objectsUsing; 
    t_symbol *objOwner;
    int clearAfterUse;
    int alreadyIncref;
    int pdout;
}t_py4pd_pValue;

// ======================================
// ======================================
// ======================================
typedef struct pdcollectItem{
    char*         key;
    PyObject*     pList;
    PyObject*     pItem;
    int           wasCleaned;
    int           aCumulative;
} pdcollectItem;

// ======================================
typedef struct pdcollectHash{
    pdcollectItem** items;
    int size;
    int count;
} pdcollectHash;

// ====================================
// ========== VIS OBJECTS =============
// ====================================
typedef struct _py4pd_edit_proxy{ 
    t_object    p_obj;
    t_symbol   *p_sym;
    t_clock    *p_clock;
    struct      _py *p_cnv;
}t_py4pd_edit_proxy;

// ====================================
// ============== PY4PD ===============
// ====================================

typedef struct _py { // It seems that all the objects are some kind of class.
    t_object            obj; // o objeto
    t_glist             *glist;
    t_canvas            *canvas; // pointer to the canvas
    t_outlet            *mainOut; // outlet 1.
    t_inlet             *mainIn; // intlet 1

    // TESTING THINGS
    t_py4pd_pValue      **pyObjArgs; // Obj Variables Arguments 
    pdcollectHash       *pdcollect; // hash table to store the objects
    t_int               recursiveCalls; // check the number of recursive calls
    t_int               stackLimit; // check the number of recursive calls
    t_clock             *recursiveClock; // clock to check the number of recursive calls 
    PyObject            *recursiveObject; // object to check the number of recursive calls
    t_atom              *pdObjArgs;
    t_int               objArgsCount;
     
    // ===========
    t_int                visMode; // is vis object
    t_int                funcCalled; // flag to check if the set function was called
    t_int                pArgsCount; // number of arguments
    t_int                outPyPointer; // flag to check if is to output the python pointer
    t_int                use_pArgs;
    t_int                use_pKwargs;
    
    // Player 
    t_clock              *playerClock;
    Dictionary           *playerDict;
    t_int                 playerRunning;
    t_int                 msOnset;
    t_int                 playable;

    // Library
    t_int                isLib; // flag to check if is to use python library
    t_int                pyObject;
    t_int                objType;
    t_int                ignoreOnNone;
    PyObject             *argsDict; // parameters
    t_symbol             *objName; // object name

    // == PYTHON
    PyObject            *pModule; // script name
    PyObject            *pFunction; // function name
    // PyObject            *showFunction; // TODO: FUTURE function to show the function
    PyObject            *kwargsDict; // arguments
    PyObject            *pObjVarDict;
    PyObject            *pArgTuple;
    t_symbol            *pFuncName; // function_name; // function name
    t_symbol            *pScriptName; // script name or pathname

    // == AUDIO AND NUMPY
    t_int               audioOutput; // flag to check if is to use audio output
    t_int               audioInput; // flag to check if is to use audio input
    t_int               useNumpyArray; // flag to check if is to use numpy array in audioInput
    t_int               numpyImported; // flag to check if numpy was imported
    t_float             py4pdAudio; // audio to fe used in CLASSMAINSIGIN
    t_int               vectorSize; // vector size
    t_int               nChs; // number of channels
    t_int               audioError; // To avoid multiple error messages

    // Pic Object
    t_int               zoom; // zoom of the patch
    t_int               width; // width of image
    t_int               height; // height of image
    t_int               edit; // patch is in edit mode or not
    // t_int               picWasInit; // flag to check if the object was initialized
    t_int               defImg; // flag to check if the object was initialized
    t_int               numInlets; // number of inlets
    t_int               numOutlets; // number of outlets
    t_int               mouseIsOver; // flag to check if the mouse is over the object
    t_symbol            *picFilePath;
    t_symbol            *canvasName; // name of the tcl variable for x
    char                *imageBase64; // image data in base64
    t_int               drawIOlets; // flag to check if the inlets and outlets were created

    // Paths
    t_symbol            *pkgPath; // packages path, where the packages are located
    t_symbol            *pdPatchPath; // where the patch is located
    t_symbol            *py4pdPath; // where py4pd object is located
    t_symbol            *tempPath; // temp path located in ~/.py4pd/, always is deleted when py4pd is closed
    t_symbol            *libraryFolder; // where the library is located

    // script_name; // script name or pathname
    t_symbol            *editorName; // editor name
    py4pdOuts           *outAUX; // outlets
    t_py4pd_edit_proxy  *x_proxy; // para lidar com inlets auxiliares

}t_py;

// =====================================
// =========== LIBRARY OBJECT ==========
// =====================================
typedef struct _py4pdInlet_proxy{
    t_object     p_ob;
    t_py        *p_master;
    int          inletIndex;
}t_py4pdInlet_proxy;

// =====================================
t_py4pd_pValue *Py4pdUtils_Run(t_py *x, PyObject *pArgs, t_py4pd_pValue *pValuePointer);
void *Py4pd_ImportNumpyForPy4pd();

// =====================================
// void Py4pd_SetParametersForFunction(t_py *x, t_symbol *s, int argc, t_atom *argv);
void Py4pd_PrintDocs(t_py *x);
void Py4pd_SetPythonPointersUsage(t_py *x, t_floatarg f);

#define PY4PD_IMAGE "R0lGODlhKgAhAPAAAP///wAAACH5BAAAAAAAIf8LSW1hZ2VNYWdpY2sOZ2FtbWE9MC40NTQ1NDUALAAAAAAqACEAAAIkhI+py+0Po5y02ouz3rz7D4biSJbmiabqyrbuC8fyTNf2jTMFADs="
#define PY4PDSIGTOTAL(s) ((t_int)((s)->s_length * (s)->s_nchans))

extern int pipePy4pdNum;
extern int object_count; 

// ============= TESTES ================
void Py4pdUtils_DECREF(PyObject *pValue);
void Py4pdUtils_MemLeakCheck(PyObject *pValue, int refcnt, char *where);
void Py4pdUtils_CopyPy4pdValueStruct(t_py4pd_pValue* src, t_py4pd_pValue* dest);
void Py4pdMod_FreePdcollectHash(pdcollectHash* hash_table);
void Py4pdUtils_CreatePicObj(t_py *x, PyObject* PdDict, t_class *object_PY4PD_Class, int argc, t_atom *argv);



#if PYTHON_REQUIRED_VERSION(3, 12)
    void Py4pdUtils_CreatePythonInterpreter(t_py* x);
#endif

// ============= UTILITIES =============
// int Py4pdUtils_ParseLibraryArguments(t_py *x, PyCodeObject *code, int argc, t_atom *argv);
int Py4pdUtils_ParseLibraryArguments(t_py *x, PyCodeObject *code, int *argc, t_atom **argv);
t_py *Py4pdUtils_GetObject(PyObject *pd_module);
void Py4pdUtils_ParseArguments(t_py *x, t_canvas *c, int argc, t_atom *argv);
char* Py4pdUtils_GetFolderName(char* path);
const char* Py4pdUtils_GetFilename(const char* path);
void Py4pdUtils_CheckPkgNameConflict(t_py *x, char *folderToCheck, t_symbol *script_file_name);
void Py4pdUtils_FindObjFolder(t_py *x);
void Py4pdUtils_CreateTempFolder(t_py *x);
void Py4pdUtils_GetEditorCommand(t_py *x, char *command, int line);
void Py4pdUtils_ExecuteSystemCommand(const char *command);
int Py4pdUtils_IsNumericOrDot(const char *str);
void Py4pdUtils_RemoveChar(char *str, char c);
char *Py4pdUtils_Mtok(char *input, char *delimiter);
void Py4pdUtils_FromSymbolSymbol(t_py *x, t_symbol *s, t_outlet *outlet);
// PyObject *Py4pdUtils_PyObjectToPointer(PyObject *pValue);
// PyObject *Py4pdUtils_PointerToPyObject(PyObject *p);
PyObject *Py4pdUtils_RunPy(t_py *x, PyObject *pArgs, PyObject* pDict);
PyObject *Py4pdUtils_RunPyAudioOut(t_py *x, PyObject *pArgs, PyObject *pKwargs);

void *Py4pdUtils_ConvertToPd(t_py *x, t_py4pd_pValue *pValue, t_outlet *outlet);
PyObject *Py4pdUtils_ConvertToPy(PyObject *listsArrays[], int argc, t_atom *argv);
void Py4pdUtils_SetObjConfig(t_py *x);
void Py4pdUtils_AddPathsToPythonPath(t_py *x);
PyObject *Py4pdUtils_AddPdObject(t_py *x);
void Py4pdUtils_ReadGifFile(t_py *x, const char* filename);
void Py4pdUtils_ReadPngFile(t_py *x, const char* filename);
uint32_t Py4pdUtils_Ntohl(uint32_t netlong);
void *Py4pdLib_FreeObj(t_py *x);

// ============= EMBEDDED MODULE =======
extern PyMethodDef PdMethods[];
PyMODINIT_FUNC PyInit_pd(void);

// ============= PLAYER ==========
void Py4pdLib_PlayerInsertThing(t_py *x, int onset, PyObject *value);
KeyValuePair* Py4pdLib_PlayerGetValue(Dictionary* dictionary, int onset);
void Py4pdLib_Play(t_py *x, t_symbol *s, int argc, t_atom *argv);
void Py4pdLib_Stop(t_py *x);
void Py4pdLib_Clear(t_py *x);

// ============= PIC =============
extern t_class *py4pd_class, *pyNewObject_VIS;
extern void Py4pdPic_Free(t_py *x);
extern void Py4pdPic_Zoom(t_py *x, t_floatarg f);
extern void Py4pdPic_InitVisMode(t_py *x, t_canvas *c, t_symbol *py4pdArgs, int index, int argc, t_atom *argv, t_class *obj_class);
extern void Py4pdPic_Erase(t_py* x, struct _glist *glist); 
const char *Py4pdPic_Filepath(t_py *x, const char *filename);
extern void Py4pdPic_Draw(t_py* x, struct _glist *glist, t_floatarg vis);
extern void Py4pdPic_GetRect(t_gobj *z, t_glist *glist, int *xp1, int *yp1, int *xp2, int *yp2);
extern void Py4pdPic_Displace(t_gobj *z, t_glist *glist, int dx, int dy);
extern void Py4pdPic_Select(t_gobj *z, t_glist *glist, int state);
extern void Py4pdPic_Delete(t_gobj *z, t_glist *glist);

// ============= EXTERNAL LIBRARIES =============
extern PyObject *Py4pdLib_AddObj(PyObject *self, PyObject *args, PyObject *keywords);

// ============= PY4PD =============
extern PyTypeObject Py4pdNewObj_Type;

#endif
