﻿/*******************************************************************************
* Copyright 2018 ROBOTIS CO., LTD.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

/* Authors: Darby Lim, Hye-Jong KIM, Ryan Shim, Yong-Ho Na */

#include "../../include/robotis_manipulator/robotis_manipulator_trajectory_generator.h"

using namespace ROBOTIS_MANIPULATOR;


MinimumJerk::MinimumJerk()
{
  coefficient_ = Eigen::VectorXd::Zero(6);
}

MinimumJerk::~MinimumJerk() {}

void MinimumJerk::calcCoefficient(Point start,
                                  Point goal,
                                  double move_time,
                                  double control_time)
{
  uint16_t step_time = uint16_t(floor(move_time / control_time) + 1.0);
  move_time = double(step_time - 1) * control_time;

  Eigen::Matrix3d A = Eigen::Matrix3d::Identity(3, 3);
  Eigen::Vector3d x = Eigen::Vector3d::Zero();
  Eigen::Vector3d b = Eigen::Vector3d::Zero();

  A << pow(move_time, 3), pow(move_time, 4), pow(move_time, 5),
      3 * pow(move_time, 2), 4 * pow(move_time, 3), 5 * pow(move_time, 4),
      6 * pow(move_time, 1), 12 * pow(move_time, 2), 20 * pow(move_time, 3);

  coefficient_(0) = start.position;
  coefficient_(1) = start.velocity;
  coefficient_(2) = 0.5 * start.acceleration;

  b << (goal.position - start.position - (start.velocity * move_time + 0.5 * start.acceleration * pow(move_time, 2))),
      (goal.velocity - start.velocity - (start.acceleration * move_time)),
      (goal.acceleration - start.acceleration);

  Eigen::ColPivHouseholderQR<Eigen::Matrix3d> dec(A);
  x = dec.solve(b);

  coefficient_(3) = x(0);
  coefficient_(4) = x(1);
  coefficient_(5) = x(2);
}

Eigen::VectorXd MinimumJerk::getCoefficient()
{
  return coefficient_;
}

//-------------------- Joint trajectory --------------------//

JointTrajectory::JointTrajectory()
{}

JointTrajectory::~JointTrajectory() {}

void JointTrajectory::init(double move_time,
                           double control_time, JointWayPoint start,
                           JointWayPoint goal)
{
  coefficient_size_ = start.size();
  coefficient_.resize(6,coefficient_size_);
  for (uint8_t index = 0; index < coefficient_size_; index++)
  {
    trajectory_generator_.calcCoefficient(start.at(index),
                                    goal.at(index),
                                    move_time,
                                    control_time);

    coefficient_.col(index) = trajectory_generator_.getCoefficient();
  }
}

JointWayPoint JointTrajectory::getJointWayPoint(double tick)
{
  JointWayPoint joint_way_point;
  for (uint8_t index = 0; index < coefficient_size_; index++)
  {
    JointValue single_joint_way_point;

    single_joint_way_point.position = coefficient_(0, index) +
             coefficient_(1, index) * pow(tick, 1) +
             coefficient_(2, index) * pow(tick, 2) +
             coefficient_(3, index) * pow(tick, 3) +
             coefficient_(4, index) * pow(tick, 4) +
             coefficient_(5, index) * pow(tick, 5);

    single_joint_way_point.velocity = coefficient_(1, index) +
             2 * coefficient_(2, index) * pow(tick, 1) +
             3 * coefficient_(3, index) * pow(tick, 2) +
             4 * coefficient_(4, index) * pow(tick, 3) +
             5 * coefficient_(5, index) * pow(tick, 4);

    single_joint_way_point.acceleration = 2 * coefficient_(2, index) +
             6 * coefficient_(3, index) * pow(tick, 1) +
             12 * coefficient_(4, index) * pow(tick, 2) +
             20 * coefficient_(5, index) * pow(tick, 3);

    single_joint_way_point.effort = 0.0;

    joint_way_point.push_back(single_joint_way_point);
  }

  return joint_way_point;
}

Eigen::MatrixXd JointTrajectory::getCoefficient()
{
  return coefficient_;
}

//-------------------- Task trajectory --------------------//

TaskTrajectory::TaskTrajectory()
{
  coefficient_ = Eigen::MatrixXd::Identity(6, 4);
}
TaskTrajectory::~TaskTrajectory() {}

void TaskTrajectory::init(double move_time,
                           double control_time, TaskWayPoint start,
                           TaskWayPoint goal)
{
  std::vector<Point> start_way_point;
  std::vector<Point> goal_way_point;

  //position
  for(uint8_t i = 0; i < 3; i++)
  {
    Point position_temp;
    position_temp.position = start.kinematic.position[i];
    position_temp.velocity = start.dynamic.linear.velocity[i];
    position_temp.acceleration = start.dynamic.linear.acceleration[i];
    position_temp.effort = 0.0;
    start_way_point.push_back(position_temp);

    position_temp.position = goal.kinematic.position[i];
    position_temp.velocity = goal.dynamic.linear.velocity[i];
    position_temp.acceleration = goal.dynamic.linear.acceleration[i];
    position_temp.effort = 0.0;
    goal_way_point.push_back(position_temp);
  }
  //orientation
  for(uint8_t i = 0; i < 3; i++)
  {
    Point position_temp;


    position_temp.position = start.kinematic.position[i];
    position_temp.velocity = start.dynamic.linear.velocity[i];
    position_temp.acceleration = start.dynamic.linear.acceleration[i];
    position_temp.effort = 0.0;
    start_way_point.push_back(position_temp);

    position_temp.position = goal.kinematic.position[i];
    position_temp.velocity = goal.dynamic.linear.velocity[i];
    position_temp.acceleration = goal.dynamic.linear.acceleration[i];
    position_temp.effort = 0.0;
    goal_way_point.push_back(position_temp);
  }



//  Point orientation_temp;

//  orientation_temp.position = RM_MATH::convertRotToOmega(start.kinematic.orientation).norm();
//  orientation_temp.velocity = start.dynamic.angular.velocity.norm();
//  orientation_temp.acceleration = start.dynamic.angular.acceleration.norm();
//  orientation_temp.effort = 0.0;
//  start_way_point.push_back(orientation_temp);

//  orientation_temp.position = RM_MATH::convertRotToOmega(goal.kinematic.orientation).norm();
//  orientation_temp.velocity = goal.dynamic.angular.velocity.norm();
//  orientation_temp.acceleration = goal.dynamic.angular.acceleration.norm();
//  orientation_temp.effort = 0.0;
//  goal_way_point.push_back(orientation_temp);
  coefficient_size_ = start_way_point.size();
  coefficient_.resize(6,coefficient_size_);
  for (uint8_t index = 0; index < coefficient_size_; index++)
  {
    trajectory_generator_.calcCoefficient(start_way_point.at(index),
                                    goal_way_point.at(index),
                                    move_time,
                                    control_time);

    coefficient_.col(index) = trajectory_generator_.getCoefficient();
  }
}

TaskWayPoint TaskTrajectory::getTaskWayPoint(double tick)
{
  std::vector<Point> result_point;
  for (uint8_t index = 0; index < coefficient_size_; index++)
  {
    Point single_task_way_point;

    single_task_way_point.position = coefficient_(0, index) +
             coefficient_(1, index) * pow(tick, 1) +
             coefficient_(2, index) * pow(tick, 2) +
             coefficient_(3, index) * pow(tick, 3) +
             coefficient_(4, index) * pow(tick, 4) +
             coefficient_(5, index) * pow(tick, 5);

    single_task_way_point.velocity = coefficient_(1, index) +
             2 * coefficient_(2, index) * pow(tick, 1) +
             3 * coefficient_(3, index) * pow(tick, 2) +
             4 * coefficient_(4, index) * pow(tick, 3) +
             5 * coefficient_(5, index) * pow(tick, 4);

    single_task_way_point.acceleration = 2 * coefficient_(2, index) +
             6 * coefficient_(3, index) * pow(tick, 1) +
             12 * coefficient_(4, index) * pow(tick, 2) +
             20 * coefficient_(5, index) * pow(tick, 3);

    single_task_way_point.effort = 0.0;

    result_point.push_back(single_task_way_point);
  }

  TaskWayPoint task_way_point;
  //position
  for(uint8_t i = 0; i < 3; i++)        //x ,y ,z
  {
    task_way_point.kinematic.position[i] = result_point.at(i).position;
    task_way_point.dynamic.linear.velocity[i] = result_point.at(i).velocity;
    task_way_point.dynamic.linear.acceleration[i] = result_point.at(i).acceleration;
  }

//  //orientation
//  task_way_point.kinematic.orientation = 0.0;
//  task_way_point.dynamic.angular.velocity = 0.0;
//  task_way_point.dynamic.angular.acceleration = 0.0;

  return task_way_point;
}

Eigen::MatrixXd TaskTrajectory::getCoefficient()
{
  return coefficient_;
}


//------------------------ trajectory ------------------------//

void Trajectory::setMoveTime(double move_time)
{
  trajectory_time_.total_move_time = move_time;
}

void Trajectory::setPresentTime(double present_time)
{
  trajectory_time_.present_time = present_time;
}

void Trajectory::setStartTimeFromPresentTime()
{
  trajectory_time_.start_time = trajectory_time_.present_time;
}

void Trajectory::setControlLoopTime(double control_time)
{
  trajectory_time_.control_loop_time = control_time;
}

double Trajectory::getMoveTime()
{
  return trajectory_time_.total_move_time;
}

double Trajectory::getControlLoopTime()
{
  return trajectory_time_.control_loop_time;
}

double Trajectory::getTickTime()
{
  return trajectory_time_.present_time - trajectory_time_.start_time;
}

void Trajectory::setTrajectoryManipulator(Manipulator manipulator)
{
  manipulator_= manipulator;
}

Manipulator* Trajectory::getTrajectoryManipulator()
{
  return &manipulator_;
}

JointTrajectory Trajectory::getJointTrajectory()
{
  return joint_;
}

TaskTrajectory Trajectory::getTaskTrajectory()
{
  return task_;
}

CustomJointTrajectory *Trajectory::getCustomJointTrajectory(Name name)
{
  return cus_joint_.at(name);
}

CustomTaskTrajectory *Trajectory::getCustomTaskTrajectory(Name name)
{
  return cus_task_.at(name);
}

void Trajectory::addCustomTrajectory(Name trajectory_name, CustomJointTrajectory *custom_trajectory)
{
  cus_joint_.insert(std::make_pair(trajectory_name, custom_trajectory));
}

void Trajectory::addCustomTrajectory(Name trajectory_name, CustomTaskTrajectory *custom_trajectory)
{
  cus_task_.insert(std::make_pair(trajectory_name, custom_trajectory));
}

void Trajectory::setCustomTrajectoryOption(Name trajectory_name, const void* arg)
{
  if(cus_joint_.find(trajectory_name) != cus_joint_.end())
    cus_joint_.at(trajectory_name)->setOption(arg);
  else if(cus_task_.find(trajectory_name) != cus_task_.end())
    cus_task_.at(trajectory_name)->setOption(arg);
}

//void Trajectory::setPresentDrawingObjectName(Name present_drawing_object_name)
//{
//  present_drawing_object_name_ = present_drawing_object_name;
//}

void Trajectory::setPresentControlToolName(Name present_control_tool_name)
{
  present_control_tool_name_ = present_control_tool_name;
}

Name Trajectory::getPresentCustomTrajectoryName()
{
  return present_custom_trajectory_name_;
}

Name Trajectory::getPresentControlToolName()
{
 return present_control_tool_name_;
}

void Trajectory::initTrajectoryWayPoint(double present_time, Manipulator present_real_manipulator, KinematicsDynamics* kinematics)
{
  setTrajectoryManipulator(present_real_manipulator);
  JointWayPoint joint_way_point_vector;
  joint_way_point_vector = getTrajectoryManipulator()->getAllActiveJointValue();

  setPresentJointWayPoint(joint_way_point_vector);
  UpdatePresentWayPoint(kinematics);
  setPresentTime(present_time);
}

void Trajectory::UpdatePresentWayPoint(KinematicsDynamics *kinematics_dynamics)
{
  //kinematics (position)
  kinematics_dynamics->updatePassiveJointValue(&manipulator_);
  kinematics_dynamics->forwardKinematics(&manipulator_);

  //dynamics (velocity)
  std::map<Name, Component>::iterator it;
  Eigen::VectorXd joint_velocity(manipulator_.getDOF());

  Eigen::VectorXd pose_velocity(6);
  Eigen::Vector3d linear_velocity;
  Eigen::Vector3d angular_velocity;

  int8_t index = 0;
  for (it = manipulator_.getIteratorBegin(); it != manipulator_.getIteratorEnd(); it++)
  {
    if (manipulator_.checkComponentType(it->first, ACTIVE_JOINT_COMPONENT)) // Check whether Active or Passive
    {
      // Active
      joint_velocity[index] = manipulator_.getJointVelocity(it->first);
      index++;
    }
  }

  for (it = manipulator_.getIteratorBegin(); it != manipulator_.getIteratorEnd(); it++)
  {
    pose_velocity = kinematics_dynamics->jacobian(&manipulator_, it->first)*joint_velocity;
    linear_velocity[0] = pose_velocity[0];
    linear_velocity[1] = pose_velocity[1];
    linear_velocity[2] = pose_velocity[2];
    angular_velocity[0] = pose_velocity[3];
    angular_velocity[1] = pose_velocity[4];
    angular_velocity[2] = pose_velocity[5];
    DynamicPose dynamic_pose;
    dynamic_pose.linear.velocity = linear_velocity;
    dynamic_pose.angular.velocity = angular_velocity;
    dynamic_pose.linear.acceleration = Eigen::Vector3d::Zero();
    dynamic_pose.angular.acceleration = Eigen::Vector3d::Zero();

    manipulator_.setComponentDynamicPoseFromWorld(it->first, dynamic_pose);
  }
}

void Trajectory::setPresentJointWayPoint(JointWayPoint joint_value_vector)
{
  manipulator_.setAllActiveJointValue(joint_value_vector);
}

void Trajectory::setPresentTaskWayPoint(Name tool_name, TaskWayPoint tool_value_vector)
{
  manipulator_.setComponentPoseFromWorld(tool_name, tool_value_vector.kinematic);
  manipulator_.setComponentDynamicPoseFromWorld(tool_name, tool_value_vector.dynamic);
}

JointWayPoint Trajectory::getPresentJointWayPoint()
{
  return manipulator_.getAllActiveJointValue();
}

TaskWayPoint Trajectory::getPresentTaskWayPoint(Name tool_name)
{
  TaskWayPoint result;
  result.kinematic = manipulator_.getComponentPoseFromWorld(tool_name);
  result.dynamic = manipulator_.getComponentDynamicPoseFromWorld(tool_name);
  return result;
}

//void Trajectory::setStartWayPoint(std::vector<WayPoint> start_way_point)
//{
//  start_way_point_ = start_way_point;
//}

//void Trajectory::setGoalWayPoint(std::vector<WayPoint> goal_way_point)
//{
//  goal_way_point_ = goal_way_point;
//}

//void Trajectory::clearStartWayPoint()
//{
//  start_way_point_.clear();
//}

//void Trajectory::clearGoalWayPoint()
//{
//  goal_way_point_.clear();
//}

//std::vector<WayPoint> Trajectory::getStartWayPoint()
//{
//  return start_way_point_;
//}

//std::vector<WayPoint> Trajectory::getGoalWayPoint()
//{
//  return goal_way_point_;
//}

JointWayPoint Trajectory::removeWayPointDynamicData(JointWayPoint value)
{
  for(uint32_t index =0; index < value.size(); index++)
  {
    value.at(index).velocity = 0.0;
    value.at(index).acceleration = 0.0;
    value.at(index).effort = 0.0;
  }
  return value;
}

TaskWayPoint Trajectory::removeWayPointDynamicData(TaskWayPoint value)
{
  value.dynamic.linear.velocity = Eigen::Vector3d::Zero(3);
  value.dynamic.linear.acceleration = Eigen::Vector3d::Zero(3);
  value.dynamic.angular.velocity = Eigen::Vector3d::Zero(3);
  value.dynamic.angular.acceleration = Eigen::Vector3d::Zero(3);
  return value;
}


//Trajectory
void Trajectory::setTrajectoryType(TrajectoryType trajectory_type)
{
  trajectory_type_ = trajectory_type;
}

bool Trajectory::checkTrajectoryType(TrajectoryType trajectory_type)
{
  if(trajectory_type_==trajectory_type)
    return true;
  else
    return false;
}

void Trajectory::makeJointTrajectory(JointWayPoint start_way_point, JointWayPoint goal_way_point)
{
  joint_.setJointNum(manipulator_.getDOF());
  joint_.init(trajectory_time_.total_move_time, trajectory_time_.control_loop_time, start_way_point, goal_way_point);
}

void Trajectory::makeTaskTrajectory(TaskWayPoint start_way_point, TaskWayPoint goal_way_point)
{
  task_.init(trajectory_time_.total_move_time, trajectory_time_.control_loop_time, start_way_point, goal_way_point);
}

void Trajectory::makeCustomTrajectory(Name trajectory_name, JointWayPoint start_way_point, const void *arg)
{
  if(cus_joint_.find(trajectory_name) != cus_joint_.end())
  {
    present_custom_trajectory_name_ = trajectory_name;
    cus_joint_.at(trajectory_name)->init(trajectory_time_.total_move_time, trajectory_time_.control_loop_time, start_way_point, arg);
  }
  else
    RM_LOG::ERROR("[makeCustomTrajectory] Wrong way point type.");
}

void Trajectory::makeCustomTrajectory(Name trajectory_name, TaskWayPoint start_way_point, const void *arg)
{
  if(cus_task_.find(trajectory_name) != cus_task_.end())
  {
    present_custom_trajectory_name_ = trajectory_name;
    cus_task_.at(trajectory_name)->init(trajectory_time_.total_move_time, trajectory_time_.control_loop_time, start_way_point, arg);
  }
  else
    RM_LOG::ERROR("[makeCustomTrajectory] Wrong way point type.");
}

//tool
void Trajectory::setToolGoalPosition(Name tool_name, double tool_goal_position)
{
  manipulator_.setJointPosition(tool_name, tool_goal_position);
}


void Trajectory::setToolGoalValue(Name tool_name, JointValue tool_goal_value)
{
  manipulator_.setJointValue(tool_name, tool_goal_value);
}

double Trajectory::getToolGoalPosition(Name tool_name)
{
  return manipulator_.getJointPosition(tool_name);
}

JointValue Trajectory::getToolGoalValue(Name tool_name)
{
  return manipulator_.getJointValue(tool_name);
}






























