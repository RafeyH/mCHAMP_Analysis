#ifndef GenTrackEcalAnalyzer_h
#define GenTrackEcalAnalyzer_h

#include <memory>
#include <vector>

#include "TTree.h"
#include "TH3.h"
#include <TObject.h>
#include <TMatrix.h>

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/InputTag.h"

#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/TrackReco/interface/TrackFwd.h"
#include "DataFormats/EcalRecHit/interface/EcalRecHitCollections.h"
#include "DataFormats/HepMCCandidate/interface/GenParticle.h"
#include "DataFormats/Math/interface/deltaR.h"
#include "DataFormats/TrackReco/interface/DeDxHitInfo.h"
#include "DataFormats/TrackReco/interface/DeDxData.h"

#include "TrackingTools/TrackAssociator/interface/TrackDetectorAssociator.h"
#include "TrackingTools/TrackAssociator/interface/TrackAssociatorParameters.h"


class GenPart : public TObject {
    
public:
    float   pt, eta, phi;
    int     id;

    // Constructor
    GenPart() : pt(-1.), eta(-999.), phi(-999.), id(-1) {}

    void reset(){
        pt=-1.; eta=-999.; phi=-999.; id=-1;
    }
    
    // ROOT macro for I/O
    ClassDef(GenPart, 1);
};

class Tracks : public TObject {
    
public:
    std::vector<float> pt, beta, eta, phi, deltaR, qoverp, lambda, dxy, dsz;
    std::vector<float> chisq, ndof, validHitsFrac;
    std::vector<bool> hasDedxRef;
    std::vector<float> dedx;
    std::vector<int> numOfStrips, numOfSatStrips, charge;
    std::vector<uint8_t> trackQual, trackAlgo; 
    std::vector<unsigned short> validHitsNum;

    // Constructor
    Tracks() {}
    
    // reset function for each genpart
    void reset() {
        pt.clear(); beta.clear(); eta.clear(); phi.clear(); deltaR.clear();
        qoverp.clear(); lambda.clear(); dxy.clear(); dsz.clear();
        charge.clear(); chisq.clear(); ndof.clear();
        trackQual.clear(); trackAlgo.clear();
        validHitsNum.clear(); validHitsFrac.clear();
        hasDedxRef.clear(); dedx.clear();
        numOfStrips.clear(); numOfSatStrips.clear();
    }
    
    // ROOT macro for I/O
    ClassDef(Tracks, 1);
};

class RecHits_Ecal : public TObject {

public:

    std::vector<std::vector<float>> energy, energyErr, time, timeErr;
    std::vector<std::vector<bool>> timeErrValid;
    std::vector<std::vector<float>> deltaR;
    std::vector<std::vector<uint32_t>> rechitFlag;
    std::vector<std::vector<int>> iEta, iPhi;

    // Constructor
    RecHits_Ecal() {}

    void reset() {
        energy.clear(); energyErr.clear(); time.clear(); timeErr.clear(); 
        timeErrValid.clear(); deltaR.clear(); rechitFlag.clear();
        iEta.clear(); iPhi.clear();
    }
    
    ClassDef(RecHits_Ecal, 1);
};

class GenTrackEcalAnalyzer : public edm::EDAnalyzer {
public:
    explicit GenTrackEcalAnalyzer(const edm::ParameterSet&);
    ~GenTrackEcalAnalyzer();
  
private:
    virtual void analyze(const edm::Event&, const edm::EventSetup&) override;
    void beginJob() override;
    void endJob() override;

    // Input Tags
    edm::EDGetTokenT<reco::GenParticleCollection> genParticlesToken_;
    edm::EDGetTokenT<reco::TrackCollection> tracksToken_;
    edm::EDGetTokenT<EcalRecHitCollection> ecalRecHitsToken_;
    
    // Parameters
    int pdgId_;
    double deltaRCutoff_tracks;
    double deltaRCutoff_EB;

	edm::EDGetTokenT<reco::DeDxHitInfoAss> dedxToken_;
	edm::EDGetTokenT<reco::DeDxDataCollection> Ih2Token_;

    // TrackDetectorAssociator and parameters
    TrackDetectorAssociator trackAssociator_;
    TrackAssociatorParameters trackAssociatorParams_;

    // TTree and variables
    TFile* outputFile_;
    std::string outputFileName_;
    TTree *tree_;
    int run_, event_;
    GenPart* cls_genpart = new GenPart;
    Tracks* cls_tracks = new Tracks;
    RecHits_Ecal* cls_rechitsEcal = new RecHits_Ecal;

    // Helper functions - NOT implemented
    //reco::TrackRef getBestMatchedTrack(const reco::GenParticle&, const reco::TrackCollection&);
    //const EcalRecHit* getMaxEnergyEcalRecHit(const GlobalPoint&, const EcalRecHitCollection&, double&);
};

#endif

