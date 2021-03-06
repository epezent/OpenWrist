#pragma once
#include "StateMachine.h"
#include "OpenWrist.h"
#include "MahiExoII.h"
#include "Cuff.h"
#include "Pendulum.h"
#include "GuiFlag.h"
#include <noise/noise.h>
#include "ExternalApp.h"
#include "PdController.h"

//----------------------------------------------------------------------------
// Hatpic guidance experiements with OpenWrist, CUFF, and MahiExo-II 
//----------------------------------------------------------------------------
// Evan Pezent, Simone Fanni, Josh Bradely (August 2017 - October 2017)
//----------------------------------------------------------------------------
// Condition 1: OpenWrist P/S w/ Perlin Noise Forces                  x10 subj
// Condition 2: OpenWrist P/S + CUFF w/ Perlin Noise Forces           x10 subj
// Condition 3: OpenWrist P/S + CUFF w/ Haptic Guidance Forces        x10 subj
// Condition 4: OpenWrist P/S + MahiExo-II w/ Haptic Guidance Forces  x10 subj
//----------------------------------------------------------------------------
// EXPERIMENT BREAKDOWN              | BLOCK | TRIAL #s | 
//----------------------------------------------------------------------------
// Familiarization Trial (x1 - 1min) | F1    | 1        | 
// Evaluation Trials (x3 - 20s ea)   | E1    | 2 - 4    | 
// Training Trials (x12 - 20s ea)    | T1    | 5 - 16   |
// Evaluation Trials (x3 - 20s ea)   | E2    | 17 - 19  | 
// Training Trials (x12 - 20s ea)    | T2    | 20	31  | 
// Evaluation Trials (x3 - 20s ea)   | E3    | 32 - 34  | 
// Training Trials (x12 - 20s ea)    | T3    | 35 - 46  | 
// 5 Minute Break                    | B1    | 47       |
// Evaluation Trials (x3 - 20s ea)   | E4    | 48 - 50  |
// Training Trials (x12 - 20s ea)    | T4    | 51 - 62  |
// Evaluation Trials (x3 - 20s ea)   | E5    | 63 - 65  |
// Training Trials (x12 - 20s ea)    | T5    | 66 - 77  | 
// Evaluation Trials (x3 - 20s ea)   | E6    | 78 - 80  |
// Training Trials (x12 - 20s ea)    | T6    | 81 - 92  |
// Evaluation Trials (x3 - 20s ea)   | E7    | 93 - 95  |
// Generalization (x12 - 20s ea)     | G     | 96 - 107 |
//----------------------------------------------------------------------------


class HapticGuidance : public mel::util::StateMachine {

public:

    //HapticGuidance(mel::util::Clock& clock, mel::core::Daq* ow_daq, mel::exo::OpenWrist& open_wrist, mel::core::Daq* meii_daq, mel::exo::MahiExoII& meii, Cuff& cuff, mel::util::GuiFlag& gui_flag, int input_mode,
    //    int subject_number, int condition, std::string start_trial = "F1-1");
    HapticGuidance(mel::util::Clock& clock, mel::core::Daq* ow_daq, mel::exo::OpenWrist& open_wrist, Cuff& cuff, mel::util::GuiFlag& gui_flag, int input_mode,
        int subject_number, int condition, std::string start_trial = "F1-1");

private:

    //-------------------------------------------------------------------------
    // STATE MACHINE SETUP
    //-------------------------------------------------------------------------

    // STATES
    enum States {
        ST_START,
        ST_FAMILIARIZATION,
        ST_EVALUATION,
        ST_TRAINING,
        ST_BREAK,
        ST_GENERALIZATION,
        ST_TRANSITION,
        ST_STOP,
        ST_NUM_STATES
    };

    // STATE FUNCTIONS
    void sf_start(const mel::util::NoEventData*);
    void sf_familiarization(const mel::util::NoEventData*);
    void sf_evaluation(const mel::util::NoEventData*);
    void sf_training(const mel::util::NoEventData*);
    void sf_break(const mel::util::NoEventData*);
    void sf_generalization(const mel::util::NoEventData*);
    void sf_transition(const mel::util::NoEventData*);
    void sf_stop(const mel::util::NoEventData*);

    // STATE ACTIONS
    mel::util::StateAction<HapticGuidance, mel::util::NoEventData, &HapticGuidance::sf_start> sa_start;
    mel::util::StateAction<HapticGuidance, mel::util::NoEventData, &HapticGuidance::sf_familiarization> sa_familiarization;
    mel::util::StateAction<HapticGuidance, mel::util::NoEventData, &HapticGuidance::sf_evaluation> sa_evaluation;
    mel::util::StateAction<HapticGuidance, mel::util::NoEventData, &HapticGuidance::sf_training> sa_training;
    mel::util::StateAction<HapticGuidance, mel::util::NoEventData, &HapticGuidance::sf_break> sa_break;
    mel::util::StateAction<HapticGuidance, mel::util::NoEventData, &HapticGuidance::sf_generalization> sa_generlization;
    mel::util::StateAction<HapticGuidance, mel::util::NoEventData, &HapticGuidance::sf_transition> sa_transition;
    mel::util::StateAction<HapticGuidance, mel::util::NoEventData, &HapticGuidance::sf_stop> sa_stop;
    
    // STATE MAP
    virtual const mel::util::StateMapRow* get_state_map() {
        static const mel::util::StateMapRow STATE_MAP[] = {
            &sa_start,
            &sa_familiarization,
            &sa_evaluation,
            &sa_training,
            &sa_break,
            &sa_generlization,
            &sa_transition,
            &sa_stop,
        };
        return &STATE_MAP[0];
    }

    // USER INPUT CONTROL
    int INPUT_MODE_;
    void wait_for_input();
    void allow_continue_input();
    bool check_stop();
    bool stop_ = false;

    //-------------------------------------------------------------------------
    // EXPERIMENT SETUP
    //-------------------------------------------------------------------------

    // SUBJECT/CONDITION
    int SUBJECT_NUMBER_;
    int CONDITION_;

    // BLOCK TYPES
    enum BlockType {
        FAMILIARIZATION = 0,
        EVALUATION = 1,
        TRAINING = 2,
        BREAK = 3,
        GENERALIZATION = 4
    };

    std::array<std::string, 5> BLOCK_NAMES_ = { "FAMILIARIZATION", "EVALUATION", "TRAINING", "BREAK", "GENERALIZATION" };
    std::array<std::string, 5> BLOCK_TAGS_ = { "F", "E", "T", "B", "G" };

    // EXPERIMENT BLOCK ORDER (SET MANUALLY)
    std::vector<BlockType> BLOCK_ORDER_ = {
        FAMILIARIZATION,
        EVALUATION, TRAINING, 
        EVALUATION, TRAINING, 
        EVALUATION, TRAINING,
        BREAK,
        EVALUATION, TRAINING, 
        EVALUATION, TRAINING, 
        EVALUATION, TRAINING,
        EVALUATION, GENERALIZATION 
    };

    // NUMBER OF BLOCKS PER BLOCK TYPE (SET DURING CONSTRUCTION)
    // [ FAMILIARIZATION, EVALUATION, TRAINING, BREAK, GENERALIZATION ]
    std::array<int, 5> NUM_BLOCKS_ = { 0,0,0,0,0 };

    // NUMBER OF TRIALS PER BLOCK TYPE PER BLOCK NUMBER (SET MANUALLY)
    // [ FAMILIARIZATION, EVALUATION, TRAINING, BREAK, GENERALIZATION ]
    std::array<int, 5> NUM_TRIALS_ = { 1, 3, 12, 1, 12 };

    // LENGTH IN SECONDS OF EACH BLOCK TYPE TRIAL (SET MANUALLY)
    // [ FAMILIARIZATION, EVALUATION, TRAINING, BREAK, GENERALIZATION ]
    std::array<double, 5> LENGTH_TRIALS_ = { 60, 20, 20, 300, 20 };

    // EXPERIMENT TRIAL ORDERING
    void build_experiment();
    int current_trial_index_ = 0;
    std::vector<BlockType> TRIALS_BLOCK_TYPES_;
    std::vector<std::string> TRIALS_BLOCK_NAMES_;
    std::vector<std::string> TRIALS_TAG_NAMES_;
    int NUM_TRIALS_TOTAL_ = 0;
    bool trials_started_ = false;

    // SUBJECT DIRECTORY
    std::string DIRECTORY_;

    //-------------------------------------------------------------------------
    // CONTROL LOOP UTILS
    //-------------------------------------------------------------------------

    bool move_to_started_ = false;
    double move_to_speed_ = 60; // [deg/s]

    void step_system(double external_torque = 0.0);

    //-------------------------------------------------------------------------
    // EXPERIMENT COMPONENTS
    //-------------------------------------------------------------------------

    // UNITY GAME
    mel::util::ExternalApp game = mel::util::ExternalApp("pendulum", "C:\\Users\\mep9\\Git\\SpacePendulum\\Builds\\SpacePendulum.exe");

    // GUI FLAGS
    mel::util::GuiFlag& gui_flag_;

    // DATA LOG
    mel::util::DataLog log_ = mel::util::DataLog("hg_log", false);
    std::vector<double> log_data_;
    void log_step();

    // HARDWARE CLOCK
    mel::util::Clock clock_;

    // HARDWARE
    mel::core::Daq* ow_daq_;
    mel::exo::OpenWrist& ow_;
    //mel::core::Daq* meii_daq_;
    //mel::exo::MahiExoII& meii_;
    Cuff& cuff_;

    // PD CONTROLLERS
    mel::core::PdController pd1_ = mel::core::PdController(60, 1);
    mel::core::PdController pd2_ = mel::core::PdController(40, 0.5);

    // CUFF PARAMETERS
    short int CUFF_NORMAL_FORCE_ = 3;
    short int CUFF_NOISE_GAIN_ = 8400;
    short int CUFF_GUIDANCE_GAIN_ = 15000;
    short int offset[2];
    short int scaling_factor[2];

    // PENDULUM 
    Pendulum pendulum_;

    // TRAJECTORY VARIABLES
    struct TrajParams {
        TrajParams(double amp, double sin, double cos) : amp_(amp), sin_(sin), cos_(cos) {}
        double amp_, sin_, cos_;
    };

    std::vector<TrajParams> TRAJ_PARAMS_FB_ = { TrajParams(250, 0.1, 0.1) };
    std::vector<TrajParams> TRAJ_PARAMS_E_ = {TrajParams(250,0.2, 0.1), TrajParams(250, 0.3, 0.2), TrajParams(250, 0.3, 0.4) };
    std::vector<TrajParams> TRAJ_PARAMS_T_; // to be generated from TRAJ_PARAMS_E_
    std::vector<TrajParams> TRAJ_PARAMS_G_ = std::vector<TrajParams>(12, TrajParams(250, 0.25, 0.5));
    std::vector<TrajParams> TRAJ_PARAMS_; // random generated for all trials

    double screen_height_ = 1080; // px

    double length_px_ = 450;   // link 1 length [px]

    double trajectory(double time);
    double amplitude_px_;      // trajectory amplitude [px]
    double sin_freq_;          // trajectory sin component frequency [Hz]
    double cos_freq_;          // trajectory cos component frequency [Hz]
    double screen_time_ = 1.0; // duration a point lasts on screen [sec]

    int num_traj_points_ = 61;
    int spacing_px_ = 20;      // px

    std::array<float, 61> trajectory_x_px_;
    std::array<float, 61> trajectory_y_px_;
    std::array<float, 2> expert_position_px_;

    double traj_error_ = 0;

    void update_trajectory(double time);
    void update_expert(double time);
    void update_trajectory_error(double joint_angle);

    mel::comm::MelShare trajectory_x_ = mel::comm::MelShare("trajectory_x", 61*4);
    mel::comm::MelShare trajectory_y_ = mel::comm::MelShare("trajectory_y", 61*4);
    mel::comm::MelShare exp_pos = mel::comm::MelShare("exp_pos");

    // ME-II VARIABLES
    mel::double_vec neutral_pos_meii_ = { -10.0 * mel::math::DEG2RAD, 0.0 * mel::math::DEG2RAD, 0.11 , 0.11,  0.11 }; // robot joint positions
    mel::double_vec speed_meii_ = { 0.25, 0.25, 0.0125, 0.0125, 0.0125 };
    mel::double_vec pos_tol_meii_ = { 1.0 * mel::math::DEG2RAD, 1.0 * mel::math::DEG2RAD, 0.01, 0.01, 0.01 };

    // SAVE VARIABLES
    double ps_comp_torque_;
    double ps_noise_torque_;
    double ps_total_torque_;

    short int cuff_pos_1_;
    short int cuff_pos_2_;
    double cuff_noise_;

    // UNITY GAMEMANAGER
    std::array<double, 2> timer_data_;
    mel::comm::MelShare timer_ = mel::comm::MelShare("timer");

    mel::comm::MelShare unity_ = mel::comm::MelShare("unity");
    mel::comm::MelShare trial_ = mel::comm::MelShare("trial");

    std::array<int, 8> visible_data_ = { 1,1,1,1,1,1,1,1 };
    void update_visible(bool background, bool pendulum, bool trajectory_region, bool trajectory_center, bool expert, bool radius, bool stars, bool ui);

    // PERLIN NOISE MODULES
    mel::comm::MelShare scope_ = mel::comm::MelShare("scope");
    noise::module::Perlin guidance_module_;
    double ow_noise_gain_ = 0.375;
    noise::module::Perlin trajectory_module_;

};