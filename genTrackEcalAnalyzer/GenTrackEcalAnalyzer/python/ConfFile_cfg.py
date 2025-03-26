import FWCore.ParameterSet.Config as cms
from FWCore.ParameterSet.VarParsing import VarParsing

options = VarParsing('analysis')
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
process.maxEvents = cms.untracked.PSet(input = cms.untracked.int32(1000)) #-1
#process.maxEvents = cms.untracked.PSet(input = cms.untracked.int32(20)) #-1

# Load your analyzer from the package and set its parameters
process.load("genTrackEcalAnalyzer.GenTrackEcalAnalyzer.genTrack_cfi")
process.GenTrackEcalAnalyzer = process.GenTrackEcalAnalyzer.clone(
    genParticles = cms.InputTag("genParticles"),
    tracks = cms.InputTag("generalTracks"),
    ecalRecHits = cms.InputTag("reducedEcalRecHitsEB"),
    offlinePV = cms.InputTag("offlinePrimaryVertices"),
    pdgId = cms.int32(17),
    deltaRCutoff_tracks = cms.double(0.1),
    deltaRCutoff_EB = cms.double(0.1),
	dedxHits = cms.InputTag("dedxHitInfo"),
	Ih2Collection = cms.InputTag("dedxHarmonic2","","RECO"),
    outputFile = cms.string(options.outputFile)
)

# Define the analyzer path
process.p = cms.Path(process.GenTrackEcalAnalyzer)

# MessageLogger settings (optional, useful for debugging)
process.MessageLogger.cerr.FwkReport.reportEvery = 100  # Prints event number every 100 events

