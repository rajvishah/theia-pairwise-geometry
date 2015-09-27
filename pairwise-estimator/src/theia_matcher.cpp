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
  // Initialize logging to --log_dir 
  google::InitGoogleLogging(argv[0]);
 
  // Read list files with image paths, image dims and key paths
  // argv[1] stores image_dims.txt, list_keys.txt, list_images.txt
  string imgListFile = argv[1];
  string keyListFile = argv[1];

  // Read keyfile names
  // Read image file names and dimensions
  reader::ImageListReader imgList(imgListFile);
  reader::KeyListReader keyList(keyListFile);
  
  bool s1 = imgList.read();
  bool s2 = keyList.read();

  if(!s1 || !s2) {
    printf("\nError reading list file or key file");
    return 0;
  }

  // Open files to write pairwise matching output 
  // As per Kyle Wilson's 1D SFM package's input requirements
  FILE* file1 = fopen("../output/cc.txt", "w"); 
  if(file1 == NULL) {
    printf("\nCould not open CC file to write");
    return -1;
  }

  FILE* file2 = fopen("../output/EGs.txt", "w");
  if(file2 == NULL) {
    printf("\nCould not open EG file to write");
    return -1;
  }

  FILE* file3 = fopen("../output/coords.txt", "w");
  if(file2 == NULL) {
    printf("\nCould not open coords file to write");
    return -1;
  }

  FILE* file4 = fopen("../output/input_tracks.txt", "w");
  if(file4 == NULL) {
    printf("\nCould not open tracks file to write");
    return -1;
  }

  // Initialize containers for various data
  int numKeys = keyList.getNumKeys();
  vector< unsigned char* > keys(numKeys);
  vector< keypt_t* > keysInfo(numKeys);
  vector< int > numFeatures(numKeys);
  vector<string> view_names(numKeys);

  vector<double> halfWidth(numKeys);
  vector<double> halfHeight(numKeys);
  vector<double> focalLengths(numKeys, 1.0f);
  vector<bool> calibrationFlag(numKeys, false);
  
  vector<CameraIntrinsicsPrior> camPriors;

  //Create a single ExifReader object to extract focal lengths
  ExifReader exReader;
  for(int i=0; i < keyList.getNumKeys(); i++) {
    printf("\nReading keyfile %d/%d", i, numKeys);
    string keyPath = keyList.getKeyName(i);
    numFeatures[i] = ReadKeyFile(keyPath.c_str(),
        &keys[i], &keysInfo[i]);
    int W = imgList.getImageWidth(i);
    int H = imgList.getImageHeight(i);

    halfWidth[i] = (double)W/2.0f;
    halfHeight[i] = (double)H/2.0f;

    // Get image name (abc.jpg) if full path is given
    string path = imgList.getImageName(i);
    size_t sep = path.find_last_of("\\/");
    if (sep != std::string::npos)
      path = path.substr(sep + 1, path.size() - sep - 1);
    view_names[i] = path; 

    // Read EXIF data find focal length 
    printf("\nReading exif data %d/%d", i, numKeys);
    CameraIntrinsicsPrior cPrior, dummyPrior;
    bool status = exReader.ExtractEXIFMetadata(imgList.getImageName(i), &cPrior);
    if(status) {
      printf("\nRead EXIF data:");
      if(cPrior.focal_length.is_set) {
        calibrationFlag[i] = true;
        focalLengths[i] = cPrior.focal_length.value;
        camPriors.push_back(cPrior);
      } else {
        camPriors.push_back(dummyPrior);
      }
    } else {
      camPriors.push_back(dummyPrior);
    }
  }

  // Read matches file and veriy two view matches
  FILE* fp = fopen( argv[2] , "r");
  if( fp == NULL ) {
    printf("\nCould not read matches file");
    return 0;
  }

  int img1, img2;
  vector<ImagePairMatch> matches;
  
  while(fscanf(fp,"%d %d",&img1,&img2) != EOF) {

    vector< pair<int,int> > matchIdxPairs;
    vector<FeatureCorrespondence> featCorrs;
    vector<FeatureCorrespondence> inlFeatCorrs;
    
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

      matchIdxPairs.push_back(make_pair(matchIdx1, matchIdx2));
    }
  
    vector<int> inliers;
    TwoViewInfo currPairInfo;
    VerifyTwoViewMatchesOptions options;
    bool status = VerifyTwoViewMatches(options,
        camPriors[img1], camPriors[img2],
        featCorrs,&currPairInfo,&inliers);

    for(int in=0; in < inliers.size(); in++) {
      inlFeatCorrs.push_back( featCorrs[inliers[in]] ); 
    }

    if(status) {
      printf("\nSuccessfully Verified Matches");
      ImagePairMatch imPair;
      imPair.image1_index = img1;
      imPair.image2_index = img2; 
      imPair.twoview_info = currPairInfo;
      imPair.correspondences = inlFeatCorrs;
      matches.push_back(imPair);

      int numInl = inliers.size();
//      fprintf(file4, "%d\n", inliers.size());
      for(int j=0; j < numInl; j++) {
        pair<int,int> p = matchIdxPairs[inliers[j]];
        fprintf(file4, "2 %d %d %d %d\n", img1, p.first, img2, p.second); 
      }
    }
  }
  
  bool status = WriteMatchesAndGeometry("view_graph.bin", view_names, camPriors, matches);
  if(!status) {
    printf("\nSuccessfully written view-graph file");
  }
  
  // Write remaining files for Kyle's 1dsfm
  for(int i=0; i < numKeys; i++) {
    fprintf(file1,"%d\n",i);
    fprintf(file3, "#index = %d, name = %s, keys = %d ",
        i, imgList.getImageName(i).c_str(), numFeatures[i]); 
    fprintf(file3, "px = %.1f, py = %.1f, focal = %.3f\n",
        halfWidth[i], halfHeight[i], focalLengths[i]); 

    for(int j=0; j < numFeatures[i]; j++) {
      fprintf(file3, "%d %f %f 0 0 %d %d %d\n",
          j, keysInfo[i][j].x, keysInfo[i][j].y,
          0,0,0); 
    }
  }

  for(int i=0; i < matches.size(); i++) {
    int image1 = matches[i].image1_index;
    int image2 = matches[i].image2_index;

    TwoViewInfo currPairInfo = matches[i].twoview_info; 
    double focal_length1 = currPairInfo.focal_length_1;
    double focal_length2 = currPairInfo.focal_length_2;

    Eigen::Vector3d rotation = currPairInfo.rotation_2;
    Eigen::Vector3d position = currPairInfo.position_2;

    double angleMag = rotation.norm();
    Eigen::Vector3d axisVec = rotation.normalized();

    Eigen::AngleAxisd angax_rotation(angleMag, axisVec);
    Eigen::Matrix3d rotationMat = angax_rotation.toRotationMatrix();

    fprintf(file2, "%d %d ", image1, image2);
    for(int r= 0; r < 3; r++) {
      Eigen::Vector3d row_i = rotationMat.row(r);
      fprintf(file2, "%f %f %f ", row_i[0], row_i[1], row_i[2]);
    } 
    fprintf(file2, "%f %f %f\n", position[0], position[1], position[2]);
  }

  fclose(file1);
  fclose(file2);
  fclose(file3);
  fclose(file4);
  return 0;
}
