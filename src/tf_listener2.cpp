/* tf_listener2.cpp
Stephanie Cockrell
7/19/2012

subscribes to the robot-to-kinect transform
transforms a set of artificial data

outputs txt files:
tf_pts - After transform.
tf_pts_before - Before transform.

*/

#include <ros/ros.h>
#include <geometry_msgs/PointStamped.h>
#include <tf/transform_listener.h>

// for the writing to file and stuff- to test the output
#include <iostream>
#include <fstream>

#include <sstream>

#include "std_msgs/String.h"

using namespace std;

bool savedPts=false;

//ofstream myfile_tf; 

void transformPoint(const tf::TransformListener& listener){
  //we'll create a point in the base_laser frame that we'd like to transform to the base_link frame
  geometry_msgs::PointStamped laser_point;
  laser_point.header.frame_id = "camera_depth_frame";

  //we'll just use the most recent transform available for our simple example
  laser_point.header.stamp = ros::Time();

	ifstream plane_file("plane_params.txt");
	string A_str, B_str, C_str, D_str;
	getline(plane_file,A_str);
	getline(plane_file,B_str);
	getline(plane_file,C_str);
	getline(plane_file,D_str);
	double A=atof(A_str.c_str());
	double B=atof(B_str.c_str());
	double C=atof(C_str.c_str());
	double D=atof(D_str.c_str());
	plane_file.close();

	int n_pts=6;
/*
	double y_list[]={-400, -400, -100, -100, 300, 300};
	double x_list[]={200, -200, 200, -200, 400, -400}; // x in kinect's ref frame
	double z_list[n_pts];
	for(int i=0;i<n_pts;i++){
		z_list[i]=(-D - A*x_list[i] - B*y_list[i])/C;
	}
*/
	double x_list[]={.5, -.5, .5, -.5, 1, -1};
	double z_list[]={.5, .5, 1, 1, 1.5, 1.5}; // x in kinect's ref frame
	double y_list[n_pts];
	for(int i=0;i<n_pts;i++){
		y_list[i]=(-D - A*x_list[i] - C*z_list[i])/B;
	}

	ofstream myfile_tf; 
	myfile_tf.open ("tf_pts.txt");
	ofstream myfile_tf2; 
	myfile_tf2.open ("tf_pts_before.txt");

	for(int i=0; i<n_pts; i++){
		laser_point.point.x=x_list[i];
		laser_point.point.y=y_list[i]; 
		laser_point.point.z=z_list[i];
		  try{
		    geometry_msgs::PointStamped base_point;
		    listener.transformPoint("base_link", laser_point, base_point);
/*
		    ROS_INFO("base_laser: (%.2f, %.2f. %.2f) -----> base_link: (%.2f, %.2f, %.2f) at time %.2f",
			laser_point.point.x, laser_point.point.y, laser_point.point.z,
			base_point.point.x, base_point.point.y, base_point.point.z, base_point.header.stamp.toSec());
*/

			myfile_tf << base_point.point.x << ", ";
			myfile_tf << base_point.point.y << ", ";
			myfile_tf << base_point.point.z << "\n";
			myfile_tf2 << laser_point.point.x << ", ";
			myfile_tf2 << laser_point.point.y << ", ";
			myfile_tf2 << laser_point.point.z << "\n";
		  }
		  catch(tf::TransformException& ex){
		    ROS_ERROR("Received an exception trying to transform a point from \"base_laser\" to \"base_link\": %s", ex.what());
		  }

	}

	myfile_tf.close();
	myfile_tf2.close();



  //just an arbitrary point in space
  //laser_point.point.x = 1.0;
  //laser_point.point.y = 0.2;
  //laser_point.point.z = 0.0;

/*
  try{
    geometry_msgs::PointStamped base_point;
    listener.transformPoint("base_link", laser_point, base_point);

    ROS_INFO("base_laser: (%.2f, %.2f. %.2f) -----> base_link: (%.2f, %.2f, %.2f) at time %.2f",
        laser_point.point.x, laser_point.point.y, laser_point.point.z,
        base_point.point.x, base_point.point.y, base_point.point.z, base_point.header.stamp.toSec());
  }
  catch(tf::TransformException& ex){
    ROS_ERROR("Received an exception trying to transform a point from \"base_laser\" to \"base_link\": %s", ex.what());
  }
*/
	savedPts=true;
}

int main(int argc, char** argv){
  ros::init(argc, argv, "robot_tf_listener");
  ros::NodeHandle n;

  tf::TransformListener listener(ros::Duration(10));
  //tf::TransformListener listener();

  //we'll transform a point once every second
  ros::Timer timer = n.createTimer(ros::Duration(1.0), boost::bind(&transformPoint, boost::ref(listener)));

	printf("okay everyone, we're right before the spin while loop\n");
	while(!savedPts){
		ros::spinOnce();
		printf("spun once\n");
	}
	printf("kay did the spin thing\n");
  //ros::spin();

}
