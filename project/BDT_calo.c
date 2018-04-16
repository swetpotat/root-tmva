#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include "TChain.h"
#include "TFile.h"
#include "TTree.h"
#include "TString.h"
#include "TObjString.h"
#include "TSystem.h"
#include "TROOT.h"
#include "TMVA/Factory.h"
#include "TMVA/DataLoader.h"
#include "TMVA/Tools.h"
#include "TMVA/TMVAGui.h"
#include "TMVA/Types.h"

int BDT_calo( TString myMethodList = "" ){

   // This loads the library
   TMVA::Tools::Instance();

   // Default MVA methods to be trained + tested
   std::map<std::string,int> Use;
   
   // Boosted Decision Trees
   //Use["BDT"]               = 0; 
   Use["BDTAC"]               = 0; 
   Use["BDTAGI"]              = 1; 
   Use["BDTGRCR"]             = 0; 
   Use["BDTGRGI"]             = 1;
   Use["BDTBC"]               = 0; 
   Use["BDTBGI"]              = 1; 

   // ---------------------------------------------------------------
   
   std::cout << std::endl;
   std::cout << "==> Start BDTClassification" << std::endl;

   // Select methods (don't look at this code - not of interest)
   if (myMethodList != "") {
      for (std::map<std::string,int>::iterator it = Use.begin(); it != Use.end(); it++) it->second = 0;
      std::vector<TString> mlist = TMVA::gTools().SplitString( myMethodList, ',' );
      for (UInt_t i=0; i<mlist.size(); i++) {
         std::string regMethod(mlist[i]);
         if (Use.find(regMethod) == Use.end()) {
            std::cout << "Method \"" << regMethod << "\" not known in TMVA under this name. Choose among the following:" << std::endl;
            for (std::map<std::string,int>::iterator it = Use.begin(); it != Use.end(); it++) std::cout << it->first << " ";
            std::cout << std::endl;
            return 1;
         }
         Use[regMethod] = 1;
      }
   }

   // --------------------------------------------------------------------------------------------------

   // Read training data
   TString fname_training_sig = "./data/MC_SigElectrons_2000000ev.root";
   if (gSystem->AccessPathName( fname_training_sig ))  
      std::cout << "FILE DOES NOT EXIST" << std::endl;
   TString fname_training_bkg = "./data/MC_BkgElectrons_2000000ev.root";
   if (gSystem->AccessPathName( fname_training_bkg ))  
      std::cout << "FILE DOES NOT EXIST" << std::endl;
   TFile *input_training_sig = TFile::Open( fname_training_sig );
   TFile *input_training_bkg = TFile::Open( fname_training_bkg );
   std::cout << "--- BDTClassification       : Using input signal training file: " << input_training_sig->GetName() << std::endl;
   std::cout << "                            : Using input background training file: " << input_training_bkg->GetName() << std::endl;

   // Register the training trees
   TTree *signalTree_training     = (TTree*)input_training_sig->Get("data");
   TTree *background_training     = (TTree*)input_training_bkg->Get("data");

   // Read test data
   TString fname_testing_sig = "./data/MC_SigElectrons_500000ev.root";
   if (gSystem->AccessPathName( fname_testing_sig ))
   	  std::cout << "FILE DOES NOT EXIST" << std::endl;
   TString fname_testing_bkg = "./data/MC_BkgElectrons_500000ev.root";
   if (gSystem->AccessPathName( fname_testing_bkg ))
   	  std::cout << "FILE DOES NOT EXIST" << std::endl;
   TFile *input_testing_sig = TFile::Open( fname_testing_sig );
   TFile *input_testing_bkg = TFile::Open( fname_testing_bkg );
   std::cout << "--- BDTClassificaion        : Using input signal testing file: " << input_testing_sig->GetName() << std::endl;
   std::cout << "                            : Using input background testing file: " << input_testing_bkg->GetName() << std::endl;

   // Register the testing trees
   TTree *signalTree_testing      = (TTree*)input_testing_sig->Get("data");
   TTree *background_testing      = (TTree*)input_testing_bkg->Get("data");

   // Create a ROOT output file where TMVA will store ntuples, histograms, etc.
   TString outfileName( "BDT_allGI_calo_results.root" );
   TFile* outputFile = TFile::Open( outfileName, "RECREATE" );

   // Create the factory object. Later you can choose the methods
   // whose performance you'd like to investigate. The factory is
   // the only TMVA object you have to interact with
   //
   // The first argument is the base of the name of all the
   // weightfiles in the directory weight/
   //
   // The second argument is the output file for the training results
   // All TMVA output can be suppressed by removing the "!" (not) in
   // front of the "Silent" argument in the option string
   TMVA::Factory *factory = new TMVA::Factory( "BDTClassification", outputFile, "!V:!Silent:Color:DrawProgressBar:Transformations=I;D;P;G,D:AnalysisType=Classification" );
   TMVA::DataLoader *dataloader=new TMVA::DataLoader("dataset");

   // Define the input variables that shall be used for the MVA training - calorimeter variables
   dataloader->AddVariable( "p_Rhad1", "Rhad1", "units", 'F' );
   dataloader->AddVariable( "p_Rhad", "Rhad", "units", 'F' );
   dataloader->AddVariable( "p_f3", "f3", "units", 'F' );
   dataloader->AddVariable( "p_weta2", "weta2", "units", 'F' );
   dataloader->AddVariable( "p_Rphi", "Rphi", "units", 'F' );
   dataloader->AddVariable( "p_Reta", "Reta", "units", 'F' );
   dataloader->AddVariable( "p_Eratio", "Eratio", "units", 'F' );
   dataloader->AddVariable( "p_f1", "f1", "units", 'F' );

   // Global event weights per tree
   Double_t signalWeight     = 1.0;
   Double_t backgroundWeight = 1.0;

   // You can add an arbitrary number of signal or background trees
   dataloader->AddSignalTree    ( signalTree_training, signalWeight, TMVA::Types::kTraining);
   dataloader->AddBackgroundTree( background_training, backgroundWeight, TMVA::Types::kTraining );
   dataloader->AddSignalTree    ( signalTree_testing, signalWeight, TMVA::Types::kTesting);
   dataloader->AddBackgroundTree( background_testing, backgroundWeight, TMVA::Types::kTesting);

   // Set individual event weights (the variables must exist in the original TTree)
   // -  for signal    : `dataloader->SetSignalWeightExpression    ("weight1*weight2");`
   // -  for background: `dataloader->SetBackgroundWeightExpression("weight1*weight2");`
   //dataloader->SetBackgroundWeightExpression( "weight" );

   // Apply additional cuts on the signal and background samples (can be different)
   TCut mycuts = ""; 
   TCut mycutb = ""; 

   // Tell the dataloader how to use the training and testing events
   dataloader->PrepareTrainingAndTestTree( mycuts, mycutb, "nTrain_Signal=848338:nTrain_Background=1875593:nTest_Signal=212567:nTest_Background=468199:SplitMode=Random:!V");
  
   // Book the MVA method
   // Boosted Decision Trees
   
   //if (Use["BDT"]) 
      //factory->BookMethod( dataloader, TMVA::Types::kBDT, "BDT","!H:!V:NTrees=200:MinNodeSize=2.5%:MaxDepth=3:SeparationType=CrossEntropy:BoostType=AdaBoost:AdaBoostBeta=0.5:UseBaggedBoost:BaggedSampleFraction=0.5:SeparationType=GiniIndex:nCuts=20"
   //);

   if (Use["BDTAC"])  // Adaptive Boost - Cross Entropy
      factory->BookMethod( dataloader, TMVA::Types::kBDT, "BDTAC","!H:!V:NTrees=200:MinNodeSize=2.5%:MaxDepth=3:SeparationType=CrossEntropy:BoostType=AdaBoost:AdaBoostBeta=0.5:UseBaggedBoost:BaggedSampleFraction=0.5:SeparationType=GiniIndex:nCuts=20"
   );

   if (Use["BDTAGI"])  // Adaptive Boost - GiniIndex
      factory->BookMethod( dataloader, TMVA::Types::kBDT, "BDTAGI","!H:!V:NTrees=200:MinNodeSize=2.5%:MaxDepth=3:SeparationType=GiniIndex:BoostType=AdaBoost:AdaBoostBeta=0.5:UseBaggedBoost:BaggedSampleFraction=0.5:SeparationType=GiniIndex:nCuts=20"
   );

   if (Use["BDTGRCR"])  // Grad Boost - Cross Entropy
      factory->BookMethod( dataloader, TMVA::Types::kBDT, "BDTGRCR","!H:!V:NTrees=200:MinNodeSize=2.5%:MaxDepth=3:SeparationType=CrossEntropy:BoostType=Grad:AdaBoostBeta=0.5:UseBaggedBoost:BaggedSampleFraction=0.5:SeparationType=GiniIndex:nCuts=20"
   );

   if (Use["BDTGRGI"])  // Grad Boost - Gini Index
      factory->BookMethod( dataloader, TMVA::Types::kBDT, "BDTGRGI","!H:!V:NTrees=200:MinNodeSize=2.5%:MaxDepth=3:SeparationType=GiniIndex:BoostType=Grad:AdaBoostBeta=0.5:UseBaggedBoost:BaggedSampleFraction=0.5:SeparationType=GiniIndex:nCuts=20"
   );

   if (Use["BDTBC"])  // Bagging - Cross Entropy
      factory->BookMethod( dataloader, TMVA::Types::kBDT, "BDTGC","!H:!V:NTrees=200:MinNodeSize=2.5%:MaxDepth=3:SeparationType=CrossEntropy:BoostType=Bagging:AdaBoostBeta=0.5:UseBaggedBoost:BaggedSampleFraction=0.5:SeparationType=GiniIndex:nCuts=20"
   );

   if (Use["BDTBGI"])  // Bagging - GiniIndex
      factory->BookMethod( dataloader, TMVA::Types::kBDT, "BDTGGI","!H:!V:NTrees=200:MinNodeSize=2.5%:MaxDepth=3:SeparationType=GiniIndex:BoostType=Bagging:AdaBoostBeta=0.5:UseBaggedBoost:BaggedSampleFraction=0.5:SeparationType=GiniIndex:nCuts=20"
   );

 
   // --------------------------------------------------------------------------------------------------
   
   // Train MVAs using the set of training events
   factory->TrainAllMethods();
   // Evaluate all MVAs using the set of test events
   factory->TestAllMethods();
   // Evaluate and compare performance of all configured MVAs
   factory->EvaluateAllMethods();

   // --------------------------------------------------------------

   // Save the output
   outputFile->Close();
   std::cout << "==> Wrote root file: " << outputFile->GetName() << std::endl;
   std::cout << "==> BDTClassification is done!" << std::endl;
   delete factory;
   delete dataloader;

   // Launch the GUI for the root macros
   if (!gROOT->IsBatch()) TMVA::TMVAGui( outfileName );
  	 return 0;

}

int main( int argc, char** argv ){

   // Select methods (don't look at this code - not of interest)
   TString methodList;
   for (int i=1; i<argc; i++) {
      TString regMethod(argv[i]);
      if(regMethod=="-b" || regMethod=="--batch") continue;
      if (!methodList.IsNull()) methodList += TString(",");
      	methodList += regMethod;
   }

   return BDT_calo(methodList);

}

