#!/bin/bash
PIPELINE_TEST_EXEC=$PWD"/build/tabletop_scenario_test"

# Directory where dataset is stored
DATASET_DIR=$PWD"/dataset/other/"

#CYLINDER FITTING PARAMETERS (HOUGH)
ANGLE_BINS=30
RADIUS_BINS=10
POSITION_BINS=100
ORIENTATION_ACCUMULATORS_NUM=1
GAUSSIAN_SPHERE_POINTS_NUM=400
ACCUMULATOR_PEAK_THRESHOLD=0.8
MIN_RADIUS=0.045
MAX_RADIUS=0.055

#PLANE FITTING PARAMETERS (RANSAC)
DISTANCE_THRESHOLD=0.02
CLUSTER_TOLERANCE=0.02
MIN_CLUSTER_SIZE=100
MAX_CLUSTER_SIZE=25000
DO_REFINE=false
TABLE_Z_FILTER_MIN=0.01
TABLE_Z_FILTER_MAX=0.5
Z_FILTER_MIN=-10.0
Z_FILTER_MAX=10.0
PLANE_DETECTION_VOXEL_SIZE=0.2
CLUSTER_VOXEL_SIZE=0.03
INLIER_THRESHOLD=50


$PIPELINE_TEST_EXEC $DATASET_DIR $ANGLE_BINS $RADIUS_BINS $POSITION_BINS $ORIENTATION_ACCUMULATORS_NUM $GAUSSIAN_SPHERE_POINTS_NUM $ACCUMULATOR_PEAK_THRESHOLD $MIN_RADIUS $MAX_RADIUS $DISTANCE_THRESHOLD $CLUSTER_TOLERANCE $MIN_CLUSTER_SIZE $MAX_CLUSTER_SIZE $DO_REFINE $TABLE_Z_FILTER_MIN $TABLE_Z_FILTER_MAX $Z_FILTER_MIN $Z_FILTER_MAX $PLANE_DETECTION_VOXEL_SIZE $CLUSTER_VOXEL_SIZE $INLIER_THRESHOLD


