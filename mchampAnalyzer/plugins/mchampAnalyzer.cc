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
	tracksToken_(consumes<reco::TrackCollection>(iConfig.getParameter<edm::InputTag>("tracks"))),
    ecalRecHitsToken_(consumes<EcalRecHitCollection>(iConfig.getParameter<edm::InputTag>("ecalRecHits"))),
    vertexToken_(consumes<reco::VertexCollection>(iConfig.getParameter<edm::InputTag>("offlinePV"))),
    pdgId_(iConfig.getParameter<int>("pdgId")),
    deltaRCutoff_tracks(iConfig.getParameter<double>("deltaRCutoff_tracks")),
    deltaRCutoff_EB(iConfig.getParameter<double>("deltaRCutoff_EB")),
	dedxToken_(consumes<reco::DeDxHitInfoAss>(iConfig.getParameter<edm::InputTag>("dedxHits"))),
	Ih2Token_(consumes<reco::DeDxDataCollection>(iConfig.getParameter<edm::InputTag>("Ih2Collection"))),
    triggerResultsToken_(consumes<edm::TriggerResults>(iConfig.getParameter<edm::InputTag>("triggerResults"))),
    triggerPaths_(iConfig.getParameter<std::vector<std::string>>("triggerPaths")),
    outputFileName_(iConfig.getParameter<std::string>("outputFile")),
    saveNtuple_(iConfig.getParameter<bool>("saveNtuple"))
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
                    

    if (saveNtuple_){
        tree_ = fs->make<TTree>("Ntuple", "Ntuple");
        tree_->Branch("Run", &run_, "Run/I");
        tree_->Branch("Event", &event_, "Event/I");
        tree_->Branch("GenPart.", &cls_genpart);
        tree_->Branch("Tracks.", &cls_tracks);
        tree_->Branch("EcalRecHits.", &cls_rechitsEcal);
        tree_->Branch("TrackAssoc.", &cls_trackAssoc);
    }

    histManager = std::make_unique<HistogramManager>(*fs);

    // Saving cutFlow cuts and values 
    // enum and structs defined in HistogramManager.h
    // histograms defined in HistogramManager.cc
    // format {std::string name, double cut, cutFlow_enum::Type, bool one_sided?  }
    trackCuts = {
        { "trigger",        1,      cutFlow_enum::triggers,     true    },
        { "pt",             15,     cutFlow_enum::pt,           true    },
        //{ "sigPtOPt",       0.25,   cutFlow_enum::count,        false   },
        { "eta",            1.4,    cutFlow_enum::eta,          false   },
        { "validHitsFrac",  0.8,    cutFlow_enum::fracValidHits,true    },
        { "dEdxHits",       10,     cutFlow_enum::numDedxHits,  true    },
        { "highPurity",     1,      cutFlow_enum::highPurity,   true    },
        { "Chi2Ondof",      5,      cutFlow_enum::chi2,         false   },
        { "dz",             0.5,    cutFlow_enum::dz,           false   },
        { "dxy",            0.5,    cutFlow_enum::dxy,          false   },
        { "Ih",             3,      cutFlow_enum::Ih,           true    },
        { "sigPtOPt2",      0.003,  cutFlow_enum::sigPtOPt2,    false   },
    };
    
    // format {std::string name, double timeCut, double energyCut, cutFlow_enum::Type }
    signalCuts = {
        {"SR",              2,          5,      cutFlow_enum::SR}
    };
    
    // Saving trigger regex
    compiledTriggerPatterns.reserve(triggerPaths_.size());

    for (const auto& pattern : triggerPaths_)
    {
        std::string regexPattern = std::regex_replace(pattern, std::regex("\\*"), ".*");
        // Need to use emplace_back instead of push_back
        compiledTriggerPatterns.emplace_back(regexPattern);
    }
}


mchampAnalyzer::~mchampAnalyzer()
{}


//
// member functions
//

// ------------ method called for each event  ------------
void
mchampAnalyzer::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup)
{
    run_ = iEvent.id().run();
    event_ = iEvent.id().event();
    
    // Get GenParticles
    edm::Handle<reco::GenParticleCollection> genParticles;
    iEvent.getByToken(genParticlesToken_, genParticles);

    // Get Tracks
    edm::Handle<reco::TrackCollection> tracks;
    iEvent.getByToken(tracksToken_, tracks);

    // Get Vertices
    edm::Handle<reco::VertexCollection> vertices;
    iEvent.getByToken(vertexToken_, vertices);
    
    // Get dedx collection
	edm::Handle<reco::DeDxHitInfoAss> dedxCollH = iEvent.getHandle(dedxToken_);
	//std::cout<<"dedxCollH size: "<<dedxCollH->size()<<"\n";
    
	// declaration for necessary stuff for compute-dedx()
		
	// Ih2 handle
	edm::Handle<reco::DeDxDataCollection> Ih2CollH = iEvent.getHandle(Ih2Token_);
    
    // Trigger Results
    edm::Handle<edm::TriggerResults> triggerResults;
    iEvent.getByToken(triggerResultsToken_, triggerResults);
    if (!triggerResults.isValid()){
        edm::LogWarning("mchampAnalyzer") << "TriggerResults not valid";
    }
    
    // Trigger names
    const edm::TriggerNames& triggerNames = iEvent.triggerNames(*triggerResults);
    
    // Get EcalRecHits
    edm::Handle<EcalRecHitCollection> EBRecHits;
    iEvent.getByToken(ecalRecHitsToken_, EBRecHits);

    // Geometry handle
	edm::ESHandle<CaloGeometry> geoHandle;
	iSetup.get<CaloGeometryRecord>().get(geoHandle);
	const CaloSubdetectorGeometry* barrelGeometry = geoHandle->getSubdetectorGeometry(DetId::Ecal, EcalBarrel);
   
    //  #### ##### #   #       #   #  ###  ##### ##### ##  #
    // ##    ##    ##  #       ## ## ##  #   #   ##    ##  #
    // ## ## ####  ### # ##### # # # ##  #   #   ##    #####
    // ##  # ##    # ###       #   # #####   #   ##    ##  #
    // ##  # ##    #  ##       #   # ##  #   #   ##    ##  #
    //  #### ##### #   #       #   # ##  #   #   ##### ##  #

    if (saveNtuple_){
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

                reco::DeDxData temp = computedEdx(run_, year, dedxHits, dedxSF, templateHisto, usePixel, 
                                    useStrip, useClusterCleaning);
                
                cls_tracks->dedx.push_back(temp.dEdx());
                cls_tracks->numOfSatStrips.push_back(temp.numberOfSaturatedMeasurements());
                cls_tracks->numOfStrips.push_back(temp.numberOfMeasurements());
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
    

    // ##### ####  #####  ####  #### ##### ####  #####
    //   #   ##  #   #   ##    ##    ##    ##  # ##   
    //   #   #####   #   ## ## ## ## ####  #####  ##  
    //   #   ##  #   #   ##  # ##  # ##    ##  #   ## 
    //   #   ##  #   #   ##  # ##  # ##    ##  #    ##
    //   #   ##  # #####  ####  #### ##### ##  # #####

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
        if (passTriggerSelection) break;
    }
    
    // #####  ###  #   # ####  ##### ####   ###  ##### #####
    // ##    ##  # ##  # ##  #   #   ##  # ##  #   #   ##
    // ##    ##  # ### # ##  #   #   ##  # ##  #   #   ####
    // ##    ##### # ### ##  #   #   ##  # #####   #   ##
    // ##    ##  # #  ## ##  #   #   ##  # ##  #   #   ##
    // ##### ##  # #   # ####  ##### ####  ##  #   #   #####

    // ##### ##### ##    ##### ##### ##### #####  ###  #   #
    // ##    ##    ##    ##    ##      #     #   ##  # ##  #
    //  ##   ####  ##    ####  ##      #     #   ##  # ### #
    //   ##  ##    ##    ##    ##      #     #   ##  # # ###
    //    ## ##    ##    ##    ##      #     #   ##  # #  ##
    // ##### ##### ##### ##### #####   #   #####  ###  #   #

    histManager->fillHistograms("Overall", "CutFlow_event", cutFlow_enum::events+1 ); 
    
    int     Num_candidates_preSel   =   0   ;
    int     Num_candidates_postSel  =   0   ;
    // To keep track for event cutflow     
    // doesn't matter if we store 11001 over 11000 as we can loop over 
    // and fill 1 till first 0 and then just break
    uint16_t largest_bitmask        =   0   ;
    uint16_t largest_bitmask_SR     =   0   ;
    // this part is to track allTracks and technical for event cutFlow
    int firstTwo                    =   0   ;
    // This is bitstring for when all conditions have passed
    uint16_t allPass = (1<<trackCuts.size()) - 1 ;
    
    // CandSel track cuts
    float   cand_trk_pT_cut     =   5       ;
    float   cand_trk_chi2_cut   =   20      ;
    int     cand_trk_hits_cut   =   3       ;
    
    // CandSel ecal cuts
    float   cand_ecal_maxE_cut       =   2   ;
    float   cand_ecal_maxE_error_cut =   0.5 ;
    float   cand_ecal_T_cut          =   1   ;
    //float   ecal_5x5_cut    =   5;

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
        float maxDep_time   = -999;
        EBDetId maxDep_E_detid; 

        bool flag_ecalSelection = 0;
        for (auto recHitItr : info.ecalRecHits)
        {
            EcalRecHit hit = *recHitItr;
            EBDetId det = hit.id();

            if (det.rawId() != maxDep.rawId()) continue;
            
            maxDep_E        = hit.energy();
            maxDep_time     = hit.time();
            maxDep_E_detid  = det;

            if ( (maxDep_E > cand_ecal_maxE_cut) && 
                    (hit.energyError() < cand_ecal_maxE_error_cut) && 
                    (hit.isTimeErrorValid()) && (maxDep_time > cand_ecal_T_cut) ) 
                flag_ecalSelection = 1;
            break;
        }

        // In case ecal thgresholds aren't cleared
        if (!flag_ecalSelection) continue;
        
        float t_pt      = track.pt();
        float t_pt_err  = track.ptError();
        
        histManager->fillHistograms("Overall", "CutFlow_candidate", 
                                                        cutFlow_enum::allTracks ); 
        
        // 3x3 energy around max E xtal
        histManager->fillHistograms("Vars_Candidate_b4PS", "Ecal_maxE", maxDep_E);
        
        histManager->fillHistograms("Vars_Candidate_b4PS", "Ecal_maxE_3x3",
                    info.nXnEnergy(maxDep_E_detid, TrackDetMatchInfo::EcalRecHits, 1));
        
        // Time for max E xtal
        histManager->fillHistograms("Vars_Candidate_b4PS", "Ecal_maxE_time", maxDep_time);
        
        // Delta R b/w Max E and track pos at ECAL 
        float EBEta = barrelGeometry->getGeometry(maxDep_E_detid)->getPosition().eta();
        float EBPhi = barrelGeometry->getGeometry(maxDep_E_detid)->getPosition().phi();
        math::XYZPoint trackPos = info.trkGlobPosAtEcal;
        double deltaR = reco::deltaR(trackPos.eta(), trackPos.phi(), EBEta, EBPhi);

        histManager->fillHistograms("Vars_Candidate_b4PS", "Ecal_maxE_dR", deltaR);

        histManager->fillHistograms("Vars_Candidate_b4PS", "sigPt_V_pT_high",
                                                    t_pt, t_pt_err);
        histManager->fillHistograms("Vars_Candidate_b4PS", "sigPt_V_pT",
                                                    t_pt, t_pt_err);
        histManager->fillHistograms("Vars_Candidate_b4PS", "sigPt_V_pT_low",
                                                    t_pt, t_pt_err);
        
        Num_candidates_preSel++; // Counting all tracks passing basic selection
        if (firstTwo == 0) firstTwo=1;

        const reco::TrackRef trackRef = reco::TrackRef(tracks, pos);
        reco::DeDxHitInfoRef dedxHitsRef = dedxCollH->get(trackRef.key());
        if (dedxHitsRef.isNull()) continue;
        histManager->fillHistograms("Overall", "CutFlow_candidate", 
                                                        cutFlow_enum::technical ); 
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
        
        // for dedx measurements without cluster cleaning
        string year = "";
        float dedxSF[] = {1.0, 1.0325};
        TH3* templateHisto = nullptr;
        bool usePixel = false;
        bool useStrip = true;
        bool useClusterCleaning = false;

        reco::DeDxData temp = computedEdx(run_, year, dedxHits, dedxSF, templateHisto, 
                            usePixel, useStrip, useClusterCleaning);
        
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
        SelectionValues["Ih"]               = temp.dEdx();
        
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
            
            histManager->fillHistograms("Preselection_No", varName, value); 
        }

        for (size_t i = 0; i<signalCuts.size(); i++)
        {
            float timeCut = signalCuts[i].time_cut;          
            float energyCut = signalCuts[i].energy_cut;          
            
            if ( (maxDep_E > energyCut) && (maxDep_time > timeCut)) 
                signalBitmask |= (1<<i);
        }

        if (selectionBitmask > largest_bitmask) largest_bitmask = selectionBitmask;
        // If all bits are turned on, fill presel AND N-1
        if (selectionBitmask == allPass)
        {
            Num_candidates_postSel++; // Counting all tracks passing all pre selection
            for (size_t i =0; i<trackCuts.size(); i++)
            {
                const std::string& varName = trackCuts[i].name;
                histManager->fillHistograms("Preselection", varName, 
                                        SelectionValues[varName]);
                histManager->fillHistograms("Preselection_Nm1", varName, 
                                        SelectionValues[varName]);
            }
            
            // 3x3 energy around max E xtal
            histManager->fillHistograms("Vars_Candidate", "Ecal_maxE", maxDep_E);
            
            histManager->fillHistograms("Vars_Candidate", "Ecal_maxE_3x3",
                        info.nXnEnergy(maxDep_E_detid, TrackDetMatchInfo::EcalRecHits, 1));
            
            // Time for max E xtal
            histManager->fillHistograms("Vars_Candidate", "Ecal_maxE_time", maxDep_time);
            
            // Delta R b/w Max E and track pos at ECAL 
            histManager->fillHistograms("Vars_Candidate", "Ecal_maxE_dR", deltaR);

            histManager->fillHistograms("Vars_Candidate", "sigPt_V_pT_high",
                                                        t_pt, t_pt_err);
            histManager->fillHistograms("Vars_Candidate", "sigPt_V_pT",
                                                        t_pt, t_pt_err);
            histManager->fillHistograms("Vars_Candidate", "sigPt_V_pT_low",
                                                        t_pt, t_pt_err);
        }
        // For only N-1 selection
        else 
        {
            for (size_t i =0; i<trackCuts.size(); i++)
            {   // Or individual pass with ith bit 1
                const std::string& varName = trackCuts[i].name;
                if (allPass == ((1<<i) | selectionBitmask))
                histManager->fillHistograms("Preselection_Nm1", varName, 
                                        SelectionValues[varName]);
            }
        }

        // Filling rest of the CutFlow
        for (size_t i = 0; i<trackCuts.size(); i++)
        {
            if (trackCuts[i].enumVal == cutFlow_enum::count) continue;
            if (selectionBitmask != ((1<<i) | selectionBitmask) ) break;
            histManager->fillHistograms("Overall", "CutFlow_candidate", 
                                                        trackCuts[i].enumVal); 
        }
        // Filling signal region cutflow only when all preselection passes
        if (selectionBitmask != allPass) continue;
        if (signalBitmask > largest_bitmask_SR) largest_bitmask_SR = signalBitmask;
        for (size_t i = 0; i<signalCuts.size(); i++){
            if (signalBitmask != ((1<<i) | signalBitmask)) continue;
            histManager->fillHistograms("Overall", "CutFlow_candidate",
                                                        signalCuts[i].enumVal);
        }
    } // END - for loop on tracks
    
    // Filling out event CutFlow
    switch(firstTwo) {
        case 0: 
            break;
        case 1: 
            histManager->fillHistograms("Overall", "CutFlow_event", 
                                                            cutFlow_enum::allTracks+1 ); 
            break;
        case 2:
            histManager->fillHistograms("Overall", "CutFlow_event", 
                                                            cutFlow_enum::allTracks+1 ); 
            histManager->fillHistograms("Overall", "CutFlow_event", 
                                                            cutFlow_enum::technical+1 ); 
            
            for (size_t i =0; i<trackCuts.size(); i++)
            { 
                // Exits after hitting the first 0
                if (largest_bitmask != ((1<<i) | largest_bitmask)) break;
                histManager->fillHistograms("Overall", "CutFlow_event",
                                                                trackCuts[i].enumVal+1 ); 
            }

            for (size_t i=0; i<signalCuts.size(); i++)
            {
                if (largest_bitmask_SR != ((1<<i) | largest_bitmask_SR)) continue;
                histManager->fillHistograms("Overall", "CutFlow_event",
                                                                signalCuts[i].enumVal+1);
            }
            
            break;
        default:
            std::cout<<"ERROR: value of 'firstTwo' (0/1/2) set to: "<<firstTwo<<std::endl;
    }

    histManager->fillHistograms("Overall", "Num_of_cand_noSel", Num_candidates_preSel ); 
    histManager->fillHistograms("Overall", "Num_of_cand_postSel", Num_candidates_postSel ); 

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
}


//define this as a plug-in
DEFINE_FWK_MODULE(mchampAnalyzer);
