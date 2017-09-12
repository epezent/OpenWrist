#include "HapticGuidance.h"
#include "Input.h"
#include <random>

using namespace mel;

//HapticGuidance::HapticGuidance(util::Clock& clock, core::Daq* ow_daq, exo::OpenWrist& open_wrist, core::Daq* meii_daq, exo::MahiExoII& meii, Cuff& cuff, util::GuiFlag& gui_flag, int input_mode,
HapticGuidance::HapticGuidance(util::Clock& clock, core::Daq* ow_daq, exo::OpenWrist& open_wrist, Cuff& cuff, util::GuiFlag& gui_flag, int input_mode,
    int subject_number, int condition, std::string start_trial):
    StateMachine(8), 
    clock_(clock),
    ow_daq_(ow_daq),
    open_wrist_(open_wrist), 
    //meii_daq_(meii_daq),
    //meii_(meii),
    cuff_(cuff),
    gui_flag_(gui_flag),
    INPUT_MODE_(input_mode),
    SUBJECT_NUMBER_(subject_number),
    CONDITION_(condition)
{
    // create subject folder
    if (subject_number < 10)
        DIRECTORY_ = "S0" + std::to_string(subject_number) + "_C" + std::to_string(condition);
    else
        DIRECTORY_ = "S" + std::to_string(subject_number) + "_C" + std::to_string(condition);

    // compund frequencies
    TRAJ_PARAMS_T_.reserve(12);
    TRAJ_PARAMS_T_.insert(TRAJ_PARAMS_T_.end(), TRAJ_PARAMS_E_.begin(), TRAJ_PARAMS_E_.end());
    TRAJ_PARAMS_T_.insert(TRAJ_PARAMS_T_.end(), TRAJ_PARAMS_E_.begin(), TRAJ_PARAMS_E_.end());
    TRAJ_PARAMS_T_.insert(TRAJ_PARAMS_T_.end(), TRAJ_PARAMS_E_.begin(), TRAJ_PARAMS_E_.end());
    TRAJ_PARAMS_T_.insert(TRAJ_PARAMS_T_.end(), TRAJ_PARAMS_E_.begin(), TRAJ_PARAMS_E_.end());

    // make perlin module
    guidance_module_.SetOctaveCount(1);
    guidance_module_.SetFrequency(1.0);
    guidance_module_.SetPersistence(0.1);

    // set up trajectory module
    trajectory_module_.SetOctaveCount(1);
    trajectory_module_.SetPersistence(0.0);

    // seed random number generator with subject num
    srand(subject_number);

    // build the experiment order
    build_experiment();

    // set the current trial index
    for (int i = 0; i < TRIALS_TAG_NAMES_.size(); ++i) {
        if (start_trial == TRIALS_TAG_NAMES_[i])
            current_trial_index_ = i-1;
    }

    // Add columns to logger
    log_.add_col("Time [s]").add_col("Amplitude [px]").add_col("Sin Freq. [Hz]").add_col("Cos Freq. [Hz]")
        .add_col("Exp_x [px]").add_col("Exp_y [px]").add_col("Angular Error [rad]").add_col("OW PS Position [rad]").add_col("OW PS Velocity [rad/s]").add_col("OW PS Total Torque [Nm]")
        .add_col("OW PS Compensation Torque [Nm]").add_col("OW PS Task Torque [Nm]").add_col("OW PS Noise Torque")
        .add_col("CUFF Motor Position 1").add_col("CUFF Motor Position 2").add_col("CUFF Noise");

    // update Unity
    update_unity(true, true, false, false, false, true, true, false);

}

void HapticGuidance::log_row() {
    std::vector<double> row;
    row.push_back(clock_.time());
    row.push_back(amplitude_px_);
    row.push_back(sin_freq_);
    row.push_back(cos_freq_);
    row.push_back(expert_position_px_[0]);
    row.push_back(expert_position_px_[1]);
    row.push_back(traj_error_);
    row.push_back(open_wrist_.joints_[0]->get_position());
    row.push_back(open_wrist_.joints_[0]->get_velocity());
    row.push_back(ps_total_torque_);
    row.push_back(ps_comp_torque_);
    row.push_back(-pendulum_.Tau[0]);
    row.push_back(ps_noise_torque_);
    row.push_back((double)cuff_pos_1_);
    row.push_back((double)cuff_pos_2_);
    row.push_back(cuff_noise_);
    log_.add_row(row);
}

void HapticGuidance::wait_for_input() {
    if (INPUT_MODE_ == 0) {
        util::Input::wait_for_key_press(util::Input::Key::Space);
    }
    else if (INPUT_MODE_ = 1) {
        gui_flag_.wait_for_flag(1);
        util::print("");
    }
}

void HapticGuidance::allow_continue_input() {
    if (INPUT_MODE_ == 1)
        gui_flag_.reset_flag(0);
}

bool HapticGuidance::check_stop() {
    return util::Input::is_key_pressed(util::Input::Escape) || (util::Input::is_key_pressed(util::Input::LControl) && util::Input::is_key_pressed(util::Input::C));
}


void HapticGuidance::build_experiment() {
    // for every block
    for (auto it = BLOCK_ORDER_.begin(); it != BLOCK_ORDER_.end(); ++it) { 
        // increment the number of blocks of this type of block
        NUM_BLOCKS_[*it] += 1;

        // generate a set of temporary traj params equal to num trails in the block
        std::vector<TrajParams> traj_params_temp;
        if (*it == FAMILIARIZATION || *it == BREAK)
            traj_params_temp = TRAJ_PARAMS_FB_;
        else if (*it == EVALUATION)
            traj_params_temp = TRAJ_PARAMS_E_;
        else if (*it == TRAINING)
            traj_params_temp = TRAJ_PARAMS_T_;
        else if (*it == GENERALIZATION)
            traj_params_temp = TRAJ_PARAMS_G_;

        // shuffle the temp traj params
        std::random_shuffle(traj_params_temp.begin(), traj_params_temp.end());
        
        // for each trial in this block type
        for (int i = 0; i < NUM_TRIALS_[*it]; i++) { 
            TRIALS_BLOCK_TYPES_.push_back(*it);
            TRIALS_BLOCK_NAMES_.push_back(BLOCK_NAMES_[*it]);
            TRIALS_TAG_NAMES_.push_back(BLOCK_TAGS_[*it] + std::to_string(NUM_BLOCKS_[*it]) + "-" + std::to_string(i + 1));
            NUM_TRIALS_TOTAL_ += 1;
            TRAJ_PARAMS_.push_back(traj_params_temp[i]);
        }
    }
}



//-----------------------------------------------------------------------------
// START STATE FUNCTION
//-----------------------------------------------------------------------------
void HapticGuidance::sf_start(const util::NoEventData*) {

    // launch game
    game.launch();
    
    // enable OpenWrist DAQ
    if (CONDITION_ >= 0) {
        util::print("\nPress Enter to enable OpenWrist Daq <" + ow_daq_->name_ + ">.");
        util::Input::wait_for_key_press(util::Input::Key::Return);
        ow_daq_->enable();
        if (!ow_daq_->is_enabled()) {
            event(ST_STOP);
            return;
        }
    }
    
    // enable and pretension CUFF
    if (CONDITION_ == 2 || CONDITION_ == 3) {
        std::cout << "\nPress Enter to enable and pretension CUFF" << std::endl;
        util::Input::wait_for_key_press(util::Input::Key::Return);
        cuff_.enable();
        if (!cuff_.is_enabled()) {
            event(ST_STOP);
            return;
        }
        cuff_.pretensioning(CUFF_NORMAL_FORCE_, offset, scaling_factor);
        cuff_.set_motor_positions(-100, 100, true);
    } 

    // enable MahiExo-II DAQ
    if (CONDITION_ == 4) {
        /*
        util::print("\nPress Enter to enable MahiExo-II Daq <" + ow_daq_->name_ + ">.");
        util::Input::wait_for_key_press(util::Input::Key::Return);
        meii_daq_->enable();
        if (!meii_daq_->is_enabled()) {
            event(ST_STOP);
            return;
        }

        // check DAQ behavior for safety
        meii_daq_->read_all();
        meii_.update_kinematics();
        if (meii_.check_all_joint_limits()) {
            event(ST_STOP);
            return;
        }
        */
    }
    
    event(ST_TRANSITION);   
}

//-----------------------------------------------------------------------------
// FAMILIARIZATION STATE FUNCTION
//-----------------------------------------------------------------------------
void HapticGuidance::sf_familiarization(const util::NoEventData*) {

    // show/hide Unity elements
    update_unity(true, true, true, true, true, true, true, true);

    bool move_started = false;

    // enter the control loop
    while (clock_.time() < LENGTH_TRIALS_[FAMILIARIZATION] && !stop_) {

        // update trajectory
        update_trajectory(clock_.time());

        // update expert position
        update_expert(clock_.time());

        // update countdown timer
        std::array<double, 2> timer = { clock_.time(), LENGTH_TRIALS_[FAMILIARIZATION] };
        timer_.write(timer);

        // read and reload DAQ
        if (CONDITION_ >= 0) {

            // read and reload DAQ
            ow_daq_->reload_watchdog();
            ow_daq_->read_all();

            // step the pendulum simuation
            pendulum_.step_simulation(clock_.time(), open_wrist_.joints_[0]->get_position(), open_wrist_.joints_[0]->get_velocity());
            // compute anglular error
            update_trajectory_error(open_wrist_.joints_[0]->get_position());
            scope_.write(traj_error_);

            // set device forces based on conditions
            ps_comp_torque_ = open_wrist_.compute_gravity_compensation(0) + 0.75 * open_wrist_.compute_friction_compensation(0);
            ps_noise_torque_ = 0;
            ps_total_torque_ = ps_comp_torque_ - pendulum_.Tau[0] + ps_noise_torque_;
            open_wrist_.joints_[0]->set_torque(ps_total_torque_);

            cuff_noise_ = 0;
            cuff_pos_1_ = 0;
            cuff_pos_2_ = 0;
            
            if (!move_started) {
                open_wrist_.joints_[1]->set_torque(pd1_.move_to_hold(0, open_wrist_.joints_[1]->get_position(), 60 * math::DEG2RAD, open_wrist_.joints_[1]->get_velocity(), clock_.delta_time_, math::DEG2RAD, true));
                open_wrist_.joints_[2]->set_torque(pd2_.move_to_hold(0, open_wrist_.joints_[2]->get_position(), 60 * math::DEG2RAD, open_wrist_.joints_[2]->get_velocity(), clock_.delta_time_, math::DEG2RAD, true));
                move_started = true;
            }
            else {
                open_wrist_.joints_[1]->set_torque(pd1_.move_to_hold(0, open_wrist_.joints_[1]->get_position(), 60 * math::DEG2RAD, open_wrist_.joints_[1]->get_velocity(), clock_.delta_time_, math::DEG2RAD, false));
                open_wrist_.joints_[2]->set_torque(pd2_.move_to_hold(0, open_wrist_.joints_[2]->get_position(), 60 * math::DEG2RAD, open_wrist_.joints_[2]->get_velocity(), clock_.delta_time_, math::DEG2RAD, false));
            }       

            // check joint limits
            if (open_wrist_.check_all_joint_limits()) {
                stop_ = true;
                break;
            }

            // update OpenWrist state
            open_wrist_.update_state_map();

            // write the DAQ
            ow_daq_->write_all();
        }

        // log data
        log_row();

        // check for stop input
        stop_ = check_stop();

        // wait for the next clock cycle
        clock_.wait();      
        clock_.log();
    }

    // transition to the next state
    event(ST_TRANSITION);
}

//-----------------------------------------------------------------------------
// EVALUATION STATE FUNCTION
//-----------------------------------------------------------------------------
void HapticGuidance::sf_evaluation(const util::NoEventData*) {

    // show/hide Unity elements
    update_unity(true, true, true, false, false, false, true, true);

    bool move_started = false;
   
    // enter the control loop
    while (clock_.time() < LENGTH_TRIALS_[EVALUATION] && !stop_) {

        // update trajectory
        update_trajectory(clock_.time());
        // update expert position
        update_expert(clock_.time());

        // update countdown timer
        std::array<double, 2> timer = { clock_.time(), LENGTH_TRIALS_[EVALUATION] };
        timer_.write(timer);

        // read and reload DAQ
        if (CONDITION_ >= 0) {

            // read and reload DAQ
            ow_daq_->reload_watchdog();
            ow_daq_->read_all();

            // step the pendulum simuation
            pendulum_.step_simulation(clock_.time(), open_wrist_.joints_[0]->get_position(), open_wrist_.joints_[0]->get_velocity());
            // compute anglular error
            update_trajectory_error(open_wrist_.joints_[0]->get_position());
            scope_.write(traj_error_);

            // set device forces based on conditions
            ps_comp_torque_ = open_wrist_.compute_gravity_compensation(0) + 0.75 * open_wrist_.compute_friction_compensation(0);
            ps_noise_torque_ = 0;
            ps_total_torque_ = ps_comp_torque_ - pendulum_.Tau[0] + ps_noise_torque_;
            open_wrist_.joints_[0]->set_torque(ps_total_torque_);

            cuff_noise_ = 0;
            cuff_pos_1_ = 0;
            cuff_pos_2_ = 0;

            if (!move_started) {
                open_wrist_.joints_[1]->set_torque(pd1_.move_to_hold(0, open_wrist_.joints_[1]->get_position(), 60 * math::DEG2RAD, open_wrist_.joints_[1]->get_velocity(), clock_.delta_time_, math::DEG2RAD, true));
                open_wrist_.joints_[2]->set_torque(pd2_.move_to_hold(0, open_wrist_.joints_[2]->get_position(), 60 * math::DEG2RAD, open_wrist_.joints_[2]->get_velocity(), clock_.delta_time_, math::DEG2RAD, true));
                move_started = true;
            }
            else {
                open_wrist_.joints_[1]->set_torque(pd1_.move_to_hold(0, open_wrist_.joints_[1]->get_position(), 60 * math::DEG2RAD, open_wrist_.joints_[1]->get_velocity(), clock_.delta_time_, math::DEG2RAD, false));
                open_wrist_.joints_[2]->set_torque(pd2_.move_to_hold(0, open_wrist_.joints_[2]->get_position(), 60 * math::DEG2RAD, open_wrist_.joints_[2]->get_velocity(), clock_.delta_time_, math::DEG2RAD, false));
            }

            // check joint limits
            if (open_wrist_.check_all_joint_limits()) {
                stop_ = true;
                break;
            }

            // update OpenWrist state
            open_wrist_.update_state_map();

            // write the DAQ
            ow_daq_->write_all();
        }

        // log data
        log_row();

        // check for stop input
        stop_ = check_stop();

        // wait for the next clock cycle
        clock_.wait();
        clock_.log();

    }

    // transition to the next state
    event(ST_TRANSITION);
}

//-----------------------------------------------------------------------------
// TRAINING STATE FUNCTION
//-----------------------------------------------------------------------------
void HapticGuidance::sf_training(const util::NoEventData*) {

    // show/hide Unity elements
    update_unity(true, true, true, false, false, false, true, true);


    bool move_started_ow = false;
    bool move_started_meii = false;

    // enter the control loop
    while (clock_.time() < LENGTH_TRIALS_[TRAINING] && !stop_) {

        // update trajectory
        update_trajectory(clock_.time());
        // update expert position
        update_expert(clock_.time());

        // update countdown timer
        std::array<double, 2> timer = { clock_.time(), LENGTH_TRIALS_[TRAINING] };
        timer_.write(timer);

        // read and reload DAQ
        if (CONDITION_ >= 0) {

            // read and reload DAQ
            ow_daq_->reload_watchdog();
            ow_daq_->read_all();

            // step the pendulum simuation
            pendulum_.step_simulation(clock_.time(), open_wrist_.joints_[0]->get_position(), open_wrist_.joints_[0]->get_velocity());
            // compute anglular error
            update_trajectory_error(open_wrist_.joints_[0]->get_position());
            scope_.write(traj_error_);

            ps_comp_torque_ = open_wrist_.compute_gravity_compensation(0) + 0.75 * open_wrist_.compute_friction_compensation(0);
            ps_noise_torque_ = 0;

            cuff_noise_ = 0;
            cuff_pos_1_ = 0;
            cuff_pos_2_ = 0;


            if (CONDITION_ == 1) {
                ps_noise_torque_ = guidance_module_.GetValue(clock_.time(), 0.0, 0.0) * ow_noise_gain_;               
            }
            else if (CONDITION_ == 2) {
                cuff_noise_ = guidance_module_.GetValue(clock_.time(), 0.0, 0.0);
                cuff_pos_1_ = (short int)(cuff_noise_ * CUFF_NOISE_GAIN_) + offset[0];
                cuff_pos_2_ = (short int)(cuff_noise_ * CUFF_NOISE_GAIN_) + offset[1];
                cuff_.set_motor_positions(cuff_pos_1_, cuff_pos_2_, true);
            }
            else if (CONDITION_ == 3) {
                cuff_pos_1_ = (short int)(-traj_error_ * CUFF_GUIDANCE_GAIN_ + offset[0]);
                cuff_pos_2_ = (short int)(-traj_error_ * CUFF_GUIDANCE_GAIN_ + offset[1]);
                cuff_.set_motor_positions(cuff_pos_1_, cuff_pos_2_, true);
            }
            else if (CONDITION_ == 4) {
                /*
                meii_daq_->reload_watchdog();
                meii_daq_->read_all();

                meii_.update_kinematics();
                meii_.check_all_joint_limits();

                double_vec meii_torques(5, 0.0);
                //double meii_torque = meii_.robot_joint_pd_controllers_[1].calculate(open_wrist_.joints_[0]->get_position(), meii_.get_anatomical_joint_position(1), 0, meii_.get_anatomical_joint_velocity(1));
                //meii_.joints_[1]->set_torque(meii_torque);

                for (auto i = 0; i < 5; ++i) {
                    if (i != 1) {                        
                        if (!move_started_meii) {
                            meii_torques[i] = meii_.robot_joint_pd_controllers_[i].move_to_hold(neutral_pos_meii_[i], meii_.joints_[i]->get_position(), speed_meii_[i], meii_.joints_[i]->get_velocity(), clock_.delta_time_, pos_tol_meii_[i], true);
                            move_started_meii = true;
                        }
                        else {
                            meii_torques[i] = meii_.robot_joint_pd_controllers_[i].move_to_hold(neutral_pos_meii_[i], meii_.joints_[i]->get_position(), speed_meii_[i], meii_.joints_[i]->get_velocity(), clock_.delta_time_, pos_tol_meii_[i], false);
                        }
                    }
                    else {
                        meii_torques[i] = 7.0 * traj_error_;
                    }
                }
                meii_daq_->write_all();
                */
            }

            ps_total_torque_ = ps_comp_torque_ - pendulum_.Tau[0] + ps_noise_torque_;
            open_wrist_.joints_[0]->set_torque(ps_total_torque_);

            if (!move_started_ow) {
                open_wrist_.joints_[1]->set_torque(pd1_.move_to_hold(0, open_wrist_.joints_[1]->get_position(), 60 * math::DEG2RAD, open_wrist_.joints_[1]->get_velocity(), clock_.delta_time_, math::DEG2RAD, true));
                open_wrist_.joints_[2]->set_torque(pd2_.move_to_hold(0, open_wrist_.joints_[2]->get_position(), 60 * math::DEG2RAD, open_wrist_.joints_[2]->get_velocity(), clock_.delta_time_, math::DEG2RAD, true));
                move_started_ow = true;
            }
            else {
                open_wrist_.joints_[1]->set_torque(pd1_.move_to_hold(0, open_wrist_.joints_[1]->get_position(), 60 * math::DEG2RAD, open_wrist_.joints_[1]->get_velocity(), clock_.delta_time_, math::DEG2RAD, false));
                open_wrist_.joints_[2]->set_torque(pd2_.move_to_hold(0, open_wrist_.joints_[2]->get_position(), 60 * math::DEG2RAD, open_wrist_.joints_[2]->get_velocity(), clock_.delta_time_, math::DEG2RAD, false));
            }

            // check joint limits
            if (open_wrist_.check_all_joint_limits()) {
                stop_ = true;
                break;
            }

            // update OpenWrist state
            open_wrist_.update_state_map();

            // write the DAQ
            ow_daq_->write_all();
        }

        // log data
        log_row();

        // check for stop input
        stop_ = check_stop();

        // wait for the next clock cycle
        clock_.wait();
        clock_.log();

    }

    // release CUFF
    if (CONDITION_ == 2 || CONDITION_ == 3)
        cuff_.set_motor_positions(-100, 100, true);


    // transition to the next state
    event(ST_TRANSITION);
}

//-----------------------------------------------------------------------------
// BREAK STATE FUNCTION
//-----------------------------------------------------------------------------
void HapticGuidance::sf_break(const util::NoEventData*) {

    // show/hide Unity elements
    update_unity(true, false, false, false, false, false, true, true);

    // enter the control loop
    while (clock_.time() < LENGTH_TRIALS_[BREAK] && !stop_) {

        // update countdown timer
        std::array<double, 2> timer = { clock_.time(), LENGTH_TRIALS_[BREAK] };
        timer_.write(timer);

        // check for stop input
        stop_ = check_stop();

        // wait for the next clock cycle
        clock_.wait();
    }

    // transition to the next state
    event(ST_TRANSITION);
}

//-----------------------------------------------------------------------------
// GENERALIZATION STATE FUNCTION
//-----------------------------------------------------------------------------
void HapticGuidance::sf_generalization(const util::NoEventData*) {

    // show/hide Unity elements
    update_unity(true, true, true, false, false, false, true, true);

    bool move_started = false;

    // enter the control loop
    while (clock_.time() < LENGTH_TRIALS_[GENERALIZATION] && !stop_) {

        // update trajectory
        update_trajectory(clock_.time());
        // update expert position
        update_expert(clock_.time());

        // update countdown timer
        std::array<double, 2> timer = { clock_.time(), LENGTH_TRIALS_[GENERALIZATION] };
        timer_.write(timer);

        // read and reload DAQ
        if (CONDITION_ >= 0) {

            // read and reload DAQ
            ow_daq_->reload_watchdog();
            ow_daq_->read_all();

            // step the pendulum simuation
            pendulum_.step_simulation(clock_.time(), open_wrist_.joints_[0]->get_position(), open_wrist_.joints_[0]->get_velocity());
            // compute anglular error
            update_trajectory_error(open_wrist_.joints_[0]->get_position());

            // set device forces based on conditions
            ps_comp_torque_ = open_wrist_.compute_gravity_compensation(0) + 0.75 * open_wrist_.compute_friction_compensation(0);
            ps_noise_torque_ = 0;
            ps_total_torque_ = ps_comp_torque_ - pendulum_.Tau[0] + ps_noise_torque_;
            open_wrist_.joints_[0]->set_torque(ps_total_torque_);

            cuff_noise_ = 0;
            cuff_pos_1_ = 0;
            cuff_pos_2_ = 0;

            if (!move_started) {
                open_wrist_.joints_[1]->set_torque(pd1_.move_to_hold(0, open_wrist_.joints_[1]->get_position(), 60 * math::DEG2RAD, open_wrist_.joints_[1]->get_velocity(), clock_.delta_time_, math::DEG2RAD, true));
                open_wrist_.joints_[2]->set_torque(pd2_.move_to_hold(0, open_wrist_.joints_[2]->get_position(), 60 * math::DEG2RAD, open_wrist_.joints_[2]->get_velocity(), clock_.delta_time_, math::DEG2RAD, true));
                move_started = true;
            }
            else {
                open_wrist_.joints_[1]->set_torque(pd1_.move_to_hold(0, open_wrist_.joints_[1]->get_position(), 60 * math::DEG2RAD, open_wrist_.joints_[1]->get_velocity(), clock_.delta_time_, math::DEG2RAD, false));
                open_wrist_.joints_[2]->set_torque(pd2_.move_to_hold(0, open_wrist_.joints_[2]->get_position(), 60 * math::DEG2RAD, open_wrist_.joints_[2]->get_velocity(), clock_.delta_time_, math::DEG2RAD, false));
            }

            // check joint limits
            if (open_wrist_.check_all_joint_limits()) {
                stop_ = true;
                break;
            }

            // update OpenWrist state
            open_wrist_.update_state_map();

            // write the DAQ
            ow_daq_->write_all();
        }

        // log data
        log_row();

        // check for stop input
        stop_ = check_stop();

        // wait for the next clock cycle
        clock_.wait();
        clock_.log();

    }

    // transition to the next state
    event(ST_TRANSITION);

}

//-----------------------------------------------------------------------------
// TRANSITION STATE FUNCTION
//-----------------------------------------------------------------------------
void HapticGuidance::sf_transition(const util::NoEventData*) {

    // suspend hardware
    if (CONDITION_ >= 0) {
        open_wrist_.disable();
        ow_daq_->stop_watchdog();
    }

    if (CONDITION_ == 4) {
        //meii_.disable();
        //meii_daq_->stop_watchdog();
    }

    // save the data log from the last trial
    if (trials_started_) {
        log_.save_and_clear_data(TRIALS_TAG_NAMES_[current_trial_index_], DIRECTORY_ + "\\_" + TRIALS_BLOCK_NAMES_[current_trial_index_], true);
    }
    clock_.save_log();

    // show/hide Unity elements
    update_unity(true, true, false, false, false, false, true, true);

    // start a new tiral if there is one or stop hasn't been requested
    if (current_trial_index_ < NUM_TRIALS_TOTAL_ - 1 && !stop_) {

        // increment the trial;
        current_trial_index_ += 1;

        // write the trial string out
        trial_.write_message(TRIALS_TAG_NAMES_[current_trial_index_]);

        // delay or ask for input
        if (TRIALS_TAG_NAMES_[current_trial_index_] == "F1-1" ||
            TRIALS_TAG_NAMES_[current_trial_index_] == "E1-1" ||
            TRIALS_TAG_NAMES_[current_trial_index_] == "T1-1" ||
            TRIALS_TAG_NAMES_[current_trial_index_] == "B1-1" ||
            TRIALS_TAG_NAMES_[current_trial_index_] == "G1-1") 
        {
            std::array<double, 2> timer = { 0, LENGTH_TRIALS_[TRIALS_BLOCK_TYPES_[current_trial_index_]] };
            timer_.write(timer);

            util::print("\nNEXT TRIAL: <" + TRIALS_TAG_NAMES_[current_trial_index_] + ">. Press SPACE to begin.");
            while (!util::Input::is_key_pressed(util::Input::Space)) {
                stop_ = check_stop();
                if (stop_) {
                    event(ST_STOP);
                    return;
                }
            }
        }
        else {
            util::print("\nNEXT TRIAL: <" + TRIALS_TAG_NAMES_[current_trial_index_] + ">.");
            clock_.start();
            while (clock_.time() < 3.0) {
                std::array<double, 2> timer = { clock_.time(), 3.0 };
                timer_.write(timer);
                stop_ = check_stop();
                if (stop_) {
                    event(ST_STOP);
                    return;
                }
                clock_.wait();
            }
        }

        std::array<double, 2> timer = { 0, LENGTH_TRIALS_[TRIALS_BLOCK_TYPES_[current_trial_index_]] };
        timer_.write(timer);

        // reset the pendlum
        pendulum_.reset();

        // set the trajectory parameters
        amplitude_px_ = TRAJ_PARAMS_[current_trial_index_].amp_;
        sin_freq_ = TRAJ_PARAMS_[current_trial_index_].sin_;
        cos_freq_ = TRAJ_PARAMS_[current_trial_index_].cos_;
        trajectory_module_.SetFrequency(cos_freq_);
        trajectory_module_.SetSeed(current_trial_index_);

        // seed the guidane with a numer unique to the current traj param
        guidance_module_.SetSeed(static_cast<int>(TRAJ_PARAMS_[current_trial_index_].cos_ * 10));
        
        // print message
        util::print("STARTING TRIAL: <" + TRIALS_TAG_NAMES_[current_trial_index_] + ">." +
            " A = " + std::to_string(amplitude_px_) + " S = " + std::to_string(sin_freq_) + " C = " + std::to_string(cos_freq_));
        util::print("Press ESC or CTRL+C to terminate the experiment.");

        trials_started_ = true;

        // resume hardware
        if (CONDITION_ >= 0) {
            open_wrist_.enable();
            ow_daq_->start_watchdog(0.1);
        }

        if (CONDITION_ == 4) {
            //meii_.enable();
            //meii_daq_->start_watchdog(0.1);
        }

        // restart the clock
        clock_.start();

        // transition to the next state
        if (TRIALS_BLOCK_TYPES_[current_trial_index_] == FAMILIARIZATION)
            event(ST_FAMILIARIZATION);
        else if (TRIALS_BLOCK_TYPES_[current_trial_index_] == EVALUATION)
            event(ST_EVALUATION);
        else if (TRIALS_BLOCK_TYPES_[current_trial_index_] == TRAINING)
            event(ST_TRAINING);
        else if (TRIALS_BLOCK_TYPES_[current_trial_index_] == BREAK)
            event(ST_BREAK);
        else if (TRIALS_BLOCK_TYPES_[current_trial_index_] == GENERALIZATION)
            event(ST_GENERALIZATION);
        return;
    }    
    else {
        event(ST_STOP);
        return;
    }
}

//-----------------------------------------------------------------------------
// STOP STATE FUNCTION
//-----------------------------------------------------------------------------
void HapticGuidance::sf_stop(const util::NoEventData*) {
    if (current_trial_index_ < 0)
        util::print("\nExperiment terminated during startup. Disabling hardware.");
    else if (current_trial_index_ < NUM_TRIALS_TOTAL_ - 1)
        util::print("\nExperiment terminated during trial " + util::namify(TRIALS_TAG_NAMES_[current_trial_index_ ]) + ". Disabling hardware.");
    else
        util::print("\nExperiment completed. Disabling hardware.");

    if (CONDITION_ >= 0)
        ow_daq_->disable();

    if (CONDITION_ == 2 || CONDITION_ == 3)
        cuff_.disable();
}

//-----------------------------------------------------------------------------
// TRAJECTORY FUNCTIONS
//-----------------------------------------------------------------------------

double HapticGuidance::trajectory(double time) {
    return amplitude_px_ * -sin(2.0 * math::PI * sin_freq_ * time) * cos(2.0 * math::PI * cos_freq_ * time);
}

void HapticGuidance::update_trajectory(double time) {
    // compute trajectory
    for (int i = 0; i < num_traj_points_; i++) {
        trajectory_y_px_[i] = (i* spacing_px_ - (num_traj_points_ - 1) * spacing_px_ * 0.5 ); 
        trajectory_x_px_[i] = trajectory(trajectory_y_px_[i] * screen_time_ / screen_height_ + time);
    }
    // send trajectory to Unity
    trajectory_x_.write(trajectory_x_px_);
    trajectory_y_.write(trajectory_y_px_);
}

void HapticGuidance::update_expert(double time) {
    double traj_point_x;
    std::array<double, 450> check_lengths;
    double min = 1080;
    int pos_min = 0;
    double traj_point_min;  

    for (int i = 0; i < 450; i++) {
        //traj_point = amplitude_ / 1000.0 * -sin(2.0*math::PI*sin_freq_*(time - (double)i    / 1080.0  )) *cos(2.0*math::PI*cos_freq_*(time - (double)i / 1080.0    ));

        check_lengths[i] = abs( sqrt(pow(traj_point_x, 2) + pow(length_px_- i, 2)) );



        if (check_lengths[i] < min) {
            min = check_lengths[i];
            pos_min = i;
            traj_point_min = traj_point_x;
        }
    }
    expert_position_px_[0] =  (int)traj_point_min;
    expert_position_px_[1] =  pos_min;
    exp_pos.write(expert_position_px_);
}


void HapticGuidance::update_trajectory_error(double joint_angle) {
    double correct_angle = asin((double)expert_position_px_[0] / length_px_);
    traj_error_ = (joint_angle - correct_angle);
}

void HapticGuidance::update_unity(bool background, bool pendulum, bool trajectory_region, bool trajectory_center, bool expert, bool radius, bool stars, bool ui) {
    unity_data_ = { 0,0,0,0,0,0,0 };
    if (background)
        unity_data_[0] = 1;
    if (pendulum)
        unity_data_[1] = 1;
    if (trajectory_region)
        unity_data_[2] = 1;
    if (trajectory_center)
        unity_data_[3] = 1;
    if (expert)
        unity_data_[4] = 1;
    if (radius)
        unity_data_[5] = 1;
    if (stars)
        unity_data_[6] = 1;
    if (ui)
        unity_data_[7] = 1;
    unity_.write(unity_data_);    
}
