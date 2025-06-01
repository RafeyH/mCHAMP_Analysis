import FWCore.ParameterSet.Config as cms

triggerList = cms.vstring(
    # Muon Triggers
    "HLT_Mu50_v*",
    "HLT_L2Mu23NoVtx_2Cha_CosmicSeed_v*",
    "HLT_IsoMu27_v*",
    # Photon Triggers 
    "HLT_Photon35_TwoProngs35_v*",
    "HLT_Photon60_R9Id90_CaloIdL_IsoL_DisplacedIdL_PFHT350MinPFJet15_v*",
    "HLT_Photon110EB_TightID_TightIso_v*",
    "HLT_Diphoton30PV_18PV_R9Id_AND_IsoCaloId_AND_HE_R9Id_PixelVeto_Mass55_v*",
    "HLT_Diphoton30_18_R9IdL_AND_HE_AND_IsoCaloId_NoPixelVeto_v*",
    "HLT_TriplePhoton_35_35_5_CaloIdLV2_R9IdVL_v*",
    "HLT_TriplePhoton_20_20_20_CaloIdLV2_v*",
    "HLT_TriplePhoton_30_30_10_CaloIdLV2_R9IdVL_v*",
    # Electron Triggers
    "HLT_DiEle27_WPTightCaloOnly_L1DoubleEG_v*",
    "HLT_Ele28_eta2p1_WPTight_Gsf_HT150_v*",
    "HLT_Ele32_WPTight_Gsf_L1DoubleEG_v*",
)

print(">>> Loaded triggerList_cff.py")
