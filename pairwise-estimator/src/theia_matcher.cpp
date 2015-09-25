#include <vector>
#include <iostream>
#include <fstream>

#include <glog/logging.h>
//#include <theia/theia.h>
#include <theia/sfm/verify_two_view_matches.h>
#include <theia/sfm/exif_reader.h>
#include <theia/matching/feature_correspondence.h>
#include <theia/matching/image_pair_match.h>
#include <theia/io/write_matches.h>
#include <theia/io/read_matches.h>
#include "Reader.h"
#include "keys2a.h"

#include <stdio.h>

using namespace std;
using namespace theia;

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);


  /*
  vector<string> imnames;
  vector<CameraIntrinsicsPrior> cps;
  vector<ImagePairMatch> imts;
  bool s0 = ReadMatchesAndGeometry("../data/view_graph.bin",&imnames,&cps,&imts);
  if(s0) {
    printf("\nSuccessfully Read Matches And Geo");
    printf("\n%d image names", imnames.size());
    printf("\n%d cam intrinsics", cps.size());
    printf("\n%d image pair matches", imts.size());
    for(int i=0; i < imnames.size(); i++) {
      printf("\nImage name %s", imnames[i].c_str());
    }

    for(int i=0; i < imts.size(); i++) { 
      ImagePairMatch imPair = imts[i];
      printf("\nImage Id 1 %d , Image Id 2 %d", imPair.image1_index,imPair.image2_index);
    }
  } else {
    printf("\nFAILED");
  }
  fflush(stdout);
  exit(-1);
*/

  string imgListFile = argv[1];
  string keyListFile = argv[1];

  reader::ImageListReader imgList(imgListFile);
  reader::KeyListReader keyList(keyListFile);
  

  bool s1 = imgList.read();
  bool s2 = keyList.read();

  if(!s1 || !s2) {
    printf("\nError reading list file or key file");
    return 0;
  }

  int numKeys = keyList.getNumKeys();
  vector< unsigned char* > keys(numKeys);
  vector< keypt_t* > keysInfo(numKeys);
  vector< int > numFeatures(numKeys);

  vector<string> view_names(numKeys);

  ExifReader exReader;
  vector<CameraIntrinsicsPrior> camPriors;
  for(int i=0; i < keyList.getNumKeys(); i++) {
    printf("\nReading keyfile %d/%d", i, numKeys);
    string keyPath = keyList.getKeyName(i);
    numFeatures[i] = ReadKeyFile(keyPath.c_str(),
        &keys[i], &keysInfo[i]);
    int W = imgList.getImageWidth(i);
    int H = imgList.getImageHeight(i);

    string path = imgList.getImageName(i);
    size_t sep = path.find_last_of("\\/");
    if (sep != std::string::npos)
      path = path.substr(sep + 1, path.size() - sep - 1);

    view_names[i] = path; 
    printf("\nImage name is %s", path.c_str());

    NormalizeKeyPoints(numFeatures[i], keysInfo[i], W, H);
    printf("\nReading exif data %d/%d", i, numKeys);
    CameraIntrinsicsPrior cPrior, dummyPrior;
    bool status = exReader.ExtractEXIFMetadata(imgList.getImageName(i), &cPrior);
    if(status) {
      printf("\nRead EXIF data:");
      if(cPrior.focal_length.is_set) {
        camPriors.push_back(cPrior);
      } else {
        camPriors.push_back(dummyPrior);
      }
    } else {
      camPriors.push_back(dummyPrior);
    }
  }


  FILE* fp = fopen( argv[2] , "r");
  if( fp == NULL ) {
    printf("\nCould not read match file");
    return 0;
  }

  int img1, img2;
  vector<ImagePairMatch> matches;
  
  while(fscanf(fp,"%d %d",&img1,&img2) != EOF){

    vector<FeatureCorrespondence> featCorrs;
    vector<FeatureCorrespondence> inlFeatCorrs;
    printf("\n%d %d", img1, img2);  
    
    int numMatches;
    fscanf(fp,"%d",&numMatches);

    printf(": num matches %d\n", numMatches);
    fflush(stdout);
    
    for(int i=0; i < numMatches; i++) {
      int matchIdx1, matchIdx2;
      fscanf(fp,"%d %d",&matchIdx1, &matchIdx2);
      float x1 = keysInfo[img1][matchIdx1].x;
      float y1 = keysInfo[img1][matchIdx1].y; 

      float x2 = keysInfo[img2][matchIdx2].x; 
      float y2 = keysInfo[img2][matchIdx2].y; 

      Feature f1(x1, y1);
      Feature f2(x2, y2);
      FeatureCorrespondence corres;
      corres.feature1 = f1;
      corres.feature2 = f2;
      featCorrs.push_back(corres);      

    }
  
    vector<int> inliers;
    TwoViewInfo currPairInfo;
    VerifyTwoViewMatchesOptions options;
    bool status = VerifyTwoViewMatches(options,
        camPriors[img1], camPriors[img2],
        featCorrs,&currPairInfo,&inliers);

    for(int in=0; in < inliers.size(); in++) {
      inlFeatCorrs.push_back(featCorrs[in]); 
    }

    if(status) {
      printf("\nSuccessfully Verified Matches");
      ImagePairMatch imPair;
      imPair.image1_index = img1;
      imPair.image2_index = img2; 
      imPair.twoview_info = currPairInfo;
      imPair.correspondences = inlFeatCorrs;
      matches.push_back(imPair);
    }
  }
  
  bool status = WriteMatchesAndGeometry("view_graph.bin", view_names, camPriors, matches);
  if(!status) {
    printf("\nSuccessfully written view-graph file");
  }
  return 0;
}
