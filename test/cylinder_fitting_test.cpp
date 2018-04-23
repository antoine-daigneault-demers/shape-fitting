﻿#include "cylinder_segmentation_hough.h"
#include <ctime>
#include <tf/transform_broadcaster.h>
#include <rosbag/view.h>
#include <active_semantic_mapping/Cylinders.h>
#include <rosbag/bag.h>
#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH
#include "cylinder_segmentation_hough.h"
#include "cylinder_segmentation_ransac.h"
#include "pcl_ros/point_cloud.h"
#include <visualization_msgs/MarkerArray.h>
#include <helpers.h>
#include <chrono>
using namespace std;
using namespace std::chrono;

int main (int argc, char** argv)
{
	/* TODO - Process options */
	if (argc < 6)
	{
		std::cout << "invalid number of arguments: rosbag_dir iterations heights radii noise clutter"<< std::endl;
		exit(-1);
	}

    	std::string input_rosbag_file = std::string(argv[1]);
	std::cout << "input_rosbag_file: " << input_rosbag_file<< std::endl;

    	std::string output_rosbag_folder = std::string(argv[2]);
	std::cout << "output_rosbag_folder: " << output_rosbag_folder<< std::endl;

	unsigned int angle_bins=atoi(argv[3]);
	std::cout << "angle_bins: " << angle_bins<< std::endl;

	unsigned int radius_bins=atoi(argv[4]);
	std::cout << "radius_bins: " << radius_bins<< std::endl;

	unsigned int position_bins=atoi(argv[5]);
	std::cout << "position_bins: " << position_bins<< std::endl;

	unsigned int orientation_accumulators_num=atoi(argv[6]);
	std::cout << "orientation_accumulators_num: " << orientation_accumulators_num<< std::endl;

	unsigned int gaussian_sphere_points_num=atoi(argv[7]);
	std::cout << "gaussian_sphere_points_num: " << gaussian_sphere_points_num<< std::endl;
	
	float accumulator_peak_threshold=atof(argv[8]);
	std::cout << "accumulator_peak_threshold: " << accumulator_peak_threshold<< std::endl;
	
	float min_radius=atof(argv[9]);
	std::cout << "min_radius: " << min_radius<< std::endl;
		
	float max_radius=atof(argv[10]);
	std::cout << "max_radius: " << max_radius<< std::endl;
	
	// RANSAC PARAMETERS
	float normal_distance_weight=atof(argv[11]);
	std::cout << "normal_distance_weight: " << normal_distance_weight<< std::endl;

	unsigned int max_iterations=atoi(argv[12]);
	std::cout << "max_iterations: " << max_iterations << std::endl;

	float distance_threshold=atof(argv[13]);
	std::cout << "distance_threshold: " << distance_threshold << std::endl;

	bool do_refine=atoi(argv[14]);
	std::cout << "do_refine: " << do_refine << std::endl;


	/*unsigned int angle_bins=30;
	unsigned int radius_bins=10;
 	unsigned int position_bins=100;
	unsigned int orientation_accumulators_num=1;
	int gaussian_sphere_points_num=450;
        float accumulator_peak_threshold=0.8;
	// TESTING PARAMS
 	float min_radius=0.45;
 	float max_radius=0.55;*/


	ros::init(argc, argv, "cylinder_publisher");

	/**
	* NodeHandle is the main access point to communications with the ROS system.
	* The first NodeHandle constructed will fully initialize this node, and the last
	* NodeHandle destructed will close down the node.
	*/

	ros::NodeHandle n;

	// For visualization purposes
	ros::Publisher cloud_pub;
	cloud_pub=n.advertise<pcl::PointCloud<PointT> >( "cylinders_pcl", 0 );


	ros::Publisher detection_pub;
	detection_pub=n.advertise<visualization_msgs::MarkerArray>( "detections", 0 );

	std::vector<double> weights;
	std::vector<Eigen::Matrix<double, 3 ,1> > means;
	std::vector<Eigen::Matrix<double, 3 ,1> > std_devs;
	weights.push_back(1.0);
	Eigen::Matrix<double, 3 ,1> mean_eigen(0,0,0);
	means.push_back(mean_eigen);
	Eigen::Matrix<double, 3 ,1> std_dev_eigen(0.5,0.5,0.5);
	std_devs.push_back(std_dev_eigen);

	GaussianMixtureModel gmm(weights, means, std_devs);
	GaussianSphere gaussian_sphere(gmm,gaussian_sphere_points_num,orientation_accumulators_num);


	std::vector<boost::shared_ptr<CylinderSegmentation> > cylinder_segmentators;

	// HOUGH NORMAL
	cylinder_segmentators.push_back(boost::shared_ptr<CylinderSegmentationHough> (new CylinderSegmentationHough(gaussian_sphere,(unsigned int)angle_bins,(unsigned int)radius_bins,(unsigned int)position_bins,(float)min_radius, (float)max_radius,(float)accumulator_peak_threshold,CylinderSegmentationHough::NORMAL)));

	// HOUGH HYBRID
	cylinder_segmentators.push_back(boost::shared_ptr<CylinderSegmentationHough> (new CylinderSegmentationHough(gaussian_sphere,(unsigned int)angle_bins,(unsigned int)radius_bins,(unsigned int)position_bins,(float)min_radius, (float)max_radius,(float)accumulator_peak_threshold,CylinderSegmentationHough::HYBRID)));

	// RANSAC NORMAL
	cylinder_segmentators.push_back(boost::shared_ptr<CylinderSegmentationRansac> (new CylinderSegmentationRansac(normal_distance_weight,(unsigned int)max_iterations,(unsigned int)distance_threshold,(unsigned int)min_radius,(float)max_radius, do_refine)));

	// HOUGH + RANSAC
	//cylinder_segmentators.push_back(boost::shared_ptr<CylinderFittingHoughRANSAC> (new CylinderSegmentationHough(gaussian_sphere,(unsigned int)angle_bins,(unsigned int)radius_bins,(unsigned int)position_bins,(float)min_radius, (float)max_radius,(float)accumulator_peak_threshold,CylinderSegmentationHough::NORMAL)));
	
	// HOUGH + PROSAC
	//cylinder_segmentators.push_back(boost::shared_ptr<CylinderFittingHoughProsacRANSAC> (new CylinderSegmentationHough(gaussian_sphere,(unsigned int)angle_bins,(unsigned int)radius_bins,(unsigned int)position_bins,(float)min_radius, (float)max_radius,(float)accumulator_peak_threshold,CylinderSegmentationHough::NORMAL)));

	std::vector<std::string> topics;
	topics.push_back(std::string("point_cloud"));
	topics.push_back(std::string("ground_truth"));

	rosbag::Bag bag;

	bag.open(input_rosbag_file, rosbag::bagmode::Read);
    	rosbag::View view(bag, rosbag::TopicQuery(topics));

	// Get ground truths
	std::vector<active_semantic_mapping::Cylinders> ground_truths;
    	foreach(rosbag::MessageInstance const m, view)
	{	
		active_semantic_mapping::Cylinders::ConstPtr s = m.instantiate<active_semantic_mapping::Cylinders>();
		if (s == NULL)
			continue;

		ground_truths.push_back(*s);
	}
	bag.close();

	unsigned int iterations=6*200;

	ROS_ERROR_STREAM("Number of ground truths:"<<ground_truths.size());

	bag.open(input_rosbag_file, rosbag::bagmode::Read);
    	rosbag::View view2(bag, rosbag::TopicQuery(topics));

	std::vector<pcl::PointCloud<pcl::PointXYZ>::Ptr > point_clouds;
    	foreach(rosbag::MessageInstance const m, view2)
	{

		pcl::PointCloud<pcl::PointXYZ>::ConstPtr s = m.instantiate<pcl::PointCloud<pcl::PointXYZ> >();
		if (s == NULL)
			continue;

		pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>);
		*cloud=*s;
		point_clouds.push_back(cloud);
	}

	bag.close();
	ROS_ERROR_STREAM("Number of point clouds:"<<point_clouds.size());

	std::string detections_frame_id="world";
	std::string marker_detections_namespace_="detections";
	// Segment cylinder and store results
	std::vector<Eigen::VectorXf > detections;
	std::vector<float> position_errors;



	for (unsigned int d=0;d < cylinder_segmentators.size() && ros::ok();++d)
	{

		std::fstream fs_orientation;
		std::fstream fs_radius;
		std::fstream fs_position;
		std::fstream fs_time;
		fs_orientation.open (output_rosbag_folder+"_orientation_noise_biased2_"+ std::to_string(d)+".txt", std::fstream::in | std::fstream::out | std::fstream::app );
		fs_radius.open (output_rosbag_folder+"_radius_noise_biased2_"          + std::to_string(d)+".txt", std::fstream::in | std::fstream::out | std::fstream::app);
		fs_position.open (output_rosbag_folder+"_position_noise_biased2_"      + std::to_string(d)+".txt", std::fstream::in | std::fstream::out | std::fstream::app);
		fs_time.open (output_rosbag_folder+"_time_noise_biased2_"      + std::to_string(d)+".txt", std::fstream::in | std::fstream::out | std::fstream::app);

		for(unsigned int i=0;i<point_clouds.size() && ros::ok();++i)
		{	//continue;

			active_semantic_mapping::Cylinders ground_truth=ground_truths[i];

			pcl::PointCloud<PointT>::Ptr point_cloud(new pcl::PointCloud<PointT>());
			*point_cloud=*point_clouds[i];

			/* FITTING */
    			high_resolution_clock::time_point t1 = high_resolution_clock::now();
			CylinderFitting model_params=cylinder_segmentators[d]->segment(point_clouds[i]);
    			high_resolution_clock::time_point t2 = high_resolution_clock::now();
    			auto duration = duration_cast<milliseconds>( t2 - t1 ).count();
			fs_time << duration << " ";
			std::cout << "total time: " << duration << " ms"<< std::endl;
			/* END FITTING */
			//detections.push_back(model_params);

			float orientation_error=acos(model_params.parameters.segment(3,3).dot(Eigen::Vector3f(ground_truth.cylinders.data[3],ground_truth.cylinders.data[4],ground_truth.cylinders.data[5])));
			if(orientation_error>M_PI/2.0)
				orientation_error=M_PI-orientation_error;
			fs_orientation << orientation_error << " ";

			float radius_error=fabs(model_params.parameters[6]-ground_truth.cylinders.data[6]);
			fs_radius << radius_error << " ";


			float position_error=(model_params.parameters.head(3)-Eigen::Vector3f(ground_truth.cylinders.data[0],
										   ground_truth.cylinders.data[1],
										   ground_truth.cylinders.data[2])).norm();
			fs_position << position_error << " ";

			fs_orientation<< "\n";
			fs_radius<< "\n";
			fs_position<< "\n";
			fs_time<< "\n";

			visualization_msgs::MarkerArray markers_;
			// First delete all markers
			visualization_msgs::Marker marker_;
			marker_.action = 3;
			markers_.markers.push_back(marker_);
			visualization_msgs::Marker marker=createMarker(model_params.parameters,visualization_msgs::Marker::CYLINDER,detections_frame_id,  0, marker_detections_namespace_);
			markers_.markers.push_back(marker);

			//if(i<0*200+200&&i>0*200)
			{
				detection_pub.publish( markers_ );
				cloud_pub.publish(point_cloud);
			}
		}

		fs_orientation.close();
		fs_radius.close();
		fs_position.close();
		fs_time.close();
	}


	return (0);
}
