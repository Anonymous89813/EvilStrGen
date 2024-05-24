# EvilStrGen
#### Towards an Effective Method of ReDoS Detection for Non-backtracking Engines


## Runtime environment
#### Here we will introduce how to set up a runtime environment

### Operating Systerm
```shell
Ubuntu20.04 # Other Ubuntu Long-Term Support (LTS) versions are also acceptable.
```


### Set up
```bash
sudo apt install build-essential  # install gcc, g++ and make
sudo apt install cmake  # install cmake
```

## Running commands
#### Here, we will introduce how to run our EvilStrGen.

### Directory structure
```shell
EvilStrGen/
├── regex_set #dataset
├── src # source code
├── attack_string # OutputDirectory
└── EvilStrGen.cpp  #main code
```

### Building and Running
```bash
cd EvilStrGen # Enter the root directory of the project
mkdir build && cd build # create build directory
cmake .. # load cmakelist file
make # compile into .exe file
./EvilStrGen [RegexFile] [OutputDirectory] [EngineType] [Attack String Length] [Is the number of RegexFile in the file greater than one] # running command
./EvilStrGen -h  # Use the command to obtain specific parameter information of the command.
```

### Running example
```bash
./EvilStrGen "../regex_set/SET736535.txt" "../attack_string" 1 100000 1 # take SET736535 dataset as input, and output the corresponding attack strings which will be store in attack_string directory.
```

