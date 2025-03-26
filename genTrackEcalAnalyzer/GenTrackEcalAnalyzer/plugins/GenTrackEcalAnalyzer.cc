// -*- C++ -*-
//
// Package:    genTrackEcalAnalyzer/GenTrackEcalAnalyzer
// Class:      GenTrackEcalAnalyzer
//
/**\class GenTrackEcalAnalyzer GenTrackEcalAnalyzer.cc genTrackEcalAnalyzer/GenTrackEcalAnalyzer/plugins/GenTrackEcalAnalyzer.cc

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

#include "GenTrackEcalAnalyzer.h"
#include "CommonFunction.h"


// Implement ROOT's dictionary for custom classes (for branches)
ClassImp(GenPart);
ClassImp(Tracks);
ClassImp(RecHits_Ecal);
ClassImp(TrackAssoc);

namespace cutFlow_enum {
    enum Type
    {
        allTracks = 0,
        technical,
        pt,
        eta,
        fracValidHits,
        numDedxHits,
        highPurity,
        chi2,
        dz,
        dxy,
        Ih,
        energy,
        time,
        count
    };
}
    
//
// constructors and destructor
//
GenTrackEcalAnalyzer::GenTrackEcalAnalyzer(const edm::ParameterSet& iConfig)
 : 	genParticlesToken_(consumes<reco::GenParticleCollection>(iConfig.getParameter<edm::InputTag>("genParticles"))),
	tracksToken_(consumes<reco::TrackCollection>(iConfig.getParameter<edm::InputTag>("tracks"))),
    ecalRecHitsToken_(consumes<EcalRecHitCollection>(iConfig.getParameter<edm::InputTag>("ecalRecHits"))),
    vertexToken_(consumes<reco::VertexCollection>(iConfig.getParameter<edm::InputTag>("offlinePV"))),
    pdgId_(iConfig.getParameter<int>("pdgId")),
    deltaRCutoff_tracks(iConfig.getParameter<double>("deltaRCutoff_tracks")),
    deltaRCutoff_EB(iConfig.getParameter<double>("deltaRCutoff_EB")),
	dedxToken_(consumes<reco::DeDxHitInfoAss>(iConfig.getParameter<edm::InputTag>("dedxHits"))),
	Ih2Token_(consumes<reco::DeDxDataCollection>(iConfig.getParameter<edm::InputTag>("Ih2Collection"))),
    outputFileName_(iConfig.getParameter<std::string>("outputFile"))
{
    
    // Initialize TrackDetectorAssociator and parameters
    edm::ParameterSet trackAssociatorParams = iConfig.getParameter<edm::ParameterSet>("TrackAssociatorParameters");
    edm::ConsumesCollector cc = consumesCollector();
    trackAssociatorParams_.loadParameters(trackAssociatorParams, cc);
    trackAssociator_.useDefaultPropagator();

	outputFile_ = new TFile(outputFileName_.c_str(), "RECREATE");
    //outputFile_->cd();

    tree_ = new TTree("Ntuple", "Ntuple");
    tree_->Branch("Run", &run_, "Run/I");
    tree_->Branch("Event", &event_, "Event/I");
    tree_->Branch("GenPart.", &cls_genpart);
    tree_->Branch("Tracks.", &cls_tracks);
    tree_->Branch("EcalRecHits.", &cls_rechitsEcal);
    tree_->Branch("TrackAssoc.", &cls_trackAssoc);

    CutFlow = new TH1I(
                "CutFlow",
                ";;Tracks / category",
                cutFlow_enum::count, -0.5, cutFlow_enum::count-0.5);

    CutFlow->SetMinimum(0);
    
    CutFlow->GetXaxis()->SetBinLabel( cutFlow_enum::allTracks+1    , "All tracks");
    CutFlow->GetXaxis()->SetBinLabel( cutFlow_enum::technical+1    , "Technical");
    //CutFlow->GetXaxis()->SetBinLabel(3, "Trigger");
    CutFlow->GetXaxis()->SetBinLabel( cutFlow_enum::pt+1           , "p_{T}");
    CutFlow->GetXaxis()->SetBinLabel( cutFlow_enum::eta+1          , "#eta");
    //CutFlow->GetXaxis()->SetBinLabel(6, "N_{no-L1 pixel hits}");
    CutFlow->GetXaxis()->SetBinLabel( cutFlow_enum::fracValidHits+1, "f_{valid/all hits}");
    CutFlow->GetXaxis()->SetBinLabel( cutFlow_enum::numDedxHits+1  , "N_{dEdx hits}");
    CutFlow->GetXaxis()->SetBinLabel( cutFlow_enum::highPurity+1   , "HighPurity");
    CutFlow->GetXaxis()->SetBinLabel( cutFlow_enum::chi2+1         , "#chi^{2} / N_{dof}");
    CutFlow->GetXaxis()->SetBinLabel( cutFlow_enum::dz+1           , "d_{z}");
    CutFlow->GetXaxis()->SetBinLabel( cutFlow_enum::dxy+1          , "d_{xy}");
    CutFlow->GetXaxis()->SetBinLabel( cutFlow_enum::Ih+1           , "Ih");
    CutFlow->GetXaxis()->SetBinLabel( cutFlow_enum::energy+1       , "Energy");
    CutFlow->GetXaxis()->SetBinLabel( cutFlow_enum::time+1         , "Time");
    //CutFlow->GetXaxis()->SetBinLabel(13, "MiniRelIsoAll");
    //CutFlow->GetXaxis()->SetBinLabel(14, "MiniRelTkIso");
    //CutFlow->GetXaxis()->SetBinLabel(15, "E/p");
    //CutFlow->GetXaxis()->SetBinLabel(16, "#sigma_{p_{T}} / p_{T}^{2}");
    //CutFlow->GetXaxis()->SetBinLabel(17, "F_{i}");
    //CutFlow->GetXaxis()->SetBinLabel(18, "SR0");
    //CutFlow->GetXaxis()->SetBinLabel(19, "SR1");
    //CutFlow->GetXaxis()->SetBinLabel(20, "SR2");
    //CutFlow->GetXaxis()->SetBinLabel(21, "SR2 with SFs");

}


GenTrackEcalAnalyzer::~GenTrackEcalAnalyzer()
{

    outputFile_->cd();
    tree_->Write();
    CutFlow->Write();
    outputFile_->Close();
}


//
// member functions
//

// ------------ method called for each event  ------------
void
GenTrackEcalAnalyzer::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup)
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
    
    // Get EcalRecHits
    edm::Handle<EcalRecHitCollection> EBRecHits;
    iEvent.getByToken(ecalRecHitsToken_, EBRecHits);

    // Geometry handle
	edm::ESHandle<CaloGeometry> geoHandle;
	iSetup.get<CaloGeometryRecord>().get(geoHandle);
	const CaloSubdetectorGeometry* barrelGeometry = geoHandle->getSubdetectorGeometry(DetId::Ecal, EcalBarrel);
    
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

    
    // Candidate Selection criteria
    int pos = -1;
    for (const auto& track : *tracks) 
    {
        pos++;

        // CandSel track cuts
        float   cand_trk_pT_cut     =   5       ;
        float   cand_trk_chi2_cut   =   20      ;
        int     cand_trk_hits_cut   =   3       ;
        
        // CandSel ecal cuts
        float   cand_ecal_maxE_cut       =   2   ;
        float   cand_ecal_maxE_error_cut =   0.5 ;
        float   cand_ecal_T_cut          =   1   ;
        //float   ecal_5x5_cut    =   5;

        // PreSel cuts
        float   trk_eta_cut             =   1.479   ;
        float   trk_pT_cut              =   5       ;
        float   trk_chi2_cut            =   5       ;
        float   trk_validFrac_cut       =   0.8     ;
        unsigned int trk_dedxHits_cut   =   10      ;
        float   trk_dz_cut              =   0.5     ;
        float   trk_dxy_cut             =   0.5     ;
        float   trk_dEdx_cut            =   3       ;
        float   ecal_energy_cut         =   5       ;
        float   ecal_time_cut           =   2       ;

        // Cut on track pT and eta
        if (track.pt() < cand_trk_pT_cut)                   continue;
        if (track.found() < cand_trk_hits_cut)              continue;
        if (track.chi2()/track.ndof() > cand_trk_chi2_cut)  continue;
        
        
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

        bool flag_ecalSelection = 0;
        for (auto recHitItr : info.ecalRecHits){
            EcalRecHit hit = *recHitItr;
            EBDetId det = hit.id();

            if (det.rawId() != maxDep.rawId()) continue;
            
            maxDep_E    = hit.energy();
            maxDep_time = hit.time();

            if ( (maxDep_E > cand_ecal_maxE_cut) && (hit.energyError() < cand_ecal_maxE_error_cut) && 
                    (hit.isTimeErrorValid()) && (maxDep_time > cand_ecal_T_cut) ) 
                flag_ecalSelection = 1;
            break;
        }

        // In case ecal thgresholds aren't cleared
        if (!flag_ecalSelection) continue;

        // All tracks that made the basic Candidate selection requirement
        CutFlow->Fill( cutFlow_enum::allTracks );
       
        const reco::TrackRef trackRef = reco::TrackRef(tracks, pos);
        reco::DeDxHitInfoRef dedxHitsRef = dedxCollH->get(trackRef.key());
        
        // Technical Check
        if (dedxHitsRef.isNull()) continue;
        CutFlow->Fill( cutFlow_enum::technical );
        
        // pT cut
        if (track.pt() < trk_pT_cut) continue;
        CutFlow->Fill( cutFlow_enum::pt );
        
        // Eta cut
        if (abs(track.eta()) > trk_eta_cut) continue;
        CutFlow->Fill( cutFlow_enum::eta );

        // Fraction valid hits cut
        if (track.validFraction() < trk_validFrac_cut) continue;
        CutFlow->Fill( cutFlow_enum::fracValidHits );

        const reco::DeDxHitInfo* dedxHits = &(*dedxHitsRef);
        
        // N dedx hits cut
        if (dedxHits->size() < trk_dedxHits_cut) continue;
        CutFlow->Fill( cutFlow_enum::numDedxHits );

        // track high purity cut
        if ( !track.quality( reco::TrackBase::highPurity )) continue;
        CutFlow->Fill( cutFlow_enum::highPurity );

        // chi2/dof cut
        if (track.chi2()/track.ndof() > trk_chi2_cut)  continue;
        CutFlow->Fill( cutFlow_enum::chi2 );
        
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
        
        // dz cut
        if (abs(track.dz(bestVertex.position())) > trk_dz_cut)  continue;
        CutFlow->Fill( cutFlow_enum::dz );
        
        // dxy cut
        if (abs(track.dxy(bestVertex.position())) > trk_dxy_cut)  continue;
        CutFlow->Fill( cutFlow_enum::dxy );
        
        // for dedx measurements without cluster cleaning
        string year = "";
        float dedxSF[] = {1.0, 1.0325};
        TH3* templateHisto = nullptr;
        bool usePixel = false;
        bool useStrip = true;
        bool useClusterCleaning = false;

        reco::DeDxData temp = computedEdx(run_, year, dedxHits, dedxSF, templateHisto, usePixel, 
                            useStrip, useClusterCleaning);
        
        // Ih cut
        if (temp.dEdx() < trk_dEdx_cut)  continue;
        CutFlow->Fill( cutFlow_enum::Ih );

        // Ecal energy cut
        if (maxDep_E < ecal_energy_cut)  continue;
        CutFlow->Fill( cutFlow_enum::energy );

        // Ecal time cut
        if (maxDep_time < ecal_time_cut)  continue;
        CutFlow->Fill( cutFlow_enum::time );

    }   // End for - TrackCollection (Cand selection)
}


// ------------ method called once each job just before starting event loop  ------------
void
GenTrackEcalAnalyzer::beginJob()
{
}

// ------------ method called once each job just after ending the event loop  ------------
void
GenTrackEcalAnalyzer::endJob()
{
}


//define this as a plug-in
DEFINE_FWK_MODULE(GenTrackEcalAnalyzer);
