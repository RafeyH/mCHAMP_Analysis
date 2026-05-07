// -*- C++ -*-
//
// Package:    mCHAMP_Analysis/mchampAnalyzer
// Class:      mchampAnalyzer
//
/**\class mchampAnalyzer mchampAnalyzer.cc mCHAMP_Analysis/mchampAnalyzer/plugins/mchampAnalyzer.cc

 Description: [one line class summary]

 Implementation:
     [Notes on implementation]
*/
//
// Original Author:  Rafey Hashmi
//         Created:  Thu, 17 Oct 2024 16:48:36 GMT
//
//


// system include files
#include <memory>

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/TrackReco/interface/TrackBase.h"
#include "DataFormats/TrackReco/interface/HitPattern.h"
#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/VertexReco/interface/VertexFwd.h"
#include "DataFormats/TrackReco/interface/TrackFwd.h"
#include "DataFormats/TrackReco/interface/DeDxHitInfo.h"

#include "DataFormats/EcalRecHit/interface/EcalRecHit.h"
#include "DataFormats/DetId/interface/DetId.h"
#include "DataFormats/EcalDetId/interface/EBDetId.h"
#include "DataFormats/HepMCCandidate/interface/GenParticle.h"
#include "DataFormats/Math/interface/deltaR.h"

#include "CommonTools/UtilAlgos/interface/TFileService.h"

#include "TrackingTools/TrackAssociator/interface/TrackDetectorAssociator.h"
#include "TrackingTools/TrackAssociator/interface/TrackAssociatorParameters.h"

#include "DataFormats/GeometryVector/interface/GlobalVector.h"
#include "DataFormats/GeometryVector/interface/GlobalPoint.h"

// Geometry
#include "Geometry/Records/interface/CaloGeometryRecord.h"
#include "Geometry/CaloGeometry/interface/CaloSubdetectorGeometry.h"
#include "Geometry/CaloGeometry/interface/CaloCellGeometry.h"
#include "Geometry/CaloGeometry/interface/CaloGeometry.h"


// ROOT files
#include "TTree.h"
#include "TFile.h"
#include "TObject.h"

#include "mchampAnalyzer.h"
#include "CommonFunction.h"
#include "DeDxUtility.h"

// Implement ROOT's dictionary for custom classes (for branches)
ClassImp(GenPart);
ClassImp(Tracks);
ClassImp(RecHits_Ecal);
ClassImp(TrackAssoc);

    
//
// constructors and destructor
//
mchampAnalyzer::mchampAnalyzer(const edm::ParameterSet& iConfig)
 :  
  	genParticlesToken_(consumes<reco::GenParticleCollection>(iConfig.getParameter<edm::InputTag>("genParticles"))),
  	genEventInfoToken_(consumes<GenEventInfoProduct>(edm::InputTag("generator"))),
	tracksToken_(consumes<reco::TrackCollection>(iConfig.getParameter<edm::InputTag>("tracks"))),
    ecalRecHitsToken_(consumes<EcalRecHitCollection>(iConfig.getParameter<edm::InputTag>("ecalRecHits"))),
    vertexToken_(consumes<reco::VertexCollection>(iConfig.getParameter<edm::InputTag>("offlinePV"))),
    pdgId_(iConfig.getParameter<int>("pdgId")),
    deltaRCutoff_tracks(iConfig.getParameter<double>("deltaRCutoff_tracks")),
    deltaRCutoff_EB(iConfig.getParameter<double>("deltaRCutoff_EB")),
	dedxToken_(consumes<reco::DeDxHitInfoAss>(iConfig.getParameter<edm::InputTag>("dedxHits"))),
	Ih2Token_(consumes<reco::DeDxDataCollection>(iConfig.getParameter<edm::InputTag>("Ih2Collection"))),
    triggerResultsToken_(consumes<edm::TriggerResults>(iConfig.getParameter<edm::InputTag>("triggerResults"))),
    eventFilterToken_(consumes<edm::TriggerResults>(iConfig.getParameter<edm::InputTag>("eventFilters"))),
    triggerPaths_(iConfig.getParameter<std::vector<std::string>>("triggerPaths")),
  	jetToken_(consumes<std::vector<reco::PFJet>>(edm::InputTag("ak4PFJetsCHS"))),
  	metToken_(consumes<std::vector<reco::PFMET>>(edm::InputTag("pfMet"))),
    lowPtEleToken_(consumes<reco::GsfElectronCollection>(edm::InputTag("lowPtGsfElectrons"))),
    lowPtScoreToken_(consumes<edm::ValueMap<float>>(edm::InputTag("lowPtGsfElectronID"))),
    muonToken_(consumes<reco::MuonCollection>(edm::InputTag("muons"))),
    trigEventToken_(consumes<trigger::TriggerEvent>(edm::InputTag("hltTriggerSummaryAOD"))),
    outputFileName_(iConfig.getParameter<std::string>("outputFile")),
    saveNtuple_(iConfig.getParameter<bool>("saveNtuple")),
    isDATA_(iConfig.getParameter<bool>("isDATA")),
    dEdxTemplate_(iConfig.getParameter<std::string>("dEdxTemplate"))
{
    
    // Initialize TrackDetectorAssociator and parameters
    edm::ParameterSet trackAssociatorParams = iConfig.getParameter<edm::ParameterSet>(
                                                            "TrackAssociatorParameters");
    edm::ConsumesCollector cc = consumesCollector();
    trackAssociatorParams_.loadParameters(trackAssociatorParams, cc);
    trackAssociator_.useDefaultPropagator();

    edm::Service<TFileService> fs;
	
    if (!fs.isAvailable()) {
        throw cms::Exception("MissingService") << "TFileService is not available!";
    }
    
    haloFilterToken_    = consumes<bool>(edm::InputTag("globalSuperTightHalo2016Filter"));
    hbheToken_          = consumes<bool>(edm::InputTag("HBHENoiseFilterResultProducer", 
                                                            "HBHENoiseFilterResult"));
    hbheIsoToken_       = consumes<bool>(edm::InputTag("HBHENoiseFilterResultProducer", 
                                                            "HBHEIsoNoiseFilterResult"));
    ecalDeadCellToken_  = consumes<bool>(edm::InputTag("EcalDeadCellTriggerPrimitiveFilter"));
    badPFMuonToken_     = consumes<bool>(edm::InputTag("BadPFMuonFilter"));
    badPFMuonDzToken_   = consumes<bool>(edm::InputTag("BadPFMuonDzFilter"));
    hfNoisyHitsToken_   = consumes<bool>(edm::InputTag("hfNoisyHitsFilter"));
    eeBadScToken_       = consumes<bool>(edm::InputTag("eeBadScFilter"));
    ecalBadCalibToken_  = consumes<bool>(edm::InputTag("ecalBadCalibFilter"));

    // Deep CSV tag for b-jets 
    deepCSV_probb_Token_    = consumes<reco::JetTagCollection>(
                                    edm::InputTag("pfDeepCSVJetTags", "probb"));
    deepCSV_probbb_Token_   = consumes<reco::JetTagCollection>(
                                    edm::InputTag("pfDeepCSVJetTags", "probbb"));
                    

    if (isDATA_){
        EventInfoTree_ = fs->make<TTree>("EventInfo", "EventInfo");
        EventInfoTree_->Branch("Run", &run_, "Run/I");
        EventInfoTree_->Branch("Event", &event_, "Event/I");
        EventInfoTree_->Branch("Lumi", &lumi_, "Lumi/I");
    }
    
    if (saveNtuple_){
        tree_ = fs->make<TTree>("Ntuple", "Ntuple");
        tree_->Branch("Run", &run_, "Run/I");
        tree_->Branch("Event", &event_, "Event/I");
        tree_->Branch("GenPart.", &cls_genpart);
        tree_->Branch("Tracks.", &cls_tracks);
        tree_->Branch("EcalRecHits.", &cls_rechitsEcal);
        tree_->Branch("TrackAssoc.", &cls_trackAssoc);
    }

    if (!dEdxTemplate_.empty()){
        bool splitByModuleType = true;
        dEdxTemplates = loadDeDxTemplate(dEdxTemplate_, splitByModuleType,false,0);
    }

    histManager = std::make_unique<HistogramManager>(*fs);

    ecalTimeResDir_ = fs->mkdir("EcalTimeRes");
    TH3F* h_ecalTimeRes = ecalTimeResDir_.make<TH3F>(
                        "ecalTimeRes",
                        "ECAL timing resulution;Aeff [GeV];t1-t2 [ns];A1/A2",
                        /*Aeff range*/      100, 0, 15,
                        /*t1-t2 range*/     100, -10, 10,
                        /*A1/A2 range*/     100, 0, 10
                        );
    ecalTimeResHists_["postSel"] = h_ecalTimeRes;
    
    
    // Dir for Trigger TOCs
    ttocDir_ = fs->mkdir("TTOC");
    if (triggerHists_.find("OR_ALL_Triggers") == triggerHists_.end()) {
        TH1F* hPass = ttocDir_.make<TH1F>("leadPt_OR_ALL_Triggers_pass", 
                                            ";pT [GeV]",
                                            100, 0., 500.
                                        );
        TH1F* hTot  = ttocDir_.make<TH1F>("leadPt_OR_ALL_Triggers_total", 
                                            ";pT [GeV]",
                                            100, 0., 500.
                                        );
        hPass->Sumw2(); hTot->Sumw2();
        TH1F* hPass_E = ttocDir_.make<TH1F>("xtalE_OR_ALL_Triggers_pass", 
                                            ";E [GeV]",
                                            200, 0., 400.
                                        );
        TH1F* hTot_E  = ttocDir_.make<TH1F>("xtalE_OR_ALL_Triggers_total", 
                                            ";E [GeV]",
                                            200, 0., 400.
                                        );
        hPass_E->Sumw2(); hTot_E->Sumw2();

        triggerHists_["OR_ALL_Triggers"] = {hPass, hTot};
        triggerHists_E_["OR_ALL_Triggers"] = {hPass_E, hTot_E};

        for (const auto& trigPattern : triggerPaths_) {
            if (triggerHists_.find(trigPattern) == triggerHists_.end()) {
                std::string safe = trigPattern;
                std::replace(safe.begin(), safe.end(), '*', '_');

                std::string passName = "leadPt_" + safe + "_pass";
                std::string totName = "leadPt_" + safe + "_total";
                std::string passName_E = "xtalE_" + safe + "_pass";
                std::string totName_E = "xtalE_" + safe + "_total";

                TH1F* hPass = ttocDir_.make<TH1F>(passName.c_str(), 
                                                (passName+";pT [GeV]").c_str(),
                                                100, 0., 500.);
                TH1F* hTot  = ttocDir_.make<TH1F>(totName.c_str(), 
                                                (totName+";pT [GeV]").c_str(),
                                                100, 0., 500.);
                hPass->Sumw2(); hTot->Sumw2();
                TH1F* hPass_E = ttocDir_.make<TH1F>(passName_E.c_str(), 
                                                (passName_E+";pT [GeV]").c_str(),
                                                200, 0., 400.);
                TH1F* hTot_E = ttocDir_.make<TH1F>(totName_E.c_str(), 
                                                (totName_E+";pT [GeV]").c_str(),
                                                200, 0., 400.);
                hPass_E->Sumw2(); hTot_E->Sumw2();

                triggerHists_[trigPattern] = {hPass, hTot};
                triggerHists_E_[trigPattern] = {hPass_E, hTot_E};
            } 
        }
    }

    // Mu trigger filters: https://twiki.cern.ch/twiki/bin/viewauth/CMS/MuonHLT2018
    //mu50Filter_ = "hltL3fL1sMu22Or25L1f0L2f10QL3Filtered50Q";
    mu50Filter_ = "hltL3crIsoL1sSingleMu22L1f0L2f10QL3f24QL3trkIsoFiltered0p07";
    
    // Dir for Trigger TOCs
    ttocDir_mu_ = fs->mkdir("TTOC_Mu");
    if (triggerHists_mu_.find("OR_ALL_Triggers") == triggerHists_mu_.end()) {
        TH1F* hPass = ttocDir_mu_.make<TH1F>("leadPt_OR_ALL_Triggers_pass_mu", 
                                            ";pT [GeV]",
                                            100, 0., 500.
                                        );
        TH1F* hTot  = ttocDir_mu_.make<TH1F>("leadPt_OR_ALL_Triggers_total_mu", 
                                            ";pT [GeV]",
                                            100, 0., 500.
                                        );
        hPass->Sumw2(); hTot->Sumw2();
        TH1F* hPass_E = ttocDir_mu_.make<TH1F>("xtalE_OR_ALL_Triggers_pass_mu", 
                                            ";E [GeV]",
                                            200, 0., 400.
                                        );
        TH1F* hTot_E = ttocDir_mu_.make<TH1F>("xtalE_OR_ALL_Triggers_total_mu", 
                                            ";E [GeV]",
                                            200, 0., 400.
                                        );
        hPass_E->Sumw2(); hTot_E->Sumw2();

        triggerHists_mu_["OR_ALL_Triggers"] = {hPass, hTot};
        triggerHists_mu_E_["OR_ALL_Triggers"] = {hPass_E, hTot_E};

        for (const auto& trigPattern : triggerPaths_) {
            if (triggerHists_mu_.find(trigPattern) == triggerHists_mu_.end()) {
                std::string safe = trigPattern;
                std::replace(safe.begin(), safe.end(), '*', '_');

                std::string passName = "leadPt_" + safe + "_pass_mu";
                std::string totName = "leadPt_" + safe + "_total_mu";
                std::string passName_E = "xtalE_" + safe + "_pass_mu";
                std::string totName_E = "xtalE_" + safe + "_total_mu";

                TH1F* hPass = ttocDir_mu_.make<TH1F>(passName.c_str(), 
                                                (passName+";pT [GeV]").c_str(),
                                                100, 0., 500.);
                TH1F* hTot = ttocDir_mu_.make<TH1F>(totName.c_str(), 
                                                (totName+";pT [GeV]").c_str(),
                                                100, 0., 500.);
                hPass->Sumw2(); hTot->Sumw2();
                TH1F* hPass_E = ttocDir_mu_.make<TH1F>(passName_E.c_str(), 
                                                (passName_E+";E [GeV]").c_str(),
                                                200, 0., 400.);
                TH1F* hTot_E = ttocDir_mu_.make<TH1F>(totName_E.c_str(), 
                                                (totName_E+";E [GeV]").c_str(),
                                                200, 0., 400.);
                hPass_E->Sumw2(); hTot_E->Sumw2();

                triggerHists_mu_[trigPattern] = {hPass, hTot};
                triggerHists_mu_E_[trigPattern] = {hPass_E, hTot_E};
            } 
        }
    }
    
    // Saving cutFlow cuts and values 
    // enum and structs defined in HistogramManager.h
    // histograms defined in HistogramManager.cc
    // format {std::string name, double cut, cutFlow_enum::Type, bool one_sided?  }
    trackCuts = {
        //{ "lowPtEle",       1,      cutFlow_enum::lowPtEle,     true    },
        { "sigPtOPt2",      0.003,  cutFlow_enum::sigPtOPt2,    false   },
        { "trackPtIso",     30,     cutFlow_enum::trackPtIso,   false   },
        { "Ih",             3.5,    cutFlow_enum::Ih,           true    },
        { "dxy",            0.5,    cutFlow_enum::dxy,          false   },
        { "dz",             0.5,    cutFlow_enum::dz,           false   },
        { "Chi2Ondof",      5,      cutFlow_enum::chi2,         false   },
        { "highPurity",     1,      cutFlow_enum::highPurity,   true    },
        { "dEdxHits",       10,     cutFlow_enum::numDedxHits,  true    },
        { "validHitsFrac",  0.8,    cutFlow_enum::fracValidHits,true    },
        { "eta",            1.0,    cutFlow_enum::eta,          false   },
        //{ "sigPtOPt",       0.25,   cutFlow_enum::count,        false   },
        { "pt",             15,     cutFlow_enum::pt,           true    },
        //{ "pt",             30,     cutFlow_enum::pt,           true    },
        { "trigger",        1,      cutFlow_enum::triggers,     true    },
    };
    
    // format {std::string name, double timeCut, double energyCut, cutFlow_enum::Type }
    if (isDATA_) signalCuts = {};
    else {
        signalCuts = {
            {"SR",              2,          5,      cutFlow_enum::SR}
        };
    }
    
    // Saving trigger regex
    compiledTriggerPatterns.reserve(triggerPaths_.size());

    for (const auto& pattern : triggerPaths_)
    {
        std::string regexPattern = std::regex_replace(pattern, std::regex("\\*"), ".*");
        // Need to use emplace_back instead of push_back
        compiledTriggerPatterns.emplace_back(regexPattern);
    }

    // hardcoded pop, change later
    ignore_trig_bit_pos = 11;
}


mchampAnalyzer::~mchampAnalyzer()
{}


//
// member functions
//

void mchampAnalyzer::beginRun(const edm::Run& iRun, const edm::EventSetup& iSetup) {
  // hlt menu might change in data and the index of these triggers might change
  bool changed = true;
  if (hltConfig_.init(iRun, iSetup, "HLT", changed)) {
    triggerIndices_.clear();

    auto cacheIndices = [&](const std::string& base) {
      std::vector<unsigned int> idx;
      for (unsigned int i = 0; i < hltConfig_.size(); ++i) {
        const std::string& name = hltConfig_.triggerName(i);
        if (name.find(base) == 0) idx.push_back(i);
      }
      return idx;
    };

    // Orthogonal triggers
    triggerIndices_["HLT_Mu50_v"] = cacheIndices("HLT_Mu50_v");
    triggerIndices_["HLT_PFMET"] = cacheIndices("HLT_PFMET");
    triggerIndices_["HLT_PFHT"]  = cacheIndices("HLT_PFHT");
    triggerIndices_["HLT_MET150"]  = cacheIndices("HLT_MET150");
    triggerIndices_["HLT_IsoMu24_v"]  = cacheIndices("HLT_IsoMu24_v");
    triggerIndices_["HLT_MonoCentralPFJet80"]  = cacheIndices("HLT_MonoCentralPFJet80");
    triggerIndices_[
        "HLT_PFMET120_PFMHT120_IDTight"] = cacheIndices("HLT_PFMET120_PFMHT120_IDTight");
    triggerIndices_[
        "HLT_PFMETNoMu120_PFMHTNoMu120_IDTight"] = cacheIndices("HLT_PFMETNoMu120_PFMHTNoMu120_IDTight");
    
    for (const auto& t : triggerPaths_) {
      std::string base = t.substr(0, t.find('*'));
      triggerIndices_[t] = cacheIndices(base);
    }
    

  } else {
    edm::LogError("mchampAnalyzer") << "HLTConfigProvider failed to initialize";
  }
}


// ------------ method called for each event  ------------
void
mchampAnalyzer::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup)
{
    run_    = iEvent.id().run();
    event_  = iEvent.id().event();
    lumi_   = iEvent.luminosityBlock();
    if (isDATA_) EventInfoTree_->Fill();

    edm::Handle<reco::GenParticleCollection> genParticles;
    edm::Handle<GenEventInfoProduct> genEventInfo;
    
    double genWeight = 1.0;
    if (!isDATA_){
        // Get GenParticles
        iEvent.getByToken(genParticlesToken_, genParticles);

        // Get GenEventInfo
        iEvent.getByToken(genEventInfoToken_, genEventInfo);
        if (genEventInfo.isValid()) genWeight = genEventInfo->weight();
    }
    
    // Get Tracks
    edm::Handle<reco::TrackCollection> tracks;
    iEvent.getByToken(tracksToken_, tracks);

    // Get Vertices
    edm::Handle<reco::VertexCollection> vertices;
    iEvent.getByToken(vertexToken_, vertices);
    
    // Good vertex definition from
    // https://twiki.cern.ch/twiki/bin/view/CMSPublic/WorkBookVertexReco
    bool goodVertices = false;
    int nVertices = 0;
    for (const auto& vtx : *vertices) {
        if (!vtx.isFake() && 
                vtx.ndof() > 4 && 
                fabs(vtx.z()) <= 24 && 
                vtx.position().Rho() <= 2) {
            nVertices++;
            if (!goodVertices) goodVertices = true;
        }
    }

    // MET Filtes
    // https://twiki.cern.ch/twiki/bin/viewauth/CMS/MissingETOptionalFiltersRun2#2018_2017_data_and_MC_UL
    edm::Handle<bool> halo;
    edm::Handle<bool> hbhe;
    edm::Handle<bool> hbheIso;
    edm::Handle<bool> ecalDead;
    edm::Handle<bool> badPFMuon;
    edm::Handle<bool> badPFMuonDz;
    edm::Handle<bool> hfNoise;
    edm::Handle<bool> eeBadSc;
    edm::Handle<bool> ecalBadCalib;
    
    iEvent.getByToken(  haloFilterToken_,     halo);
    iEvent.getByToken(  hbheToken_,           hbhe);
    iEvent.getByToken(hbheIsoToken_,      hbheIso);
    iEvent.getByToken(  ecalDeadCellToken_,   ecalDead);
    iEvent.getByToken(  badPFMuonToken_,      badPFMuon);
    iEvent.getByToken(  badPFMuonDzToken_,    badPFMuonDz);
    iEvent.getByToken(  hfNoisyHitsToken_,    hfNoise);
    iEvent.getByToken(  eeBadScToken_,        eeBadSc);
    iEvent.getByToken(  ecalBadCalibToken_,   ecalBadCalib);

    bool passMETFilters = 
        (goodVertices) &&
        (*halo) &&
        (*hbhe) &&
        (*hbheIso) &&
        (*ecalDead) &&
        (*badPFMuon) &&
        (*badPFMuonDz) &&
        (*hfNoise) &&
        (*eeBadSc) &&
        (*ecalBadCalib);
    
    histManager->fillHistograms("Event_Filters", "METFilters", 
                        metFilter_enum::total, genWeight);

    if (goodVertices)  histManager->fillHistograms("Event_Filters", "METFilters", 
                                            metFilter_enum::goodVertices, genWeight);
    if (*halo)          histManager->fillHistograms("Event_Filters", "METFilters", 
                                            metFilter_enum::halo, genWeight);
    if (*hbhe)          histManager->fillHistograms("Event_Filters", "METFilters", 
                                            metFilter_enum::HBHE, genWeight);
    if (*hbheIso)       histManager->fillHistograms("Event_Filters", "METFilters", 
                                            metFilter_enum::HBHEIso, genWeight);
    if (*ecalDead)      histManager->fillHistograms("Event_Filters", "METFilters", 
                                            metFilter_enum::EcalDeadCell, genWeight);
    if (*badPFMuon)     histManager->fillHistograms("Event_Filters", "METFilters", 
                                            metFilter_enum::BadPFMuon, genWeight);
    if (*badPFMuonDz)   histManager->fillHistograms("Event_Filters", "METFilters", 
                                            metFilter_enum::BadPFMuonDz, genWeight);
    if (*hfNoise)       histManager->fillHistograms("Event_Filters", "METFilters", 
                                            metFilter_enum::HfNoisyHits, genWeight);
    if (*eeBadSc)       histManager->fillHistograms("Event_Filters", "METFilters", 
                                            metFilter_enum::eeBadSc, genWeight);
    if (*ecalBadCalib)  histManager->fillHistograms("Event_Filters", "METFilters", 
                                            metFilter_enum::ecalBadCalib, genWeight);

    if (!passMETFilters) {
        std::cout<<"MET Filters not passed! Event: "<<event_<<" rejected\n";
        return;
    }
    
    // Getting total gen weights ONLY from approved/allowed events.
    // MET Filters are filtering out bad physics events and NOT physics cuts
    if (!isDATA_){
        sum_gen_weights += genWeight;
    }
    
    histManager->fillHistograms("Event_Filters", "METFilters", 
                                    metFilter_enum::all, genWeight);
    
    // Get dedx collection
	edm::Handle<reco::DeDxHitInfoAss> dedxCollH = iEvent.getHandle(dedxToken_);
	//std::cout<<"dedxCollH size: "<<dedxCollH->size()<<"\n";
    
	// Ih2 handle
	edm::Handle<reco::DeDxDataCollection> Ih2CollH = iEvent.getHandle(Ih2Token_);
    
    // Trigger Results
    edm::Handle<edm::TriggerResults> triggerResults;
    iEvent.getByToken(triggerResultsToken_, triggerResults);
    if (!triggerResults.isValid()){
        edm::LogWarning("mchampAnalyzer") << "TriggerResults not valid\n";
    }

    // Trigger names
    const edm::TriggerNames& triggerNames = iEvent.triggerNames(*triggerResults);
    
    // Jets collection - why is pfJet collection not a default thing?
    edm::Handle<std::vector<reco::PFJet>> jetCollection;
    iEvent.getByToken(jetToken_, jetCollection);
   
    // MET collection - is a vector and not just a number
    edm::Handle<std::vector<reco::PFMET>> metCollection;
    iEvent.getByToken(metToken_, metCollection);
    
    // Get EcalRecHits
    edm::Handle<EcalRecHitCollection> EBRecHits;
    iEvent.getByToken(ecalRecHitsToken_, EBRecHits);

    // Geometry handle
	edm::ESHandle<CaloGeometry> geoHandle;
	iSetup.get<CaloGeometryRecord>().get(geoHandle);
	const CaloSubdetectorGeometry* barrelGeometry = geoHandle->getSubdetectorGeometry(DetId::Ecal, EcalBarrel);
   
    /* ********************************************************
       ____                  __  __       _       _     
      / ___| ___ _ __       |  \/  | __ _| |_ ___| |__  
     | |  _ / _ \ '_ \ _____| |\/| |/ _` | __/ __| '_ \
     | |_| |  __/ | | |_____| |  | | (_| | || (__| | | |
      \____|\___|_| |_|     |_|  |_|\__,_|\__\___|_| |_| 
       
      ********************************************************
    */

    if (saveNtuple_ && !isDATA_){
    // Loop over genParticles and match to PDG id
    for (const auto& genParticle : *genParticles) {
        
        if (std::abs(genParticle.pdgId()) != pdgId_) continue;
        if (genParticle.status() != 1) continue;
        
        cls_genpart->reset();
        
        cls_genpart->pt   = genParticle.pt();
        cls_genpart->eta  = genParticle.eta();
        cls_genpart->phi  = genParticle.phi();
        cls_genpart->id   = genParticle.pdgId();

        cls_tracks->reset();
        cls_rechitsEcal->reset();
        cls_trackAssoc->reset();
        
        int pos = -1;

        for (const auto& track : *tracks) {
            pos++;
            
            // Calculate dR between the genParticle and the track projected to ECAL
            double deltaR_trackGen = reco::deltaR(cls_genpart->eta, cls_genpart->phi, track.eta(), track.phi());
            if (deltaR_trackGen > deltaRCutoff_tracks || track.pt()<5) continue;
                
            reco::Vertex bestVertex;
            double maxSumPt2 = -1.0;

            for (const auto& vertex : *vertices) {
                if (vertex.isFake() || !vertex.isValid()) continue;
                
                double sumPt2 = 0.0;
                for (reco::Vertex::trackRef_iterator it = vertex.tracks_begin(); it != vertex.tracks_end(); it++){
                    double pt = (**it).pt();
                    if ((**it).ptError() > pt) continue;
                    sumPt2 += pt * pt;
                }
                
                if (sumPt2 > maxSumPt2) {
                    maxSumPt2 = sumPt2;
                    bestVertex = vertex;
                }
            }
            
            cls_tracks->pt.push_back(track.pt());
            cls_tracks->ptError.push_back(track.ptError());
            cls_tracks->beta.push_back(track.beta());
            cls_tracks->eta.push_back(track.eta());
            cls_tracks->phi.push_back(track.phi());
            
            cls_tracks->deltaR.push_back(deltaR_trackGen);
            
            cls_tracks->qoverp.push_back(track.qoverp());
            cls_tracks->lambda.push_back(track.lambda());
            cls_tracks->dxy.push_back(track.dxy(bestVertex.position()));
            cls_tracks->dz.push_back(track.dz(bestVertex.position()));
            cls_tracks->charge.push_back(track.charge());
            cls_tracks->chisq.push_back(track.chi2());
            cls_tracks->ndof.push_back(track.ndof());
            
            cls_tracks->validHitsNum.push_back(track.numberOfValidHits());
            cls_tracks->validHitsFrac.push_back(track.validFraction());
            
            cls_tracks->trackQual.push_back(track.qualityMask());
            cls_tracks->trackAlgo.push_back(track.algo());

            // Dedx hit info access
            const reco::TrackRef trackRef = reco::TrackRef(tracks, pos);
            reco::DeDxHitInfoRef dedxHitsRef = dedxCollH->get(trackRef.key());
            if (dedxHitsRef.isNull()) {
                cls_tracks->hasDedxRef.push_back(0);
                
                cls_tracks->dedx.push_back(-1);
                cls_tracks->numOfSatStrips.push_back(-1);
                cls_tracks->numOfStrips.push_back(-1);
            }
            else {
                cls_tracks->hasDedxRef.push_back(1);
                const reco::DeDxHitInfo* dedxHits = &(*dedxHitsRef);
                
                // for dedx measurements without cluster cleaning
                string year = "";
                float dedxSF[] = {1.0, 1.0325};
                TH3* templateHisto = nullptr;
                bool usePixel = false;
                bool useStrip = true;
                bool useClusterCleaning = false;

                reco::DeDxData temp   = computedEdx(run_, year, dedxHits, dedxSF, templateHisto, usePixel, 
                                    useStrip, useClusterCleaning);
                
                reco::DeDxData t_Ias  = computedEdx(run_, year, dedxHits, dedxSF, dEdxTemplates, usePixel, 
                                    useStrip, useClusterCleaning);
                
                //reco::DeDxData t_ProbQ= computedEdx(run_, year, dedxHits, dedxSF, dEdxTemplates, usePixel, 
                //                    useStrip, useClusterCleaning, skip_templates_ias = 0, 
                //                    symmetricSmirnov = false, useMorrisMethod = true);
                
                cls_tracks->dedx.push_back(temp.dEdx());
                cls_tracks->numOfSatStrips.push_back(temp.numberOfSaturatedMeasurements());
                cls_tracks->numOfStrips.push_back(temp.numberOfMeasurements());
                cls_tracks->Ias.push_back(t_Ias.dEdx());
                //cls_tracks->ProbQ.push_back(t_ProbQ.dEdx());
            }   // End of if Else - dedxHitsRef check
            
            //Extrapolating track info 
            //float pTavg = track.momentum().R();
            float pTavg = track.pt();
            float R_ecal = 1.290;
            float outer_phi_d = -1000;
            float charge = track.charge();
            //outer_phi_d = asin( ( -track.charge() * (R_ecal / pTavg) * 3.8 * 1.6 ) / (2 * 100 * 5.36) );
            if (charge == 1){
                outer_phi_d = asin( (- (R_ecal) / pTavg) * 3.8 * 1.6 / (2 * 100 * 5.36));
            }
            else{
                if(track.charge() == -1){
                    outer_phi_d = asin( ((R_ecal) / pTavg) * 3.8 * 1.6 / (2 * 100 * 5.36));
                }
            }
            //float deltaPhi = GetPhiDiff(track.outerPhi(), EEPhi);
            outer_phi_d += track.phi();

            // Find and save ECAL info
            
            std::vector<float> energy, energyErr, time, timeErr, deltaR;
            std::vector<bool> timeErrValid;
            std::vector<uint32_t> rechitFlag;
            std::vector<int> iEta, iPhi;
            
            EBRecHitCollection::const_iterator EBRecItr;
            for (EBRecItr = EBRecHits->begin(); EBRecItr != EBRecHits->end(); EBRecItr++)
            {
                EcalRecHit hit = *EBRecItr;
                EBDetId det = hit.id();
                float EBEta = barrelGeometry->getGeometry(det)->getPosition().eta();
                float EBPhi = barrelGeometry->getGeometry(det)->getPosition().phi();
                
                float deltaEta = track.eta() - EBEta;
                float deltaPhi = reco::deltaPhi(EBPhi, outer_phi_d);
                
                float dR = sqrt( pow(deltaEta, 2.0) + pow(deltaPhi, 2.0) );

                // Adding a cut to delta R (and ecal rechit energy?)
                if (dR < deltaRCutoff_EB && hit.energy() > 0) {
                   energy.push_back(hit.energy());
                   energyErr.push_back(hit.energyError());
                   time.push_back(hit.time());
                   timeErr.push_back(hit.timeError());
                   timeErrValid.push_back(hit.isTimeErrorValid());
                   deltaR.push_back(dR);
                   iEta.push_back(det.ieta());
                   iPhi.push_back(det.iphi());
                   rechitFlag.push_back(hit.flagsBits());
                }
            }   // END of loop over EB RecHit collection

            cls_rechitsEcal->energy.push_back(energy);
            cls_rechitsEcal->energyErr.push_back(energyErr);
            cls_rechitsEcal->time.push_back(time);
            cls_rechitsEcal->timeErr.push_back(timeErr);
            cls_rechitsEcal->timeErrValid.push_back(timeErrValid);
            cls_rechitsEcal->deltaR.push_back(deltaR);
            cls_rechitsEcal->iEta.push_back(iEta);
            cls_rechitsEcal->iPhi.push_back(iPhi);
            cls_rechitsEcal->rechitFlag.push_back(rechitFlag);
            
            // Looking at Ecal rechit near projected track location
            // Use TrackDetectorAssociator to propagate the track to ECAL
            
            TrackDetMatchInfo info = trackAssociator_.associate(
                                        iEvent, 
                                        iSetup, 
                                        trackAssociator_.getFreeTrajectoryState(iSetup, track),
                                        trackAssociatorParams_);

            float tempE = 0;
            for (auto e : energy) tempE += e;
           
            if (abs(track.eta()) > 1.479 ){
                std::cout<<"Track not in barrel\n";
            }
            else{
                if (info.ecalRecHits.size()<1) continue;
                
                cls_trackAssoc->ecalXEnergy.push_back(info.ecalCrossedEnergy());
                cls_trackAssoc->ecal3x3Energy.push_back(
                    info.nXnEnergy(TrackDetMatchInfo::EcalRecHits, 1)
                    );
                cls_trackAssoc->ecal5x5Energy.push_back(
                    info.nXnEnergy(TrackDetMatchInfo::EcalRecHits, 2)
                    );
                
                DetId maxDep = info.findMaxDeposition(TrackDetMatchInfo::EcalRecHits);

                for (EBRecItr = EBRecHits->begin(); EBRecItr != EBRecHits->end(); EBRecItr++)
                {
                    EcalRecHit hit = *EBRecItr;
                    EBDetId det = hit.id();

                    if (det.rawId() != maxDep.rawId()) continue;
                    
                    cls_trackAssoc->ecalMaxE.push_back(hit.energy());
                    cls_trackAssoc->ecalMaxETime.push_back(hit.time());
                    float EBEta = barrelGeometry->getGeometry(det)->getPosition().eta();
                    float EBPhi = barrelGeometry->getGeometry(det)->getPosition().phi();
                    math::XYZPoint trackPos = info.trkGlobPosAtEcal;
                    double deltaR = reco::deltaR(trackPos.eta(), trackPos.phi(), EBEta, EBPhi);
                    cls_trackAssoc->ecalMaxEdR.push_back(deltaR);
                    break;
                }   // END for - EB rechits matching to maxE deposit
            }   // END if - cut on track eta (limit to EB)
        }   // END for - TrackCollection

        tree_->Fill();
    }   // END for - GenParticles
    }   // END if - saveNtuple
    

    /* ********************************************************
       _____       _                                
      |_   _|_ __ (_)  __ _   __ _   ___  _ __  ___ 
        | | | '__|| | / _` | / _` | / _ \| '__|/ __|
        | | | |   | || (_| || (_| ||  __/| |   \__ \
        |_| |_|   |_| \__, | \__, | \___||_|   |___/
                      |___/  |___/                   
      ********************************************************
    */
    
    bool passTriggerSelection = false;
    
    for (size_t i = 0; i < triggerResults->size(); i++)
    {
        std::string name = triggerNames.triggerName(i);

        for (const auto& regexPattern : compiledTriggerPatterns)
        {
            if ( ! std::regex_match(name, regexPattern) ) continue;
            if (triggerResults->accept(i)) {
                passTriggerSelection = true;
                break;
            }
        }
        // No need to look further if triggers clear
        if (passTriggerSelection)
        {
            histManager->fillHistograms("Overall", "Num_Events", 1, genWeight);
            break;
        }
    }

    for (size_t i = 0; i < triggerResults->size(); i++) {
        if (!triggerResults->accept(i)) continue;

        std::string name = triggerNames.triggerName(i);

        bool regexMatch = false;
        for (const auto& regexPattern : compiledTriggerPatterns) {
            if (std::regex_match(name, regexPattern)) {
                regexMatch = true;
                break;
            }
        }

        bool indexMatch = false;
        for (const auto& t : triggerPaths_) {
            for (auto idx : triggerIndices_[t]) {
                if (idx == i) indexMatch = true;
            }
        }

        if (indexMatch && !regexMatch) {
            std::cout << "MISSED BY REGEX: " << name << std::endl;
        }
    }

    /* ********************************************************
         _____                 _                          
        | ____|_   _____ _ __ | |_                        
        |  _| \ \ / / _ \ '_ \| __|                       
        | |___ \ V /  __/ | | | |_                        
        |_____| \_/ \___|_| |_|\__|                       
         _  ___                            _   _          
        | |/ (_)_ __   ___ _ __ ___   __ _| |_(_) ___ ___ 
        | ' /| | '_ \ / _ \ '_ ` _ \ / _` | __| |/ __/ __|
        | . \| | | | |  __/ | | | | | (_| | |_| | (__\__ \
        |_|\_\_|_| |_|\___|_| |_| |_|\__,_|\__|_|\___|___/
 
       ********************************************************
    */

    if (passTriggerSelection){
    
    // Get DeepCSV b-tag collectio
    edm::Handle<reco::JetTagCollection> deepCSV_probb;
    edm::Handle<reco::JetTagCollection> deepCSV_probbb;

    iEvent.getByToken(deepCSV_probb_Token_, deepCSV_probb);
    iEvent.getByToken(deepCSV_probbb_Token_, deepCSV_probbb);
    
    histManager->fillHistograms("Event_Kinematics", "num_PV", nVertices, genWeight);
    
    double ht = 0.0;
    const reco::PFJet* leadJet = nullptr;
    double maxPt = -1.0;
    int num_of_jets = 0;

    // DeepCSV recommended WP:
    // https://twiki.cern.ch/twiki/bin/view/CMS/BtagRecommendation#Recommendation_for_13_TeV_Data: 
    // Actual numbers from here: https://gitlab.cern.ch/groups/cms-btv/-/wikis/SFCampaigns/UL2018
    // Loose:   0.1208
    // Medium:  0.4168
    // Tight:   0.7665
    //const float deepCSV_WP = 0.4168; // Medium WP (UL 2018)
    //bool hasBJet = false;

    // AK4PFJetsCHS recommended cleaning criteria from twiki:
    // https://twiki.cern.ch/twiki/bin/view/CMS/JetID13TeVUL
    
    std::vector<reco::PFJet>::const_iterator itJet;
    for (itJet = (*jetCollection).begin(); itJet != (*jetCollection).end(); ++itJet) 
    {
        int jetIndex = std::distance(jetCollection->begin(), itJet);

        // 2017/18 UL requirements
        // This motivated by QCD 15GeV min pT sample
        if (itJet->pt() < 30)   continue;
        // --------------------------
        // Changing eta threshold to 2.4 instead of recommended 2.6 for DeepCSV
        //if (fabs(itJet->eta()) > 2.6) continue;
        if (fabs(itJet->eta()) > 2.4) continue;
        if (itJet->neutralHadronEnergyFraction() >= 0.90)   continue;
        if (itJet->neutralEmEnergyFraction()  >= 0.90)      continue;
        float NumConst = itJet->chargedMultiplicity() + itJet->neutralMultiplicity();
        if (NumConst <= 1)      continue;
        if (itJet->muonEnergyFraction() >= 0.80)        continue;
        if (itJet->chargedHadronEnergyFraction()  <= 0) continue;
        if (itJet->chargedMultiplicity()  <= 0)         continue;
        if (itJet->chargedEmEnergyFraction() >= 0.80)   continue;
        // -------------------------

        // =========================
        //  B-TAGGING
        // =========================

        reco::PFJetRef jetRef(jetCollection, jetIndex);
        edm::RefToBase<reco::Jet> jetRefBase(jetRef);

        float probb  = (*deepCSV_probb)[jetRefBase];
        float probbb = (*deepCSV_probbb)[jetRefBase]; 

        float deepCSV = probb + probbb;

        // Apply Medium WP
        //if (deepCSV > deepCSV_WP) {
        //    hasBJet = true;
        //}

        num_of_jets += 1;
        histManager->fillHistograms("Event_Kinematics",
                                    "All_jet_Pt",
                                    itJet->pt(), 
                                    genWeight);
        histManager->fillHistograms("Event_Kinematics",
                                    "jetPt_Vs_DeepCSV",
                                    itJet->pt(), 
                                    deepCSV,
                                    genWeight);
        ht += itJet->pt();
        if (itJet->pt() > maxPt) 
        {
            maxPt = itJet->pt();
            leadJet = &(*itJet);
        }
    }
    
    if (leadJet){
        histManager->fillHistograms("Event_Kinematics",
                                    "Lead_jet_Pt", leadJet->pt(), genWeight);
        histManager->fillHistograms("Event_Kinematics","HT", ht, genWeight);
        histManager->fillHistograms("Event_Kinematics","Num_of_jets", 
                                                        num_of_jets, genWeight);
    }


    const reco::MET met = (*metCollection).front(); // Getting the first element of the vector
    histManager->fillHistograms("Event_Kinematics","MET", met.pt(), genWeight);

    // REMOVIGN BJET VETO
    //if (hasBJet) {
        //std::cout<<"Event has bjet! rejected!\n";
    //    return;
    //}
    
    } // Pass trigger selection - to match data

    /* ********************************************************
        ____                   _  _      _         _        
       / ___| __ _  _ __    __| |(_)  __| |  __ _ | |_  ___ 
      | |    / _` || '_ \  / _` || | / _` | / _` || __|/ _ \
      | |___| (_| || | | || (_| || || (_| || (_| || |_|  __/
       \____|\__,_||_| |_| \__,_||_| \__,_| \__,_| \__|\___|
       ____         _              _    _                   
      / ___|   ___ | |  ___   ___ | |_ (_)  ___   _ __      
      \___ \  / _ \| | / _ \ / __|| __|| | / _ \ | '_ \
       ___) ||  __/| ||  __/| (__ | |_ | || (_) || | | |    
      |____/  \___||_| \___| \___| \__||_| \___/ |_| |_|    
                                                             
       ********************************************************
    */

    bool passLowPt = passLowPtElectronSelection(iEvent);

    histManager->fillHistograms("Overall", "CutFlow_event", 
                    cutFlow_enum::events+1, genWeight); 
    histManager->fillHistograms("Overall", "Num_Events", 0, genWeight);

    std::vector<reco::Track> candTrks_b4PS, candTrks;
    
    int     Num_candidates_preSel   =   0   ;
    int     Num_candidates_postSel  =   0   ;
    // To keep track for event cutflow     
    // doesn't matter if we store 11001 over 11000 as we can loop over 
    // and fill 1 till first 0 and then just break
    uint16_t largest_bitmask        =   0   ;
    uint16_t largest_bitmask_noTrig =   0   ;
    uint16_t largest_bitmask_SR     =   0   ;
    // this part is to track allTracks and technical for event cutFlow
    int firstTwo                    =   0   ;
    // This is bitstring for when all conditions have passed
    uint16_t allPass = (1 << trackCuts.size()) - 1;
    uint16_t preselPass = (1 << (trackCuts.size()-1) ) - 1;
    
    // CandSel track cuts
    float   cand_trk_pT_cut     =   5       ;
    float   cand_trk_chi2_cut   =   20      ;
    int     cand_trk_hits_cut   =   3       ;
    
    // CandSel ecal cuts
    float   cand_ecal_maxE_cut       =   2   ;
    // Removed timing cut and energy error
    //float   cand_ecal_maxE_error_cut =   0.5 ;
    
    // Removed timing cut and energy error

    //float   cand_ecal_T_cut          =   1   ;
    //float   cand_ecal_T_cut          =   0.5   ;
    //float   cand_ecal_T_cut          =   0.25   ;
    //float   cand_ecal_T_cut          =   0   ;
    //float   ecal_5x5_cut    =   5;

    // for dedx measurements without cluster cleaning
    string year = "";
    float dedxSF[] = {1.0, 1.0325};
    TH3* templateHisto = nullptr;
    bool usePixel = false;
    bool useStrip = true;
    bool useClusterCleaning = false;

    std::vector<Candidates> cands, cands_b4PS, cands_onlyPS;

    int pos = -1;
    for (const auto& track : *tracks)
    {
        pos++;
        // Cut on track pT and eta
        if (track.pt() < cand_trk_pT_cut)                   continue;
        if (track.found() < cand_trk_hits_cut)              continue;
        if (track.chi2()/track.ndof() > cand_trk_chi2_cut)  continue;
        
        // To store values of variables to be used in no selection/n-1/preselection plots
        std::map<std::string, double> SelectionValues;      
        TrackDetMatchInfo info = trackAssociator_.associate(
                                    iEvent, 
                                    iSetup, 
                                    trackAssociator_.getFreeTrajectoryState(iSetup, track),
                                    trackAssociatorParams_);
  
        // If no associated rechits - no candidate selection
        if (info.ecalRecHits.size() == 0) continue;

        DetId maxDep = info.findMaxDeposition(TrackDetMatchInfo::EcalRecHits);
        float maxDep_E      = -999;
        float maxDep_EErr   = -999;
        float maxDep_time   = -999;
        EBDetId maxDep_E_detid;
        const EcalRecHit* candRechit; 

        bool flag_ecalSelection = 0;
        for (auto recHitItr : info.ecalRecHits)
        {
            EcalRecHit hit = *recHitItr;
            EBDetId det = hit.id();

            if (det.rawId() != maxDep.rawId()) continue;
            
            maxDep_E        = hit.energy();
            maxDep_EErr     = hit.energyError();
            maxDep_time     = hit.time();
            maxDep_E_detid  = det;

            if ( (maxDep_E > cand_ecal_maxE_cut) && 
                    //(hit.energyError() < cand_ecal_maxE_error_cut) && 
                    (hit.isTimeErrorValid()) )// && (maxDep_time > cand_ecal_T_cut) ) 
                flag_ecalSelection = 1;
            
            candRechit = &(*recHitItr);
            break;
        }

        // In case ecal thgresholds aren't cleared
        if (!flag_ecalSelection) continue;
        
        candTrks_b4PS.push_back(track);
        
        float t_pt      = track.pt();
        float t_pt_err  = track.ptError();
       
        const reco::HitPattern& hp = track.hitPattern();
        int pix_barrel_hits = 0;
        int pix_endcap_hits = 0;
        for (int i = 0; i < hp.numberOfAllHits(reco::HitPattern::TRACK_HITS); i++)
        {
            uint32_t hit = hp.getHitPattern(reco::HitPattern::HitCategory::TRACK_HITS, i);
            if (!hp.validHitFilter(hit)) continue;
            if (hp.pixelBarrelHitFilter(hit) && hp.getLayer(hit)>1) pix_barrel_hits += 1;
            if (hp.pixelEndcapHitFilter(hit)) pix_endcap_hits += 1;
            
        }
        
        histManager->fillHistograms("Overall", "CutFlow_candidate", 
                    cutFlow_enum::allTracks, genWeight ); 
            
        // Delta R b/w Max E and track pos at ECAL 
        float EBEta = barrelGeometry->getGeometry(maxDep_E_detid)->getPosition().eta();
        float EBPhi = barrelGeometry->getGeometry(maxDep_E_detid)->getPosition().phi();
        float maxDep_dist = 1.290 * cosh(EBEta); // REcal * cosh (eta)
        math::XYZPoint trackPos = info.trkGlobPosAtEcal;
        double deltaR_ECAL = reco::deltaR(trackPos.eta(), trackPos.phi(), EBEta, EBPhi);

        // tSig = tSM + t_atEcal
        // beta = v/c = tSM/tSig = 1/ (1 + t_atEcal/tSM)
        float beta = 1 / (1 + maxDep_time/( maxDep_dist * 10 /3 )); // [ns] units
            
        if (passTriggerSelection){
            histManager->fillHistograms("Vars_Candidate_b4PS", "beta",
                        beta, genWeight);
            histManager->fillHistograms("Vars_Candidate_b4PS", "noL1_pixB_hits",
                        pix_barrel_hits, genWeight);
            histManager->fillHistograms("Vars_Candidate_b4PS", "pixE_hits",
                        pix_endcap_hits, genWeight);
            histManager->fillHistograms("Vars_Candidate_b4PS", "all_pix_hits",
                        pix_barrel_hits+pix_endcap_hits, genWeight);
            
            // 3x3 energy around max E xtal
            histManager->fillHistograms("Vars_Candidate_b4PS", "Ecal_maxE", 
                        maxDep_E, genWeight);
            
            histManager->fillHistograms("Vars_Candidate_b4PS", "Ecal_maxE_V_EErr", 
                        maxDep_E, maxDep_EErr, genWeight);
            
            histManager->fillHistograms("Vars_Candidate_b4PS", "Ecal_maxE_3x3",
                        info.nXnEnergy(maxDep_E_detid, TrackDetMatchInfo::EcalRecHits, 1),
                        genWeight);
            
            // Time for max E xtal
            histManager->fillHistograms("Vars_Candidate_b4PS", "Ecal_maxE_time", 
                        maxDep_time, genWeight);
            
            histManager->fillHistograms("Vars_Candidate_b4PS", "Ecal_maxE_dR", 
                                                        deltaR_ECAL, genWeight);

            histManager->fillHistograms("Vars_Candidate_b4PS", "sigPt_V_pT_high",
                                                        t_pt, t_pt_err, genWeight);
            histManager->fillHistograms("Vars_Candidate_b4PS", "sigPt_V_pT",
                                                        t_pt, t_pt_err, genWeight);
            histManager->fillHistograms("Vars_Candidate_b4PS", "sigPt_V_pT_low",
                                                        t_pt, t_pt_err, genWeight);
        } // Passing trigger - to match data

        Num_candidates_preSel++; // Counting all tracks passing basic selection
        if (firstTwo == 0) firstTwo=1;
        
        if (passTriggerSelection){ 
            cands_b4PS.push_back({t_pt, (float)track.eta(), (float)track.phi(),
                                    (int)track.charge(), candRechit});
        }

        const reco::TrackRef trackRef = reco::TrackRef(tracks, pos);
        reco::DeDxHitInfoRef dedxHitsRef = dedxCollH->get(trackRef.key());
        if (dedxHitsRef.isNull()) continue;
        histManager->fillHistograms("Overall", "CutFlow_candidate", 
                    cutFlow_enum::technical, genWeight); 
        if (firstTwo == 1) firstTwo=2;

        const reco::DeDxHitInfo* dedxHits = &(*dedxHitsRef);
        
        // Find max sum pt^2 vertex
        reco::Vertex bestVertex;
        double maxSumPt2 = -1.0;
        for (const auto& vertex : *vertices) 
        {
            if (vertex.isFake() || !vertex.isValid()) continue;
            
            double sumPt2 = 0.0;
            for (reco::Vertex::trackRef_iterator it = vertex.tracks_begin(); 
                it != vertex.tracks_end(); it++)
            {
                double pt = (**it).pt();
                if ((**it).ptError() > pt) continue;
                sumPt2 += pt * pt;
            }
            
            if (sumPt2 > maxSumPt2) 
            {
                maxSumPt2 = sumPt2;
                bestVertex = vertex;
            }
        }
        
        reco::DeDxData temp   = computedEdx(run_, year, dedxHits, dedxSF, templateHisto, 
                            usePixel, useStrip, useClusterCleaning);
        
        reco::DeDxData t_Ias  = computedEdx(run_, year, dedxHits, dedxSF, dEdxTemplates, 
                            usePixel, useStrip, useClusterCleaning);
        
        //reco::DeDxData t_ProbQ= computedEdx(run_, year, dedxHits, dedxSF, dEdxTemplates, usePixel, 
        //                    useStrip, useClusterCleaning, skip_templates_ias = 0, 
        //                    symmetricSmirnov = false, useMorrisMethod = true);
        
        SelectionValues["trigger"]          = static_cast<float>(passTriggerSelection);
        SelectionValues["pt"]               = t_pt;
        SelectionValues["sigPtOPt"]         = t_pt_err / t_pt;
        SelectionValues["sigPtOPt2"]        = t_pt_err / (t_pt * t_pt);
        SelectionValues["validHitsFrac"]    = track.validFraction();
        SelectionValues["eta"]              = track.eta();
        SelectionValues["dxy"]              = track.dxy(bestVertex.position());
        SelectionValues["dz"]               = track.dz(bestVertex.position());
        SelectionValues["dEdxHits"]         = dedxHits->size();
        SelectionValues["highPurity"]       = track.quality( reco::TrackBase::highPurity );
        SelectionValues["Chi2Ondof"]        = track.chi2()/track.ndof();
        SelectionValues["trackPtIso"]       = trackIsolation(*tracks, track);
        SelectionValues["Ih"]               = temp.dEdx();
        SelectionValues["lowPtEle"]         = passLowPt;
        
        // To store selection and fill no-preselection
        uint16_t selectionBitmask = 0;        
        uint16_t signalBitmask = 0;        
        
        for (size_t i =0; i<trackCuts.size(); i++)
        {
            const std::string& varName = trackCuts[i].name;
            
            float value = SelectionValues[varName]; 
            float cutValue = trackCuts[i].cutValue;          

            if (trackCuts[i].isMinCut){
                if (value >= cutValue) selectionBitmask |= (1<<i); //  setting ith bit to 1
            }
            else
            {
                if (abs(value) <= cutValue) selectionBitmask |= (1<<i); // setting ith bit 1
            }
            
            if (passTriggerSelection){ // to match data - background
                histManager->fillHistograms("Preselection_No", varName, value, genWeight); 
            }
        }

        for (size_t i = 0; i<signalCuts.size(); i++)
        {
            if ( (maxDep_E > signalCuts[i].energy_cut) && 
                    (maxDep_time > signalCuts[i].time_cut)) 
                signalBitmask |= (1<<i);
        }

        if (selectionBitmask > largest_bitmask) largest_bitmask = selectionBitmask;
        
        if (largest_bitmask_noTrig < (selectionBitmask & ~(1<<ignore_trig_bit_pos)) )  
            largest_bitmask_noTrig = selectionBitmask;
        // store candidates that pass presel, no trigger selection requirement
        if ( preselPass == (selectionBitmask & ~(1<<ignore_trig_bit_pos)) ){
            cands_onlyPS.push_back({t_pt, (float)track.eta(), (float)track.phi(), 
                                (int)track.charge(), candRechit});
        }
        
        // If all bits are turned on, fill presel AND N-1
        if (selectionBitmask == allPass)
        {
            candTrks.push_back(track);
            Num_candidates_postSel++; // Counting all tracks passing all pre selection
            cands.push_back({t_pt, (float)track.eta(), (float)track.phi(), 
                                (int)track.charge(), candRechit});

            for (size_t i =0; i<trackCuts.size(); i++)
            {
                const std::string& varName = trackCuts[i].name;
                histManager->fillHistograms("Preselection", varName, 
                                        SelectionValues[varName], genWeight);
                histManager->fillHistograms("Preselection_Nm1", varName, 
                                        SelectionValues[varName], genWeight);
            }
            
            if (t_Ias.dEdx()<0.5){
                histManager->fillHistograms("Overall", "time_V_eta", 
                            track.eta(), candRechit->time(), genWeight);
                histManager->fillHistograms("Overall", "time_V_pT", 
                            t_pt, candRechit->time(), genWeight);
            }
            
            if (!isDATA_ || t_Ias.dEdx()<0.5 || 1./beta<1.15){
                
                histManager->fillHistograms("Overall", "Ih_V_pT", 
                            t_pt, temp.dEdx(), genWeight);
                histManager->fillHistograms("Overall", "Ih_V_ToF", 
                            temp.dEdx(), maxDep_time, genWeight);
                histManager->fillHistograms("Overall", "Ih_V_Beta", 
                            beta, temp.dEdx(), genWeight);
            
                histManager->fillHistograms("Overall", "Ias_V_Beta", 
                            beta, t_Ias.dEdx(), genWeight);
                histManager->fillHistograms("Overall", "Ias_V_InvBeta", 
                            1./beta, t_Ias.dEdx(), genWeight);
                
                //histManager->fillHistograms("Overall", "ProbQ_V_Beta", 
                //            beta, t_ProbQ.dEdx(), genWeight);
                histManager->fillHistograms("Overall", "InvIh_V_Beta", 
                            beta, 3.5/temp.dEdx(), genWeight);
            }

            histManager->fillHistograms("Vars_Candidate", "beta",
                        beta, genWeight);
            // Pixel hits
            histManager->fillHistograms("Vars_Candidate", "noL1_pixB_hits",
                        pix_barrel_hits, genWeight);
            histManager->fillHistograms("Vars_Candidate", "pixE_hits",
                        pix_endcap_hits, genWeight);
            histManager->fillHistograms("Vars_Candidate", "all_pix_hits",
                        pix_barrel_hits+pix_endcap_hits, genWeight);
            
            // 3x3 energy around max E xtal
            histManager->fillHistograms("Vars_Candidate", "Ecal_maxE", maxDep_E, genWeight);
            
            histManager->fillHistograms("Vars_Candidate", "Ecal_maxE_V_EErr", 
                        maxDep_E, maxDep_EErr, genWeight);
        
            histManager->fillHistograms("Vars_Candidate", "Ecal_maxE_3x3",
                        info.nXnEnergy(maxDep_E_detid, TrackDetMatchInfo::EcalRecHits, 1),
                        genWeight);
            
            // Time for max E xtal
            histManager->fillHistograms("Vars_Candidate", "Ecal_maxE_time", maxDep_time,
                        genWeight);
            
            // Delta R b/w Max E and track pos at ECAL 
            histManager->fillHistograms("Vars_Candidate", "Ecal_maxE_dR", deltaR_ECAL,
                        genWeight);

            histManager->fillHistograms("Vars_Candidate", "sigPt_V_pT_high",
                                                        t_pt, t_pt_err, genWeight);
            histManager->fillHistograms("Vars_Candidate", "sigPt_V_pT",
                                                        t_pt, t_pt_err, genWeight);
            histManager->fillHistograms("Vars_Candidate", "sigPt_V_pT_low",
                                                        t_pt, t_pt_err, genWeight);
        }
        // For only N-1 selection
        else 
        {
            for (size_t i =0; i<trackCuts.size(); i++)
            {   // Or individual pass with ith bit 1
                const std::string& varName = trackCuts[i].name;
                if (allPass == ((1<<i) | selectionBitmask))
                {
                    histManager->fillHistograms("Preselection_Nm1", varName, 
                                            SelectionValues[varName], genWeight);
                }
            }
        }

        // Filling rest of the CutFlow
        for (int i = trackCuts.size()-1; i >= 0; --i)
        {
            if (trackCuts[i].enumVal == cutFlow_enum::count) continue;
            if (selectionBitmask != ((1<<i) | selectionBitmask) ) break;
            histManager->fillHistograms("Overall", "CutFlow_candidate", 
                                                        trackCuts[i].enumVal, genWeight); 
        }
        // Filling signal region cutflow only when all preselection passes
        if (selectionBitmask != allPass) continue;
        if (signalBitmask > largest_bitmask_SR) largest_bitmask_SR = signalBitmask;
        for (size_t i = 0; i<signalCuts.size(); i++)
        {
            if (signalBitmask != ((1<<i) | signalBitmask)) continue;
            histManager->fillHistograms("Overall", "CutFlow_candidate",
                                                        signalCuts[i].enumVal, genWeight);
        }
    } // END - for loop on tracks
    
    // Filling out event CutFlow
    switch(firstTwo) {
        case 0: 
            break;
        case 1: 
            histManager->fillHistograms("Overall", "Num_Events", 2, genWeight);
            histManager->fillHistograms("Overall", "CutFlow_event", 
                            cutFlow_enum::allTracks+1, genWeight); 
            break;
        case 2:
            histManager->fillHistograms("Overall", "Num_Events", 2, genWeight);
            histManager->fillHistograms("Overall", "CutFlow_event", 
                            cutFlow_enum::allTracks+1, genWeight); 
            histManager->fillHistograms("Overall", "CutFlow_event", 
                            cutFlow_enum::technical+1, genWeight); 
            
            if (largest_bitmask == allPass)
            {
                histManager->fillHistograms("Overall", "Num_Events", 3, genWeight);
                histManager->fillHistograms("Overall", "Num_Events", 4, genWeight);
            }
            else{
                //if ( (largest_bitmask ^ allPass) == (1<<0) )
                if (largest_bitmask_noTrig == preselPass)
                histManager->fillHistograms("Overall", "Num_Events", 3, genWeight);
            }

            
            for (int i = trackCuts.size()-1; i >= 0; --i)
            { 
                // Exits after hitting the first 0
                if (largest_bitmask != ((1<<i) | largest_bitmask)) break;
                histManager->fillHistograms("Overall", "CutFlow_event",
                            trackCuts[i].enumVal+1, genWeight); 
            }

            for (size_t i=0; i<signalCuts.size(); i++)
            {
                if (largest_bitmask_SR != ((1<<i) | largest_bitmask_SR)) continue;
                histManager->fillHistograms("Overall", "CutFlow_event",
                            signalCuts[i].enumVal+1, genWeight);
            }
            
            break;
        default:
            std::cout<<"ERROR: value of 'firstTwo' (0/1/2) set to: "<<firstTwo<<std::endl;
    }

    if (cands_onlyPS.size()>0){
        std::sort(cands_onlyPS.begin(), 
                    cands_onlyPS.end(), 
                    [](const Candidates& a, const Candidates& b) {
            return a.pt > b.pt;
        });

        std::vector<Candidates> cands_energySorted = cands_onlyPS;

        std::sort(cands_energySorted.begin(),
                  cands_energySorted.end(),
                  [](const Candidates& a, const Candidates& b) {
            float ea = a.rechit ? a.rechit->energy() : -1.0f;
            float eb = b.rechit ? b.rechit->energy() : -1.0f;
            return ea > eb;
        });

        triggerStudy(*triggerResults, 
                        cands_onlyPS[0].pt, 
                        cands_energySorted[0].rechit->energy(), 
                        passTriggerSelection, 
                        genWeight
        );
        mu50OrthogonalStudy(iEvent, 
                        cands_onlyPS, 
                        cands_energySorted[0].rechit->energy(), 
                        *triggerResults, 
                        *vertices, 
                        genWeight
        );

    }
    
    if (Num_candidates_postSel){
        ecalTimeRecoStudy(cands, *EBRecHits, genWeight);
    }
    
    if (Num_candidates_postSel >= 2){
        std::sort(cands.begin(), cands.end(), [](const Candidates& a, const Candidates& b) {
            return a.pt > b.pt;
        });
        
        const Candidates& cand1 = cands[0];
        Candidates cand2;
        bool found = false;

        for (size_t i = 1; i < cands.size(); ++i) {
            if (cands[i].charge == -(cand1.charge) ) {
                cand2 = cands[i];
                found = true;
                break;
            }
        }
        
        if (found){
            //float d_phi = reco::deltaPhi(cand1.phi, cand2.phi);
            //float d_eta = cand1.eta - cand2.eta;

            //float inv_mass = std::sqrt( 2 * cand1.pt * cand2.pt * (
            //                            std::cosh( d_eta ) -
            //                            std::cos(  d_phi ) ));

            double inv_mass = diCandMass(cand1, cand2);
            histManager->fillHistograms("Mass_Plots", "MaxPt_Invariant_Mass",
                                inv_mass, genWeight);
            histManager->fillHistograms("Mass_Plots", "MaxPt_Invariant_Mass_0to20",
                                inv_mass, genWeight);
        }

        for (size_t i = 0; i < cands_b4PS.size(); ++i) 
        {
            for (size_t j = i + 1; j < cands_b4PS.size(); ++j) 
            {
                const Candidates& ci = cands_b4PS[i];
                const Candidates& cj = cands_b4PS[j];

                double inv_mass = diCandMass(ci, cj);

                // Fill all pairs
                histManager->fillHistograms("Mass_Plots", "AllCand_Invariant_Mass", 
                                                inv_mass, genWeight);
                histManager->fillHistograms("Mass_Plots", "AllCand_Invariant_Mass_0to20", 
                                                inv_mass, genWeight);

                if (ci.charge * cj.charge < 0) { // Opposite sign
                    histManager->fillHistograms("Mass_Plots", "All_OS_Invariant_Mass", 
                                                inv_mass, genWeight);
                    histManager->fillHistograms("Mass_Plots", "All_OS_Invariant_Mass_0to20", 
                                                inv_mass, genWeight);
                } else if (ci.charge * cj.charge > 0) { // Same sign
                    histManager->fillHistograms("Mass_Plots", "All_SS_Invariant_Mass", 
                                                inv_mass, genWeight);
                    histManager->fillHistograms("Mass_Plots", "All_SS_Invariant_Mass_0to20", 
                                                inv_mass, genWeight);
                }
            }
        }
    }

    if ( cands_b4PS.size() >= 2){
        std::sort(cands_b4PS.begin(), cands_b4PS.end(), 
                    [](const Candidates& a, const Candidates& b) {
            return a.pt > b.pt;
        });
        
        const Candidates& cand1 = cands_b4PS[0];
        Candidates cand2;
        bool found = false;

        for (size_t i = 1; i < cands_b4PS.size(); ++i) {
            if (cands_b4PS[i].charge == -(cand1.charge) ) {
                cand2 = cands_b4PS[i];
                found = true;
                break;
            }
        }
        
        if (found){
            //float d_phi = reco::deltaPhi(cand1.phi, cand2.phi);
            //float d_eta = cand1.eta - cand2.eta;
            //
            //float inv_mass = std::sqrt( 2 * cand1.pt * cand2.pt * (
            //                            std::cosh( d_eta ) -
            //                            std::cos(  d_phi ) ));
            double inv_mass = diCandMass(cand1, cand2);
            histManager->fillHistograms("Mass_Plots_b4PS", "MaxPt_Invariant_Mass",
                                inv_mass, genWeight);
            histManager->fillHistograms("Mass_Plots_b4PS", "MaxPt_Invariant_Mass_0to20",
                                inv_mass, genWeight);
        }

        for (size_t i = 0; i < cands_b4PS.size(); ++i) 
        {
            for (size_t j = i + 1; j < cands_b4PS.size(); ++j) 
            {
                const Candidates& ci = cands_b4PS[i];
                const Candidates& cj = cands_b4PS[j];

                double inv_mass = diCandMass(ci, cj);

                // Fill all pairs
                histManager->fillHistograms("Mass_Plots_b4PS", 
                                            "AllCand_Invariant_Mass", 
                                            inv_mass, genWeight);
                histManager->fillHistograms("Mass_Plots_b4PS", 
                                            "AllCand_Invariant_Mass_0to20", 
                                            inv_mass, genWeight);

                if (ci.charge * cj.charge < 0) { // Opposite sign
                    histManager->fillHistograms("Mass_Plots_b4PS", 
                                                "All_OS_Invariant_Mass", 
                                                inv_mass, genWeight);
                    histManager->fillHistograms("Mass_Plots_b4PS", 
                                                "All_OS_Invariant_Mass_0to20", 
                                                inv_mass, genWeight);
                } else if (ci.charge * cj.charge > 0) { // Same sign
                    histManager->fillHistograms("Mass_Plots_b4PS", 
                                                "All_SS_Invariant_Mass", 
                                                inv_mass, genWeight);
                    histManager->fillHistograms("Mass_Plots_b4PS", 
                                                "All_SS_Invariant_Mass_0to20", 
                                                inv_mass, genWeight);
                }
            }
        }
    }


    // Calculate Track pt isolation and fill hists
    std::map<const reco::Track*, float> isolationTrk_b4PS = trackIsolation(iEvent, 
                                                                    candTrks_b4PS);
    std::map<const reco::Track*, float> isolationTrk      = trackIsolation(iEvent, 
                                                                    candTrks);

    for (auto const mapIsoTrk_b4PS : isolationTrk_b4PS){
        histManager->fillHistograms("Vars_Candidate_b4PS", "candPt_vs_isolation",
                            mapIsoTrk_b4PS.first->pt(),
                            mapIsoTrk_b4PS.second,
                            genWeight ); 
    }

    for (auto const mapIsoTrk : isolationTrk){
        histManager->fillHistograms("Vars_Candidate", "candPt_vs_isolation",
                            mapIsoTrk.first->pt(),
                            mapIsoTrk.second,
                            genWeight ) ;
    }

    histManager->fillHistograms("Overall", "Num_of_cand_noSel", 
                            Num_candidates_preSel, genWeight); 
    histManager->fillHistograms("Overall", "Num_of_cand_postSel", 
                            Num_candidates_postSel, genWeight); 

}


// ------------ method called once each job just before starting event loop  ------------
void
mchampAnalyzer::beginJob()
{
}

// ------------ method called once each job just after ending the event loop  ------------
void
mchampAnalyzer::endJob()
{
    // Scale all histograms by (1 / sum of gen_weights)
    if (sum_gen_weights == 0) sum_gen_weights = 1;
    histManager->fillHistograms("Overall", "Total_Gen_Weight", 1, sum_gen_weights);
    //histManager->scaleAllHistograms( 1/sum_gen_weights );
}

// ----------- method to calculate di candidate mass -------------------------------------
double
mchampAnalyzer::diCandMass(const Candidates &cand1, const Candidates &cand2)
{
    float d_phi = reco::deltaPhi(cand1.phi, cand2.phi);
    float d_eta = cand1.eta - cand2.eta;
    
    float inv_mass = std::sqrt( 2 * cand1.pt * cand2.pt * (
                                std::cosh( d_eta ) -
                                std::cos(  d_phi ) ));
    
    return(inv_mass);
}

bool 
mchampAnalyzer::passLowPtElectronSelection(
                const edm::Event& iEvent )
{
    // Get the low pt selection
    edm::Handle<reco::GsfElectronCollection> lowPtEles;
    iEvent.getByToken(lowPtEleToken_, lowPtEles);

    // lowPt score
    edm::Handle<edm::ValueMap<float>> lowPtScore;
    bool haveLowPtScore = iEvent.getByToken(lowPtScoreToken_, lowPtScore) && lowPtScore.isValid();
    
    bool passCondition = false;

    if (lowPtEles.isValid()) {
        for (size_t i = 0; i < lowPtEles->size(); ++i) {
            const auto &ele = lowPtEles->at(i);

            if (std::abs(ele.eta()) > 1.4) continue;
            if (ele.full5x5_sigmaIetaIeta() > 0.03) continue;
            if (ele.gsfTrack().isNonnull()) {
                if (ele.gsfTrack()->normalizedChi2() > 0.5) continue;
            }
            else continue;
            if (ele.dr03TkSumPt() > 10) continue;
            if (ele.dr03EcalRecHitSumEt() > 10) continue;
            if (ele.dr03HcalTowerSumEt() > 25) continue;
            if (haveLowPtScore) {
                edm::Ref<reco::GsfElectronCollection> eleRef(lowPtEles, i);
                edm::RefToBase<reco::GsfElectron> eleRefToBase(eleRef);
                if ( ((*lowPtScore)[eleRefToBase] < -3) || ((*lowPtScore)[eleRefToBase] > 1)) continue;
            }
            else continue;
            
            // If it passes all above!
            passCondition = true; break;
        }
    }
    return(passCondition);
}

// ----------- method to calculate track isolation for a track ---------------------------
double
mchampAnalyzer::trackIsolation(
                reco::TrackCollection const &tracks,
                reco::Track const &cand_track )
{
    double sum_pt = 0;
    const float eta_cand = cand_track.eta();
    const float phi_cand = cand_track.phi();
    
    for (const auto& track : tracks){
        if (std::abs(track.eta() - eta_cand) > 0.3) continue;
        if (std::abs(reco::deltaPhi(track.phi(), phi_cand)) > 0.3) continue;

        if ( reco::deltaR(cand_track.eta(), cand_track.phi(), 
                            track.eta(), track.phi()) < 0.3 )
            sum_pt += track.pt(); 
    }
    sum_pt -= cand_track.pt();
    return(sum_pt);
}

// ----------- method to calculate track isolation for a vector of tracks ----------------
std::map<const reco::Track*, float> 
mchampAnalyzer::trackIsolation(
                const edm::Event& iEvent,
                std::vector<reco::Track> const &trk_list )
{
    // Get Tracks
    edm::Handle<reco::TrackCollection> tracks;
    iEvent.getByToken(tracksToken_, tracks);

    // Set-up map for track isolation
    std::map<const reco::Track*, float> isolation_map;
    
    for (const reco::Track& cand_track : trk_list){
        float sum_pt = 0;
        for (const reco::Track track : *tracks){
            if ( reco::deltaR(cand_track.eta(), cand_track.phi(), 
                                track.eta(), track.phi()) < 0.3 )
                sum_pt += track.pt(); 
        }
        
        sum_pt -= cand_track.pt();
        isolation_map[&cand_track] = sum_pt;
    }
    return(isolation_map);
}

// ----------------------------- Method for trigger study -------------------------------- 
void 
mchampAnalyzer::triggerStudy(
                const edm::TriggerResults& trigResults,
                double leadPt,
                double maxE,
                bool passTriggerSelection,
                double genWeight
) {
    //if (!preselPass) return;

    // --- check orthogonal triggers ---
    bool passOrtho = false;
    for (auto idx : triggerIndices_["HLT_PFMET120_PFMHT120_IDTight"]){
        if (trigResults.accept(idx)) { 
            passOrtho = true; 
            //std::string name = hltConfig_.triggerName(idx);
            //std::cout << "FIRED MET: " << name << std::endl;
            break; 
        }
    }
    if (!passOrtho){
        for (auto idx : triggerIndices_["HLT_PFMETNoMu120_PFMHTNoMu120_IDTight"]){
            if (trigResults.accept(idx)) { 
                passOrtho = true; 
                //std::string name = hltConfig_.triggerName(idx);
                //std::cout << "FIRED MET: " << name << std::endl;
                break; 
            }
        }
    }
    //for (auto idx : triggerIndices_["HLT_PFMETNoMu120_PFMHTNoMu120_IDTight"]){
    //    if (trigResults.accept(idx)) { 
    //        //passOrtho = true; 
    //        std::string name = hltConfig_.triggerName(idx);
    //        std::cout << "FIRED MET: " << name << std::endl;
    //        //break; 
    //    }
    //}
    //for (auto idx : triggerIndices_["HLT_MET150"]){
    //    if (trigResults.accept(idx)) { 
    //        //passOrtho = true; 
    //        std::string name = hltConfig_.triggerName(idx);
    //        std::cout << "FIRED MET: " << name << std::endl;
    //        //break; 
    //    }
    //}
    //for (auto idx : triggerIndices_["HLT_MonoCentralPFJet80"]){
    //    if (trigResults.accept(idx)) { 
    //        //passOrtho = true; 
    //        std::string name = hltConfig_.triggerName(idx);
    //        std::cout << "FIRED MET: " << name << std::endl;
    //        //break; 
    //    }
    //}

    //if (!passOrtho) {
    //for (auto idx : triggerIndices_["HLT_PFHT"]){
    //    if (trigResults.accept(idx)) { 
    //        passOrtho = true;
    //
    //        std::string name = hltConfig_.triggerName(idx);
    //        std::cout << "FIRED MHT: " << name << std::endl;
    //        //break; 
    //    }
    //}
    //}

    
    if (!passOrtho) return;
    
    triggerHists_["OR_ALL_Triggers"].second->Fill(leadPt, genWeight);
    triggerHists_E_["OR_ALL_Triggers"].second->Fill(maxE, genWeight);
    
    if (passTriggerSelection){  
        triggerHists_["OR_ALL_Triggers"].first->Fill(leadPt, genWeight);
        triggerHists_E_["OR_ALL_Triggers"].first->Fill(maxE, genWeight);
    }

    // --- loop over triggers of interest ---
    for (const auto& trigPattern : triggerPaths_) {
        bool trigFired = false;
        for (auto idx : triggerIndices_[trigPattern]) {
            if (trigResults.accept(idx)) { trigFired = true; break; }
        }

        // Fill
        triggerHists_[trigPattern].second->Fill(leadPt, genWeight);
        triggerHists_E_[trigPattern].second->Fill(maxE, genWeight);
        
        if (trigFired){ 
            triggerHists_[trigPattern].first->Fill(leadPt, genWeight);
            triggerHists_E_[trigPattern].first->Fill(maxE, genWeight);
        }
    }
}

// ---------------------- Method for ECAL time resolution study --------------------------- 

void 
mchampAnalyzer::ecalTimeRecoStudy(
    const std::vector<Candidates>& candidates,
    const EcalRecHitCollection& EBRecHits,
    //const CaloGeometry* geom,
    double genWeight
)
{
    std::map<const EcalRecHit*, const EcalRecHit*> rechitMap;

    for (const auto& cand : candidates) {

        const EcalRecHit* seedHit = cand.rechit;
        if (!seedHit) continue;

        EBDetId seedId(seedHit->detid());

        int ieta = seedId.ieta();
        int iphi = seedId.iphi();

        float A1 = seedHit->energy();
        if (A1 <= 0) continue;

        const EcalRecHit* bestNeighbor = nullptr;
        float bestMetric = 1e9;

        // --- loop over ±1 neighbors ---
        for (int deta = -1; deta <= 1; ++deta) {
            for (int dphi = -1; dphi <= 1; ++dphi) {

                //if (abs(deta+dphi) != 1) continue;
                if (deta == 0 && dphi == 0) continue;

                int newIeta = ieta + deta;
                int newIphi = iphi + dphi;

                // --- phi wrapping ---
                if (newIphi < 1) newIphi += 360;
                if (newIphi > 360) newIphi -= 360;

                // --- EB limits ---
                if (newIeta == 0) continue;
                if (std::abs(newIeta) > 85) continue;

                EBDetId neighId(newIeta, newIphi);

                auto it = EBRecHits.find(neighId);
                if (it == EBRecHits.end()) continue;

                const EcalRecHit* neighHit = &(*it);

                float A2 = neighHit->energy();
                if (A2 <= 0) continue;

                // --- your criterion: closest energy ---
                float metric = std::abs(A1 - A2);

                if (metric < bestMetric) {
                    bestMetric = metric;
                    bestNeighbor = neighHit;
                }
            }
        }

        if (!bestNeighbor) continue;

        rechitMap[seedHit] = bestNeighbor;
    }

    // loop over map and fill TH3F

    for (const auto& kv : rechitMap) {

        const EcalRecHit* h1 = kv.first;
        const EcalRecHit* h2 = kv.second;

        float A1 = h1->energy();
        float A2 = h2->energy();

        float t1 = h1->time();
        float t2 = h2->time();

        if (A1 < 0.5 || A2 < 0.5) continue;
        if (std::abs(t1) > 50 || std::abs(t2) > 50) continue;

        float deltaT = t1 - t2;

        float Aeff = (A1 * A2) / sqrt(A1*A1 + A2*A2);

        float ratio = A1 / A2;

        if (ratio > 10 || ratio < 0.1) continue;

        //h_ecalTimeRes->Fill(Aeff, deltaT, ratio, genWeight);
        ecalTimeResHists_["postSel"]->Fill(Aeff, deltaT, ratio, genWeight);
    }
}

void mchampAnalyzer::mu50OrthogonalStudy(
    const edm::Event& iEvent,
    const std::vector<Candidates>& candidates,
    double maxE,
    const edm::TriggerResults& trigResults,
    const reco::VertexCollection& vertices,
    double genWeight)
{
    if (vertices.empty()) return;
    const reco::Vertex& pv = vertices[0];

    // ============================================================
    // STEP 1: Check HLT_Mu50 fired
    // ============================================================
    bool passMu50 = false;

    for (auto idx : triggerIndices_["HLT_IsoMu24_v"]) {
    //for (auto idx : triggerIndices_["HLT_Mu50_v"]) {
        if (trigResults.accept(idx)) {
            passMu50 = true;
            break;
        }
    }

    if (!passMu50) return;

    // ============================================================
    // STEP 2: Get triggerEvent + muons from event
    // ============================================================
    edm::Handle<trigger::TriggerEvent> trigEvent;
    iEvent.getByToken(trigEventToken_, trigEvent);

    if (!trigEvent.isValid()) {
        return;
    }
    edm::Handle<reco::MuonCollection> muons;
    iEvent.getByToken(muonToken_, muons);

    if (!muons.isValid()) return;

    // ============================================================
    // STEP 3: Extract Mu50 trigger objects
    // ============================================================
    //std::vector<math::XYZTLorentzVector> trigObjs;
    std::vector<std::pair<float, float>> trigObjs;

    const trigger::TriggerObjectCollection& allObjs = trigEvent->getObjects();

    size_t filterIndex =
        trigEvent->filterIndex(edm::InputTag(mu50Filter_, "", "HLT"));

    if (filterIndex < trigEvent->sizeFilters()) {

        const trigger::Keys& keys = trigEvent->filterKeys(filterIndex);

        for (auto key : keys) {
            //trigObjs.push_back(allObjs[key].p4());
            const auto& obj = allObjs[key];
            trigObjs.emplace_back(obj.eta(), obj.phi());
        }
    }

    if (trigObjs.empty()) {
        return;
    }

    for (const auto& mu : *muons) {

        // --------------------------------------------------------
        // STEP 4: Match to trigger object
        // --------------------------------------------------------
        bool matchedToTrigger = false;

        for (const auto& obj : trigObjs) {
            if (reco::deltaR(mu.eta(), mu.phi(),
                             obj.first, obj.second) < 0.1) {
                matchedToTrigger = true;
                break;
            }
        }

        if (!matchedToTrigger) continue;

        // --------------------------------------------------------
        // STEP 5: Tight ID (official CMS)
        // --------------------------------------------------------
        if (!muon::isTightMuon(mu, pv)) continue;

        // --------------------------------------------------------
        // STEP 6: Remove overlap with candidates
        // --------------------------------------------------------
        bool overlaps = false;

        for (const auto& cand : candidates) {
            if (reco::deltaR(mu.eta(), mu.phi(),
                             cand.eta, cand.phi) < 0.1) {
                overlaps = true;
                break;
            }
        }

        if (overlaps) continue;

        // --------------------------------------------------------
        // STEP 7: Fill PASS / TOTAL
        // --------------------------------------------------------
        
        // already ordered before function call
        float pt = candidates[0].pt;
        bool passTriggerSelection = false;
        for (const auto& trigPattern : triggerPaths_) {
            bool trigFired = false;
            for (auto idx : triggerIndices_[trigPattern]) {
                if (trigResults.accept(idx)) {
                    trigFired = true;
                    passTriggerSelection = true;
                    break;
                }
            }

            // TOTAL
            triggerHists_mu_[trigPattern].second->Fill(pt, genWeight);
            triggerHists_mu_E_[trigPattern].second->Fill(maxE, genWeight);

            // PASS
            if (trigFired) {
                triggerHists_mu_[trigPattern].first->Fill(pt, genWeight);
                triggerHists_mu_E_[trigPattern].first->Fill(maxE, genWeight);
            }
        }
        triggerHists_mu_["OR_ALL_Triggers"].second->Fill(pt, genWeight);
        triggerHists_mu_E_["OR_ALL_Triggers"].second->Fill(maxE, genWeight);
        if (passTriggerSelection){  
            triggerHists_mu_["OR_ALL_Triggers"].first->Fill(pt, genWeight);
            triggerHists_mu_E_["OR_ALL_Triggers"].first->Fill(maxE, genWeight);
        }
    }
}


//define this as a plug-in
DEFINE_FWK_MODULE(mchampAnalyzer);
