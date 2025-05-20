#ifndef mchampAnalyzer_h
#define mchampAnalyzer_h

#include <memory>
#include <vector>
#include <string>
#include <regex>

#include "TTree.h"
#include "TH1.h"
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
#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/VertexReco/interface/VertexFwd.h"
#include "DataFormats/HepMCCandidate/interface/GenParticle.h"
#include "DataFormats/Math/interface/deltaR.h"
#include "DataFormats/TrackReco/interface/DeDxHitInfo.h"
#include "DataFormats/TrackReco/interface/DeDxData.h"

#include "TrackingTools/TrackAssociator/interface/TrackDetectorAssociator.h"
#include "TrackingTools/TrackAssociator/interface/TrackAssociatorParameters.h"

#include "FWCore/Common/interface/TriggerNames.h"
#include "DataFormats/Common/interface/TriggerResults.h"

#include "SimDataFormats/GeneratorProducts/interface/GenEventInfoProduct.h"

#include "HistogramManager.h"

// Setup for the TTrees
// Add pragma in the Linkdef file if adding new classes for TTree

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
    std::vector<float>  pt, beta, eta, phi, deltaR, qoverp, lambda, dxy, dz;
    std::vector<float>  ptError, chisq, ndof, validHitsFrac;
    std::vector<bool>   hasDedxRef;
    std::vector<float>  dedx;
    std::vector<int>    numOfStrips, numOfSatStrips, charge;
    std::vector<uint8_t>        trackQual, trackAlgo; 
    std::vector<unsigned short> validHitsNum;

    // Constructor
    Tracks() {}
    
    // reset function for each genpart
    void reset() {
        pt.clear(); beta.clear(); eta.clear(); phi.clear(); deltaR.clear();
        qoverp.clear(); lambda.clear(); dxy.clear(); dz.clear();
        charge.clear(); chisq.clear(); ndof.clear(); ptError.clear();
        trackQual.clear(); trackAlgo.clear();
        validHitsNum.clear(); validHitsFrac.clear();
        hasDedxRef.clear(); dedx.clear();
        numOfStrips.clear(); numOfSatStrips.clear();
    }
    
    // ROOT macro for I/O
    ClassDef(Tracks, 1);
};

class TrackAssoc : public TObject {

public: 
    std::vector<float>  ecalXEnergy, ecal3x3Energy, ecal5x5Energy; 
    std::vector<float>  ecalMaxE, ecalMaxEdR, ecalMaxETime;

    TrackAssoc() {}

    void reset() {
        ecalXEnergy.clear(); ecal3x3Energy.clear(); ecal5x5Energy.clear();
        ecalMaxE.clear(); ecalMaxEdR.clear(); ecalMaxETime.clear();
    }

    ClassDef(TrackAssoc, 1);
};

class RecHits_Ecal : public TObject {

public:

    std::vector<std::vector<float>>     energy, energyErr, time, timeErr;
    std::vector<std::vector<bool>>      timeErrValid;
    std::vector<std::vector<float>>     deltaR;
    std::vector<std::vector<uint32_t>>  rechitFlag;
    std::vector<std::vector<int>>       iEta, iPhi;

    // Constructor
    RecHits_Ecal() {}

    void reset() {
        energy.clear(); energyErr.clear(); time.clear(); timeErr.clear(); 
        timeErrValid.clear(); deltaR.clear(); rechitFlag.clear();
        iEta.clear(); iPhi.clear();
    }
    
    ClassDef(RecHits_Ecal, 1);
};

class mchampAnalyzer : public edm::EDAnalyzer {
public:
    explicit mchampAnalyzer(const edm::ParameterSet&);
    ~mchampAnalyzer();
  
private:
    virtual void analyze(const edm::Event&, const edm::EventSetup&) override;
    void beginJob() override;
    void endJob() override;

    // Input Tags
    edm::EDGetTokenT<reco::GenParticleCollection>   genParticlesToken_;
    edm::EDGetTokenT<GenEventInfoProduct>           genEventInfoToken_;
    edm::EDGetTokenT<reco::TrackCollection>         tracksToken_;
    edm::EDGetTokenT<EcalRecHitCollection>          ecalRecHitsToken_;
    edm::EDGetTokenT<reco::VertexCollection>        vertexToken_;
    
    // Parameters
    int     pdgId_;
    double  deltaRCutoff_tracks;
    double  deltaRCutoff_EB;

	edm::EDGetTokenT<reco::DeDxHitInfoAss>      dedxToken_;
	edm::EDGetTokenT<reco::DeDxDataCollection>  Ih2Token_;

    edm::EDGetTokenT<edm::TriggerResults>       triggerResultsToken_;
    std::vector<std::string>                    triggerPaths_;

    // TrackDetectorAssociator and parameters
    TrackDetectorAssociator     trackAssociator_;
    TrackAssociatorParameters   trackAssociatorParams_;

    // TTree and variables
    TFile*      outputFile_;
    std::string outputFileName_;
    bool        saveNtuple_;
    TTree       *tree_;
    int         run_, event_;

    GenPart*        cls_genpart     = new GenPart;
    Tracks*         cls_tracks      = new Tracks;
    RecHits_Ecal*   cls_rechitsEcal = new RecHits_Ecal;
    TrackAssoc*     cls_trackAssoc  = new TrackAssoc;
    
    int ignore_trig_bit_pos;

    double sum_gen_weights = 0;

    // Definitions in HistogramManager.h
    std::unique_ptr<HistogramManager> histManager;
    std::vector<TrackCut> trackCuts;
    std::vector<SRCut> signalCuts;
    
    // Saving trigger regex patterns
    std::vector<std::regex> compiledTriggerPatterns;
};

#endif

