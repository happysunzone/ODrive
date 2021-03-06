#ifndef __CONTROLLER_HPP
#define __CONTROLLER_HPP

#ifndef __ODRIVE_MAIN_H
#error "This file should not be included directly. Include odrive_main.h instead."
#endif

// Note: these should be sorted from lowest level of control to
// highest level of control, to allow "<" style comparisons.
typedef enum {
    CTRL_MODE_VOLTAGE_CONTROL = 0,
    CTRL_MODE_CURRENT_CONTROL = 1,
    CTRL_MODE_VELOCITY_CONTROL = 2,
    CTRL_MODE_POSITION_CONTROL = 3,
    CTRL_MODE_COUPLED_CONTROL = 4,
    CTRL_MODE_XY_CONTROL = 5
} Motor_control_mode_t;

struct ControllerConfig_t {
    Motor_control_mode_t control_mode = CTRL_MODE_POSITION_CONTROL;  //see: Motor_control_mode_t
    float pos_gain = 0.01f;  // [(counts/s) / counts]
    float vel_gain = 5.0f / 10000.0f;  // [A/(counts/s)]
    // float vel_gain = 5.0f / 200.0f, // [A/(rad/s)] <sensorless example>
    float vel_integrator_gain = 10.0f / 10000.0f;  // [A/(counts/s * s)]
    float vel_limit = 20000.0f;           // [counts/s]

    float kp_theta = 0.04f * 6000.0f / (2.0f * M_PI);
    float kd_theta = 5.0f / 10000.0f * 6000.0f / (2.0f * M_PI);
    float kp_gamma = 0.01f * 6000.0f / (2.0f * M_PI);
    float kd_gamma = 5.0f / 10000.0f * 6000.0f / (2.0f * M_PI);

    float kp_x = 0;
    float kd_x = 0;
    float kp_y = 0;
    float kd_y = 0;

    float gear_ratio = 4.0;
};

class Controller {
public:
    Controller(ControllerConfig_t& config);
    void reset();

    void set_pos_setpoint(float pos_setpoint, float vel_feed_forward, float current_feed_forward);
    void set_vel_setpoint(float vel_setpoint, float current_feed_forward);
    void set_current_setpoint(float current_setpoint);
    void set_coupled_setpoints(float theta_setpoint, float gamma_setpoint);
    void set_coupled_gains(float kp_theta, float kd_theta, float kp_gamma, float kd_gamma);
    void set_xy_setpoints(float x_setpoint, float y_setpoint);
    void set_xy_gains(float kp_x, float kd_x, float kp_y, float kd_y);

    float encoder_to_rad(float x);

    // TODO: make this more similar to other calibration loops
    void start_anticogging_calibration();
    bool anticogging_calibration(float pos_estimate, float vel_estimate);

    bool update(float pos_estimate, float vel_estimate, float* current_setpoint);

    ControllerConfig_t& config_;
    Axis* axis_ = nullptr; // set by Axis constructor

    // TODO: anticogging overhaul:
    // - expose selected (all?) variables on protocol
    // - make calibration user experience similar to motor & encoder calibration
    // - use python tools to Fourier transform and write back the smoothed map or Fourier coefficients
    // - make the calibration persistent

    typedef struct {
        int index;
        float *cogging_map;
        bool use_anticogging;
        bool calib_anticogging;
        float calib_pos_threshold;
        float calib_vel_threshold;
    } Anticogging_t;
    Anticogging_t anticogging_ = {
        .index = 0,
        .cogging_map = nullptr,
        .use_anticogging = false,
        .calib_anticogging = false,
        .calib_pos_threshold = 1.0f,
        .calib_vel_threshold = 1.0f,
    };

    // variables exposed on protocol
    float pos_setpoint_ = 0.0f;
    float vel_setpoint_ = 0.0f;

    float theta_ = 0.0f;
    float gamma_ = 1.57f;

    // float vel_setpoint = 800.0f; <sensorless example>
    float vel_integrator_current_ = 0.0f;  // [A]
    float current_setpoint_ = 0.0f;        // [A]

    float theta_setpoint_ = 0.0f;
    float gamma_setpoint_ = M_PI/2.0f;

    float x_setpoint_ = 0.0f;
    float y_setpoint_ = 0.13f;

    float force_x_ = 0;
    float force_y_ = 0;

    float x_pos_ = 0;
    float y_pos_ = 0;

    float tau_theta_ = 0;
    float tau_gamma_ = 0;

    float J00=0.0f; float J01=0.0f; float J11=0.0f; float J10=0.0f;

    // Communication protocol definitions
    auto make_protocol_definitions() {
        return make_protocol_member_list(
            make_protocol_property("pos_setpoint", &pos_setpoint_),
            make_protocol_property("vel_setpoint", &vel_setpoint_),
            make_protocol_property("vel_integrator_current", &vel_integrator_current_),
            make_protocol_property("current_setpoint", &current_setpoint_),
            make_protocol_property("theta_setpoint", &theta_setpoint_),
            make_protocol_property("gamma_setpoint", &gamma_setpoint_),
            make_protocol_property("x_setpoint", &x_setpoint_),
            make_protocol_property("y_setpoint", &y_setpoint_),
            make_protocol_property("force_x", &force_x_),
            make_protocol_property("force_y", &force_y_),
            make_protocol_property("x_pos", &x_pos_),
            make_protocol_property("y_pos", &y_pos_),
            make_protocol_property("tau_theta", &tau_theta_),
            make_protocol_property("tau_gamma", &tau_gamma_),
            make_protocol_property("J00", &J00),
            make_protocol_property("J01", &J01),
            make_protocol_property("J10", &J10),
            make_protocol_property("J11", &J11),
            make_protocol_property("theta", &theta_),
            make_protocol_property("gamma", &gamma_),

            make_protocol_object("config",
                make_protocol_property("control_mode", &config_.control_mode),
                make_protocol_property("pos_gain", &config_.pos_gain),
                make_protocol_property("vel_gain", &config_.vel_gain),
                make_protocol_property("vel_integrator_gain", &config_.vel_integrator_gain),
                make_protocol_property("vel_limit", &config_.vel_limit),
                make_protocol_property("kp_theta", &config_.kp_theta),
                make_protocol_property("kd_theta", &config_.kd_theta),
                make_protocol_property("kp_gamma", &config_.kp_gamma),
                make_protocol_property("kd_gamma", &config_.kd_gamma),
                make_protocol_property("kp_x", &config_.kp_x),
                make_protocol_property("kd_x", &config_.kd_x),
                make_protocol_property("kp_y", &config_.kp_y),
                make_protocol_property("kd_y", &config_.kd_y)
            ),
            make_protocol_function("set_pos_setpoint", *this, &Controller::set_pos_setpoint,
                "pos_setpoint",
                "vel_feed_forward",
                "current_feed_forward"),
            make_protocol_function("set_vel_setpoint", *this, &Controller::set_vel_setpoint,
                "vel_setpoint",
                "current_feed_forward"),
            make_protocol_function("set_current_setpoint", *this, &Controller::set_current_setpoint,
                "current_setpoint"),
            make_protocol_function("set_coupled_setpoints", *this, &Controller::set_coupled_setpoints,
                "theta_setpoint",
                "gamma_setpoint"),
            make_protocol_function("set_xy_setpoints", *this, &Controller::set_xy_setpoints,
                "x_setpoint",
                "y_setpoint"),
                make_protocol_function("set_xy_gains", *this, &Controller::set_xy_gains,
                "kp_x",
                "kd_x",
                "kp_y",
                "kd_y"),
            make_protocol_function("start_anticogging_calibration", *this, &Controller::start_anticogging_calibration)
        );
    }
};

#endif // __CONTROLLER_HPP
