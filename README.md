

# py4pd 
#### [Download](https://github.com/charlesneimog/py4pd/releases)

py4pd allows the use of Python inside PureData. It has the same objective as py/pyext being much simpler to keep compile, mainly for Windows OS. It was, in first place, a trying to compile py/pyext for Windows OS, but I couldn't. Then I did the object. It is based in samples of code from Internet and how py-pyext works, mainly in the fork of SOPI research group.
1. [How to call Python from C](https://stackoverflow.com/questions/1056051/how-do-you-call-python-code-from-c-code);
2. [C api from Python Docs](https://docs.python.org/3/extending/embedding.html);


### Documentation

**[ !!! ]** On MacOS and Linux, for now, you need to install Python 3.10.

#### Using Python Modules

The py4pd is built to work with site packages from `venv` (on Windows venv/Lib/site-packages), but it will work with conda too. For example, if my environment is installed in `C:/Users/Neimog/Git/py4pd/py4pd/`, you can send a message `packages  C:/Users/Neimog/Git/py4pd/py4pd/Lib/site-packages` that you will be able to use all modules installed in the environment.

#### Run 

You need to define functions in the `.py` file to use PureData. For example, let's say that I want to build a Diamond Tonality from Harry Partch. Then I define the `otonal_diamond` function in the `partch.py` script inside the folder where my Pd Patch is saved. 

``` Python
def otonal_diamond(limit, diagonal, fundamental):
    otonal = []
    for o in range(1, limit, 2):
        tonality = []
        for u in range (1, limit, 2):
            tonality.append(fundamental * (u / o))
        otonal.append(tonality)
        tonality = [] 
    return otonal[diagonal]
```

Then a sent the message `set {script_without_.py} {function_name}`, in this case, `set partch otonal_diamond` and run it using `run 13 3 440`. That means that the object will return a Diamond limit of 13, the third Diagonal (in this case, not the same that the figures), with the A4 how fundamental.

<img src="https://user-images.githubusercontent.com/31707161/200134314-fb994d6a-dfa7-4b19-a578-1966a54adf5f.png" width=40% height=40%> 

### Building

For now, I just am using it for Windows OS. I think that py/pyext works well in Linux and Mac/OS. To compile for Windows, you need msys2. 
Then, in mingw64 terminal:

#### Windows OS - replace `<username>` for your username.

``` bash 
pacman -S make autoconf automake libtool mingw-w64-x86_64-libwinpthread-git mingw64/mingw-w64-x86_64-gcc
make PYTHON_INCLUDE="C:/Users/<username>/AppData/Local/Programs/Python/Python310/include" PYTHON_DLL="C:/Users/<username>/AppData/Local/Programs/Python/Python310/python310.dll"
```
OBS.: **Important**, Do not use the `Python310.dll` of `miniconda`, `conda` or other to compile `py4pd`. With the 'original' dll it is possible to replace where the `python310.dll` look for dynamics libraries.

-----------------
If you want to build it on Linux:

#### Linux - version 3.10
* First you need to run: 
    `sudo add-apt-repository ppa:deadsnakes/ppa`.
    `sudo apt-get install python3.10-dev`.

``` bash 
make PYTHON_INCLUDE=/usr/include/python3.10/ PYTHON_VERSION=python3.10 
```

#### MacOS - version 3.10
* First you need to install Python 3.10 (https://www.python.org/downloads/release/python-3108/) then run:

``` bash 
sudo ln -s /Library/Frameworks/Python.framework/Versions/3.10/lib/libpython3.10.dylib /usr/local/lib/libpython3.10.dylib
make PYTHON_INCLUDE=/Library/Frameworks/Python.framework/Versions/3.10/include/python3.10 PYTHON_VERSION=python3.10
```



