// Copyright 2020 Open Source Robotics Foundation, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "controller_manager/controller_manager.hpp"
#include "controller_manager_msgs/srv/list_controllers.hpp"
#include "controller_manager_test_common.hpp"
#include "lifecycle_msgs/msg/state.hpp"
#include "rclcpp/parameter.hpp"
#include "test_chainable_controller/test_chainable_controller.hpp"
#include "test_controller/test_controller.hpp"

// The tests in this file are implementing example of chained-control for DiffDrive robot example:
// https://github.com/ros-controls/roadmap/blob/9f32e215a84347aee0b519cb24d081f23bbbb224/design_drafts/cascade_control.md#motivation-purpose-and-use
// The controller have the names as stated in figure, but they are simply forwarding values without
// functionality that their name would suggest

class TestControllerChainingWithControllerManager;
class TestControllerChainingWithControllerManagerParameter;

class TestableTestChainableController : public test_chainable_controller::TestChainableController
{
  friend TestControllerChainingWithControllerManager;
  friend TestControllerChainingWithControllerManagerParameter;

  FRIEND_TEST(TestControllerChainingWithControllerManager, test_chained_controllers);
  FRIEND_TEST(
    TestControllerChainingWithControllerManagerParameter,
    test_chained_controllers_auto_switch_to_chained_mode);
};

class TestableControllerManager : public controller_manager::ControllerManager
{
  friend TestControllerChainingWithControllerManager;
  friend TestControllerChainingWithControllerManagerParameter;

  FRIEND_TEST(
    TestControllerChainingWithControllerManagerAndChainedControllersParameter,
    test_cm_reading_chained_controllers_parameter);
  FRIEND_TEST(
    TestControllerChainingWithControllerManagerAndChainedControllersParameter,
    test_cm_reading_chained_controllers_parameter_failure_group0);
  FRIEND_TEST(
    TestControllerChainingWithControllerManagerAndChainedControllersParameter,
    test_cm_reading_chained_controllers_parameter_failure_wrong_type);
  FRIEND_TEST(
    TestControllerChainingWithControllerManagerAndChainedControllersParameter,
    test_cm_reading_chained_controllers_parameter_failure_duplicated_controller);

  FRIEND_TEST(TestControllerChainingWithControllerManager, test_chained_controllers);
  FRIEND_TEST(
    TestControllerChainingWithControllerManagerParameter,
    test_chained_controllers_auto_switch_to_chained_mode);

public:
  TestableControllerManager(
    std::unique_ptr<hardware_interface::ResourceManager> resource_manager,
    std::shared_ptr<rclcpp::Executor> executor,
    const std::string & manager_node_name = "controller_manager",
    const std::string & namespace_ = "")
  : controller_manager::ControllerManager(
      std::move(resource_manager), executor, manager_node_name, namespace_)
  {
  }
};

class TestControllerChainingWithControllerManagerAndChainedControllersParameter
: public ControllerManagerFixture<TestableControllerManager>
{
public:
  void SetUp()
  {
    executor_ = std::make_shared<rclcpp::executors::SingleThreadedExecutor>();
    cm_ = std::make_shared<TestableControllerManager>(
      std::make_unique<hardware_interface::ResourceManager>(), executor_, TEST_CM_NAME);
    run_updater_ = false;
  }
};

TEST_F(
  TestControllerChainingWithControllerManagerAndChainedControllersParameter,
  test_cm_reading_chained_controllers_parameter)
{
  std::vector<std::string> parallel_group_1 = {"position_tracking_controller"};
  std::vector<std::string> parallel_group_2 = {"diff_drive_controller"};
  std::vector<std::string> parallel_group_3 = {
    "pid_left_wheel_controller", "pid_right_wheel_controller"};

  cm_->set_parameter(rclcpp::Parameter("chained_controllers.parallel_group_1", parallel_group_1));
  cm_->set_parameter(rclcpp::Parameter("chained_controllers.parallel_group_2", parallel_group_2));
  cm_->set_parameter(rclcpp::Parameter("chained_controllers.parallel_group_3", parallel_group_3));

  ASSERT_TRUE(load_chained_controller_configuration(
    cm_->get_node_parameters_interface(), cm_->get_node_logging_interface(),
    cm_->chained_controllers_configuration_));

  ASSERT_EQ(cm_->chained_controllers_configuration_.size(), 3u);
  EXPECT_EQ(cm_->chained_controllers_configuration_[0].size(), 1u);
  EXPECT_EQ(cm_->chained_controllers_configuration_[1].size(), 1u);
  EXPECT_EQ(cm_->chained_controllers_configuration_[2].size(), 2u);

  ASSERT_THAT(
    cm_->chained_controllers_configuration_[0], testing::ElementsAreArray(parallel_group_1));
  ASSERT_THAT(
    cm_->chained_controllers_configuration_[1], testing::ElementsAreArray(parallel_group_2));
  ASSERT_THAT(
    cm_->chained_controllers_configuration_[2], testing::ElementsAreArray(parallel_group_3));
}

TEST_F(
  TestControllerChainingWithControllerManagerAndChainedControllersParameter,
  test_cm_reading_chained_controllers_parameter_failure_group0)
{
  std::vector<std::string> parallel_group_0 = {"great_controller"};
  std::vector<std::string> parallel_group_1 = {"position_tracking_controller"};
  std::string parallel_group_2 = "diff_drive_controller";
  std::vector<std::string> parallel_group_3 = {
    "pid_left_wheel_controller", "pid_right_wheel_controller"};

  cm_->set_parameter(rclcpp::Parameter("chained_controllers.parallel_group_0", parallel_group_0));
  cm_->set_parameter(rclcpp::Parameter("chained_controllers.parallel_group_1", parallel_group_1));
  cm_->set_parameter(rclcpp::Parameter("chained_controllers.parallel_group_2", parallel_group_2));
  cm_->set_parameter(rclcpp::Parameter("chained_controllers.parallel_group_3", parallel_group_3));

  ASSERT_FALSE(load_chained_controller_configuration(
    cm_->get_node_parameters_interface(), cm_->get_node_logging_interface(),
    cm_->chained_controllers_configuration_));
}

TEST_F(
  TestControllerChainingWithControllerManagerAndChainedControllersParameter,
  test_cm_reading_chained_controllers_parameter_failure_wrong_type)
{
  std::vector<std::string> parallel_group_1 = {"position_tracking_controller"};
  std::string parallel_group_2 = "diff_drive_controller";
  std::vector<std::string> parallel_group_3 = {
    "pid_left_wheel_controller", "pid_right_wheel_controller"};

  cm_->set_parameter(rclcpp::Parameter("chained_controllers.parallel_group_1", parallel_group_1));
  cm_->set_parameter(rclcpp::Parameter("chained_controllers.parallel_group_2", parallel_group_2));
  cm_->set_parameter(rclcpp::Parameter("chained_controllers.parallel_group_3", parallel_group_3));

  ASSERT_FALSE(load_chained_controller_configuration(
    cm_->get_node_parameters_interface(), cm_->get_node_logging_interface(),
    cm_->chained_controllers_configuration_));
}

TEST_F(
  TestControllerChainingWithControllerManagerAndChainedControllersParameter,
  test_cm_reading_chained_controllers_parameter_failure_duplicated_controller)
{
  std::vector<std::string> parallel_group_1 = {"position_tracking_controller"};
  std::string parallel_group_2 = "diff_drive_controller";
  std::vector<std::string> parallel_group_3 = {
    "pid_left_wheel_controller", "pid_right_wheel_controller", "position_tracking_controller"};

  cm_->set_parameter(rclcpp::Parameter("chained_controllers.parallel_group_1", parallel_group_1));
  cm_->set_parameter(rclcpp::Parameter("chained_controllers.parallel_group_2", parallel_group_2));
  cm_->set_parameter(rclcpp::Parameter("chained_controllers.parallel_group_3", parallel_group_3));

  ASSERT_FALSE(load_chained_controller_configuration(
    cm_->get_node_parameters_interface(), cm_->get_node_logging_interface(),
    cm_->chained_controllers_configuration_));
}

class TestControllerChainingWithControllerManager
: public ControllerManagerFixture<TestableControllerManager>,
  public testing::WithParamInterface<Strictness>
{
public:
  void SetUp()
  {
    executor_ = std::make_shared<rclcpp::executors::SingleThreadedExecutor>();
    cm_ = std::make_shared<TestableControllerManager>(
      std::make_unique<hardware_interface::ResourceManager>(
        ros2_control_test_assets::diffbot_urdf, true, true),
      executor_, TEST_CM_NAME);
    run_updater_ = false;
  }

  void SetupControllers()
  {
    test_param = GetParam();

    pid_left_wheel_controller = std::make_shared<TestableTestChainableController>();
    pid_right_wheel_controller = std::make_shared<TestableTestChainableController>();
    diff_drive_controller = std::make_shared<TestableTestChainableController>();
    position_tracking_controller = std::make_shared<test_controller::TestController>();

    // configure Left Wheel controller
    controller_interface::InterfaceConfiguration pid_left_cmd_ifs_cfg = {
      controller_interface::interface_configuration_type::INDIVIDUAL, {"wheel_left/velocity"}};
    controller_interface::InterfaceConfiguration pid_left_state_ifs_cfg = {
      controller_interface::interface_configuration_type::INDIVIDUAL, {"wheel_left/velocity"}};
    pid_left_wheel_controller->set_command_interface_configuration(pid_left_cmd_ifs_cfg);
    pid_left_wheel_controller->set_state_interface_configuration(pid_left_state_ifs_cfg);
    pid_left_wheel_controller->set_reference_interface_names({"velocity"});

    // configure Left Wheel controller
    controller_interface::InterfaceConfiguration pid_right_cmd_ifs_cfg = {
      controller_interface::interface_configuration_type::INDIVIDUAL, {"wheel_right/velocity"}};
    controller_interface::InterfaceConfiguration pid_right_state_ifs_cfg = {
      controller_interface::interface_configuration_type::INDIVIDUAL, {"wheel_right/velocity"}};
    pid_right_wheel_controller->set_command_interface_configuration(pid_right_cmd_ifs_cfg);
    pid_right_wheel_controller->set_state_interface_configuration(pid_right_state_ifs_cfg);
    pid_right_wheel_controller->set_reference_interface_names({"velocity"});

    // configure Diff Drive controller
    controller_interface::InterfaceConfiguration diff_drive_cmd_ifs_cfg = {
      controller_interface::interface_configuration_type::INDIVIDUAL,
      {std::string(PID_LEFT_WHEEL) + "/velocity", std::string(PID_RIGHT_WHEEL) + "/velocity"}};
    controller_interface::InterfaceConfiguration diff_drive_state_ifs_cfg = {
      controller_interface::interface_configuration_type::INDIVIDUAL,
      {"wheel_left/velocity", "wheel_right/velocity"}};
    diff_drive_controller->set_command_interface_configuration(diff_drive_cmd_ifs_cfg);
    diff_drive_controller->set_state_interface_configuration(diff_drive_state_ifs_cfg);
    diff_drive_controller->set_reference_interface_names({"vel_x", "vel_y", "rot_z"});

    // configure Position Tracking controller
    controller_interface::InterfaceConfiguration position_tracking_cmd_ifs_cfg = {
      controller_interface::interface_configuration_type::INDIVIDUAL,
      {std::string(DIFF_DRIVE_CONTROLLER) + "/vel_x",
       std::string(DIFF_DRIVE_CONTROLLER) + "/vel_y"}};
    // in this simple example "vel_x" == "velocity left wheel" and "vel_y" == "velocity right wheel"
    position_tracking_controller->set_command_interface_configuration(
      position_tracking_cmd_ifs_cfg);
  }

  void CheckIfControllersAreAddedCorrectly()
  {
    EXPECT_EQ(4u, cm_->get_loaded_controllers().size());
    EXPECT_EQ(2, pid_left_wheel_controller.use_count());
    EXPECT_EQ(2, pid_right_wheel_controller.use_count());
    EXPECT_EQ(2, diff_drive_controller.use_count());
    EXPECT_EQ(2, position_tracking_controller.use_count());

    EXPECT_EQ(
      lifecycle_msgs::msg::State::PRIMARY_STATE_UNCONFIGURED,
      pid_left_wheel_controller->get_state().id());
    EXPECT_EQ(
      lifecycle_msgs::msg::State::PRIMARY_STATE_UNCONFIGURED,
      pid_right_wheel_controller->get_state().id());
    EXPECT_EQ(
      lifecycle_msgs::msg::State::PRIMARY_STATE_UNCONFIGURED,
      diff_drive_controller->get_state().id());
    EXPECT_EQ(
      lifecycle_msgs::msg::State::PRIMARY_STATE_UNCONFIGURED,
      position_tracking_controller->get_state().id());
  }

  // order or controller configuration is not important therefore we can reuse the same method
  void ConfigureAndCheckControllers()
  {
    // Store initial values of command interfaces
    size_t number_of_cmd_itfs = cm_->resource_manager_->command_interface_keys().size();

    // configure chainable controller and check exported interfaces
    cm_->configure_controller(PID_LEFT_WHEEL);
    EXPECT_EQ(
      pid_left_wheel_controller->get_state().id(),
      lifecycle_msgs::msg::State::PRIMARY_STATE_INACTIVE);
    EXPECT_EQ(cm_->resource_manager_->command_interface_keys().size(), number_of_cmd_itfs + 1);
    for (const auto & interface : {"pid_left_wheel_controller/velocity"})
    {
      EXPECT_TRUE(cm_->resource_manager_->command_interface_exists(interface));
      EXPECT_FALSE(cm_->resource_manager_->command_interface_is_available(interface));
      EXPECT_FALSE(cm_->resource_manager_->command_interface_is_claimed(interface));
    }

    cm_->configure_controller(PID_RIGHT_WHEEL);
    EXPECT_EQ(
      pid_right_wheel_controller->get_state().id(),
      lifecycle_msgs::msg::State::PRIMARY_STATE_INACTIVE);
    EXPECT_EQ(cm_->resource_manager_->command_interface_keys().size(), number_of_cmd_itfs + 2);
    for (const auto & interface : {"pid_right_wheel_controller/velocity"})
    {
      EXPECT_TRUE(cm_->resource_manager_->command_interface_exists(interface));
      EXPECT_FALSE(cm_->resource_manager_->command_interface_is_available(interface));
      EXPECT_FALSE(cm_->resource_manager_->command_interface_is_claimed(interface));
    }

    cm_->configure_controller(DIFF_DRIVE_CONTROLLER);
    EXPECT_EQ(
      diff_drive_controller->get_state().id(), lifecycle_msgs::msg::State::PRIMARY_STATE_INACTIVE);
    EXPECT_EQ(cm_->resource_manager_->command_interface_keys().size(), number_of_cmd_itfs + 5);
    for (const auto & interface :
         {"diff_drive_controller/vel_x", "diff_drive_controller/vel_y",
          "diff_drive_controller/rot_z"})
    {
      EXPECT_TRUE(cm_->resource_manager_->command_interface_exists(interface));
      EXPECT_FALSE(cm_->resource_manager_->command_interface_is_available(interface));
      EXPECT_FALSE(cm_->resource_manager_->command_interface_is_claimed(interface));
    }

    cm_->configure_controller(POSITION_TRACKING_CONTROLLER);
    EXPECT_EQ(
      position_tracking_controller->get_state().id(),
      lifecycle_msgs::msg::State::PRIMARY_STATE_INACTIVE);
    EXPECT_EQ(cm_->resource_manager_->command_interface_keys().size(), number_of_cmd_itfs + 5);
  }

  template <
    typename T, typename std::enable_if<
                  std::is_convertible<T *, controller_interface::ControllerInterface *>::value,
                  T>::type * = nullptr>
  void SetToChainedModeAndCheckInterfaces(
    std::shared_ptr<T> & controller, const std::string & controller_name,
    const std::vector<std::string> & reference_interfaces)
  {
    controller->set_chained_mode(true);
    EXPECT_TRUE(controller->is_in_chained_mode());

    // make reference interface command_interfaces available
    cm_->resource_manager_->make_controller_reference_interfaces_available(controller_name);
    for (const auto & interface : reference_interfaces)
    {
      EXPECT_TRUE(cm_->resource_manager_->command_interface_exists(interface));
      EXPECT_TRUE(cm_->resource_manager_->command_interface_is_available(interface));
      EXPECT_FALSE(cm_->resource_manager_->command_interface_is_claimed(interface));
    }
  }

  template <
    typename T, typename std::enable_if<
                  std::is_convertible<T *, controller_interface::ControllerInterface *>::value,
                  T>::type * = nullptr>
  void ActivateAndCheckController(
    std::shared_ptr<T> & controller, const std::string & controller_name,
    const std::vector<std::string> & claimed_command_itfs, size_t expected_internal_counter)
  {
    switch_test_controllers({controller_name}, {}, test_param.strictness);
    for (const auto & interface : claimed_command_itfs)
    {
      EXPECT_TRUE(cm_->resource_manager_->command_interface_exists(interface));
      EXPECT_TRUE(cm_->resource_manager_->command_interface_is_available(interface));
      EXPECT_TRUE(cm_->resource_manager_->command_interface_is_claimed(interface));
    }
    ASSERT_EQ(controller->internal_counter, expected_internal_counter);
  }

  void UpdateAllControllerAndCheck(
    const std::vector<double> & reference, size_t exp_internal_counter_pos_ctrl)
  {
    // test value that could cause bad-memory access --> cleaner error during writing tests
    ASSERT_EQ(reference.size(), 2u);

    position_tracking_controller->external_commands_for_testing_[0] = reference[0];
    position_tracking_controller->external_commands_for_testing_[1] = reference[1];

    cm_->update(rclcpp::Time(0), rclcpp::Duration::from_seconds(0.01));
    cm_->resource_manager_->read();

    // check if all controllers are updated
    ASSERT_EQ(position_tracking_controller->internal_counter, exp_internal_counter_pos_ctrl);
    ASSERT_EQ(diff_drive_controller->internal_counter, exp_internal_counter_pos_ctrl + 2);
    ASSERT_EQ(pid_left_wheel_controller->internal_counter, exp_internal_counter_pos_ctrl + 6);
    ASSERT_EQ(pid_right_wheel_controller->internal_counter, exp_internal_counter_pos_ctrl + 4);

    // check if values are set properly in controllers
    ASSERT_EQ(
      diff_drive_controller->reference_interfaces_[0], reference[0]);  // command from Position to
    ASSERT_EQ(
      diff_drive_controller->reference_interfaces_[1], reference[1]);  // DiffDrive is forwarded

    // Command of DiffDrive controller are references of PID controllers
    EXP_LEFT_WHEEL_REF = chained_ctrl_calculation(reference[0], EXP_LEFT_WHEEL_HW_STATE);
    EXP_RIGHT_WHEEL_REF = chained_ctrl_calculation(reference[1], EXP_RIGHT_WHEEL_HW_STATE);
    ASSERT_EQ(diff_drive_controller->command_interfaces_[0].get_value(), EXP_LEFT_WHEEL_REF);
    ASSERT_EQ(diff_drive_controller->command_interfaces_[1].get_value(), EXP_RIGHT_WHEEL_REF);
    ASSERT_EQ(pid_left_wheel_controller->reference_interfaces_[0], EXP_LEFT_WHEEL_REF);
    ASSERT_EQ(pid_right_wheel_controller->reference_interfaces_[0], EXP_RIGHT_WHEEL_REF);

    EXP_LEFT_WHEEL_CMD = chained_ctrl_calculation(EXP_LEFT_WHEEL_REF, EXP_LEFT_WHEEL_HW_STATE);
    EXP_LEFT_WHEEL_HW_STATE = hardware_calculation(EXP_LEFT_WHEEL_CMD);
    ASSERT_EQ(pid_left_wheel_controller->command_interfaces_[0].get_value(), EXP_LEFT_WHEEL_CMD);
    ASSERT_EQ(pid_left_wheel_controller->state_interfaces_[0].get_value(), EXP_LEFT_WHEEL_HW_STATE);
    // DiffDrive uses the same state
    ASSERT_EQ(diff_drive_controller->state_interfaces_[0].get_value(), EXP_LEFT_WHEEL_HW_STATE);

    EXP_RIGHT_WHEEL_CMD = chained_ctrl_calculation(EXP_RIGHT_WHEEL_REF, EXP_RIGHT_WHEEL_HW_STATE);
    EXP_RIGHT_WHEEL_HW_STATE = hardware_calculation(EXP_RIGHT_WHEEL_CMD);
    ASSERT_EQ(pid_right_wheel_controller->command_interfaces_[0].get_value(), EXP_RIGHT_WHEEL_CMD);
    ASSERT_EQ(
      pid_right_wheel_controller->state_interfaces_[0].get_value(), EXP_RIGHT_WHEEL_HW_STATE);
    // DiffDrive uses the same state
    ASSERT_EQ(diff_drive_controller->state_interfaces_[1].get_value(), EXP_RIGHT_WHEEL_HW_STATE);
  }

  // check data propagation through controllers and if individual controllers are working
  double chained_ctrl_calculation(double reference, double state) { return reference - state; }
  double hardware_calculation(double command) { return command / 2.0; }

  // set controllers' names
  static constexpr char PID_LEFT_WHEEL[] = "pid_left_wheel_controller";
  static constexpr char PID_RIGHT_WHEEL[] = "pid_right_wheel_controller";
  static constexpr char DIFF_DRIVE_CONTROLLER[] = "diff_drive_controller";
  static constexpr char POSITION_TRACKING_CONTROLLER[] = "position_tracking_controller";

  const std::vector<std::string> PID_LEFT_WHEEL_REFERENCE_INTERFACES = {
    "pid_left_wheel_controller/velocity"};
  const std::vector<std::string> PID_RIGHT_WHEEL_REFERENCE_INTERFACES = {
    "pid_right_wheel_controller/velocity"};
  const std::vector<std::string> DIFF_DRIVE_REFERENCE_INTERFACES = {
    "diff_drive_controller/vel_x", "diff_drive_controller/vel_y", "diff_drive_controller/rot_z"};

  const std::vector<std::string> PID_LEFT_WHEEL_CLAIMED_INTERFACES = {"wheel_left/velocity"};
  const std::vector<std::string> PID_RIGHT_WHEEL_CLAIMED_INTERFACES = {"wheel_right/velocity"};
  const std::vector<std::string> DIFF_DRIVE_CLAIMED_INTERFACES = {
    "pid_left_wheel_controller/velocity", "pid_right_wheel_controller/velocity"};
  const std::vector<std::string> POSITION_CONTROLLER_CLAIMED_INTERFACES = {
    "diff_drive_controller/vel_x", "diff_drive_controller/vel_y"};

  // controllers objects
  std::shared_ptr<TestableTestChainableController> pid_left_wheel_controller;
  std::shared_ptr<TestableTestChainableController> pid_right_wheel_controller;
  std::shared_ptr<TestableTestChainableController> diff_drive_controller;
  std::shared_ptr<test_controller::TestController> position_tracking_controller;

  testing::WithParamInterface<Strictness>::ParamType test_param;

  // expected values for tests - shared between multiple test runs
  double EXP_LEFT_WHEEL_CMD = 0.0;
  double EXP_LEFT_WHEEL_HW_STATE = 0.0;
  double EXP_RIGHT_WHEEL_CMD = 0.0;
  double EXP_RIGHT_WHEEL_HW_STATE = 0.0;
  double EXP_LEFT_WHEEL_REF = 0.0;
  double EXP_RIGHT_WHEEL_REF = 0.0;
};

// The tests are implementing example of chained-control for DiffDrive robot shown here:
// https://github.com/ros-controls/roadmap/blob/9f32e215a84347aee0b519cb24d081f23bbbb224/design_drafts/cascade_control.md#motivation-purpose-and-use
// The controller have the names as stated in figure, but they are simply forwarding values without
// functionality that their name would suggest
TEST_P(TestControllerChainingWithControllerManager, test_chained_controllers)
{
  SetupControllers();

  // add all controllers - CONTROLLERS HAVE TO ADDED IN EXECUTION ORDER
  cm_->add_controller(
    position_tracking_controller, POSITION_TRACKING_CONTROLLER,
    test_chainable_controller::TEST_CONTROLLER_CLASS_NAME);
  cm_->add_controller(
    diff_drive_controller, DIFF_DRIVE_CONTROLLER,
    test_chainable_controller::TEST_CONTROLLER_CLASS_NAME);
  cm_->add_controller(
    pid_left_wheel_controller, PID_LEFT_WHEEL,
    test_chainable_controller::TEST_CONTROLLER_CLASS_NAME);
  cm_->add_controller(
    pid_right_wheel_controller, PID_RIGHT_WHEEL,
    test_chainable_controller::TEST_CONTROLLER_CLASS_NAME);

  CheckIfControllersAreAddedCorrectly();

  ConfigureAndCheckControllers();

  SetToChainedModeAndCheckInterfaces(
    pid_left_wheel_controller, PID_LEFT_WHEEL, PID_LEFT_WHEEL_REFERENCE_INTERFACES);
  SetToChainedModeAndCheckInterfaces(
    pid_right_wheel_controller, PID_RIGHT_WHEEL, PID_RIGHT_WHEEL_REFERENCE_INTERFACES);
  SetToChainedModeAndCheckInterfaces(
    diff_drive_controller, DIFF_DRIVE_CONTROLLER, DIFF_DRIVE_REFERENCE_INTERFACES);

  EXPECT_THROW(
    cm_->resource_manager_->make_controller_reference_interfaces_available(
      POSITION_TRACKING_CONTROLLER),
    std::out_of_range);

  // Set ControllerManager into Debug-Mode output to have detailed output on updating controllers
  cm_->get_logger().set_level(rclcpp::Logger::Level::Debug);

  // activate controllers - CONTROLLERS HAVE TO ADDED REVERSE EXECUTION ORDER
  // (otherwise, interface will be missing)
  ActivateAndCheckController(
    pid_left_wheel_controller, PID_LEFT_WHEEL, PID_LEFT_WHEEL_CLAIMED_INTERFACES, 1u);
  ActivateAndCheckController(
    pid_right_wheel_controller, PID_RIGHT_WHEEL, PID_RIGHT_WHEEL_CLAIMED_INTERFACES, 1u);
  ASSERT_EQ(pid_left_wheel_controller->internal_counter, 3u);

  // Diff-Drive Controller claims the reference interfaces of PID controllers
  ActivateAndCheckController(
    diff_drive_controller, DIFF_DRIVE_CONTROLLER, DIFF_DRIVE_CLAIMED_INTERFACES, 1u);
  ASSERT_EQ(pid_right_wheel_controller->internal_counter, 3u);
  ASSERT_EQ(pid_left_wheel_controller->internal_counter, 5u);

  // Position-Tracking Controller uses reference interfaces of Diff-Drive Controller
  ActivateAndCheckController(
    position_tracking_controller, POSITION_TRACKING_CONTROLLER,
    POSITION_CONTROLLER_CLAIMED_INTERFACES, 1u);
  // 'rot_z' reference interface is not claimed
  for (const auto & interface : {"diff_drive_controller/rot_z"})
  {
    EXPECT_TRUE(cm_->resource_manager_->command_interface_exists(interface));
    EXPECT_TRUE(cm_->resource_manager_->command_interface_is_available(interface));
    EXPECT_FALSE(cm_->resource_manager_->command_interface_is_claimed(interface));
  }
  ASSERT_EQ(diff_drive_controller->internal_counter, 3u);
  ASSERT_EQ(pid_right_wheel_controller->internal_counter, 5u);
  ASSERT_EQ(pid_left_wheel_controller->internal_counter, 7u);

  // update controllers
  std::vector<double> reference = {32.0, 128.0};

  // update 'Position Tracking' controller
  for (auto & value : diff_drive_controller->reference_interfaces_)
  {
    ASSERT_EQ(value, 0.0);  // default reference values are 0.0
  }
  position_tracking_controller->external_commands_for_testing_[0] = reference[0];
  position_tracking_controller->external_commands_for_testing_[1] = reference[1];
  position_tracking_controller->update(rclcpp::Time(0), rclcpp::Duration::from_seconds(0.01));
  ASSERT_EQ(position_tracking_controller->internal_counter, 2u);

  ASSERT_EQ(diff_drive_controller->reference_interfaces_[0], reference[0]);  // position_controller
  ASSERT_EQ(diff_drive_controller->reference_interfaces_[1], reference[1]);  // is pass-through

  // update 'Diff Drive' Controller
  diff_drive_controller->update(rclcpp::Time(0), rclcpp::Duration::from_seconds(0.01));
  ASSERT_EQ(diff_drive_controller->internal_counter, 4u);
  // default reference values are 0.0 - they should be changed now
  EXP_LEFT_WHEEL_REF = chained_ctrl_calculation(reference[0], EXP_LEFT_WHEEL_HW_STATE);    // 32-0
  EXP_RIGHT_WHEEL_REF = chained_ctrl_calculation(reference[1], EXP_RIGHT_WHEEL_HW_STATE);  // 128-0
  ASSERT_EQ(pid_left_wheel_controller->reference_interfaces_[0], EXP_LEFT_WHEEL_REF);
  ASSERT_EQ(pid_right_wheel_controller->reference_interfaces_[0], EXP_RIGHT_WHEEL_REF);

  // update PID controllers that are writing to hardware
  pid_left_wheel_controller->update(rclcpp::Time(0), rclcpp::Duration::from_seconds(0.01));
  ASSERT_EQ(pid_left_wheel_controller->internal_counter, 8u);
  pid_right_wheel_controller->update(rclcpp::Time(0), rclcpp::Duration::from_seconds(0.01));
  ASSERT_EQ(pid_right_wheel_controller->internal_counter, 6u);

  // update hardware ('read' is  sufficient for test hardware)
  cm_->resource_manager_->read();
  // 32 - 0
  EXP_LEFT_WHEEL_CMD = chained_ctrl_calculation(EXP_LEFT_WHEEL_REF, EXP_LEFT_WHEEL_HW_STATE);
  // 32 / 2
  EXP_LEFT_WHEEL_HW_STATE = hardware_calculation(EXP_LEFT_WHEEL_CMD);
  ASSERT_EQ(pid_left_wheel_controller->command_interfaces_[0].get_value(), EXP_LEFT_WHEEL_CMD);
  ASSERT_EQ(pid_left_wheel_controller->state_interfaces_[0].get_value(), EXP_LEFT_WHEEL_HW_STATE);
  // DiffDrive uses the same state
  ASSERT_EQ(diff_drive_controller->state_interfaces_[0].get_value(), EXP_LEFT_WHEEL_HW_STATE);

  // 128 - 0
  EXP_RIGHT_WHEEL_CMD = chained_ctrl_calculation(EXP_RIGHT_WHEEL_REF, EXP_RIGHT_WHEEL_HW_STATE);
  // 128 / 2
  EXP_RIGHT_WHEEL_HW_STATE = hardware_calculation(EXP_RIGHT_WHEEL_CMD);
  ASSERT_EQ(pid_right_wheel_controller->command_interfaces_[0].get_value(), EXP_RIGHT_WHEEL_CMD);
  ASSERT_EQ(pid_right_wheel_controller->state_interfaces_[0].get_value(), EXP_RIGHT_WHEEL_HW_STATE);
  // DiffDrive uses the same state
  ASSERT_EQ(diff_drive_controller->state_interfaces_[1].get_value(), EXP_RIGHT_WHEEL_HW_STATE);

  // update all controllers at once and see that all have expected values --> also checks the order
  // of controller execution

  reference = {1024.0, 4096.0};
  UpdateAllControllerAndCheck(reference, 3u);

  // TODO(destogl): Add here also slow disabling of controllers

  // TODO(destogl): Activate test parameter use
}

INSTANTIATE_TEST_SUITE_P(
  test_strict_best_effort, TestControllerChainingWithControllerManager,
  testing::Values(strict, best_effort));

class TestControllerChainingWithControllerManagerParameter
: public TestControllerChainingWithControllerManager
{
public:
  void SetUp()
  {
    executor_ = std::make_shared<rclcpp::executors::SingleThreadedExecutor>();
    cm_ = std::make_shared<TestableControllerManager>(
      std::make_unique<hardware_interface::ResourceManager>(), executor_, TEST_CM_NAME);
    run_updater_ = false;

    // set chained_controller parameter
    std::vector<std::string> parallel_group_1 = {"position_tracking_controller"};
    std::vector<std::string> parallel_group_2 = {"diff_drive_controller"};
    std::vector<std::string> parallel_group_3 = {
      "pid_left_wheel_controller", "pid_right_wheel_controller"};

    cm_->set_parameter(rclcpp::Parameter("chained_controllers.parallel_group_1", parallel_group_1));
    cm_->set_parameter(rclcpp::Parameter("chained_controllers.parallel_group_2", parallel_group_2));
    cm_->set_parameter(rclcpp::Parameter("chained_controllers.parallel_group_3", parallel_group_3));

    // set robot_description parameter
    cm_->set_parameter(
      rclcpp::Parameter("robot_description", ros2_control_test_assets::diffbot_urdf));

    std::string robot_description = "";
    cm_->get_parameter("robot_description", robot_description);
    if (robot_description.empty())
    {
      throw std::runtime_error(
        "Unable to initialize resource manager, no robot description found.");
    }

    cm_->init_resource_manager(robot_description);
  }
};

TEST_P(
  TestControllerChainingWithControllerManagerParameter,
  test_chained_controllers_auto_switch_to_chained_mode)
{
  ASSERT_TRUE(load_chained_controller_configuration(
    cm_->get_node_parameters_interface(), cm_->get_node_logging_interface(),
    cm_->chained_controllers_configuration_));

  SetupControllers();

  // add all controllers - CONTROLLERS HAVE TO ADDED IN EXECUTION ORDER
  cm_->add_controller(
    position_tracking_controller, POSITION_TRACKING_CONTROLLER,
    test_chainable_controller::TEST_CONTROLLER_CLASS_NAME);
  cm_->add_controller(
    diff_drive_controller, DIFF_DRIVE_CONTROLLER,
    test_chainable_controller::TEST_CONTROLLER_CLASS_NAME);
  cm_->add_controller(
    pid_left_wheel_controller, PID_LEFT_WHEEL,
    test_chainable_controller::TEST_CONTROLLER_CLASS_NAME);
  cm_->add_controller(
    pid_right_wheel_controller, PID_RIGHT_WHEEL,
    test_chainable_controller::TEST_CONTROLLER_CLASS_NAME);

  CheckIfControllersAreAddedCorrectly();

  ConfigureAndCheckControllers();

  SetToChainedModeAndCheckInterfaces(
    pid_left_wheel_controller, PID_LEFT_WHEEL, PID_LEFT_WHEEL_REFERENCE_INTERFACES);
  SetToChainedModeAndCheckInterfaces(
    pid_right_wheel_controller, PID_RIGHT_WHEEL, PID_RIGHT_WHEEL_REFERENCE_INTERFACES);
  SetToChainedModeAndCheckInterfaces(
    diff_drive_controller, DIFF_DRIVE_CONTROLLER, DIFF_DRIVE_REFERENCE_INTERFACES);

  // Set ControllerManager into Debug-Mode output to have detailed output on updating controllers
  cm_->get_logger().set_level(rclcpp::Logger::Level::Debug);

  ActivateAndCheckController(
    pid_left_wheel_controller, PID_LEFT_WHEEL, PID_LEFT_WHEEL_CLAIMED_INTERFACES, 1u);
  ActivateAndCheckController(
    pid_right_wheel_controller, PID_RIGHT_WHEEL, PID_RIGHT_WHEEL_CLAIMED_INTERFACES, 1u);
  ActivateAndCheckController(
    diff_drive_controller, DIFF_DRIVE_CONTROLLER, DIFF_DRIVE_CLAIMED_INTERFACES, 1u);
  ActivateAndCheckController(
    position_tracking_controller, POSITION_TRACKING_CONTROLLER,
    POSITION_CONTROLLER_CLAIMED_INTERFACES, 1u);

  UpdateAllControllerAndCheck({32.0, 128.0}, 2u);
  UpdateAllControllerAndCheck({1024.0, 4096.0}, 3u);
}

// TODO(destogl): Add test case with controllers added in "random" order

INSTANTIATE_TEST_SUITE_P(
  test_strict_best_effort, TestControllerChainingWithControllerManagerParameter,
  testing::Values(strict, best_effort));