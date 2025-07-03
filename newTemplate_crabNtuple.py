from CRABClient.UserUtilities import config
import datetime, sys, os

config = config()

cmssw_version = 'CMSSW_10_6_39'
era = '2018'
workflow = 'WORKFLOW_MODE'  
bkg_type = 'BKG_TYPE_I'
bkg_subType = 'BKG_SUBTYPE_I'
StorageSite = 'T3_CH_CERNBOX'
data_files = False
splitting = 'FileBased'
Nunits = 5               # Files per job
total_units = 600         # Total files 
pSet = '/eos/home-r/rhashmi/work/mchamp/CMSSW_10_6_39/src/mCHAMP_Analysis/mchampAnalyzer/python/ConfFile_cfg.py'

if workflow == 'Background':
    JOBID = "{}_{}_{}_{}_NoTCut".format(workflow, bkg_type, bkg_subType, era)
    dataset = 'DATASET_I'
    workArea = "bkg_{}_{}_".format(bkg_type, bkg_subType)
    outFile = "bkg_{}_{}.root".format(bkg_type, bkg_subType)

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
config.JobType.pyCfgParams = ['outputFile=%s' % outFile, 'isDATA=False']

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
#config.Data.inputBlocks = ['/WJetsToLNu_0J_TuneCP5_13TeV-amcatnloFXFX-pythia8/RunIISummer20UL18RECO-106X_upgrade2018_realistic_v11_L1v1-v2/AODSIM#03358336-0f0f-43ef-9782-ee14bb90c507']

config.section_('Site')
config.Site.storageSite = StorageSite

