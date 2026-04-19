import FWCore.ParameterSet.Config as cms
from FWCore.ParameterSet.VarParsing import VarParsing

options = VarParsing('analysis')

options.register('saveNtuple', False,
    VarParsing.multiplicity.singleton,
    VarParsing.varType.bool,
    "Save genMatched TTrees? True or False"
)

options.register('isDATA', False,
    VarParsing.multiplicity.singleton,
    VarParsing.varType.bool,
    "Check if input dataset is data - and switch off signal region"
)

options.parseArguments()

process = cms.Process("MyAnalyzerProcess")

# Load standard configurations
process.load("FWCore.MessageService.MessageLogger_cfi")

# Load geometry and global tag
process.load("Configuration.StandardSequences.GeometryRecoDB_cff")
process.load("Configuration.StandardSequences.MagneticField_cff")
process.load("Configuration.StandardSequences.FrontierConditions_GlobalTag_cff")

# Add required ESProducer for DetIdAssociatorRecord
process.load("TrackingTools.TrackAssociator.DetIdAssociatorESProducer_cff")

# For propagators
process.load("TrackingTools.MaterialEffects.MaterialPropagator_cfi")
process.load("TrackingTools.MaterialEffects.OppositeMaterialPropagator_cfi")
process.load("TrackingTools.GeomPropagators.SmartPropagator_cff")

# Set the global tag
from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, '106X_upgrade2018_realistic_v11_L1v1', '')

# Specify the input AOD file(s)
process.source = cms.Source("PoolSource",
    fileNames = cms.untracked.vstring(
        options.inputFiles
        #'/store/mc/RunIISummer20UL18RECO/HSCPmchamp_Q-18_M-1000_TuneCP2_13TeV_pythia8/AODSIM/106X_upgrade2018_realistic_v11_L1v1-v2/2530000/F06C9CB9-60A5-D842-B398-40204B738670.root'
    )
)

# Set the maximum number of events to process
#process.maxEvents = cms.untracked.PSet(input = cms.untracked.int32(3000)) #-1
#process.maxEvents = cms.untracked.PSet(input = cms.untracked.int32(20000)) #-1
process.maxEvents = cms.untracked.PSet(input = cms.untracked.int32(10)) #-1

# Load your analyzer from the package and set its parameters
from mCHAMP_Analysis.mchampAnalyzer.triggerList_cff import triggerList

process.load("mCHAMP_Analysis.mchampAnalyzer.genTrack_cfi")
process.mchampAnalyzer = process.mchampAnalyzer.clone(
    genParticles    = cms.InputTag("genParticles"),
    tracks          = cms.InputTag("generalTracks"),
    ecalRecHits     = cms.InputTag("reducedEcalRecHitsEB"),
    offlinePV       = cms.InputTag("offlinePrimaryVertices"),
    pdgId           = cms.int32(17),
    deltaRCutoff_tracks = cms.double(0.1),
    deltaRCutoff_EB = cms.double(0.1),
	dedxHits        = cms.InputTag("dedxHitInfo"),
	Ih2Collection   = cms.InputTag("dedxHarmonic2","","RECO"),
    triggerResults  = cms.InputTag("TriggerResults","","HLT"),
    eventFilters    = cms.InputTag("TriggerResults","","RECO"),
    triggerPaths    = triggerList,
    outputFile      = cms.string(options.outputFile),
    saveNtuple      = cms.bool(options.saveNtuple),
    isDATA          = cms.bool(options.isDATA),
    #dEdxTemplate    = cms.string("mchampAnalyzer/data/template_2018MC_v5.root")
    #dEdxTemplate    = cms.string("template_2018D_v5.root")
    dEdxTemplate    = cms.string("template_2018MC_v5.root")
)


process.TFileService = cms.Service("TFileService",
    fileName = cms.string(options.outputFile)
)
 
# Define the analyzer path
process.p = cms.Path(process.mchampAnalyzer)

# Prints message after every N-events
#process.MessageLogger.cerr.FwkReport.reportEvery = 500  

