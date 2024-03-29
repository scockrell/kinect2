#include "/home/stephanie/code/dev_stacks/kinect/NITE/Include/XnVNite.h"

// the following 3 things i'm adding because now i'm gonna run this in ros.  source: http://www.ros.org/wiki/ROS/Tutorials/WritingPublisherSubscriber%28c%2B%2B%29
#include "ros/ros.h"
#include "std_msgs/String.h"

#include <sstream>

#include <math.h>

// for the writing to file and stuff- to test the output
#include <iostream>
#include <fstream>

// to do the ros msg i have this source http://www.ros.org/wiki/navigation/Tutorials/RobotSetup/Sensors
#include </opt/ros/electric/stacks/common_msgs/sensor_msgs/msg_gen/cpp/include/sensor_msgs/LaserScan.h>
//#include <sensor_msgs/LaserScan.h>

#include <geometry_msgs/PointStamped.h>
#include <tf/transform_listener.h>

using namespace std;
//using namespace sensor_msgs;



// This macro checks the return code that all OpenNI calls make
// and throws an error if the return code is an error. Use this
// after all calls to OpenNI objects. Great for debugging.
#define CHECK_RC(rc, what)											\
	if (rc != XN_STATUS_OK)											\
	{																\
		printf("%s failed: %s\n", what, xnGetStatusString(rc));		\
		return rc;													\
	}

#define _TCHAR char
#define _tmain main

void xYToRTheta(double x, double y, double* result);
void rotate(double x, double y, double z, double theta, double* result);

bool savedPts=false;

//int _tmain(int argc, _TCHAR* argv[]){

void kinectToLaserScan(const tf::TransformListener& listener){
	//
	// Variables

	// Keep track of the return code of all OpenNI calls
	XnStatus nRetVal = XN_STATUS_OK;
	// context is the object that holds most of the things related to OpenNI
	xn::Context context;
	// The DepthGenerator generates a depth map that we can then use to do 
	// cool stuff with. Other interesting generators are gesture generators
	// and hand generators.
	xn::DepthGenerator depth;

	//
	// Initialization
	
	// Initialize context object
	nRetVal = context.Init();
	//CHECK_RC(nRetVal, "Initialize context");
	// Create the depth object
	nRetVal = depth.Create(context);
	//CHECK_RC(nRetVal, "Create Depth");
	
	// Tell the context object to start generating data
	nRetVal = context.StartGeneratingAll();
	//CHECK_RC(nRetVal, "Start Generating All Data");

	// to publish:
	//ros::init(argc, argv, "laser_scan_publisher");
	ros::NodeHandle n;
	ros::Publisher scan_pub = n.advertise<sensor_msgs::LaserScan>("scan", 50);

	double laser_frequency = 40;
	ros::Rate r(15); //run at 15 Hz		

	int angleMax=90;
	int* distances=new int[2*angleMax];
	int* distances_index=new int[2*angleMax];
	int unknownDist=-1; // so, this is in mm, yes?

	//double* rotatedResult=new double[3];
	double* tempRTheta=new double[2];
	double floorThreshold=-950; //-950; // magical floor threshold: -950 yay!

	//int angleIndex;
	int angleIndex2;
	//int tiltAngle=-13; //-20; //60; //45; //15; // in degrees
	// magical angle: -13 yay!

	//int total_iterations=10;
	//int count_iterations=0;

	int real_x_dim=640;
	int real_y_dim=480;

	int x_res=XN_VGA_X_RES;
	int y_res=XN_VGA_Y_RES;	

	int minDist;
	int minIndexJ;


	const XnDepthPixel* pDepthMap;
	XnPoint3D realWorld[real_x_dim*real_y_dim];
	// source: https://groups.google.com/group/openni-dev/browse_thread/thread/e5aebba0852f8803?pli=1
	XnPoint3D pointList[XN_VGA_Y_RES*XN_VGA_X_RES];

	ros::Time scan_time;
	sensor_msgs::LaserScan scan;

	ofstream myfile_tf; 
	myfile_tf.open ("tf_pts.txt");
	ofstream myfile_tf2; 
	myfile_tf2.open ("tf_pts_before.txt");

	// Main loop
	//while(count_iterations<1)
	//while (ros::ok()) //(n.ok()) // hey i don't know man, I just commented that out because of this http://www.ros.org/wiki/ROS/Tutorials/WritingPublisherSubscriber%28c%2B%2B%29
	//{
		// Wait for new data to be available
		nRetVal = context.WaitOneUpdateAll(depth);
		//CHECK_RC(nRetVal, "Updating depth");
		// Get the new depth map
		pDepthMap = depth.GetDepthMap();

		for (int y=0; y<XN_VGA_Y_RES; y++){
			for(int x=0;x<XN_VGA_X_RES;x++){
			        pointList[y * XN_VGA_X_RES + x ].X =x;
			        pointList[y * XN_VGA_X_RES + x ].Y =y;
			        pointList[y * XN_VGA_X_RES + x ].Z = (short) pDepthMap[y *XN_VGA_X_RES + x];
			}
		} 



		depth.ConvertProjectiveToRealWorld(XN_VGA_Y_RES*XN_VGA_X_RES, pointList, realWorld); 

  //we'll create a point in the base_laser frame that we'd like to transform to the base_link frame
  geometry_msgs::PointStamped laser_point;
  //laser_point.header.frame_id = "base_laser";
  laser_point.header.frame_id = "camera_depth_frame";

  //we'll just use the most recent transform available for our simple example
  laser_point.header.stamp = ros::Time();

		// hey guys, x is horizontal, y is vertical, z is depth.

		for(int i=0; i<2*angleMax; i++){
			distances[i]=unknownDist;
			distances_index[i]=unknownDist;
		}
/*
		ofstream myfile_x;
		myfile_x.open ("rotate_x.txt");
		ofstream myfile_y;
		myfile_y.open ("rotate_y.txt");
		ofstream myfile_z;
		myfile_z.open ("rotate_z.txt");
*/

		geometry_msgs::PointStamped base_point;

		for(int i=0; i<x_res; i++){ 
			minDist=1000000;
			for(int j=0; j<y_res; j++){
				laser_point.point.x=realWorld[j*XN_VGA_X_RES + i].Y; 
				laser_point.point.y=-realWorld[j*XN_VGA_X_RES + i].X; 
				laser_point.point.z=realWorld[j*XN_VGA_X_RES + i].Z;
				try{
					//geometry_msgs::PointStamped base_point;
					listener.transformPoint("base_link", laser_point, base_point);

					myfile_tf << base_point.point.x << ", ";
					myfile_tf << base_point.point.y << ", ";
					myfile_tf << base_point.point.z << "\n";
					myfile_tf2 << laser_point.point.x << ", ";
					myfile_tf2 << laser_point.point.y << ", ";
					myfile_tf2 << laser_point.point.z << "\n";

				}
				catch(tf::TransformException& ex){
					ROS_ERROR("Received an exception trying to transform a point from \"camera_depth_frame\" to \"base_link\": %s", ex.what());
				}






				//rotate(realWorld[j*x_res + i].X,realWorld[j*x_res + i].Y,realWorld[j*x_res + i].Z,-tiltAngle, rotatedResult);
				//if(rotatedResult[1]>floorThreshold && rotatedResult[2]<minDist && rotatedResult[2]>0){
				if(base_point.point.z>floorThreshold && base_point.point.y<minDist && base_point.point.y>0){
					minDist=base_point.point.y;
					minIndexJ=j;
				}

			}


			//xYToRTheta(minDist,realWorld[minIndexJ*x_res + i].X, tempRTheta);
			xYToRTheta(minDist,base_point.point.x, tempRTheta);
/*
			if(angleMax-(int)tempRTheta[1]<90){ 
				angleIndex2=90-angleMax+(int)tempRTheta[1];
			}
			else{
				angleIndex2=270-angleMax+(int)tempRTheta[1];
			}
			//gprintf("%d ",angleIndex2);
*/
			printf("%d ",(int) tempRTheta[1]);
			angleIndex2=(int)tempRTheta[1]+90;

			if(distances[angleIndex2]<0 || distances[angleIndex2]>tempRTheta[0]){
				distances[angleIndex2]=(int)(tempRTheta[0]);
				distances_index[angleIndex2]=i;

			}			
		}


		myfile_tf.close();
		myfile_tf2.close();

		// write to file
		ofstream myfile;
		myfile.open ("lidar_result.txt");
		if(!myfile){
			printf("oh my goodness, the file didn't open\n");
		}
		for(int i=0;i<2*angleMax; i++){
			//myfile << "Writing this to a file.\n";
			myfile << distances[i] << ", " << distances_index[i] << endl;
			//printf("wrote to file, i = %d\n",i);
		}

		myfile.close();

/*
		myfile_x.close();
		myfile_y.close();
		myfile_z.close();
*/

	//	count_iterations++;
	//	printf("count_iterations = %d\n",count_iterations);

		scan_time = ros::Time::now();

		//populate the LaserScan message
		//sensor_msgs::LaserScan scan; //declaring this BEFORE the while loop instead
		scan.header.stamp = scan_time;
		//scan.header.frame_id = "laser_frame";
		scan.header.frame_id = "/base_link";
		scan.angle_min = -1.57;
		scan.angle_max = 1.57;
		scan.angle_increment = 3.14 / (2*angleMax);
		scan.time_increment = (1 / laser_frequency) / (2*angleMax);
		scan.range_min = -1; //0.0;
		scan.range_max = 100.0;

		scan.set_ranges_size(2*angleMax);
		//scan.set_intensities_size(2*angleMax);
		for(unsigned int i = 0; i < 2*angleMax; ++i){
			//scan.ranges[i] = distances[i];
			scan.ranges[i] = .01*distances[i]; // this is such a hack- i just want to see if costmap will display smaller numbers.
								// deal with the accuracy issue later
			//scan.intensities[i] = intensities[i];
		}
		savedPts=true;

		scan_pub.publish(scan);
		r.sleep();
	//}


	// eh my attempt to free memory
	delete [] distances;
	// Clean-up
	context.Shutdown();
	//return 0;
}
void xYToRTheta(double x, double y, double* result){
	result[0]=sqrt(x*x+y*y);
	result[1]=atan2(y,x)*180/3.14159;
}
void rotate(double x, double y, double z, double theta, double* result){
	// so for this case, it needs to be (x, depth, height)
	double thetaRad=theta*3.14159/180;
	result[0]=x;
	result[1]=cos(thetaRad)*y - sin(thetaRad)*z;
	result[2]=sin(thetaRad)*y + cos(thetaRad)*z;
}

int main(int argc, char** argv){
  ros::init(argc, argv, "robot_tf_listener");
  ros::NodeHandle n;

  tf::TransformListener listener(ros::Duration(10));
  //tf::TransformListener listener();

  //we'll transform a point once every second
  ros::Timer timer = n.createTimer(ros::Duration(1.0), boost::bind(&kinectToLaserScan, boost::ref(listener)));

	printf("okay everyone, we're right before the spin while loop\n");
	while(!savedPts){
		ros::spinOnce();
		printf("spun once\n");
	}
	//ros::spin();
	printf("kay did the spin thing\n");
  //ros::spin();

}
