import FWCore.ParameterSet.Config as cms

from TrackingTools.TrackAssociator.default_cfi import TrackAssociatorParameterBlock

GenTrackEcalAnalyzer = cms.EDAnalyzer('GenTrackEcalAnalyzer',
    genParticles = cms.InputTag("genParticles"),
    tracks = cms.InputTag("generalTracks"),
    ecalRecHits = cms.InputTag("reducedEcalRecHitsEB"),
    pdgId = cms.int32(17),
    deltaRCutoff_tracks = cms.double(0.1),
    
    # Import the default TrackAssociatorParameters and override necessary values
    TrackAssociatorParameters = TrackAssociatorParameterBlock.TrackAssociatorParameters.clone(
        useHO   = cms.bool(False),
        useEcal = cms.bool(True),            # Use ECAL for track association
        useHcal = cms.bool(False),           # Don't use HCAL
        useMuon = cms.bool(False),           # Don't use muons
        #accountForTrajectoryChangeCalo = cms.bool(True),  # Account for track bending
        dREcal = cms.double(0.2),            # Delta R cutoff for ECAL RecHits
        #dRHcal = cms.double(1.0),            # Larger for HCAL, but not used
        EBRecHitCollectionLabel = cms.InputTag("reducedEcalRecHitsEB"),  # Barrel RecHits
        EERecHitCollectionLabel = cms.InputTag("reducedEcalRecHitsEE"),  # Endcap RecHits
        #propagateAllDirections = cms.bool(True),  # Propagate track in all directions
        #dREcalPreselection = cms.double(0.05)  # Preselection cutoff for ECAL RecHits
    )
)

