from CRABClient.UserUtilities import config
import datetime, sys, os

config = config()

cmssw_version = 'CMSSW_10_6_39'
era = '2018'
workflow = 'WORKFLOW_MODE'  
data_type = 'DATA_TYPE_I'
data_year = 'DATA_YEAR_I'
StorageSite = 'T3_CH_CERNBOX'
data_files = False
splitting = 'FileBased'
Nunits = 5               # Files per job
total_units = 600         # Total files 
pSet = '/eos/home-r/rhashmi/work/mchamp/CMSSW_10_6_39/src/mCHAMP_Analysis/mchampAnalyzer/python/ConfFile_cfg.py'

if workflow == 'DATA':
    JOBID = "{}_{}_{}".format(workflow, data_type, data_year)
    dataset = 'DATASET_I'
    workArea = "data_{}_{}_".format(data_type, data_year)
    outFile = "data_{}_{}.root".format(data_type, data_year)

print 'Ntuples will appear in subdirectory ' + JOBID

output = '/store/user/rhashmi/public/Histograms/central/' + JOBID

config.section_('General')
config.General.transferOutputs = True
config.General.transferLogs = True
config.General.workArea = workArea + datetime.datetime.now().strftime('%Y-%m-%d_%H-%M-%S')
config.General.requestName = JOBID

config.section_('JobType')
config.JobType.psetName = pSet
config.JobType.pluginName = 'Analysis'
config.JobType.maxMemoryMB = 2500
config.JobType.outputFiles = [outFile]
config.JobType.pyCfgParams = ['outputFile=%s' % outFile, 'isDATA=True']

config.section_('Data')
config.Data.ignoreLocality = False
config.Data.inputDBS = 'global'
config.Data.splitting = splitting
config.Data.unitsPerJob = Nunits
config.Data.totalUnits = total_units
config.Data.inputDataset = dataset
config.Data.outLFNDirBase = output
config.Data.publication = False
config.Data.partialDataset = True

config.section_('Site')
config.Site.storageSite = StorageSite

