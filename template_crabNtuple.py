"""
Code to produce Ntuples from signal samples produced centrally by either providing
text files with the files to run over for a configuration OR by providing dataset 
to run over

crab automatically uses default python version -> 2.7.14 -> no f strings

plan to add control panel entries as CLI arguments in the future
"""

from CRABClient.UserUtilities import config
import datetime,sys,os

config = config()

# -----------------------------------------------
#				CONTROL PANEL
# -----------------------------------------------

cmssw_version	        =	'CMSSW_10_6_39'
era			            =	'2018' 
#NJOBS			        =	-1
workflow		        =	'Signal' # Can be modified in future for data
StorageSite		        =	'T3_CH_CERNBOX'
data_files		        =	False 

charge                  =   'CHARGE_I' # in Q units
mass                    =   'MASS_I' # in GeV

splitting               =   'FileBased' #'Automatic'
# -----------------------------------------------

all_files = []
if data_files:
	with open(data_files,'r') as f:
		for line in f:
			all_files.append(line.split()[0])

if workflow == 'Signal':
    pSet    = '/eos/home-r/rhashmi/work/mchamp/CMSSW_10_6_39/src/mCHAMP_Analysis/mchampAnalyzer/python/ConfFile_cfg.py'
    JOBID   = workflow + '_Q' + charge + '_M' + mass + '_' + era
    dataset = '/HSCPmchamp_Q-' + charge + '_M-' + mass + \
                '_TuneCP2_13TeV_pythia8/RunIISummer20UL18RECO-106X_upgrade' + era + \
                '_realistic_v11_L1v1-v2/AODSIM'

print('Nutples will appear in subdiretory '+JOBID)

Nunits		= 1 # Units per job
output		= '/store/user/rhashmi/public/Histograms/central/' + JOBID

config.section_('General')
config.General.transferOutputs = True	# transfer output files to storage
config.General.transferLogs = True	# transfer job logs to storage
config.General.workArea =  'mchamp_q%s_m%s_'%(charge,mass) + str(datetime.datetime.now().strftime('%Y-%m-%d_%H-%M-%S'))
#############
config.General.requestName = JOBID
#############
config.section_('JobType')
config.JobType.psetName = pSet
config.JobType.pluginName = 'Analysis'
config.JobType.maxMemoryMB = 2500
config.JobType.outputFiles = ['mchamp_q%s_m%s.root'%(charge,mass)]
config.JobType.pyCfgParams = ['outputFile=mchamp_q%s_m%s.root'%(charge,mass)] 

config.section_('Data')
config.Data.ignoreLocality = False # allows jobs to run at any site
config.Data.inputDBS = 'global'
config.Data.splitting = splitting
config.Data.outLFNDirBase = output

#############
config.Data.inputDataset = dataset
#config.Data.userInputFiles = all_files
config.Data.unitsPerJob = Nunits
config.Data.publication=False
#############

config.section_('Site')
config.Site.storageSite =  StorageSite




