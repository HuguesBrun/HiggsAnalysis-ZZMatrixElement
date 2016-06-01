//-----------------------------------------------------------------------------
//
// Class EventProb Module
//
//   EventProb Module
//
// March 21 2011
// S. Jindariani (sergo@fnal.gov)
// Y. Gao (ygao@fnal.gov)
// K. Burkett (burkett@fnal.gov)
//-----------------------------------------------------------------------------

#include <ZZMatrixElement/MELA/interface/TEvtProb.hh>


ClassImp(TEvtProb)


using namespace std;
using namespace TUtil;


//-----------------------------------------------------------------------------
// Constructors and Destructor
//-----------------------------------------------------------------------------
TEvtProb::TEvtProb(
  const char* path, double ebeam, const char* pathtoPDFSet, int PDFMember
  ) :
  verbosity(TVar::ERROR),
  EBEAM(ebeam)
{
  mcfm_init_((char *)"input.DAT", (char *)"./");
  SetEwkCouplingParameters();
  energy_.sqrts = 2.*EBEAM;
  coupling_();
  string path_string = path;
  myCSW_ = new HiggsCSandWidth_MELA(path_string);
  //std::cout << path << std::endl;
  SetLeptonInterf(TVar::DefaultLeptonInterf);

  // First resonance constant parameters
  spinzerohiggs_anomcoupl_.LambdaBSM=1000;
  spinzerohiggs_anomcoupl_.Lambda_z1=10000;
  spinzerohiggs_anomcoupl_.Lambda_z2=10000;
  spinzerohiggs_anomcoupl_.Lambda_z3=10000;
  spinzerohiggs_anomcoupl_.Lambda_z4=10000;
  spinzerohiggs_anomcoupl_.Lambda_zgs1=10000;
  spinzerohiggs_anomcoupl_.Lambda_Q=10000;
  // Second resonance constant parameters
  spinzerohiggs_anomcoupl_.Lambda2BSM=1000;
  spinzerohiggs_anomcoupl_.Lambda2_z1=10000;
  spinzerohiggs_anomcoupl_.Lambda2_z2=10000;
  spinzerohiggs_anomcoupl_.Lambda2_z3=10000;
  spinzerohiggs_anomcoupl_.Lambda2_z4=10000;
  spinzerohiggs_anomcoupl_.Lambda2_zgs1=10000;
  spinzerohiggs_anomcoupl_.Lambda2_Q=10000;

  InitJHUGenMELA(pathtoPDFSet, PDFMember);

  ResetCouplings();
  ResetRenFacScaleMode();
  ResetInputEvent();
  SetHiggsMass(125., 4.07e-3, -1);
}


TEvtProb::~TEvtProb(){
  ResetInputEvent();
  if (myCSW_!=0) delete myCSW_;
}

/*
void TEvtProb::ResetMCFM_EWKParameters(double ext_Gf, double ext_aemmz, double ext_mW, double ext_mZ){
  ewinput_.Gf_inp = ext_Gf;
  ewinput_.aemmz_inp = ext_aemmz;
  ewinput_.wmass_inp = ext_mW;
  ewinput_.zmass_inp = ext_mZ;
  ewinput_.xw_inp = 1.-pow(ext_mW/ext_mZ,2);
  coupling_();
}
*/

// Set NNPDF driver path
void TEvtProb::Set_LHAgrid(const char* path, int pdfmember){
  char path_nnpdf_c[200];
  sprintf(path_nnpdf_c, "%s", path);
  int pathLength = strlen(path_nnpdf_c);
  nnpdfdriver_(path_nnpdf_c, &pathLength);
  nninitpdf_(&pdfmember);
}
void TEvtProb::SetProcess(TVar::Process tmp) { process = tmp; }
void TEvtProb::SetMatrixElement(TVar::MatrixElement tmp){ matrixElement = tmp; }
void TEvtProb::SetProduction(TVar::Production tmp){ production = tmp; }
void TEvtProb::SetVerbosity(TVar::VerbosityLevel tmp){ verbosity = tmp; }
void TEvtProb::SetLeptonInterf(TVar::LeptonInterference tmp){ leptonInterf = tmp; }
void TEvtProb::SetRenFacScaleMode(TVar::EventScaleScheme renormalizationSch, TVar::EventScaleScheme factorizationSch, double ren_sf, double fac_sf){
  event_scales.renomalizationScheme = renormalizationSch;
  event_scales.factorizationScheme = factorizationSch;
  event_scales.ren_scale_factor = ren_sf;
  event_scales.fac_scale_factor = fac_sf;
}
void TEvtProb::AllowSeparateWWCouplings(bool doAllow){ SetJHUGenDistinguishWWCouplings(doAllow); selfDSpinZeroCoupl.allow_WWZZSeparation(doAllow); }
void TEvtProb::SetHiggsMass(double mass, double wHiggs, int whichResonance){
  // Regular, first resonance
  if (whichResonance==1 || whichResonance==-1){
    if (mass<0.){
      _hmass = -1;
      _hwidth = 0;
    }
    else if (wHiggs<0.){
      _hmass = mass;
      _hwidth = myCSW_->HiggsWidth(0, min(_hmass, 1000.));
    }
    else{
      _hmass = mass;
      _hwidth = wHiggs;
    }
    masses_mcfm_.hmass = _hmass;
    masses_mcfm_.hwidth = _hwidth;

    if (_hmass<0.) SetJHUGenHiggsMassWidth(0., _hwidth);
    else SetJHUGenHiggsMassWidth(_hmass, _hwidth);
  }

  // Second resonance
  if (whichResonance==2){
    if (wHiggs<=0. || mass<0.){
      _h2mass = -1;
      _h2width = 0;
    }
    else{
      _h2mass = mass;
      _h2width = wHiggs;
    }
    spinzerohiggs_anomcoupl_.h2mass = _h2mass;
    spinzerohiggs_anomcoupl_.h2width = _h2width;
    //SetJHUGenHiggsMassWidth(_h2mass, _h2width); // Second resonance is not implemented in JHUGen yet.
  }
  else if (whichResonance==-1){
    _h2mass = -1;
    _h2width = 0;
    spinzerohiggs_anomcoupl_.h2mass = _h2mass;
    spinzerohiggs_anomcoupl_.h2width = _h2width;
    //SetJHUGenHiggsMassWidth(_h2mass, _h2width); // Second resonance is not implemented in JHUGen yet.
  }
}
void TEvtProb::SetInputEvent(
  SimpleParticleCollection_t* pDaughters,
  SimpleParticleCollection_t* pAssociated,
  SimpleParticleCollection_t* pMothers,
  bool isGen
  ){
  MELACandidate* cand = ConvertVectorFormat(
    pDaughters,
    pAssociated,
    pMothers,
    isGen,
    &particleList, &candList // push_back is done automatically
    );
  if (cand!=0) melaCand=cand;
}
void TEvtProb::AppendTopCandidate(SimpleParticleCollection_t* TopDaughters){
  if (!CheckInputPresent()){
    cerr << "TEvtProb::AppendTopCandidate: No MELACandidates are present to append this top!" << endl;
    return;
  }
  MELATopCandidate* cand = ConvertTopCandidate(
    TopDaughters,
    &particleList, &topCandList // push_back is done automatically
    );
  if (cand!=0) melaCand->addAssociatedTops(cand);
}
void TEvtProb::SetRcdCandPtr(){ RcdME.melaCand = melaCand; }
void TEvtProb::SetCurrentCandidate(unsigned int icand){
  if (candList.size()>icand) melaCand = candList.at(icand);
  else cerr << "TEvtProb::SetCurrentCandidate: icand=" << icand << ">=candList.size()=" << candList.size() << endl;
}
void TEvtProb::SetCurrentCandidate(MELACandidate* cand){
  melaCand = cand;
  if (verbosity>=TVar::INFO && melaCand==0) cout << "TEvtProb::SetCurrentCandidate: BE CAREFUL! melaCand==0!" << endl;
  if (verbosity>=TVar::INFO && GetCurrentCandidateIndex()<0) cout << "TEvtProb::SetCurrentCandidate: The current candidate is not in the list of candidates. It is the users' responsibility to delete this candidate and all of its associated particles." << endl;
}

// Reset functions
void TEvtProb::ResetIORecord(){ RcdME.reset(); }
void TEvtProb::ResetRenFacScaleMode(){ SetRenFacScaleMode(TVar::DefaultScaleScheme, TVar::DefaultScaleScheme, 0.5, 0.5); }
void TEvtProb::ResetMCFM_EWKParameters(double ext_Gf, double ext_aemmz, double ext_mW, double ext_mZ, double ext_xW, int ext_ewscheme){
  if (ext_ewscheme<-1 || ext_ewscheme>3) ext_ewscheme=3;
  ewinput_.Gf_inp = ext_Gf;
  ewinput_.aemmz_inp = ext_aemmz;
  ewinput_.wmass_inp = ext_mW;
  ewinput_.zmass_inp = ext_mZ;
  ewinput_.xw_inp = ext_xW;
  ewscheme_.ewscheme = ext_ewscheme;
  coupling_();
}
void TEvtProb::ResetCouplings(){
  selfDSpinZeroCoupl.reset();
  selfDSpinOneCoupl.reset();
  selfDSpinTwoCoupl.reset();
  AllowSeparateWWCouplings(false);
}
void TEvtProb::ResetInputEvent(){
  RcdME.melaCand = 0;
  melaCand = 0;

  // Clear bookkeeping objects
  // Clean MELACandidates first since they contain all other objects
  for (unsigned int p=0; p<candList.size(); p++){
    MELACandidate* tmpCand = (MELACandidate*)candList.at(p);
    if (tmpCand!=0) delete tmpCand;
  }
  candList.clear();
  // Clean MELATopCandidates next since they contain MELAParticles
  for (unsigned int p=0; p<topCandList.size(); p++){
    MELATopCandidate* tmpCand = (MELATopCandidate*)topCandList.at(p);
    if (tmpCand!=0) delete tmpCand;
  }
  topCandList.clear();
  // Clean all remaining MELAPArticles
  for (unsigned int p=0; p<particleList.size(); p++){
    MELAParticle* tmpPart = (MELAParticle*)particleList.at(p);
    if (tmpPart!=0) delete tmpPart;
  }
  particleList.clear();
}

// Get-functions
SpinZeroCouplings* TEvtProb::GetSelfDSpinZeroCouplings(){ return selfDSpinZeroCoupl.getRef(); }
SpinOneCouplings* TEvtProb::GetSelfDSpinOneCouplings(){ return selfDSpinOneCoupl.getRef(); }
SpinTwoCouplings* TEvtProb::GetSelfDSpinTwoCouplings(){ return selfDSpinTwoCoupl.getRef(); }
MelaIO* TEvtProb::GetIORecord(){ return RcdME.getRef(); }
std::vector<MELATopCandidate*>* TEvtProb::GetTopCandidates(){ return &topCandList; }
MELACandidate* TEvtProb::GetCurrentCandidate(){ return melaCand; }
int TEvtProb::GetCurrentCandidateIndex(){
  if (melaCand==0) return -1;
  for (unsigned int icand=0; icand<candList.size(); icand++){
    if (candList.at(icand)==melaCand) return (int)icand;
  }
  return -1;
}

// Check/test functions
bool TEvtProb::CheckInputPresent(){
  if (melaCand==0 || candList.size()==0){
    cerr
      << "TEvtProb::XsecCalc_XVV: melaCand==" << melaCand << " is nullPtr"
      << " or candList.size()==" << candList.size() << " is problematic!"
      << endl;
    if (candList.size()==0) return false;
    else{
      SetCurrentCandidate(candList.size()-1);
      cerr << "TEvtProb::XsecCalc_XVV: melaCand now points to the latest candidate (cand" << (candList.size()-1) << ")" << endl;
    }
  }
  SetRcdCandPtr(); // If input event is present, set the RcdME pointer to melaCand
  return true;
}


// ME functions

//
// Directly calculate the VV->4f differential cross-section
//
double TEvtProb::XsecCalc_XVV(
  TVar::Process process_, TVar::Production production_,
  TVar::VerbosityLevel verbosity_
  ){
  double dXsec=0;
  ResetIORecord();
  if (!CheckInputPresent()) return dXsec;

  //Initialize Process
  SetProcess(process_);
  SetProduction(production_);
  SetVerbosity(verbosity_);

  bool forceUseMCFM = (matrixElement == TVar::MCFM || process == TVar::bkgZZ_SMHiggs);
  bool calculateME=true;

  bool needBSMHiggs=false;
  if (forceUseMCFM){
    // Check self-defined couplings are specified.
    for (int vv = 0; vv < SIZE_HVV; vv++){
      if (
        (selfDSpinZeroCoupl.Hzzcoupl)[vv][1] != 0 || (selfDSpinZeroCoupl.Hzzcoupl)[vv][0] != 0
        ||
        (selfDSpinZeroCoupl.Hwwcoupl)[vv][1] != 0 || (selfDSpinZeroCoupl.Hwwcoupl)[vv][0] != 0
        ){
        needBSMHiggs = true; break;
      } // Only possible if selfDefine is called.
    }
    if (_h2mass>=0. && _h2width>0. && !needBSMHiggs){
      for (int vv = 0; vv < SIZE_HVV; vv++){
        if (
          (selfDSpinZeroCoupl.H2zzcoupl)[vv][1] != 0 || (selfDSpinZeroCoupl.H2zzcoupl)[vv][0] != 0
          ||
          (selfDSpinZeroCoupl.H2wwcoupl)[vv][1] != 0 || (selfDSpinZeroCoupl.H2wwcoupl)[vv][0] != 0
          ){
          needBSMHiggs = true; break;
        } // Only possible if selfDefine is called.
      }
    }
    if (needBSMHiggs) SetLeptonInterf(TVar::InterfOn); // All anomalous coupling computations have to use lepton interference
    SetMCFMSpinZeroVVCouplings(needBSMHiggs, &selfDSpinZeroCoupl, false);
    calculateME = MCFM_chooser(process, production, leptonInterf, melaCand);
  }

  // Final check before moving on to ME calculations
  if (!calculateME) return dXsec;

  // ME calculations
  if (forceUseMCFM) dXsec = SumMatrixElementPDF(process, production, matrixElement, &event_scales, &RcdME, EBEAM, (selfDSpinZeroCoupl.Hvvcoupl_freenorm), verbosity);
  else if (matrixElement == TVar::JHUGen){
    AllowSeparateWWCouplings(false); // HZZ couplings are used for both in spin-0
    // all the possible couplings
    double Hggcoupl[SIZE_HGG][2] ={ { 0 } };
    double Hvvcoupl[SIZE_HVV][2] ={ { 0 } };
    double HvvLambda_qsq[4][3] ={ { 0 } };
    int HvvCLambda_qsq[3] ={ 0 };

    double Zqqcoupl[SIZE_ZQQ][2] ={ { 0 } };
    double Zvvcoupl[SIZE_ZVV][2] ={ { 0 } };

    double Gqqcoupl[SIZE_GQQ][2] ={ { 0 } };
    double Gggcoupl[SIZE_GGG][2] ={ { 0 } };
    double Gvvcoupl[SIZE_GVV][2] ={ { 0 } };

    //
    // set spin 0 default numbers
    //
    // By default set the Spin 0 couplings for SM case (0+m)
    Hggcoupl[0][0]=1.0;  Hggcoupl[0][1]=0.0;
    Hvvcoupl[0][0]=1.0;  Hvvcoupl[0][1]=0.0;
    for (int ic=0; ic<4; ic++){ for (int ik=0; ik<3; ik++) HvvLambda_qsq[ic][ik]=100.; }
    //
    // set spin 2 default numbers (2+m)
    //
    Gqqcoupl[0][0]=1.0;  Gqqcoupl[0][1]=0.0;
    Gqqcoupl[1][0]=1.0;  Gqqcoupl[1][1]=0.0;
    Gggcoupl[0][0]=1.0;  Gggcoupl[0][1]=0.0;
    Gvvcoupl[0][0]=1.0;  Gvvcoupl[0][1]=0.0;
    Gvvcoupl[4][0]=1.0;  Gvvcoupl[4][1]=0.0;
    //
    // set spin 1 default numbers (1-)
    //
    Zqqcoupl[0][0]=1.0;  Zqqcoupl[0][1]=0.0;
    Zqqcoupl[1][0]=1.0;  Zqqcoupl[1][1]=0.0;
    Zvvcoupl[0][0]=1.0;  Zvvcoupl[0][1]=0.0; // 1-

    bool isSpinZero = false;
    bool isSpinOne = false;
    bool isSpinTwo = false;

    if (process == TVar::HSMHiggs) isSpinZero = true; // Already the default
    // 0-
    else if (process == TVar::H0minus) {
      Hvvcoupl[0][0] = 0.0;
      Hvvcoupl[1][0] = 0.0;
      Hvvcoupl[2][0] = 0.0;
      Hvvcoupl[3][0] = 1.0;
      isSpinZero = true;
    }
    // 0h+
    else if (process == TVar::H0hplus) {
      Hvvcoupl[0][0] = 0.0;
      Hvvcoupl[1][0] = 1.0;
      Hvvcoupl[2][0] = 0.0;
      Hvvcoupl[3][0] = 0.0;
      isSpinZero = true;
    }
    // 0+L1
    else if (process == TVar::H0_g1prime2){
      Hvvcoupl[0][0] = 0.;
      Hvvcoupl[11][0] = -12046.01;
      isSpinZero = true;
    }
    else if (process == TVar::H0_Zgs) {
      Hvvcoupl[4][0] = 0.0688;
      Hvvcoupl[0][0] = 0.;
      isSpinZero = true;
    }
    else if (process == TVar::H0_gsgs) {
      Hvvcoupl[7][0] = -0.0898;
      Hvvcoupl[0][0] = 0.;
      isSpinZero = true;
    }
    else if (process == TVar::H0_Zgs_PS) {
      Hvvcoupl[6][0] = 0.0855;
      Hvvcoupl[0][0] = 0.;
      isSpinZero = true;
    }
    else if (process == TVar::H0_gsgs_PS) {
      Hvvcoupl[9][0] = -0.0907;
      Hvvcoupl[0][0] = 0.;
      isSpinZero = true;
    }
    else if (process == TVar::H0_Zgsg1prime2) {
      Hvvcoupl[30][0] = -7591.914; // +- 6.613
      Hvvcoupl[0][0] = 0.;
      isSpinZero = true;
    }
    else if (process == TVar::SelfDefine_spin0){
      for (int j=0; j<2; j++){
        for (int i=0; i<SIZE_HGG; i++) Hggcoupl[i][j] = (selfDSpinZeroCoupl.Hggcoupl)[i][j];
        for (int i=0; i<SIZE_HVV; i++) Hvvcoupl[i][j] = (selfDSpinZeroCoupl.Hzzcoupl)[i][j];
      }
      for (int j=0; j<3; j++){
        for (int i=0; i<4; i++) HvvLambda_qsq[i][j] = (selfDSpinZeroCoupl.HzzLambda_qsq)[i][j];
        HvvCLambda_qsq[j] = (selfDSpinZeroCoupl.HzzCLambda_qsq)[j];
      }
      isSpinZero = true;
    }


    else if (process == TVar::H2_g1g5) isSpinTwo = true; // Already the default
    // 2h-
    else if (process == TVar::H2_g8){
      // gg production coupling constants
      Gggcoupl[0][0]=0.0;  Gggcoupl[0][1]=0.0;
      Gggcoupl[1][0]=0.0;  Gggcoupl[1][1]=0.0;
      Gggcoupl[2][0]=0.0;  Gggcoupl[2][1]=0.0;
      Gggcoupl[3][0]=0.0;  Gggcoupl[3][1]=0.0;
      Gggcoupl[4][0]=1.0;  Gggcoupl[4][1]=0.0; // 2h-

      // Graviton->ZZ coupling constants
      Gvvcoupl[0][0]=0.0;  Gvvcoupl[0][1]=0.0;
      Gvvcoupl[1][0]=0.0;  Gvvcoupl[1][1]=0.0;
      Gvvcoupl[2][0]=0.0;  Gvvcoupl[2][1]=0.0;
      Gvvcoupl[3][0]=0.0;  Gvvcoupl[3][1]=0.0;
      Gvvcoupl[4][0]=0.0;  Gvvcoupl[4][1]=0.0;
      Gvvcoupl[5][0]=0.0;  Gvvcoupl[5][1]=0.0;
      Gvvcoupl[6][0]=0.0;  Gvvcoupl[6][1]=0.0;
      Gvvcoupl[7][0]=1.0;  Gvvcoupl[7][1]=0.0; // 2h-
      Gvvcoupl[8][0]=0.0;  Gvvcoupl[8][1]=0.0;
      Gvvcoupl[9][0]=0.0;  Gvvcoupl[9][1]=0.0;

      isSpinTwo = true;
    }
    // 2h+
    else if (process == TVar::H2_g4){
      // gg production coupling constants
      Gggcoupl[0][0]=0.0;  Gggcoupl[0][1]=0.0;
      Gggcoupl[1][0]=0.0;  Gggcoupl[1][1]=0.0;
      Gggcoupl[2][0]=0.0;  Gggcoupl[2][1]=0.0;
      Gggcoupl[3][0]=1.0;  Gggcoupl[3][1]=0.0; // 2h+
      Gggcoupl[4][0]=0.0;  Gggcoupl[4][1]=0.0;

      // Graviton->ZZ coupling constants
      Gvvcoupl[0][0]=0.0;  Gvvcoupl[0][1]=0.0;
      Gvvcoupl[1][0]=0.0;  Gvvcoupl[1][1]=0.0;
      Gvvcoupl[2][0]=0.0;  Gvvcoupl[2][1]=0.0;
      Gvvcoupl[3][0]=1.0;  Gvvcoupl[3][1]=0.0; // 2h+
      Gvvcoupl[4][0]=0.0;  Gvvcoupl[4][1]=0.0;
      Gvvcoupl[5][0]=0.0;  Gvvcoupl[5][1]=0.0;
      Gvvcoupl[6][0]=0.0;  Gvvcoupl[6][1]=0.0;
      Gvvcoupl[7][0]=0.0;  Gvvcoupl[7][1]=0.0;
      Gvvcoupl[8][0]=0.0;  Gvvcoupl[8][1]=0.0;
      Gvvcoupl[9][0]=0.0;  Gvvcoupl[9][1]=0.0;

      isSpinTwo = true;
    }
    // 2b+
    else if (process == TVar::H2_g5){
      // gg production coupling constants
      Gggcoupl[0][0]=1.0;  Gggcoupl[0][1]=0.0;  // 2b+
      Gggcoupl[1][0]=0.0;  Gggcoupl[1][1]=0.0;
      Gggcoupl[2][0]=0.0;  Gggcoupl[2][1]=0.0;
      Gggcoupl[3][0]=0.0;  Gggcoupl[3][1]=0.0;
      Gggcoupl[4][0]=0.0;  Gggcoupl[4][1]=0.0;

      // Graviton->ZZ coupling constants
      Gvvcoupl[0][0]=0.0;  Gvvcoupl[0][1]=0.0;
      Gvvcoupl[1][0]=0.0;  Gvvcoupl[1][1]=0.0;
      Gvvcoupl[2][0]=0.0;  Gvvcoupl[2][1]=0.0;
      Gvvcoupl[3][0]=0.0;  Gvvcoupl[3][1]=0.0;
      Gvvcoupl[4][0]=1.0;  Gvvcoupl[4][1]=0.0; // 2b+
      Gvvcoupl[5][0]=0.0;  Gvvcoupl[5][1]=0.0;
      Gvvcoupl[6][0]=0.0;  Gvvcoupl[6][1]=0.0;
      Gvvcoupl[7][0]=0.0;  Gvvcoupl[7][1]=0.0;
      Gvvcoupl[8][0]=0.0;  Gvvcoupl[8][1]=0.0;
      Gvvcoupl[9][0]=0.0;  Gvvcoupl[9][1]=0.0;

      isSpinTwo = true;
    }
    else if (process == TVar::H2_g2){
      // gg production coupling constants
      Gggcoupl[0][0]=0.0;  Gggcoupl[0][1]=0.0;
      Gggcoupl[1][0]=1.0;  Gggcoupl[1][1]=0.0;
      Gggcoupl[2][0]=0.0;  Gggcoupl[2][1]=0.0;
      Gggcoupl[3][0]=0.0;  Gggcoupl[3][1]=0.0;
      Gggcoupl[4][0]=0.0;  Gggcoupl[4][1]=0.0;

      // Graviton->ZZ coupling constants
      Gvvcoupl[0][0]=0.0;  Gvvcoupl[0][1]=0.0;
      Gvvcoupl[1][0]=1.0;  Gvvcoupl[1][1]=0.0;
      Gvvcoupl[2][0]=0.0;  Gvvcoupl[2][1]=0.0;
      Gvvcoupl[3][0]=0.0;  Gvvcoupl[3][1]=0.0;
      Gvvcoupl[4][0]=0.0;  Gvvcoupl[4][1]=0.0;
      Gvvcoupl[5][0]=0.0;  Gvvcoupl[5][1]=0.0;
      Gvvcoupl[6][0]=0.0;  Gvvcoupl[6][1]=0.0;
      Gvvcoupl[7][0]=0.0;  Gvvcoupl[7][1]=0.0;
      Gvvcoupl[8][0]=0.0;  Gvvcoupl[8][1]=0.0;
      Gvvcoupl[9][0]=0.0;  Gvvcoupl[9][1]=0.0;

      isSpinTwo = true;
    }
    // 2h3plus
    else if (process == TVar::H2_g3){
      // gg production coupling constants
      Gggcoupl[0][0]=0.0;  Gggcoupl[0][1]=0.0;
      Gggcoupl[1][0]=0.0;  Gggcoupl[1][1]=0.0;
      Gggcoupl[2][0]=1.0;  Gggcoupl[2][1]=0.0;
      Gggcoupl[3][0]=0.0;  Gggcoupl[3][1]=0.0;
      Gggcoupl[4][0]=0.0;  Gggcoupl[4][1]=0.0;

      // Graviton->ZZ coupling constants
      Gvvcoupl[0][0]=0.0;  Gvvcoupl[0][1]=0.0;
      Gvvcoupl[1][0]=0.0;  Gvvcoupl[1][1]=0.0;
      Gvvcoupl[2][0]=1.0;  Gvvcoupl[2][1]=0.0;
      Gvvcoupl[3][0]=0.0;  Gvvcoupl[3][1]=0.0;
      Gvvcoupl[4][0]=0.0;  Gvvcoupl[4][1]=0.0;
      Gvvcoupl[5][0]=0.0;  Gvvcoupl[5][1]=0.0;
      Gvvcoupl[6][0]=0.0;  Gvvcoupl[6][1]=0.0;
      Gvvcoupl[7][0]=0.0;  Gvvcoupl[7][1]=0.0;
      Gvvcoupl[8][0]=0.0;  Gvvcoupl[8][1]=0.0;
      Gvvcoupl[9][0]=0.0;  Gvvcoupl[9][1]=0.0;

      isSpinTwo = true;
    }
    // 2h6+
    else if (process == TVar::H2_g6){
      // gg production coupling constants
      Gggcoupl[0][0]=1.0;  Gggcoupl[0][1]=0.0;
      Gggcoupl[1][0]=0.0;  Gggcoupl[1][1]=0.0;
      Gggcoupl[2][0]=0.0;  Gggcoupl[2][1]=0.0;
      Gggcoupl[3][0]=0.0;  Gggcoupl[3][1]=0.0;
      Gggcoupl[4][0]=0.0;  Gggcoupl[4][1]=0.0;

      // Graviton->ZZ coupling constants
      Gvvcoupl[0][0]=0.0;  Gvvcoupl[0][1]=0.0;
      Gvvcoupl[1][0]=0.0;  Gvvcoupl[1][1]=0.0;
      Gvvcoupl[2][0]=0.0;  Gvvcoupl[2][1]=0.0;
      Gvvcoupl[3][0]=0.0;  Gvvcoupl[3][1]=0.0;
      Gvvcoupl[4][0]=0.0;  Gvvcoupl[4][1]=0.0;
      Gvvcoupl[5][0]=1.0;  Gvvcoupl[5][1]=0.0;
      Gvvcoupl[6][0]=0.0;  Gvvcoupl[6][1]=0.0;
      Gvvcoupl[7][0]=0.0;  Gvvcoupl[7][1]=0.0;
      Gvvcoupl[8][0]=0.0;  Gvvcoupl[8][1]=0.0;
      Gvvcoupl[9][0]=0.0;  Gvvcoupl[9][1]=0.0;

      isSpinTwo = true;
    }
    // 2h7plus
    else if (process == TVar::H2_g7){
      // gg production coupling constants
      Gggcoupl[0][0]=1.0;  Gggcoupl[0][1]=0.0;
      Gggcoupl[1][0]=0.0;  Gggcoupl[1][1]=0.0;
      Gggcoupl[2][0]=0.0;  Gggcoupl[2][1]=0.0;
      Gggcoupl[3][0]=0.0;  Gggcoupl[3][1]=0.0;
      Gggcoupl[4][0]=0.0;  Gggcoupl[4][1]=0.0;

      // Graviton->ZZ coupling constants
      Gvvcoupl[0][0]=0.0;  Gvvcoupl[0][1]=0.0;
      Gvvcoupl[1][0]=0.0;  Gvvcoupl[1][1]=0.0;
      Gvvcoupl[2][0]=0.0;  Gvvcoupl[2][1]=0.0;
      Gvvcoupl[3][0]=0.0;  Gvvcoupl[3][1]=0.0;
      Gvvcoupl[4][0]=0.0;  Gvvcoupl[4][1]=0.0;
      Gvvcoupl[5][0]=0.0;  Gvvcoupl[5][1]=0.0;
      Gvvcoupl[6][0]=1.0;  Gvvcoupl[6][1]=0.0;
      Gvvcoupl[7][0]=0.0;  Gvvcoupl[7][1]=0.0;
      Gvvcoupl[8][0]=0.0;  Gvvcoupl[8][1]=0.0;
      Gvvcoupl[9][0]=0.0;  Gvvcoupl[9][1]=0.0;

      isSpinTwo = true;
    }
    // 2h9minus
    else if (process == TVar::H2_g9){
      // gg production coupling constants
      Gggcoupl[0][0]=0.0;  Gggcoupl[0][1]=0.0;
      Gggcoupl[1][0]=0.0;  Gggcoupl[1][1]=0.0;
      Gggcoupl[2][0]=0.0;  Gggcoupl[2][1]=0.0;
      Gggcoupl[3][0]=0.0;  Gggcoupl[3][1]=0.0;
      Gggcoupl[4][0]=1.0;  Gggcoupl[4][1]=0.0;

      // Graviton->ZZ coupling constants
      Gvvcoupl[0][0]=0.0;  Gvvcoupl[0][1]=0.0;
      Gvvcoupl[1][0]=0.0;  Gvvcoupl[1][1]=0.0;
      Gvvcoupl[2][0]=0.0;  Gvvcoupl[2][1]=0.0;
      Gvvcoupl[3][0]=0.0;  Gvvcoupl[3][1]=0.0;
      Gvvcoupl[4][0]=0.0;  Gvvcoupl[4][1]=0.0;
      Gvvcoupl[5][0]=0.0;  Gvvcoupl[5][1]=0.0;
      Gvvcoupl[6][0]=0.0;  Gvvcoupl[6][1]=0.0;
      Gvvcoupl[7][0]=0.0;  Gvvcoupl[7][1]=0.0;
      Gvvcoupl[8][0]=1.0;  Gvvcoupl[8][1]=0.0;
      Gvvcoupl[9][0]=0.0;  Gvvcoupl[9][1]=0.0;

      isSpinTwo = true;
    }
    // 2h10minus
    else if (process == TVar::H2_g10){
      // gg production coupling constants
      Gggcoupl[0][0]=0.0;  Gggcoupl[0][1]=0.0;
      Gggcoupl[1][0]=0.0;  Gggcoupl[1][1]=0.0;
      Gggcoupl[2][0]=0.0;  Gggcoupl[2][1]=0.0;
      Gggcoupl[3][0]=0.0;  Gggcoupl[3][1]=0.0;
      Gggcoupl[4][0]=1.0;  Gggcoupl[4][1]=0.0;

      // Graviton->ZZ coupling constants
      Gvvcoupl[0][0]=0.0;  Gvvcoupl[0][1]=0.0;
      Gvvcoupl[1][0]=0.0;  Gvvcoupl[1][1]=0.0;
      Gvvcoupl[2][0]=0.0;  Gvvcoupl[2][1]=0.0;
      Gvvcoupl[3][0]=0.0;  Gvvcoupl[3][1]=0.0;
      Gvvcoupl[4][0]=0.0;  Gvvcoupl[4][1]=0.0;
      Gvvcoupl[5][0]=0.0;  Gvvcoupl[5][1]=0.0;
      Gvvcoupl[6][0]=0.0;  Gvvcoupl[6][1]=0.0;
      Gvvcoupl[7][0]=0.0;  Gvvcoupl[7][1]=0.0;
      Gvvcoupl[8][0]=0.0;  Gvvcoupl[8][1]=0.0;
      Gvvcoupl[9][0]=1.0;  Gvvcoupl[9][1]=0.0;

      isSpinTwo = true;
    }
    else if (process == TVar::SelfDefine_spin2){
      for (int j=0; j<2; j++){
        for (int i=0; i<SIZE_GGG; i++) Gggcoupl[i][j] = (selfDSpinTwoCoupl.Gggcoupl)[i][j];
        for (int i=0; i<SIZE_GQQ; i++) Gqqcoupl[i][j] = (selfDSpinTwoCoupl.Gqqcoupl)[i][j];
        for (int i=0; i<SIZE_GVV; i++) Gvvcoupl[i][j] = (selfDSpinTwoCoupl.Gvvcoupl)[i][j];
      }
      isSpinTwo = true;
    }

    // 1+
    else if (process == TVar::H1plus) {
      // z->vv coupling constants
      Zvvcoupl[0][0]=0.0;  Zvvcoupl[0][1]=0.0;
      Zvvcoupl[1][0]=1.0;  Zvvcoupl[1][1]=0.0; // 1+
      isSpinOne = true;
    }
    // 1-
    else if (process == TVar::H1minus) isSpinOne = true; // Already the default
    else if (process == TVar::SelfDefine_spin1){
      for (int j=0; j<2; j++){
        for (int i=0; i<SIZE_ZQQ; i++) Zqqcoupl[i][j] = (selfDSpinOneCoupl.Zqqcoupl)[i][j];
        for (int i=0; i<SIZE_ZVV; i++) Zvvcoupl[i][j] = (selfDSpinOneCoupl.Zvvcoupl)[i][j];
      }
      isSpinOne = true;
    }

    if (isSpinZero){
      SetJHUGenSpinZeroGGCouplings(Hggcoupl);
      SetJHUGenSpinZeroVVCouplings(Hvvcoupl, HvvCLambda_qsq, HvvLambda_qsq, false);
    }
    else if (isSpinOne) SetJHUGenSpinOneCouplings(Zqqcoupl, Zvvcoupl);
    else if (isSpinTwo) SetJHUGenSpinTwoCouplings(Gggcoupl, Gvvcoupl, Gqqcoupl);

    if (isSpinZero || isSpinOne || isSpinTwo) dXsec = JHUGenMatEl(process, production, matrixElement, &event_scales, &RcdME, EBEAM, verbosity);
    else cerr
      << "TEvtProb::XsecCalc_XVV: JHUGen ME is not spin zero, one or two! The process is described by "
      << "Process: " << process << ", Production: " << production << ", and ME: " << matrixElement
      << endl;
  } // end of JHUGen matrix element calculations

  if (verbosity >= TVar::DEBUG) cout
    << "Process " << TVar::ProcessName(process)
    << " TEvtProb::XsecCalc(): dXsec=" << dXsec
    << endl;

  ResetCouplings(); // Should come first
  if (forceUseMCFM){ // Set defaults. Should come next...
    if (needBSMHiggs) SetLeptonInterf(TVar::DefaultLeptonInterf);
    SetMCFMSpinZeroVVCouplings(false, &selfDSpinZeroCoupl, true); // ... because of this!
  }
  ResetRenFacScaleMode();
  return dXsec;
}

double TEvtProb::XsecCalc_VVXVV(
  TVar::Process process_, TVar::Production production_,
  TVar::VerbosityLevel verbosity_
  ){
  double dXsec=0;
  ResetIORecord();
  if (!CheckInputPresent()) return dXsec;

  //Initialize Process
  SetProcess(process_);
  SetProduction(production_);
  SetVerbosity(verbosity_);

  bool forceUseMCFM = (matrixElement == TVar::MCFM || process == TVar::bkgZZ_SMHiggs);
  bool needBSMHiggs=false;
  bool calculateME=true;
  // process == TVar::bkgZZ_SMHiggs && matrixElement == TVar::JHUGen is still MCFM
  if (forceUseMCFM){ // Always uses MCFM
    // Check self-defined couplings are specified.
    for (int vv = 0; vv < SIZE_HVV; vv++){
      if (
        (selfDSpinZeroCoupl.Hzzcoupl)[vv][1] != 0 || (selfDSpinZeroCoupl.Hzzcoupl)[vv][0] != 0
        ||
        (selfDSpinZeroCoupl.Hwwcoupl)[vv][1] != 0 || (selfDSpinZeroCoupl.Hwwcoupl)[vv][0] != 0
        ){
        needBSMHiggs = true; break;
      } // Only possible if selfDefine is called.
    }
    if (_h2mass>=0. && _h2width>0. && !needBSMHiggs){
      for (int vv = 0; vv < SIZE_HVV; vv++){
        if (
          (selfDSpinZeroCoupl.H2zzcoupl)[vv][1] != 0 || (selfDSpinZeroCoupl.H2zzcoupl)[vv][0] != 0
          ||
          (selfDSpinZeroCoupl.H2wwcoupl)[vv][1] != 0 || (selfDSpinZeroCoupl.H2wwcoupl)[vv][0] != 0
          ){
          needBSMHiggs = true; break;
        } // Only possible if selfDefine is called.
      }
    }
    if (needBSMHiggs) SetLeptonInterf(TVar::InterfOn); // All anomalous coupling computations have to use lepton interference
    SetMCFMSpinZeroVVCouplings(needBSMHiggs, &selfDSpinZeroCoupl, false);
    calculateME = MCFM_chooser(process, production, leptonInterf, melaCand);
  }

  // Last check before ME calculations
  if (!calculateME) return dXsec;

  // ME calculation
  if (forceUseMCFM) dXsec = SumMatrixElementPDF(process, production, matrixElement, &event_scales, &RcdME, EBEAM, (selfDSpinZeroCoupl.Hvvcoupl_freenorm), verbosity);
  else cerr << "Non-MCFM Mes are not supported." << endl;

  if (verbosity >= TVar::DEBUG) cout
    << "Process " << TVar::ProcessName(process)
    << " TEvtProb::XsecCalc(): dXsec=" << dXsec
    << endl;

  ResetCouplings(); // Should come first
  if (forceUseMCFM){ // Set defaults. Should come next...
    if (needBSMHiggs) SetLeptonInterf(TVar::DefaultLeptonInterf);
    SetMCFMSpinZeroVVCouplings(false, &selfDSpinZeroCoupl, true); // ... because of this!
  }
  ResetRenFacScaleMode();
  return dXsec;
}

// Cross-section calculations for H + 2 jets
double TEvtProb::XsecCalcXJJ(TVar::Process process_, TVar::Production production_, TVar::VerbosityLevel verbosity_){
  double dXsec = 0;
  ResetIORecord();
  if (!CheckInputPresent()) return dXsec;

  // Initialize Process
  SetProcess(process_);
  SetProduction(production_);
  SetVerbosity(verbosity_);

  // first/second number is the real/imaginary part
  double Hggcoupl[SIZE_HGG][2] ={ { 0 } };
  double Hzzcoupl[SIZE_HVV][2] ={ { 0 } };
  double Hwwcoupl[SIZE_HVV][2] ={ { 0 } };
  double HzzLambda_qsq[4][3] ={ { 0 } };
  double HwwLambda_qsq[4][3] ={ { 0 } };
  int HzzCLambda_qsq[3] ={ 0 };
  int HwwCLambda_qsq[3] ={ 0 };

  Hggcoupl[0][0]=1.0;  Hggcoupl[0][1]=0.0; // g2
  Hzzcoupl[0][0]=1.0;  Hzzcoupl[0][1]=0.0; // g1
  Hwwcoupl[0][0]=1.0;  Hwwcoupl[0][1]=0.0; // g1
  for (int ic=0; ic<4; ic++){
    for (int ik=0; ik<3; ik++){
      HzzLambda_qsq[ic][ik]=100.;
      HwwLambda_qsq[ic][ik]=100.;
    }
  }

  // 0-
  if (process == TVar::H0minus){
    Hggcoupl[0][0] = 0.0;
    Hggcoupl[2][0] = 1.0;

    Hzzcoupl[0][0] = 0.0;
    Hzzcoupl[3][0] = 1.0;
    Hwwcoupl[0][0] = 0.0;
    Hwwcoupl[3][0] = 1.0;
  }
  // 0+h
  else if (process == TVar::H0hplus) { // No need to re-set ggcoupl
    Hzzcoupl[0][0] = 0.0;
    Hzzcoupl[1][0] = 1.0;
    Hwwcoupl[0][0] = 0.0;
    Hwwcoupl[1][0] = 1.0;
  }
  // 0+L1
  else if (process == TVar::H0_g1prime2){ // No need to re-set ggcoupl
    Hzzcoupl[0][0] = 0.;
    Hzzcoupl[11][0] = -12046.01;
    Hwwcoupl[0][0] = 0.;
    Hwwcoupl[11][0] = -12046.01;
  }
  else if (process == TVar::SelfDefine_spin0){
    for (int j=0; j<2; j++){
      for (int i=0; i<SIZE_HGG; i++) Hggcoupl[i][j] = (selfDSpinZeroCoupl.Hggcoupl)[i][j];
      for (int i=0; i<SIZE_HVV; i++){
        Hzzcoupl[i][j] = (selfDSpinZeroCoupl.Hzzcoupl)[i][j];
        Hwwcoupl[i][j] = (selfDSpinZeroCoupl.Hwwcoupl)[i][j];
      }
    }
    for (int j=0; j<3; j++){
      for (int i=0; i<4; i++){
        HzzLambda_qsq[i][j] = (selfDSpinZeroCoupl.HzzLambda_qsq)[i][j];
        HwwLambda_qsq[i][j] = (selfDSpinZeroCoupl.HwwLambda_qsq)[i][j];
      }
      HzzCLambda_qsq[j] = (selfDSpinZeroCoupl.HzzCLambda_qsq)[j];
      HwwCLambda_qsq[j] = (selfDSpinZeroCoupl.HwwCLambda_qsq)[j];
    }
  }
  SetJHUGenSpinZeroGGCouplings(Hggcoupl);
  SetJHUGenSpinZeroVVCouplings(Hzzcoupl, HzzCLambda_qsq, HzzLambda_qsq, false);
  SetJHUGenSpinZeroVVCouplings(Hwwcoupl, HwwCLambda_qsq, HwwLambda_qsq, true);

  // ME calculations
  if (matrixElement == TVar::JHUGen){
    dXsec = HJJMatEl(process, production, matrixElement, &event_scales, &RcdME, EBEAM, verbosity);
    if (verbosity >= TVar::DEBUG) cout <<"Process " << TVar::ProcessName(process) << " TEvtProb::XsecCalcXJJ(): dXsec=" << dXsec << endl;
  }
  else cerr << "Non-JHUGen vbfMELA MEs are not supported!" << endl;

  ResetCouplings();
  ResetRenFacScaleMode();
  return dXsec;
}

// Cross-section calculations for H (SM) + 1 jet
double TEvtProb::XsecCalcXJ(TVar::Process process_, TVar::Production production_, TVar::VerbosityLevel verbosity_){
  double dXsec = 0;
  ResetIORecord();
  if (!CheckInputPresent()) return dXsec;

  // Initialize Process
  SetProcess(process_);
  SetProduction(production_);
  SetVerbosity(verbosity_);

  // Calculate the ME
  if (matrixElement == TVar::JHUGen){
    dXsec = HJJMatEl(process, production, matrixElement, &event_scales, &RcdME, EBEAM, verbosity);
    if (verbosity >= TVar::DEBUG) std::cout << "Process " << TVar::ProcessName(process) << " TEvtProb::XsecCalcXJ(): dXsec=" << dXsec << endl;
  }
  else cerr << "Non-JHUGen HJ MEs are not supported!" << endl;

  ResetCouplings();
  ResetRenFacScaleMode();
  return dXsec;
}


double TEvtProb::XsecCalc_VX(
  TVar::Process process_, TVar::Production production_,
  TVar::VerbosityLevel verbosity_,
  bool includeHiggsDecay
  ){
  double dXsec = 0;
  ResetIORecord();
  if (!CheckInputPresent()) return dXsec;

  //Initialize Process
  SetProcess(process_);
  SetProduction(production_);
  SetVerbosity(verbosity_);
  AllowSeparateWWCouplings(false); // HZZ couplings are used for both in spin-0

  if (matrixElement == TVar::JHUGen){
    // Set Couplings at the HVV* vertex
    double Hvvcoupl[SIZE_HVV][2] ={ { 0 } };
    double HvvLambda_qsq[4][3] ={ { 0 } };
    int HvvCLambda_qsq[3] ={ 0 };

    // By default set the Spin 0 couplings for SM case
    Hvvcoupl[0][0]=1.0;  Hvvcoupl[0][1]=0.0;   // first/second number is the real/imaginary part
    for (int ic=0; ic<4; ic++){ for (int ik=0; ik<3; ik++) HvvLambda_qsq[ic][ik]=100.; }

    // 0-
    if (process == TVar::H0minus) {
      Hvvcoupl[0][0] = 0.0;
      Hvvcoupl[1][0] = 0.0;
      Hvvcoupl[2][0] = 0.0;
      Hvvcoupl[3][0] = 1.0;
    }
    // 0h+
    else if (process == TVar::H0hplus) {
      Hvvcoupl[0][0] = 0.0;
      Hvvcoupl[1][0] = 1.0;
      Hvvcoupl[2][0] = 0.0;
      Hvvcoupl[3][0] = 0.0;
    }
    // 0+L1
    else if (process == TVar::H0_g1prime2){
      Hvvcoupl[0][0] = 0.;
      Hvvcoupl[11][0] = -12046.01;
    }
    else if (process == TVar::SelfDefine_spin0){
      for (int i=0; i<SIZE_HVV; i++){ for (int j=0; j<2; j++) Hvvcoupl[i][j] = (selfDSpinZeroCoupl.Hzzcoupl)[i][j]; }
      for (int j=0; j<3; j++){
        for (int i=0; i<4; i++) HvvLambda_qsq[i][j] = (selfDSpinZeroCoupl.HzzLambda_qsq)[i][j];
        HvvCLambda_qsq[j] = (selfDSpinZeroCoupl.HzzCLambda_qsq)[j];
      }
    }
    SetJHUGenSpinZeroVVCouplings(Hvvcoupl, HvvCLambda_qsq, HvvLambda_qsq, false);

    dXsec = VHiggsMatEl(process, production, matrixElement, &event_scales, &RcdME, EBEAM, includeHiggsDecay, verbosity);
    if (verbosity >= TVar::DEBUG) std::cout << "Process " << TVar::ProcessName(process) << " TEvtProb::XsecCalc_VX(): dXsec=" << dXsec << endl;
  } // end of JHUGen matrix element calculations
  else cerr << "Non-JHUGen VH MEs are not supported!" << endl;

  ResetCouplings();
  ResetRenFacScaleMode();
  return dXsec;
}

// Cross-section calculations for ttbar -> H
double TEvtProb::XsecCalc_TTX(
  TVar::Process process_, TVar::Production production_,
  TVar::VerbosityLevel verbosity_,
  int topProcess, int topDecay
  ){
  double dXsec = 0;
  ResetIORecord();
  if (!CheckInputPresent()) return dXsec;

  //Initialize Process
  SetProcess(process_);
  SetProduction(production_);
  SetVerbosity(verbosity_);

  // Set couplings at the qqH vertex
  double Hqqcoupl[SIZE_HQQ][2]={ { 0 } };

  if (matrixElement == TVar::JHUGen){
    // By default set the spin-0 couplings for SM case
    Hqqcoupl[0][0]=1;  Hqqcoupl[0][1]=0;   // first/second number is the real/imaginary part
    for (int i = 1; i<SIZE_HQQ; i++){ for (int com=0; com<2; com++) Hqqcoupl[i][com] = 0; }

    // 0-
    if (process == TVar::H0minus) {
      Hqqcoupl[0][0] = 0;
      Hqqcoupl[1][0] = 1;
    }
    else if (process == TVar::SelfDefine_spin0){
      for (int i=0; i<SIZE_HQQ; i++){ for (int j=0; j<2; j++) Hqqcoupl[i][j] = (selfDSpinZeroCoupl.Hqqcoupl)[i][j]; }
    }
    SetJHUGenSpinZeroQQCouplings(Hqqcoupl);

    if (production==TVar::ttH) dXsec = TTHiggsMatEl(process, production, matrixElement, &event_scales, &RcdME, EBEAM, topDecay, topProcess, verbosity);
    else if (production==TVar::bbH) dXsec = BBHiggsMatEl(process, production, matrixElement, &event_scales, &RcdME, EBEAM, topProcess, verbosity);
    if (verbosity >= TVar::DEBUG) std::cout << "Process " << TVar::ProcessName(process) << " TEvtProb::XsecCalc_TTX(): dXsec=" << dXsec << endl;
  }
  else cerr << "Non-JHUGen ttH MEs are not supported!" << endl;

  ResetCouplings();
  ResetRenFacScaleMode();
  return dXsec;
}





