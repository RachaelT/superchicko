#include <ros/ros.h>

// MoveIt!
#include <moveit/robot_model_loader/robot_model_loader.h>
#include <moveit/robot_model/robot_model.h>
#include <moveit/robot_state/robot_state.h>

int main(int argc, char **argv)
{
  ros::init (argc, argv, "base_bladder_kinematics");
  ros::AsyncSpinner spinner(1);
  spinner.start();

  // BEGIN_TUTORIAL
  // Start
  // ^^^^^
  // Setting up to start using the RobotModel class is very easy. In
  // general, you will find that most higher-level components will
  // return a shared pointer to the RobotModel. You should always use
  // that when possible. In this example, we will start with such a
  // shared pointer and discuss only the basic API. You can have a
  // look at the actual code API for these classes to get more
  // information about how to use more features provided by these
  // classes.
  //
  // We will start by instantiating a
  // `RobotModelLoader`_
  // object, which will look up
  // the robot description on the ROS parameter server and construct a
  // :moveit_core:`RobotModel` for us to use.
  //
  // .. _RobotModelLoader: http://docs.ros.org/indigo/api/moveit_ros_planning/html/classrobot__model__loader_1_1RobotModelLoader.html
  robot_model_loader::RobotModelLoader robot_model_loader("robot_description");
  robot_model::RobotModelPtr RobotModel = robot_model_loader.getModel();
  ROS_INFO("Model frame: %s", RobotModel->getModelFrame().c_str());

  // Using the :moveit_core:`RobotModel`, we can construct a
  // :moveit_core:`RobotState` that maintains the configuration
  // of the robot. We will set all joints in the state to their
  // default values. We can then get a
  // :moveit_core:`JointModelGroup`, which represents the robot
  // model for a particular group, e.g. the "right_arm" of the PR2
  // robot.
  robot_state::RobotStatePtr RobotState(new robot_state::RobotState(RobotModel));
  RobotState->setToDefaultValues();
  const robot_state::JointModelGroup* joint_model_group = RobotModel->getJointModelGroup("base_bladder");

  const std::vector<std::string> &joint_names = joint_model_group->getJointModelNames();

  // Get Joint Values
  // ^^^^^^^^^^^^^^^^
  // We can retreive the current set of joint values stored in the state for the base bladder.
  std::vector<double> joint_values;
  RobotState->copyJointGroupPositions(joint_model_group, joint_values);
  for(std::size_t i = 0; i < joint_names.size(); ++i)
  {
    ROS_INFO("Joint: %s: %f", joint_names[i].c_str(), joint_values[i]);
  }

  // Joint Limits
  // ^^^^^^^^^^^^
  // setJointGroupPositions() does not enforce joint limits by itself, but a call to enforceBounds() will do it.
  /* Set one joint in the right arm outside its joint limit */
  joint_values[0] = 1.57;
  RobotState->setJointGroupPositions(joint_model_group, joint_values);

  /* Check whether any joint is outside its joint limits */
  ROS_INFO_STREAM("Pre-Enforce Joint Limits: Current state is " << (RobotState->satisfiesBounds() ? "valid" : "not valid"));

  /* Enforce the joint limits for this state and check again*/
  RobotState->enforceBounds();
  ROS_INFO_STREAM("Post-Enforce Joint Limits: Current state is " << (RobotState->satisfiesBounds() ? "valid" : "not valid"));

  // Forward Kinematics
  // ^^^^^^^^^^^^^^^^^^
  // Now, we can compute forward kinematics for a set of random joint
  // values. Note that we would like to find the pose of the
  // "headnball link" which is the most distal link in the
  // "head" of the robot.
  RobotState->setToRandomPositions(joint_model_group);
  const Eigen::Affine3d &end_effector_state = RobotState->getGlobalLinkTransform("headnball_link");

  /* Print end-effector pose. Remember that this is in the model frame */
  ROS_INFO_STREAM("Translation: " << end_effector_state.translation());
  ROS_INFO_STREAM("Rotation: " << end_effector_state.rotation());

  // Inverse Kinematics
  // ^^^^^^^^^^^^^^^^^^
  // We can now solve inverse kinematics (IK) for the head of the
  // chick robot. To solve IK, we will need the following:
  // * The desired pose of the end-effector (by default, this is the last link in the "right_arm" chain): end_effector_state that we computed in the step above.
  // * The number of attempts to be made at solving IK: 5
  // * The timeout for each attempt: 0.1 s
  bool found_ik = RobotState->setFromIK(joint_model_group, end_effector_state, 10, 0.1);

  // Now, we can print out the IK solution (if found):
  if (found_ik)
  {
    RobotState->copyJointGroupPositions(joint_model_group, joint_values);
    for(std::size_t i=0; i < joint_names.size(); ++i)
    {
      ROS_INFO("Joint %s: %f", joint_names[i].c_str(), joint_values[i]);
    }
  }
  else
  {
    ROS_INFO("Did not find IK solution");
  }

  // Get the Jacobian
  // ^^^^^^^^^^^^^^^^
  // We can also get the Jacobian from the :moveit_core:`RobotState`.
  Eigen::Vector3d reference_point_position(0.0,0.0,0.0);
  Eigen::MatrixXd jacobian;
  RobotState->getJacobian(joint_model_group, RobotState->getLinkModel(joint_model_group->getLinkModelNames().back()),
                               reference_point_position,
                               jacobian);
  ROS_INFO_STREAM("Jacobian: " << jacobian);
  // END_TUTORIAL

  ros::shutdown();
  return 0;
}