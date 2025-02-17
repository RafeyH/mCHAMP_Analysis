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

//
// constructors and destructor
//
GenTrackEcalAnalyzer::GenTrackEcalAnalyzer(const edm::ParameterSet& iConfig)
 : 	genParticlesToken_(consumes<reco::GenParticleCollection>(iConfig.getParameter<edm::InputTag>("genParticles"))),
	tracksToken_(consumes<reco::TrackCollection>(iConfig.getParameter<edm::InputTag>("tracks"))),
    ecalRecHitsToken_(consumes<EcalRecHitCollection>(iConfig.getParameter<edm::InputTag>("ecalRecHits"))),
    pdgId_(iConfig.getParameter<int>("pdgId")),
    deltaRCutoff_tracks(iConfig.getParameter<double>("deltaRCutoff_tracks")),
    deltaRCutoff_EB(iConfig.getParameter<double>("deltaRCutoff_EB")),
	dedxToken_(consumes<reco::DeDxHitInfoAss>(iConfig.getParameter<edm::InputTag>("dedxHits"))),
	Ih2Token_(consumes<reco::DeDxDataCollection>(iConfig.getParameter<edm::InputTag>("Ih2Collection"))),
    outputFileName_(iConfig.getParameter<std::string>("outputFile"))
{
    
    // Initialize TrackDetectorAssociator and parameters
    //edm::ParameterSet trackAssociatorParams = iConfig.getParameter<edm::ParameterSet>("TrackAssociatorParameters");
    //trackAssociator_.useDefaultPropagator();
    // Store the consumes collector in a variable
    edm::ConsumesCollector cc = consumesCollector();
    trackAssociatorParams_.loadParameters(iConfig.getParameter<edm::ParameterSet>("TrackAssociatorParameters"), cc);
    //trackAssociatorParams_.loadParameters(trackAssociatorParams, consumesCollector());

	outputFile_ = new TFile(outputFileName_.c_str(), "RECREATE");
    
    tree_ = new TTree("Ntuple", "Ntuple");
    tree_->Branch("Run", &run_, "Run/I");
    tree_->Branch("Event", &event_, "Event/I");
    tree_->Branch("GenPart.", &cls_genpart);
    tree_->Branch("Tracks.", &cls_tracks);
    tree_->Branch("EcalRecHits.", &cls_rechitsEcal);
}


GenTrackEcalAnalyzer::~GenTrackEcalAnalyzer()
{

    outputFile_->cd();
    tree_->Write();
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
        
        int pos = -1;

        for (const auto& track : *tracks) {
            // Use TrackDetectorAssociator to propagate the track to ECAL
            //TrackDetMatchInfo info = trackAssociator_.associate(iEvent, iSetup, 
            //                        reco::TrackRef(&tracks, &track - &tracks->front()), 
            //                        trackAssociatorParams_);

            // Get the position at the ECAL surface
            //GlobalPoint ecalPosition = info.positionAtEcal;

            pos++;
            // Calculate dR between the genParticle and the track projected to ECAL
            double deltaR = reco::deltaR(cls_genpart->eta, cls_genpart->phi, track.eta(), track.phi());
            if (deltaR < deltaRCutoff_tracks && track.pt()>5) {
                
                cls_tracks->pt.push_back(track.pt());
                cls_tracks->beta.push_back(track.beta());
                cls_tracks->eta.push_back(track.eta());
                cls_tracks->phi.push_back(track.phi());
                
                cls_tracks->deltaR.push_back(deltaR);
                
                cls_tracks->qoverp.push_back(track.qoverp());
                cls_tracks->lambda.push_back(track.lambda());
                cls_tracks->dxy.push_back(track.dxy());
                cls_tracks->dsz.push_back(track.dsz());
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
                }
                
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
                }

                cls_rechitsEcal->energy.push_back(energy);
                cls_rechitsEcal->energyErr.push_back(energyErr);
                cls_rechitsEcal->time.push_back(time);
                cls_rechitsEcal->timeErr.push_back(timeErr);
                cls_rechitsEcal->timeErrValid.push_back(timeErrValid);
                cls_rechitsEcal->deltaR.push_back(deltaR);
                cls_rechitsEcal->iEta.push_back(iEta);
                cls_rechitsEcal->iPhi.push_back(iPhi);
                cls_rechitsEcal->rechitFlag.push_back(rechitFlag);
                
                // UNDER DEVELOPMENT
                /*
                
                // Looking at Ecal rechit near projected track location
                // Use TrackDetectorAssociator to propagate the track to ECAL
                
                GlobalVector trackMomentum(track.momentum().x(),
                                            track.momentum().y(),
                                            track.momentum().z());

                GlobalPoint trackVertex(track.vertex().x(),
                                        track.vertex().y(),
                                        track.vertex().z()); 
                
                TrackDetMatchInfo info = trackAssociator_.associate(
                                            iEvent, 
                                            iSetup, 
                                            trackMomentum,
                                            trackVertex,
                                            track.charge(),
                                            trackAssociatorParams_);

                //// Get the position at the ECAL surface
                //GlobalPoint trackAtECAL = info.positionAtEcal;
                //
                //for (const auto& hit : *ecalRecHits) {
                //    // Get position of ECAL RecHit in global coordinates
                //    const GlobalPoint& recHitPos = caloGeometry->getPosition(hit.id());

                //    // Calculate ΔR between RecHit and extrapolated track position
                //    float deltaEta = recHitPos.eta() - trackAtECAL.eta();
                //    float deltaPhi = reco::deltaPhi(recHitPos.phi(), trackAtECAL.phi());
                //    float deltaR = std::sqrt(deltaEta * deltaEta + deltaPhi * deltaPhi);

                //    // If within deltaRMax, store the hit
                //    if (deltaR < deltaRMax) {
                //        std::cout<<hit->energy()<<" ";
                //    }
                //}
                //std::cout<<std::endl;
               
                for (std::vector<const EcalRecHit*>::const_iterator hit = info.crossedEcalRecHits.begin();
                    hit != info.crossedEcalRecHits.end();
                    hit++){
                    
                    std::cout<<(*hit)->energy()<<" "; 
                }
                std::cout<<std::endl;
                */
            }

        }

        //ecalRecHitEnergies_.push_back(ecalEnergies);
        //deltaRwithTracks_.push_back(deltaRs);
        tree_->Fill();
    }

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
