# Setup
---
To setup the repository, set up a sl7 space on lxplus
```sh
cmssw-el7
cmsrel CMSSW_10_6_39
cd CMSSW_10_6_39/src
cmsenv
git clone git@github.com:RafeyH/mCHAMP_Analysis.git
```

# Runnig the Analyzer
---
To produce Ntuples, the analyer must be compiled with the right CMSSW envirnment (cmssw-el7).
```sh
voms-proxy-init -voms cms
cd genTrackEcalAnalyzer
cmsenv
scram build -b8
```
There is python script (run\_script\_multiprocess.py) to run the analyzer locally. It relies on a YAML file to provide it with path to MC files and metadata to produce output root files. The YAML files can be produced by simply running:
```sh
python3 produce_dataset_yaml_ALL.py
```
One can simply change the name of the input YAML file and the output directory in run\_script\_multiprocess.py and produce the Ntuples by:
```sh
python3 run_script_multiprocess.py
```

# Producing Histograms
---
To produce histograms with selections of choice on the Ntuples, an example is provided in Histograms folder showing how to access the data in the Ntuples and producing/saving histogrms as pkl files.

