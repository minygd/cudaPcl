/* Copyright (c) 2014, Julian Straub <jstraub@csail.mit.edu>
 * Licensed under the MIT license. See the license file LICENSE.
 */

#include <iostream>
#include <fstream>
#include <string>

// Utilities and system includes
//#include <helper_functions.h>
#include <boost/program_options.hpp>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/contrib/contrib.hpp>

#include <cudaPcl/depthGuidedFilter.hpp>
#include <cudaPcl/normalExtractSimpleGpu.hpp>

#include <Eigen/Dense>
#include <pcl/io/ply_io.h>

namespace po = boost::program_options;
using namespace Eigen;
using std::cout;
using std::endl;


int main (int argc, char** argv)
{

  // Declare the supported options.
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help,h", "produce help message")
    ("input,i", po::value<string>(), "path to input depth image (16bit .png)")
    ("output,o", po::value<string>(), "path to output surface normals (csv)")
    ("f_d,f", po::value<double>(), "focal length of depth camera")
    ("eps,e", po::value<double>(), "sqrt of the epsilon parameter of the guided filter")
    ("B,b", po::value<int>(), "guided filter windows size (size will be (2B+1)x(2B+1))")
    ("compress,c", "compress the computed normals")
    ("display,d", "display the computed normals")
//    ("out,o", po::value<std::string>(), "output path where surfae normal images are saved to")
    ;

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);    

  if (vm.count("help")) {
    cout << desc << "\n";
    return 1;
  }

  double f_d = 540.;
  double eps = 0.2*0.2;
  int32_t B = 10;
  bool compress = false;
  bool display = false;
  string inputPath = "";
  string outputPath = "";
  if(vm.count("input")) inputPath = vm["input"].as<string>();
  if(vm.count("output")) outputPath = vm["output"].as<string>();
  if(vm.count("f_d")) f_d = vm["f_d"].as<double>();
  if(vm.count("eps")) eps = vm["eps"].as<double>();
  if(vm.count("B")) B = vm["B"].as<int>();
  if(vm.count("compress")) compress = true;
  if(vm.count("display")) display = true;
//  std::string outPath = ""
//  if(vm.count("out")) outPath = vm["out"].as<std::string>();

  if(inputPath.compare("") == 0)
  {
    cout<<"provide an input to a depth image file"<<endl;
    exit(1);
  }

  findCudaDevice(argc,(const char**)argv);
  cv::Mat depth = cv::imread(inputPath,CV_LOAD_IMAGE_ANYDEPTH);
  std::cout << "reading depth from " << inputPath << std::endl;
  inputPath.replace(inputPath.end()-5, inputPath.end(), "rgb.png");

  std::cout << "reading rgb from " << inputPath << std::endl;
  cv::Mat rgb = cv::imread(inputPath);
  std::cout << "reading gray from " << inputPath << std::endl;
  cv::Mat gray = cv::imread(inputPath, CV_LOAD_IMAGE_GRAYSCALE);
  uint32_t w = depth.cols;
  uint32_t h = depth.rows;

  if (w==0 || h==0) {
    std::cout << "could not load images" << std::endl;
    return (1);
  }

  cudaPcl::DepthGuidedFilterGpu<float>* depthFilter = 
    new cudaPcl::DepthGuidedFilterGpu<float>(w,h,eps,B);
  cudaPcl::NormalExtractSimpleGpu<float>* normalExtract = 
    new cudaPcl::NormalExtractSimpleGpu<float>(f_d,w,h,compress);

  depthFilter->filter(depth);
  normalExtract->computeGpu(depthFilter->getDepthDevicePtr());
  cv::Mat normalsImg = normalExtract->normalsImg();

  if(outputPath.compare("") != 0)
  {
    std::ofstream out((outputPath+".normals").data(), std::ofstream::out | std::ofstream::binary);
    out<<h<<" "<<w<<" "<<3<<endl;
    char* data = reinterpret_cast<char*>(normalsImg.data);
    out.write(data, w*h*3*sizeof(float));
//    for (uint32_t i=0; i<h; ++i)
//      for (uint32_t j=0; j<w; ++j)
//        out<< normalsImg.at<cv::Vec3f>(i,j)[0] << " "
//          << normalsImg.at<cv::Vec3f>(i,j)[1] << " "
//          << normalsImg.at<cv::Vec3f>(i,j)[2] <<endl;
    out.close();
    cout << "surface normals writen to "<<outputPath<< ".normals" <<endl;
  
    cv::Mat dSmooth = depthFilter->getOutput();
    cv::Mat dSmoothS;
    dSmooth.convertTo(dSmoothS, CV_16U, 1000.);
    cv::imwrite((outputPath+"_dSmooth.png").data(), dSmoothS);
    std::cout << "smooth depth written to " << (outputPath+"_dSmooth.png") << std::endl;

//    cv::Mat depth2 = cv::imread((outputPath+"_dSmooth.png").data(),CV_LOAD_IMAGE_ANYDEPTH);
//    cv::imshow("n",dSmoothS);
//    cv::imshow("d",depth);
//    cv::imshow("d2",depth2);
//    while(42) cv::waitKey(10);
//    uint32_t count = 0;
//    for (uint32_t i=0; i<h; ++i)
//      for (uint32_t j=0; j<w; ++j) {
//        pcl::PointXYZINormal p;
//        p.z = depth.at<uint16_t>(i,j)*0.001;
//        p.normal[0] = normalsImg.at<cv::Vec3f>(i,j)[0];
//        if (p.z == p.z && p.normal[0] == p.normal[0]) {
//          ++count;
//        }
//      }
//    out.open((outputPath+".ply").c_str());
//    out << "ply" << std::endl;
//    out << "format ascii 1.0" << std::endl;
//    out << "comment generated by openniSmoothNormals_file of the cudaPcl package" << std::endl;
//    out << "element vertex " << count << std::endl;
//    out << "property float x " << std::endl;
//    out << "property float y " << std::endl;
//    out << "property float z " << std::endl;
//    out << "property float nx " << std::endl;
//    out << "property float ny " << std::endl;
//    out << "property float nz " << std::endl;
//    out << "property uchar red " << std::endl;
//    out << "property uchar green " << std::endl;
//    out << "property uchar blue " << std::endl;
//    out << "end_header" << std::endl;
    pcl::PointCloud<pcl::PointXYZRGBNormal> pcOut;
    for (uint32_t i=0; i<h; ++i)
      for (uint32_t j=0; j<w; ++j) {
        pcl::PointXYZRGBNormal p;
        p.z = depth.at<uint16_t>(i,j)*0.001;
        p.x = (j-float(w/2)+0.5)*p.z/f_d;
        p.y = (i-float(h/2)+0.5)*p.z/f_d;
        p.normal[0] = normalsImg.at<cv::Vec3f>(i,j)[0];
        p.normal[1] = normalsImg.at<cv::Vec3f>(i,j)[1];
        p.normal[2] = normalsImg.at<cv::Vec3f>(i,j)[2];
        p.rgb = ((int)rgb.at<cv::Vec3b>(i,j)[2]) << 16 
          | ((int)rgb.at<cv::Vec3b>(i,j)[1]) << 8 
          | ((int)rgb.at<cv::Vec3b>(i,j)[0]); 
        if (p.x == p.x && p.normal[0] == p.normal[0]) {
          pcOut.push_back(p);
        }
      }
    pcl::PLYWriter writer;
    writer.write(outputPath+".ply", pcOut, false, false);
    std::cout << "output pc ("<<pcOut.size() << ") written to " << (outputPath+".ply") << std::endl;
//    out.close();
  }

  if(display)
  {
    cv::imshow("d",depth);
    cv::imshow("n",normalsImg);
    cv::waitKey(0);
  }

  cout<<cudaDeviceReset()<<endl;
  return (0);
}


