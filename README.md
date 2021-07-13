# Lab: Parallel grep
 [Following archive](https://drive.google.com/file/d/1SjA1pEz_H6OBQq4jmOnTnsKqsW0V1WDE/view) was used

Specifications of PC used (Danyil Hutsul):
16Gb RAM, 4 Cores, 2.6 GHz per Core

### Results: 
The time doesn't change much in correlation to rarity of string chosen. Number of lines only affects the speed of writing the output file.

[Result Google Drive link](https://drive.google.com/drive/folders/1CBiDOnEO_dS9i6WU9HhQWAXH3_kgXrda?usp=sharing)

### Time:

Total time. Word searched `Jim`: 14727.53

Total time. Word searched `teapot`: 14631.33s

##

 - [Danyil Hutsul](https://github.com/DanHutsul)

## Usage
1. Fill `config.dat` by example (it already has some parameters' values). All lines must be filled
2. Run script (Searched string is mandatory. If number_of_executions is not given it is 1 by default)
```bash python script.py *number_of_executions*```
3. After the script has finished tunning, run build_plot.py in the same way

`config.dat` contains the program configuration

`py_script` folder contains python scripts

`script.py` is used to execute the program for string `s`, `n` number of times (Default value 1)

`build_plot.py` creates a graphical representation of data contained in `../results/thread_time_dependency.txt`

## Prerequisites

 - **C++ compiler** - needs to support **C++17** standard
 - **CMake** 3.15+
 - **Python** 3.7+
   - matplotlib library
 For Windows:
 - **WSL** - needs to support Python, C++ compiler, cmake, make
