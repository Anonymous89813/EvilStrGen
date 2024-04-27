# EvilStrGen
an Effective Method of ReDoS Detection for Non-backtracking Marchers

Here we introduce how to set up a runtime environment and how to use the **tool**
## runtime environment

### Operating Systerm
Ubuntu

### Set up
```c++
sudo apt install build-essential  // install gcc, g++ and make
sudo apt install cmake  // install cmake
```

### How to run 
```c++
cd EvilStrGen // Enter the root directory of the project
mkdir build && cd build // create build directory
cmake .. //load cmakelist file
make // compile into .exe file
./EvilStrGen [Regex] [OutputFile] [EngineType] [Attack String Length] 
```

### Batch Processing
```c++
cd EvilStrGen
python3 Batch_Processing.py [RegexDataset] [OutputDirectory]
```
