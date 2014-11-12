//
//  main_frameToModel.cpp
//  PointPairFeatures
//
//  Created by Adrian Haarbach on 29.10.14.
//  Copyright (c) 2014 Adrian Haarbach. All rights reserved.
//
#include "Visualize.h"
#include "LoadingSaving.h"
#include "PointPairFeatures.h"
#include "PointCloudManipulation.h"
#include <iostream>
#include <string>
#include "Constants.h"

using namespace std;

int main(int argc, char * argv[])
{
    Visualize* inst = Visualize::getInstance();

    MatrixXf m=LoadingSaving::loadMatrixXf("bunny/model.xyz");
    MatrixXf mSmall=PointCloudManipulation::downSample(m,false);
    Translation3f traCentroid=PointCloudManipulation::getTranslationToCentroid(mSmall);
    mSmall=PointCloudManipulation::projectPointsAndNormals(Isometry3f(traCentroid),mSmall);


    inst->model=mSmall; //PointCloudManipulation::projectPointsAndNormals(Isometry3f(traCentroid),m);
    Visualize::visualize();


    GlobalModelDescription model = PointPairFeatures::buildGlobalModelDescription(mSmall);

    for(int i=0; i<=36; i+=2){
        stringstream ss,ss1,ss2;

        ss<<"bunny/depth-cloud/cloudXYZ_"<<i<<".xyz";
        MatrixXf cloud=LoadingSaving::loadMatrixXf(ss.str());

        ss1<<"bunny/depth-poses/poses_"<<i<<".txt";
        Isometry3f P(LoadingSaving::loadMatrix4f(ss1.str()));

        ss2<<"poses_"<<i<<":";

        PointPairFeatures::printPose(P,ss2.str());

        cloud=PointCloudManipulation::projectPointsAndNormals(P, cloud);

        MatrixXf sSmall=PointCloudManipulation::downSample(cloud,false);
        inst->scene=sSmall;

        Isometry3f Pg = PointPairFeatures::getTransformationBetweenPointClouds(mSmall,sSmall,model);

        PointPairFeatures::printPose(Pg, "Pinitial ppf");

        Isometry3f P_Iterative_ICP = Pg; //initialize with ppf coarse alignment
        MatrixXf cloud2;

        int j = 0;
        for (; j < 100; ++j) {  //5 ICP steps

            cloud2=PointCloudManipulation::projectPointsAndNormals(P_Iterative_ICP.inverse(), sSmall);
            vector<Vector3f> src,dst;

            PointCloudManipulation::getClosesPoints(mSmall,cloud2,src,dst,0.03f);

            inst->modelT=cloud2;
            inst->src = src;
            inst->dst = dst;

            cout<<"Iteration "<<i<<endl;
            cout<<"# Scene to Model Correspondences: "<<src.size()<<"=="<<dst.size()<<endl;

            Visualize::spin(3);

            cout<<"ICP "<<endl;

            Isometry3f P_incemental = ICP::computeStep(src,dst,false);            
            PointPairFeatures::printPose(P_incemental, "P incremental ICP");
            if(PointPairFeatures::isPoseCloseToIdentity(P_incemental,0.000001)){
                break;
            }
            P_Iterative_ICP = P_Iterative_ICP * P_incemental;

            PointPairFeatures::printPose(P_Iterative_ICP, "Piterative ICP");

        }

        cout<<"finished ICP after "<<j<<" iterations, press q to continue with next frame"<<endl;

        inst->ms.push_back(make_pair(cloud2,Colormap::Jet(i/36.0)));
        cout<<"cloud size"<<inst->ms.size()<<endl;
        cout<<"cloud "<<i<<" #rows="<<inst->ms[i].first.rows()<<endl;

        Visualize::spin();

    }
}
