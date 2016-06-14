{
  TFile *f = new TFile("compare.root", "RECREATE");
  Int_t N = 7;
  std::string *fileNames = new std::string[N];
  std::string *date = new std::string[N];
//   for(int n = 0 ; n < N ; n++)
//   {
//      fileNames[0] = "/data/acqmanager/calibration/resumes/2015-02-20_04:24:50.root";
//      fileNames[0] = "/data/acqmanager/calibration/resumes/2015-02-20_04:24:50.root";
//      fileNames[1] = "/data/acqmanager/calibration/resumes/2015-07-17_02:04:06.root";
//      fileNames[2] = "/data/acqmanager/calibration/resumes/2015-09-15_05:19:05.root";
    fileNames[0] = "/home/marco/Universita/ClearPEM/Monza/CalibrationReport/2014-05-19_13:18:24.root";
    fileNames[1] = "/home/marco/Universita/ClearPEM/Monza/CalibrationReport/2015-02-20_04_24_50.root";
    fileNames[2] = "/home/marco/Universita/ClearPEM/Monza/CalibrationReport/2015-07-17_02_04_06.root";
    fileNames[3] = "/home/marco/Universita/ClearPEM/Monza/CalibrationReport/2016-03-21_16:02:36.root";
    fileNames[4] = "/home/marco/Universita/ClearPEM/Monza/CalibrationReport/2016-05-18_16:33:10.root";
    fileNames[5] = "/home/marco/Universita/ClearPEM/Monza/CalibrationReport/2016-06-01_10:49:35.root";
    fileNames[6] = "/home/marco/Universita/ClearPEM/Monza/CalibrationReport/2016-06-06_18:07:19.root";
//   }
  for (int n = 0 ; n < N ; n++)
  {
    std::size_t found = fileNames[n].find_last_of("/\\");
    date[n] = fileNames[n].substr(found+1,fileNames[n].substr(found+1).size()-5);
    std::cout << " date: " << fileNames[n].substr(found+1,fileNames[n].substr(found+1).size()-5) << std::endl;
  }
  
  
  gSystem->Load("libCalibration.so");
  gStyle->SetPalette(1);
  gStyle->SetOptFit(1);
  gStyle->SetOptStat(0);
  
  
  
  Int_t crystalX [6144];
  Int_t crystalY [6144];	
  ifstream cmap ("crystal_map.txt");
  while(!cmap.eof()) {
    int cid, x, y;
    cmap >> cid >> x >> y;
    
    if(cid >= 6144/2)
      y += 48;
    
    crystalX[cid] = x;
    crystalY[cid] = y;
  }
  
  
  char *smPrefix = "K";
  TH2F **hTop = new TH2F*[N];
  TH2F **hBottom = new TH2F*[N];;
  
  TH2F **hDifference = new TH2F*[N-1];
  TH2F **hRatioTop = new TH2F*[N-1];
  TH2F **hRatioBottom = new TH2F*[N-1];
  
  
  TCanvas **cComparisons = new TCanvas*[N-1];
  TCanvas **cCoefficients = new TCanvas*[N];	
  
  for(Int_t n = 0; n < N; n++) {
    char hName[128];
    char hTitle[128];
    //kTop
    sprintf(hName, "hTop _ %s", date[n].c_str());
    sprintf(hTitle, "%s, Top APD  %s", smPrefix, date[n].c_str());
    hTop[n] = new TH2F(hName, hTitle, 64, 0, 64, 48+48, 0, 48+48);
    //kBottom
    sprintf(hName, "hBottom_%s", date[n].c_str());
    sprintf(hTitle, "%s, Bottom APD %s", smPrefix, date[n].c_str());
    hBottom[n] = new TH2F(hName, hTitle, 64, 0, 64, 48+48, 0, 48+48);
    
    
    
    
//     sprintf(hName, "hDifference_%s", date[n]);
//     sprintf(hTitle, "%s Difference Top - Bottom (comparison %s)", smPrefix, date[n]);
//     hDifference[n] = new TH2F(hName, hTitle, 64, 0, 64, 48+48, 0, 48+48);
    
    TFile * calibrationFile = new TFile(fileNames[n].c_str(), "READ");
    TTree * calibrationTree = (TTree *)calibrationFile->Get("T2");
    
    CalibrationSummaryData *csd;
    calibrationTree->SetBranchAddress("event", &csd);
    
    for(int i = 0; i < calibrationTree->GetEntries(); i++) {
      calibrationTree->GetEntry(i);
      if(csd == NULL) 
	continue;
      
      Int_t crystalID = csd->GetCrystalID();
      Int_t cx = crystalX[crystalID]+1;
      Int_t cy = crystalY[crystalID]+1;		
      Int_t asic = (crystalID / 192) % 2;
      
      hTop[n]->SetBinContent(cx, cy, csd->Get22NaKAbsolute());
      hBottom[n]->SetBinContent(cx, cy, csd->Get22NaKAbsolute() * csd->Get176LuKrel());
      hTop[n]->GetZaxis()->SetRangeUser(0, 10);
      hBottom[n]->GetZaxis()->SetRangeUser(0, 10);
    }
  }
  
  
  //
  for(Int_t n = N-1; n > 0; n--) {
    
    //comparisons
    //ktop
    char hName[128];
    char hTitle[128];
    sprintf(hName, "ratio hTop_ %s - %s", date[n].c_str(),date[n-1].c_str());
    sprintf(hTitle, "(%s2 - %s1)/%s1, Top APD. %s1 = %s , %s2 %s",smPrefix,smPrefix,smPrefix,smPrefix,date[n-1].c_str(),smPrefix,date[n].c_str());
    hRatioTop[n-1] = new TH2F(hName, hTitle, 64, 0, 64, 48+48, 0, 48+48);
    //kbottom
    sprintf(hName, "ratio hBottom_ %s - %s", date[n].c_str(),date[n-1].c_str());
    sprintf(hTitle, "(%s2 - %s1)/%s1, Bottom APD. %s1 = %s , %s2 %s",smPrefix,smPrefix,smPrefix,smPrefix,date[n-1].c_str(),smPrefix,date[n].c_str());
    hRatioBottom[n-1] = new TH2F(hName, hTitle, 64, 0, 64, 48+48, 0, 48+48);
    
    //difference
    sprintf(hName, "difference hdifferece_%s %s", date[n].c_str(),date[n-1].c_str());
    sprintf(hTitle, "Top - Bottom");
    hDifference[n-1] = new TH2F(hName, hTitle, 64, 0, 64, 48+48, 0, 48+48);
    
    for(Int_t cx = 0; cx < 64; cx++) {
      for(Int_t cy = 0; cy < 2*48; cy++) {
	Float_t v1 = hTop[n-1]->GetBinContent(cx+1, cy+1);
	Float_t v2 = hTop[n]->GetBinContent(cx+1, cy+1);
	Float_t d = 0;
	if(v1 != 0 && v2 != 0)
	  d = (v2 - v1)/v1;
	hRatioTop[n-1]->SetBinContent(cx+1, cy+1, d );
      }
    }
    hRatioTop[n-1]->GetZaxis()->SetRangeUser(-0.2, 0.2);
    
    for(Int_t cx = 0; cx < 64; cx++) {
      for(Int_t cy = 0; cy < 2*48; cy++) {
	Float_t v1 = hBottom[n-1]->GetBinContent(cx+1, cy+1);
	Float_t v2 = hBottom[n]->GetBinContent(cx+1, cy+1);
	Float_t d = 0;
	if(v1 != 0 && v2 != 0)
	  d = (v2 - v1)/v1;
	hRatioBottom[n-1]->SetBinContent(cx+1, cy+1, d );
      }
    }
    hRatioBottom[n-1]->GetZaxis()->SetRangeUser(-0.2, 0.2);
    
    
    for(Int_t cx = 0; cx < 64; cx++) {
      for(Int_t cy = 0; cy < 2*48; cy++) {
	Float_t v1 = hRatioBottom[n-1]->GetBinContent(cx+1, cy+1);
	Float_t v2 = hRatioTop[n-1]->GetBinContent(cx+1, cy+1);
	Float_t d = 0;
	d = (v2 - v1);
	hDifference[n-1]->SetBinContent(cx+1, cy+1, d );
      }
    }
    hDifference[n-1]->GetZaxis()->SetRangeUser(-0.4, 0.4);
//     h1 = hBottom[n-1];
//     h2 = hBottom[n];
//     
//     
//     for(Int_t cx = 0; cx < 64; cx++) {
//       for(Int_t cy = 0; cy < 2*48; cy++) {
// 	Float_t v1 = h1->GetBinContent(cx+1, cy+1);
// 	Float_t v2 = h2->GetBinContent(cx+1, cy+1);
// 	Float_t d = 0;
// 	if(v1 != 0 && v2 != 0)
// 	  d = (v2 - v1)/v1;
// 	h2->SetBinContent(cx+1, cy+1, d );
//       }
//     }
//     h2->GetZaxis()->SetRangeUser(-0.2, 0.2);
    
  }
  
  for(Int_t n = 0; n < N; n++) 
  {
    char cName[128];
    char cTitle[128];
    sprintf(cName, "canvas_%s", date[n].c_str());
    sprintf(cTitle, "%s Coefficients - Calibration %s", smPrefix, date[n].c_str());
    cCoefficients[n] = new TCanvas(cName,cTitle,1200,800);
    cCoefficients[n]->Divide(2,1);
    cCoefficients[n]->cd(1);
    hTop[n]->Draw("COLZ");
    cCoefficients[n]->cd(2);
    hBottom[n]->Draw("COLZ");
    f->cd();
    cCoefficients[n]->Write();
  }
  
  for(Int_t n = 0; n < N-1; n++) 
  {
    char cName[128];
    char cTitle[128];
    sprintf(cName, "canvas %s %s", date[n].c_str(),date[n+1].c_str());
    sprintf(cTitle, "%s canvas comparison %s %s", smPrefix, date[n].c_str(),date[n+1].c_str());
    cComparisons[n] = new TCanvas(cName,cTitle,1200,800);
    cComparisons[n]->Divide(3,1);
    
    cComparisons[n]->cd(1);
    hRatioTop[n]->Draw("COLZ");
    cComparisons[n]->cd(2);
    hRatioBottom[n]->Draw("COLZ");
    cComparisons[n]->cd(3);
    hDifference[n]->Draw("COLZ");
    f->cd();
    cComparisons[n]->Write();
    
  }
  
  f->Close();
  
}
