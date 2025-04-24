#ifndef HISTOGRAM_MANAGER_H
#define HISTOGRAM_MANAGER_H

#include "FWCore/ServiceRegistry/interface/Service.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "TH1.h"
#include "TH2.h"
#include <map>
#include <string>

// Namespace for cutflow bins
namespace cutFlow_enum {
    enum Type
    {
        events = -1,
        allTracks = 0,
        technical,
        triggers,
        pt,
        eta,
        fracValidHits,
        numDedxHits,
        highPurity,
        chi2,
        dz,
        dxy,
        Ih,
        sigPtOPt2,
        //energy,
        //time,
        SR,
        count
    };
}

// To store Pre-selection cut values and refrence to cutflow
struct TrackCut {
    std::string name;
    double cutValue;
    cutFlow_enum::Type enumVal;
    bool isMinCut;  // true = value >= cut, false = abs(value) <= cut
}; 
   
// To store SR cut values and refrence to cutflow
struct SRCut {
    std::string name;
    double time_cut;
    double energy_cut;
    cutFlow_enum::Type enumVal;
}; 
   
class HistogramManager {
public:
    //explicit HistogramManager(const edm::ParameterSet& iConfig);
    explicit HistogramManager( TFileService& fs );
    void fillHistograms(const std::string& category, 
                                    const std::string& variable, 
                                    float value);
    void fillHistograms(const std::string& category, 
                                    const std::string& variable, 
                                    float value_x,
                                    float value_y);

private:
    std::map<std::string, TFileDirectory> dirs;
    std::map<std::string, std::map<std::string, TH1F*>> histograms;
    std::map<std::string, std::map<std::string, TH2F*>> histograms_2d;
};

#endif

