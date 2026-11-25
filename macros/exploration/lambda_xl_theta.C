// Generated Lambdas (status 1 or 2): x_L vs theta (wrt proton beam), |t|, pT. Beam frame: rotate by +25 mrad about y.
void lambda_xl_theta(const char* label, const char* files, double Ep){
  std::vector<std::string> fl; TString s(files); TObjArray* a=s.Tokenize(" "); for(auto o:*a) fl.push_back(((TObjString*)o)->GetString().Data());
  ROOT::RDataFrame df("events", fl);
  auto d=df.Define("sel","MCParticles.PDG==3122&&(MCParticles.generatorStatus==1||MCParticles.generatorStatus==2)")
           .Define("px","ROOT::VecOps::RVec<double>(MCParticles.momentum.x[sel])")
           .Define("py","ROOT::VecOps::RVec<double>(MCParticles.momentum.y[sel])")
           .Define("pz","ROOT::VecOps::RVec<double>(MCParticles.momentum.z[sel])");
  auto nev=d.Count(); auto PX=d.Take<ROOT::VecOps::RVec<double>>("px"), PY=d.Take<ROOT::VecOps::RVec<double>>("py"), PZ=d.Take<ROOT::VecOps::RVec<double>>("pz");
  const double m=1.115683, mp=0.938272, c=cos(0.025), sn=sin(0.025); double Eb=sqrt(Ep*Ep+mp*mp);
  double xe[6]={0,0.2,0.4,0.6,0.8,1.01}; double te[6]={0,4.5,5.5,20,25,1e9};
  int H[5][5]={{0}}; int n=0; std::vector<double> tH, ptH, tB, ptB, thH;
  for(size_t e=0;e<PX->size();e++) for(size_t i=0;i<(*PX)[e].size();i++){
    double px=(*PX)[e][i], py=(*PY)[e][i], pz=(*PZ)[e][i]; n++;
    double x=px*c+pz*sn, z=-px*sn+pz*c;
    double p=sqrt(x*x+py*py+z*z), pt=sqrt(x*x+py*py), th=1000*pt/p, xl=z/Ep, E=sqrt(p*p+m*m);
    double t=(Eb-E)*(Eb-E)-(x*x+py*py+(Ep-z)*(Ep-z));
    int ix=-1,it=-1; for(int k=0;k<5;k++){ if(xl>=xe[k]&&xl<xe[k+1]) ix=k; if(th>=te[k]&&th<te[k+1]) it=k; }
    if(ix>=0&&it>=0) H[ix][it]++;
    if(xl>0.6){ tH.push_back(-t); ptH.push_back(pt); thH.push_back(th); if(th>=5.5&&th<20){tB.push_back(-t); ptB.push_back(pt);} }
  }
  printf("\n=== %s: %llu events, %d generated Lambdas (Ep=%g)\n",label,*nev,n,Ep);
  printf("  x_L \\ theta[mrad] |  <4.5 | 4.5-5.5 | 5.5-20 (B0) | 20-25 | >25 | all\n");
  for(int ix=0;ix<5;ix++){int r=0; for(int it=0;it<5;it++) r+=H[ix][it]; printf("  %.1f-%.1f          | %5d | %7d | %11d | %5d | %4d | %d\n",xe[ix],std::min(xe[ix+1],1.0),H[ix][0],H[ix][1],H[ix][2],H[ix][3],H[ix][4],r);}
  auto q=[](std::vector<double> v,const char* nm){ if(v.empty()){printf("  %s: none\n",nm);return;} std::sort(v.begin(),v.end()); auto Q=[&](double f){return v[(size_t)(f*(v.size()-1))];}; printf("  %-28s n=%4zu: 10%% %.3f  50%% %.3f  90%% %.3f  max %.3f\n",nm,v.size(),Q(.1),Q(.5),Q(.9),v.back()); };
  q(tH,"|t| [GeV2], x_L>0.6"); q(ptH,"pT [GeV], x_L>0.6"); q(thH,"theta [mrad], x_L>0.6"); q(tB,"|t| [GeV2], x_L>0.6 & B0"); q(ptB,"pT [GeV], x_L>0.6 & B0");
}
