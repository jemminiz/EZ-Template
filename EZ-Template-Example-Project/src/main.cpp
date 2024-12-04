#include "main.h"

/////
// For installation, upgrading, documentations, and tutorials, check out our website!
// https://ez-robotics.github.io/EZ-Template/
/////

// Chassis constructor
ez::Drive chassis(
    // These are your drive motors, the first motor is used for sensing!
    {1, 2, 3},     // Left Chassis Ports (negative port will reverse it!)
    {-4, -5, -6},  // Right Chassis Ports (negative port will reverse it!)

    7,      // IMU Port
    4.125,  // Wheel Diameter (Remember, 4" wheels without screw holes are actually 4.125!)
    343);   // Wheel RPM = cartridge * (motor gear / wheel gear)

// Are you using tracking wheels?  Comment out which ones you're using here!
//  `2.75` is the wheel diameter
//  `4.0` is the distance from the center of the wheel to the center of the robot
// ez::tracking_wheel right_tracker({-'A', -'B'}, 2.75, 4.0);  // ADI Encoders
// ez::tracking_wheel left_tracker(1, {'C', 'D'}, 2.75, 4.0);  // ADI Encoders plugged into a Smart port
// ez::tracking_wheel horiz_tracker(1, 2.75, 4.0);             // Rotation sensors

/**
 * Runs initialization code. This occurs as soon as the program is started.
 *
 * All other competition modes are blocked by initialize; it is recommended
 * to keep execution time for this mode under a few seconds.
 */
void initialize() {
  // Print our branding over your terminal :D
  ez::ez_template_print();

  pros::delay(500);  // Stop the user from doing anything while legacy ports configure

  // Are you using tracking wheels?  Comment out which ones you're using here!
  // chassis.odom_tracker_right_set(&right_tracker);
  // chassis.odom_tracker_left_set(&left_tracker);
  // chassis.odom_tracker_back_set(&horiz_tracker);  // Replace `back` to `front` if your tracker is in the front!

  // Configure your chassis controls
  chassis.opcontrol_curve_buttons_toggle(true);   // Enables modifying the controller curve with buttons on the joysticks
  chassis.opcontrol_drive_activebrake_set(0.0);   // Sets the active brake kP. We recommend ~2.  0 will disable.
  chassis.opcontrol_curve_default_set(0.0, 0.0);  // Defaults for curve. If using tank, only the first parameter is used. (Comment this line out if you have an SD card!)

  // Set the drive to your own constants from autons.cpp!
  default_constants();

  // These are already defaulted to these buttons, but you can change the left/right curve buttons here!
  // chassis.opcontrol_curve_buttons_left_set(pros::E_CONTROLLER_DIGITAL_LEFT, pros::E_CONTROLLER_DIGITAL_RIGHT);  // If using tank, only the left side is used.
  // chassis.opcontrol_curve_buttons_right_set(pros::E_CONTROLLER_DIGITAL_Y, pros::E_CONTROLLER_DIGITAL_A);

  // Autonomous Selector using LLEMU
  ez::as::auton_selector.autons_add({
      {"Example Drive\n\nDrive forward and come back.", drive_example},
      {"Example Turn\n\nTurn 3 times.", turn_example},
      {"Drive and Turn\n\nDrive forward, turn, come back. ", drive_and_turn},
      {"Drive and Turn\n\nSlow down during drive.", wait_until_change_speed},
      {"Swing Example\n\nSwing in an 'S' curve", swing_example},
      {"Motion Chaining\n\nDrive forward, turn, and come back, but blend everything together :D", motion_chaining},
      {"Combine all 3 movements", combining_movements},
      {"Interference\n\nAfter driving forward, robot performs differently if interfered or not.", interfered_example},
  });

  // Initialize chassis and auton selector
  chassis.initialize();
  ez::as::initialize();
  master.rumble(chassis.drive_imu_calibrated() ? "." : "---");
}

/**
 * Runs while the robot is in the disabled state of Field Management System or
 * the VEX Competition Switch, following either autonomous or opcontrol. When
 * the robot is enabled, this task will exit.
 */
void disabled() {
  // . . .
}

/**
 * Runs after initialize(), and before autonomous when connected to the Field
 * Management System or the VEX Competition Switch. This is intended for
 * competition-specific initialization routines, such as an autonomous selector
 * on the LCD.
 *
 * This task will exit when the robot is enabled and autonomous or opcontrol
 * starts.
 */
void competition_initialize() {
  // . . .
}

/**
 * Runs the user autonomous code. This function will be started in its own task
 * with the default priority and stack size whenever the robot is enabled via
 * the Field Management System or the VEX Competition Switch in the autonomous
 * mode. Alternatively, this function may be called in initialize or opcontrol
 * for non-competition testing purposes.
 *
 * If the robot is disabled or communications is lost, the autonomous task
 * will be stopped. Re-enabling the robot will restart the task, not re-start it
 * from where it left off.
 */
void autonomous() {
  chassis.pid_targets_reset();                 // Resets PID targets to 0
  chassis.drive_imu_reset();                   // Reset gyro position to 0
  chassis.drive_sensor_reset();                // Reset drive sensors to 0
  chassis.odom_pose_set({0_in, 0_in, 0_deg});  // Set the current position, you can start at a specific position with this
  chassis.drive_brake_set(MOTOR_BRAKE_HOLD);   // Set motors to hold.  This helps autonomous consistency

  /*
  Odometry and Pure Pursuit are not magic

  It is possible to get perfectly consistent results without tracking wheels,
  but it is also possible to have extremely inconsistent results without tracking wheels.
  When you don't use tracking wheels, you need to:
   - avoid wheel slip
   - avoid wheelies
   - avoid throwing momentum around (super harsh turns, like in the example below)
  You can do cool curved motions, but you have to give your robot the best chance
  to be consistent
  */

  // Drive to 12, 24 and pass through -4, 8 on the way
  chassis.pid_odom_set({{{-4_in, 8_in}, fwd, 110},
                        {{12_in, 24_in}, fwd, 110}},
                       true);
  chassis.pid_wait();

  // Drive to 0, 0 but make sure the final heading is 0 deg
  chassis.pid_odom_set({{0_in, 0_in, 0_deg}, rev, 110},
                       true);
  chassis.pid_wait();

  // Uncomment this to use the auton selector
  // ez::as::auton_selector.selected_auton_call();  // Calls selected auton from autonomous selector
}

/**
 * Simplifies printing tracker values to the brain screen
 */
void screen_print_trackers(std::vector<std::pair<ez::tracking_wheel*, std::string>> tracker, int line) {
  int amount_of_trackers = 0;
  std::string output = "";
  for (auto i : tracker) {
    if (i.first == nullptr) continue;  // Exit if there's no tracker

    // Create text for tracker and width values
    output += (i.second + " tracker: " + util::to_string_with_precision(i.first->get()) +
               "  width: " + util::to_string_with_precision(i.first->distance_to_center_get()) + "\n");
    amount_of_trackers++;
  }

  // Print tracker + width to the screen, but push the text to the bottom of the screen
  ez::screen_print(output, (tracker.size() - amount_of_trackers) + line);
}

/**
 * Gives you some extras to run in your opcontrol:
 * - run your autonomous routine in opcontrol by pressing DOWN and B
 *   - to prevent this from accidentally happening at a competition, this
 *     is only enabled when you're not connected to competition control.
 * - gives you a GUI to change your PID values live by pressing X
 */
void ez_template_extras() {
  if (!pros::competition::is_connected()) {
    // PID Tuner
    // - after you find values that you're happy with, you'll have to set them in auton.cpp

    // Enable / Disable PID Tuner
    //  When enabled:
    //  * use A and Y to increment / decrement the constants
    //  * use the arrow keys to navigate the constants
    if (master.get_digital_new_press(DIGITAL_X))
      chassis.pid_tuner_toggle();

    // Trigger the selected autonomous routine
    if (master.get_digital(DIGITAL_B) && master.get_digital(DIGITAL_DOWN)) {
      pros::motor_brake_mode_e_t preference = chassis.drive_brake_get();
      autonomous();
      chassis.drive_brake_set(preference);
    }

    // Blank pages for odom debugging
    if (chassis.odom_enabled() && !chassis.pid_tuner_enabled()) {
      // If we're on the first blank page...
      if (ez::as::page_blank_is_on(0)) {
        // Display X, Y, and Angle
        ez::screen_print("x: " + util::to_string_with_precision(chassis.odom_x_get()) +
                             "\ny: " + util::to_string_with_precision(chassis.odom_y_get()) +
                             "\na: " + util::to_string_with_precision(chassis.odom_theta_get()),
                         1);  // Don't override the top Page line

        // Display all trackers that are being used
        screen_print_trackers(
            {{chassis.odom_tracker_left, "l"},
             {chassis.odom_tracker_right, "r"},
             {chassis.odom_tracker_back, "b"},
             {chassis.odom_tracker_front, "f"}},
            4);  // Start printing on line 4
      }
    }

    // Allow PID Tuner to iterate
    chassis.pid_tuner_iterate();
  } else {
    // Remove all blank pages when connected to a comp switch
    if (ez::as::page_blank_amount() > 0)
      ez::as::page_blank_remove_all();

    // Disable PID Tuner when connected to a comp switch
    if (chassis.pid_tuner_enabled())
      chassis.pid_tuner_disable();
  }
}

/**
 * Runs the operator control code. This function will be started in its own task
 * with the default priority and stack size whenever the robot is enabled via
 * the Field Management System or the VEX Competition Switch in the operator
 * control mode.
 *
 * If no competition control is connected, this function will run immediately
 * following initialize().
 *
 * If the robot is disabled or communications is lost, the
 * operator control task will be stopped. Re-enabling the robot will restart the
 * task, not resume it from where it left off.
 */
void opcontrol() {
  // This is preference to what you like to drive on
  chassis.drive_brake_set(MOTOR_BRAKE_COAST);

  while (true) {
    // Gives you some extras to make EZ-Template ezier
    ez_template_extras();

    chassis.opcontrol_tank();  // Tank control
    // chassis.opcontrol_arcade_standard(ez::SPLIT);   // Standard split arcade
    // chassis.opcontrol_arcade_standard(ez::SINGLE);  // Standard single arcade
    // chassis.opcontrol_arcade_flipped(ez::SPLIT);    // Flipped split arcade
    // chassis.opcontrol_arcade_flipped(ez::SINGLE);   // Flipped single arcade

    // . . .
    // Put more user control code here!
    // . . .

    pros::delay(ez::util::DELAY_TIME);  // This is used for timer calculations!  Keep this ez::util::DELAY_TIME
  }
}
