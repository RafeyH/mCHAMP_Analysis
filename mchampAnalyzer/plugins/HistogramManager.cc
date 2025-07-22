#include "HistogramManager.h"

HistogramManager::HistogramManager(TFileService& fs) {
    
    // Setting up directories and histograms
    
    //dirs["Trigger_Turn_On"]     = fs.mkdir("Trigger_Turn_On");
    dirs["Vars_Candidate_b4PS"] = fs.mkdir("Vars_Candidate_b4PS");
    dirs["Preselection_No"]     = fs.mkdir("Preselection_No");
    dirs["Preselection_Nm1"]    = fs.mkdir("Preselection_Nm1");
    dirs["Preselection"]        = fs.mkdir("Preselection");
    dirs["Vars_Candidate"]      = fs.mkdir("Vars_Candidate");
    dirs["Event_Kinematics"]    = fs.mkdir("Event_Kinematics");
    dirs["Mass_Plots_b4PS"]     = fs.mkdir("Mass_Plots_b4PS");
    dirs["Mass_Plots"]          = fs.mkdir("Mass_Plots");

    // Define histograms w/ binning and axis labels
    struct HistDefinition {
        //std::vector<std::string>    dir_name;
        std::string                 name;
        int                         bins;
        double                      min;
        double                      max;
        std::string                 x_label;
        std::string                 y_label;
    };

    std::vector<HistDefinition> histDefinitions = {
        {"pt",          100, 0, 500,   "p_{T} [GeV]",       "Tracks / 5 GeV"},
        //{"sigPtOPt",    50, 0, 1,      "#sigma(p_{T})/p_{T}",
        //                                                    "Tracks / 0.02"},
        {"sigPtOPt2",   50, 0, 0.01,   "#sigma(p_{T})/p_{T}^{2} [GeV^{-1}]", 
                                                            "Tracks / 0.0002 GeV^{-1}"},
        {"eta",         50, -2.5, 2.5, "#eta",              "Tracks / 0.1"},
        {"validHitsFrac",20, 0, 1,     "f_{valid/all hits}","Tracks / 0.05"},
        {"dEdxHits",    40, 0, 40,     "N_{dEdx hits}",     "Tracks / 1"},
        {"highPurity",  2, -0.5, 1.5,  "High Purity",       "Tracks / bin"},
        {"Chi2Ondof",   20, 0, 20,     "#chi^{2} / N_{dof}","Tracks / 1"},
        {"dxy",         50, -0.1, 0.1, "d_{xy} [cm]",       "Tracks / 0.004 cm"},
        {"dz",          50, -0.3, 0.3, "d_{z} [cm]",        "Tracks / 0.012 cm"},
        {"trigger",     2, -0.5, 1.5,  "Trigger Pass",      "Tracks / bin"},
        {"Ih",          50, 0, 50,     "I_{h} [MeV/cm]",    "Tracks / 1 MeV/cm"},
        {"trackPtIso",  100, 0, 100,   "#Sigma_{#DeltaR<0.3} p_{T} [GeV]",    
                                                            "Tracks / 1 GeV"}
    };

    // Loop over categories and histograms to create them
    for (const auto& category : {"Preselection_No", "Preselection_Nm1", "Preselection"}) {
        for (const auto& hist : histDefinitions) {
            histograms[category][hist.name] = dirs[category].make<TH1D>(
                (hist.name + "_" + category).c_str(),
                (hist.name + " distribution;" + hist.x_label + ";" + hist.y_label).c_str(),
                hist.bins, hist.min, hist.max
            );
        }
    }
    
    // Setting up CutFlow histograms
    
    histograms["Overall"]["CutFlow_candidate"] = fs.make<TH1D>(
                            "CutFlow_per_candidate",
                            "CutFlow per Candidate;;Tracks / category",
                            cutFlow_enum::count, -0.5, cutFlow_enum::count-0.5);

    histograms["Overall"]["CutFlow_candidate"]->SetMinimum(0);
    histograms["Overall"]["CutFlow_candidate"]->GetXaxis()->SetBinLabel( 
                                                        cutFlow_enum::allTracks+1, 
                                                        "All tracks");
    histograms["Overall"]["CutFlow_candidate"]->GetXaxis()->SetBinLabel( 
                                                        cutFlow_enum::technical+1, 
                                                        "Technical");
    histograms["Overall"]["CutFlow_candidate"]->GetXaxis()->SetBinLabel( 
                                                        cutFlow_enum::triggers+1, 
                                                        "Triggers");
    histograms["Overall"]["CutFlow_candidate"]->GetXaxis()->SetBinLabel( 
                                                        cutFlow_enum::pt+1, 
                                                        "p_{T}");
    histograms["Overall"]["CutFlow_candidate"]->GetXaxis()->SetBinLabel( 
                                                        cutFlow_enum::eta+1, 
                                                        "#eta");
    //histograms["Overall"]["CutFlow_candidate"]->GetXaxis()->SetBinLabel(6, "N_{no-L1 pixel hits}");
    histograms["Overall"]["CutFlow_candidate"]->GetXaxis()->SetBinLabel( 
                                                        cutFlow_enum::fracValidHits+1, 
                                                        "f_{valid/all hits}");
    histograms["Overall"]["CutFlow_candidate"]->GetXaxis()->SetBinLabel( 
                                                        cutFlow_enum::numDedxHits+1, 
                                                        "N_{dEdx hits}");
    histograms["Overall"]["CutFlow_candidate"]->GetXaxis()->SetBinLabel( 
                                                        cutFlow_enum::highPurity+1, 
                                                        "HighPurity");
    histograms["Overall"]["CutFlow_candidate"]->GetXaxis()->SetBinLabel( 
                                                        cutFlow_enum::chi2+1, 
                                                        "#chi^{2} / N_{dof}");
    histograms["Overall"]["CutFlow_candidate"]->GetXaxis()->SetBinLabel( 
                                                        cutFlow_enum::dz+1, 
                                                        "d_{z}");
    histograms["Overall"]["CutFlow_candidate"]->GetXaxis()->SetBinLabel( 
                                                        cutFlow_enum::dxy+1, 
                                                        "d_{xy}");
    histograms["Overall"]["CutFlow_candidate"]->GetXaxis()->SetBinLabel( 
                                                        cutFlow_enum::Ih+1, 
                                                        "Ih");
    histograms["Overall"]["CutFlow_candidate"]->GetXaxis()->SetBinLabel( 
                                                        cutFlow_enum::trackPtIso+1, 
                                                        "#Sigma_{#DeltaR<0.3} p_{T}");
    histograms["Overall"]["CutFlow_candidate"]->GetXaxis()->SetBinLabel( 
                                                        cutFlow_enum::sigPtOPt2+1, 
                                                        "#sigma(p_{T})/p_{T}^{2}");
    //histograms["Overall"]["CutFlow_candidate"]->GetXaxis()->SetBinLabel( cutFlow_enum::energy+1       , "Energy");
    //histograms["Overall"]["CutFlow_candidate"]->GetXaxis()->SetBinLabel( cutFlow_enum::time+1         , "Time");
    histograms["Overall"]["CutFlow_candidate"]->GetXaxis()->SetBinLabel( 
                                                        cutFlow_enum::SR+1, 
                                                        "SR");
    //histograms["Overall"]["CutFlow_candidate"]->GetXaxis()->SetBinLabel(13, "MiniRelIsoAll");
    //histograms["Overall"]["CutFlow_candidate"]->GetXaxis()->SetBinLabel(14, "MiniRelTkIso");
    //histograms["Overall"]["CutFlow_candidate"]->GetXaxis()->SetBinLabel(15, "E/p");
    //histograms["Overall"]["CutFlow_candidate"]->GetXaxis()->SetBinLabel(16, "#sigma_{p_{T}} / p_{T}^{2}");
    //histograms["Overall"]["CutFlow_candidate"]->GetXaxis()->SetBinLabel(17, "F_{i}");
    //histograms["Overall"]["CutFlow_candidate"]->GetXaxis()->SetBinLabel(18, "SR0");
    //histograms["Overall"]["CutFlow_candidate"]->GetXaxis()->SetBinLabel(19, "SR1");
    //histograms["Overall"]["CutFlow_candidate"]->GetXaxis()->SetBinLabel(20, "SR2");
    //histograms["Overall"]["CutFlow_candidate"]->GetXaxis()->SetBinLabel(21, "SR2 with SFs");
    
    histograms["Overall"]["CutFlow_event"] = fs.make<TH1D>(
                            "CutFlow_per_event",
                            "CutFlow per Event;;Tracks / category",
                            cutFlow_enum::count+1, -0.5, cutFlow_enum::count+0.5);

    histograms["Overall"]["CutFlow_event"]->SetMinimum(0);
    histograms["Overall"]["CutFlow_event"]->GetXaxis()->SetBinLabel( 
                                                        cutFlow_enum::events+2, 
                                                        "Events");
    histograms["Overall"]["CutFlow_event"]->GetXaxis()->SetBinLabel( 
                                                        cutFlow_enum::allTracks+2, 
                                                        "All tracks");
    histograms["Overall"]["CutFlow_event"]->GetXaxis()->SetBinLabel( 
                                                        cutFlow_enum::technical+2, 
                                                        "Technical");
    histograms["Overall"]["CutFlow_event"]->GetXaxis()->SetBinLabel( 
                                                        cutFlow_enum::triggers+2, 
                                                        "Triggers");
    histograms["Overall"]["CutFlow_event"]->GetXaxis()->SetBinLabel( 
                                                        cutFlow_enum::pt+2, 
                                                        "p_{T}");
    histograms["Overall"]["CutFlow_event"]->GetXaxis()->SetBinLabel( 
                                                        cutFlow_enum::eta+2, 
                                                        "#eta");
    histograms["Overall"]["CutFlow_event"]->GetXaxis()->SetBinLabel( 
                                                        cutFlow_enum::fracValidHits+2, 
                                                        "f_{valid/all hits}");
    histograms["Overall"]["CutFlow_event"]->GetXaxis()->SetBinLabel( 
                                                        cutFlow_enum::numDedxHits+2, 
                                                        "N_{dEdx hits}");
    histograms["Overall"]["CutFlow_event"]->GetXaxis()->SetBinLabel( 
                                                        cutFlow_enum::highPurity+2, 
                                                        "HighPurity");
    histograms["Overall"]["CutFlow_event"]->GetXaxis()->SetBinLabel( 
                                                        cutFlow_enum::chi2+2, 
                                                        "#chi^{2} / N_{dof}");
    histograms["Overall"]["CutFlow_event"]->GetXaxis()->SetBinLabel( 
                                                        cutFlow_enum::dz+2, 
                                                        "d_{z}");
    histograms["Overall"]["CutFlow_event"]->GetXaxis()->SetBinLabel( 
                                                        cutFlow_enum::dxy+2, 
                                                        "d_{xy}");
    histograms["Overall"]["CutFlow_event"]->GetXaxis()->SetBinLabel( 
                                                        cutFlow_enum::Ih+2, 
                                                        "Ih");
    histograms["Overall"]["CutFlow_event"]->GetXaxis()->SetBinLabel( 
                                                        cutFlow_enum::trackPtIso+2, 
                                                        "#Sigma_{#DeltaR<0.3} p_{T}");
    histograms["Overall"]["CutFlow_event"]->GetXaxis()->SetBinLabel( 
                                                        cutFlow_enum::sigPtOPt2+2, 
                                                        "#sigma(p_{T})/p_{T}^{2}");
    histograms["Overall"]["CutFlow_event"]->GetXaxis()->SetBinLabel( 
                                                        cutFlow_enum::SR+2, 
                                                        "SR");
    // To save sum of gen weights

    histograms["Overall"]["Total_Gen_Weight"] = fs.make<TH1D>(
                        "Total_Gen_Weight",
                        "Sum of Gen Weights;;Total",
                        1, 0.5, 1.5);
    histograms["Overall"]["Total_Gen_Weight"]->GetXaxis()->SetBinLabel(1, "Total Gen Weight");

    // Event Kinematics - Jet MET
    
    histograms["Event_Kinematics"]["Lead_jet_Pt"] = dirs["Event_Kinematics"].make<TH1D>(
                        "Lead_jet_Pt",
                        "p_{T} of leading Jet in Event;p_T [GeV];Entries / 10 GeV",
                        120, 0, 1200);
    
    histograms["Event_Kinematics"]["All_jet_Pt"] = dirs["Event_Kinematics"].make<TH1D>(
                        "All_jet_Pt",
                        "p_{T} of all Jets in Event;p_T [GeV];Entries / 10 GeV",
                        120, 0, 1200);
    
    histograms["Event_Kinematics"]["Num_of_jets"] = dirs["Event_Kinematics"].make<TH1D>(
                        "Num_of_jets",
                        "Number of Jets in Event;p_T [GeV];Entries / 10 GeV",
                        10, -0.5, 9.5);
    
    histograms["Event_Kinematics"]["HT"] = dirs["Event_Kinematics"].make<TH1D>(
                        "HT",
                        "Event HT (Scalar sum of Jet p_{T});p_T [GeV];Entries / 20 GeV",
                        150, 0, 3000);
    
    histograms["Event_Kinematics"]["MET"] = dirs["Event_Kinematics"].make<TH1D>(
                        "MET",
                        "Event MET;p_T [GeV];Entries / 10 GeV",
                        50, 0, 500);
    
    histograms["Event_Kinematics"]["num_PV"] = dirs["Event_Kinematics"].make<TH1D>(
                        "num_PV",
                        "Number of primary vertices;PVs;Entries / PV",
                        101, -0.5, 100.5);
    

    // Number of candidates in an event
    
    histograms["Overall"]["Num_of_cand_noSel"] = fs.make<TH1D>(
                        "Num_of_cand_noSel",
                        "Number of candidates before preselection;Candidates;Events",
                        10, -0.5, 9.5);
    
    histograms["Overall"]["Num_of_cand_postSel"] = fs.make<TH1D>(
                        "Num_of_cand_postSel",
                        "Number of candidates after preselection;Candidates;Events",
                        10, -0.5, 9.5);
    
    histograms["Overall"]["Num_Events"] = fs.make<TH1D>(
                        "Num_Events",
                        "Number of Events;;Entries / category",
                        5, -0.5, 4.5);
    histograms["Overall"]["Num_Events"]->SetMinimum(0);
    histograms["Overall"]["Num_Events"]->GetXaxis()->SetBinLabel(1, "Total");
    histograms["Overall"]["Num_Events"]->GetXaxis()->SetBinLabel(2, "Passing Trigger");
    histograms["Overall"]["Num_Events"]->GetXaxis()->SetBinLabel(3, "With Candidates");
    histograms["Overall"]["Num_Events"]->GetXaxis()->SetBinLabel(4, "Pass PreSel");
    histograms["Overall"]["Num_Events"]->GetXaxis()->SetBinLabel(5, "Pass PreSel and Trigger");
    
    histograms_2d["Overall"]["Ih_V_pT"] = fs.make<TH2F>(
                        "Ih_V_pT",
                        "I_{h} vs p_{T};p_{T} [GeV];I_{h} [MeV/cm]",
                        /*pT range*/    600, 0, 600,
                        /*Ih range*/    100, 0, 50);
    
    // Varibales associates with candidates 
    
    histograms["Vars_Candidate_b4PS"]["Ecal_maxE"] = 
                    dirs["Vars_Candidate_b4PS"].make<TH1D>(
                        "Ecal_maxE",
                        "Energy of maxE xtal associated w/ track before PreSelection;\
                        Energy [GeV]; Candidates / 0.5 GeV",
                        120, 0, 60);
    
    histograms["Vars_Candidate_b4PS"]["Ecal_maxE_3x3"] = 
                    dirs["Vars_Candidate_b4PS"].make<TH1D>(
                        "Ecal_maxE_3x3",
                        "Total energy in #DeltaR<0.05 of maxE xtal associated w/ track \
                        before PreSelection; Energy [GeV]; Candidates / 0.5 GeV",
                        120, 0, 60);
    
    histograms["Vars_Candidate_b4PS"]["Ecal_maxE_time"] = 
                    dirs["Vars_Candidate_b4PS"].make<TH1D>(
                        "Ecal_maxE_time",
                        "Time of arrival from maxE xtal associated w/ track \
                        before PreSelection;Time [ns]; Candidates / 0.5 ns",
                        50, -5, 20);
    
    histograms["Vars_Candidate_b4PS"]["Ecal_maxE_dR"] = 
                    dirs["Vars_Candidate_b4PS"].make<TH1D>(
                        "Ecal_maxE_dR",
                        "#DeltaR b/w track at ECAL and maxE xtal before PreSelection;\
                        #DeltaR; Candidates / 0.005",
                        20, 0, 0.1);
    
    histograms_2d["Vars_Candidate_b4PS"]["Ecal_maxE_V_EErr"] = 
                    dirs["Vars_Candidate_b4PS"].make<TH2F>(
                        "Ecal_maxE_V_EErr",
                        "ECAL maxE vs ECAL maxE Err before PreSelection;\
                        ECAL maxE Energy [GeV];maxE Err [GeV]",
                        /*maxE range*/    120, 0, 60,
                        /*EErr range*/    50, 0, 5);
    
    histograms_2d["Vars_Candidate_b4PS"]["sigPt_V_pT_high"] = 
                    dirs["Vars_Candidate_b4PS"].make<TH2F>(
                        "sigPt_V_pT_high",
                        "#sigma(p_{T}) vs p_{T} before PreSelection;\
                        p_{T} [GeV];#sigma(p_{T}) [GeV]",
                        /*pT range*/    60, 0, 600,
                        /*sig range*/   40, 0, 200);
    
    histograms_2d["Vars_Candidate_b4PS"]["sigPt_V_pT"] = 
                    dirs["Vars_Candidate_b4PS"].make<TH2F>(
                        "sigPt_V_pT",
                        "#sigma(p_{T}) vs p_{T} before PreSelection;\
                        p_{T} [GeV];#sigma(p_{T}) [GeV]",
                        /*pT range*/    60, 0, 150,
                        /*sig range*/   40, 0, 20);
    
    histograms_2d["Vars_Candidate_b4PS"]["sigPt_V_pT_low"] = 
                    dirs["Vars_Candidate_b4PS"].make<TH2F>(
                        "sigPt_V_pT_low",
                        "#sigma(p_{T}) vs p_{T} before PreSelection;\
                        p_{T} [GeV];#sigma(p_{T}) [GeV]",
                        /*pT range*/    60, 0, 60,
                        /*sig range*/   40, 0, 6);
    
    histograms_2d["Vars_Candidate_b4PS"]["candPt_vs_isolation"] = 
                    dirs["Vars_Candidate_b4PS"].make<TH2F>(
                        "candPt_vs_isolation",
                        "Candidate p_{T} vs Track Isolation  before PreSelection;\
                        p_{T} [GeV];#Sigma_{#DeltaR<0.3} p_{T} [GeV]",
                        /*pT range*/    200, 0, 200,
                        /*sig range*/   100, 0, 100);
    
    histograms["Vars_Candidate"]["Ecal_maxE"] = dirs["Vars_Candidate"].make<TH1D>(
                        "Ecal_maxE",
                        "Energy of maxE xtal associated w/ track;\
                        Energy [GeV]; Candidates / 0.5 GeV",
                        120, 0, 60);
    
    histograms["Vars_Candidate"]["Ecal_maxE_3x3"] = dirs["Vars_Candidate"].make<TH1D>(
                        "Ecal_maxE_3x3",
                        "Total energy in #DeltaR<0.05 of maxE xtal associated w/ track;\
                        Energy [GeV]; Candidates / 0.5 GeV",
                        120, 0, 60);
    
    histograms["Vars_Candidate"]["Ecal_maxE_time"] = dirs["Vars_Candidate"].make<TH1D>(
                        "Ecal_maxE_time",
                        "Time of arrival from maxE xtal associated w/ track;\
                        Time [ns]; Candidates / 0.5 ns",
                        50, -5, 20);
    
    histograms["Vars_Candidate"]["Ecal_maxE_dR"] = dirs["Vars_Candidate"].make<TH1D>(
                        "Ecal_maxE_dR",
                        "#DeltaR b/w track at ECAL and maxE xtal;\
                        #DeltaR; Candidates / 0.005",
                        20, 0, 0.1);
    
    histograms_2d["Vars_Candidate"]["Ecal_maxE_V_EErr"] = 
                    dirs["Vars_Candidate"].make<TH2F>(
                        "Ecal_maxE_V_EErr",
                        "ECAL maxE vs ECAL maxE Err;\
                        ECAL maxE Energy [GeV];maxE Err [GeV]",
                        /*maxE range*/    120, 0, 60,
                        /*EErr range*/    50, 0, 5);
    
    histograms_2d["Vars_Candidate"]["sigPt_V_pT_high"] = dirs["Vars_Candidate"].make<TH2F>(
                        "sigPt_V_pT_high",
                        "#sigma(p_{T}) vs p_{T};p_{T} [GeV];#sigma(p_{T}) [GeV]",
                        /*pT range*/    60, 0, 600,
                        /*sig range*/   40, 0, 200);
    
    histograms_2d["Vars_Candidate"]["sigPt_V_pT"] = dirs["Vars_Candidate"].make<TH2F>(
                        "sigPt_V_pT",
                        "#sigma(p_{T}) vs p_{T};p_{T} [GeV];#sigma(p_{T}) [GeV]",
                        /*pT range*/    60, 0, 150,
                        /*sig range*/   40, 0, 20);
    
    histograms_2d["Vars_Candidate"]["sigPt_V_pT_low"] = dirs["Vars_Candidate"].make<TH2F>(
                        "sigPt_V_pT_low",
                        "#sigma(p_{T}) vs p_{T};p_{T} [GeV];#sigma(p_{T}) [GeV]",
                        /*pT range*/    60, 0, 60,
                        /*sig range*/   40, 0, 6);
    
    histograms_2d["Vars_Candidate"]["candPt_vs_isolation"] = 
                    dirs["Vars_Candidate"].make<TH2F>(
                        "candPt_vs_isolation",
                        "Candidate p_{T} vs Track Isolation;\
                        p_{T} [GeV];#Sigma_{#DeltaR<0.3} p_{T} [GeV]",
                        /*pT range*/    200, 0, 200,
                        /*sig range*/   100, 0, 100);
    
    // Mass plots
    
    histograms["Mass_Plots_b4PS"]["MaxPt_Invariant_Mass"] = dirs["Mass_Plots_b4PS"].make<TH1D>(
                        "MaxPt_Invariant_Mass",
                        "Invariant Mass of two lead candidates;Mass [GeV];Entries",
                        200, 0, 200);
    
    histograms["Mass_Plots_b4PS"]["MaxPt_Invariant_Mass_0to20"] = dirs["Mass_Plots_b4PS"].make<TH1D>(
                        "MaxPt_Invariant_Mass_0to20",
                        "Invariant Mass of two lead candidates;Mass [GeV];Entries",
                        200, 0, 20);
    
    histograms["Mass_Plots_b4PS"]["AllCand_Invariant_Mass"] = dirs["Mass_Plots_b4PS"].make<TH1D>(
                        "AllCand_Invariant_Mass",
                        "Invariant Mass of all candidates;Mass [GeV];Entries",
                        200, 0, 200);
    
    histograms["Mass_Plots_b4PS"]["AllCand_Invariant_Mass_0to20"] = dirs["Mass_Plots_b4PS"].make<TH1D>(
                        "AllCand_Invariant_Mass_0to20",
                        "Invariant Mass of all candidates;Mass [GeV];Entries",
                        200, 0, 20);
    
    histograms["Mass_Plots_b4PS"]["All_OS_Invariant_Mass"] = dirs["Mass_Plots_b4PS"].make<TH1D>(
                        "All_OS_Invariant_Mass",
                        "Invariant Mass of all OS candidates;Mass [GeV];Entries",
                        200, 0, 200);
    
    histograms["Mass_Plots_b4PS"]["All_OS_Invariant_Mass_0to20"] = dirs["Mass_Plots_b4PS"].make<TH1D>(
                        "All_OS_Invariant_Mass_0to20",
                        "Invariant Mass of all OS candidates;Mass [GeV];Entries",
                        200, 0, 20);
    
    histograms["Mass_Plots_b4PS"]["All_SS_Invariant_Mass"] = dirs["Mass_Plots_b4PS"].make<TH1D>(
                        "All_SS_Invariant_Mass",
                        "Invariant Mass of all SS candidates;Mass [GeV];Entries",
                        200, 0, 200);
    
    histograms["Mass_Plots_b4PS"]["All_SS_Invariant_Mass_0to20"] = dirs["Mass_Plots_b4PS"].make<TH1D>(
                        "All_SS_Invariant_Mass_0to20",
                        "Invariant Mass of all SS candidates;Mass [GeV];Entries",
                        200, 0, 20);
    
    histograms["Mass_Plots"]["MaxPt_Invariant_Mass"] = dirs["Mass_Plots"].make<TH1D>(
                        "MaxPt_Invariant_Mass",
                        "Invariant Mass of two lead candidates;Mass [GeV];Entries",
                        200, 0, 200);
    
    histograms["Mass_Plots"]["MaxPt_Invariant_Mass_0to20"] = dirs["Mass_Plots"].make<TH1D>(
                        "MaxPt_Invariant_Mass_0to20",
                        "Invariant Mass of two lead candidates;Mass [GeV];Entries",
                        200, 0, 20);
    
    histograms["Mass_Plots"]["AllCand_Invariant_Mass"] = dirs["Mass_Plots"].make<TH1D>(
                        "AllCand_Invariant_Mass",
                        "Invariant Mass of all candidates;Mass [GeV];Entries",
                        200, 0, 200);
    
    histograms["Mass_Plots"]["AllCand_Invariant_Mass_0to20"] = dirs["Mass_Plots"].make<TH1D>(
                        "AllCand_Invariant_Mass_0to20",
                        "Invariant Mass of all candidates;Mass [GeV];Entries",
                        200, 0, 20);
    
    histograms["Mass_Plots"]["All_OS_Invariant_Mass"] = dirs["Mass_Plots"].make<TH1D>(
                        "All_OS_Invariant_Mass",
                        "Invariant Mass of all OS candidates;Mass [GeV];Entries",
                        200, 0, 200);
    
    histograms["Mass_Plots"]["All_OS_Invariant_Mass_0to20"] = dirs["Mass_Plots"].make<TH1D>(
                        "All_OS_Invariant_Mass_0to20",
                        "Invariant Mass of all OS candidates;Mass [GeV];Entries",
                        200, 0, 20);
    
    histograms["Mass_Plots"]["All_SS_Invariant_Mass"] = dirs["Mass_Plots"].make<TH1D>(
                        "All_SS_Invariant_Mass",
                        "Invariant Mass of all SS candidates;Mass [GeV];Entries",
                        200, 0, 200);
    
    histograms["Mass_Plots"]["All_SS_Invariant_Mass_0to20"] = dirs["Mass_Plots"].make<TH1D>(
                        "All_SS_Invariant_Mass_0to20",
                        "Invariant Mass of all SS candidates;Mass [GeV];Entries",
                        200, 0, 20);
    
}

void HistogramManager::fillHistograms(const std::string& category, 
                                            const std::string& variable, 
                                            float value,
                                            float weight) 
{
    if (histograms.find(category) != histograms.end() && 
                histograms[category].find(variable) != histograms[category].end()) {
        histograms[category][variable]->Fill(value, weight);
    }
    else { 
        throw std::invalid_argument("No such 1D histogram found: " + category + " " +
                                                                        variable);
    }
}

void HistogramManager::fillHistograms(const std::string& category, 
                                            const std::string& variable, 
                                            float value_x, 
                                            float value_y,
                                            float weight) 
{
    if (histograms_2d.find(category) != histograms_2d.end() && 
                histograms_2d[category].find(variable) != histograms_2d[category].end()) {
        histograms_2d[category][variable]->Fill(value_x, value_y, weight);
    }
    else { 
        throw std::invalid_argument("No such 2D histogram found: " + category + " " +
                                                                        variable);
    }
}

void HistogramManager::scaleAllHistograms(const double value) 
{
    for (auto& cat : histograms) 
    {
        for (auto& hist : cat.second) 
            hist.second->Scale(value);
    }
    for (auto& cat : histograms_2d) 
    {
        for (auto& hist : cat.second) 
            hist.second->Scale(value);
    }
}

