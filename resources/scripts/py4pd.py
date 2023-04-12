import pd
from py4pd_scripts.py4pdlist import *
from py4pd_scripts.py4pdmath import *
from py4pd_scripts.py4pdpip import *
from py4pd_scripts.py4pdconvertion import *
from py4pd_scripts.py4pdloop import *
from py4pd_scripts.py4pdinfo import *
from py4pd_scripts.py4pdscore import *


def py4pdLoadObjects():
    # Pip install
    if addpip:
        pd.addobject(pipinstall, "py.pip")

    # info
    pd.addobject(pdprint, "py.print", no_outlet=True)
    
    # Convertion Objects
    pd.addobject(py2pd, "py2pd")
    pd.addobject(pd2py, "pd2py", pyout=True)

    # List Functions
    pd.addobject(pylen, "py.len")
    pd.addobject(nth, "py.nth", pyout=True)
    pd.addobject(omlist, "py.list", pyout=True)
    pd.addobject(pymax, "py.max")
    pd.addobject(pymin, "py.min")
    pd.addobject(pyreduce, "py.reduce", pyout=True)
    pd.addobject(mat_trans, "py.mattrans", pyout=True)

    # Loop Functions
    pd.addobject(pyiterate, "py.iterate") # these are special objects, they don't have a pyout argument but output py data types
    pd.addobject(pycollect, "py.collect") # these are special objects, they don't have a pyout argument but output py data types

    # Math Functions
    pd.addobject(omsum, "py.sum")
    pd.addobject(omminus, "py.minus")
    pd.addobject(omtimes, "py.times")
    pd.addobject(omdiv, "py.div")
    pd.addobject(omabs, "py.abs")

    # Score Functions
    pd.addobject(note, "py.note", objtype="VIS")
    pd.addobject(chord, "py.chord", objtype="VIS")
    

