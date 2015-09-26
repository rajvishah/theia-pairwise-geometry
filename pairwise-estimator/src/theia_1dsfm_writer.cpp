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

  //Prepare files to run Kyle's 1DSFM Code
  
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
  
  FILE* file4 = fopen("../output/verified_matches.txt", "w");
  if(file4 == NULL) {
    printf("\nCould not open tracks file to write");
    return -1;
  }

  /*
  for(int i=0; i < numKeys; i++) {
    fprintf(file1,"%d\n",i);
  }*/


  vector<string> imnames;
  vector<CameraIntrinsicsPrior> cps;
  vector<ImagePairMatch> matches;

  bool s0 = ReadMatchesAndGeometry("../data/view_graph.bin",&imnames,&cps,&matches);
  if(s0) {
    printf("\nSuccessfully Read Matches And Geo, matches size %d", matches.size());

    for(int i=0; i < matches.size(); i++) {
    //for(int i=0; i < 5; i++) {
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

      //std::cout << "Matrix " << i << ", rows" << rotationMat.rows() << ", cols" << rotationMat.cols() << endl;
      //std::cout << rotationMat;

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
  } else { 
    printf("\nCould not read bin file");
    return -1;
  }


  return 0;
}
