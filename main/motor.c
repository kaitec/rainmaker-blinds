#include <stdint.h>
#include <stdbool.h>
#include <driver/gpio.h>
#include "hardware.h"
#include "flash.h"

motor_t user_motor_var;
bool position_point; //set when need send notification to server? payload are current position
bool Alarm = false; //set when corrupt motor timing recommendations

uint32_t hall_ticks = 0; // feedback from hall sensor
uint32_t esp_logi_ticks = 0; //for debug

uint16_t feedback_timer_counter = 0;

C_STATUS_CODE_t CS_RESP = c_s_success;

motor_movement_t motor_driver_state(motor_movement_t state) {
	static motor_movement_t direction = M_STOPED;
	//if found get parameter return current value 
	if (state == M_DIR_GET)
		return direction;
	else if ((state == M_DIR_UP) && (direction != M_STOPED))
		return M_DIR_UP_ERR;
	else if ((state == M_DIR_DOWN) && (direction != M_STOPED))
		return M_DIR_DOWN_ERR;
	//if found set parameter
	if (state == M_DIR_UP) {
		if (ON_LEVEL)
			gpio_set_level(UP_DIR, 1);
		else
			gpio_set_level(UP_DIR, 0);
		direction = M_DIR_UP;
	}
	if (state == M_DIR_DOWN) { 
		if (ON_LEVEL)
			gpio_set_level(DOWN_DIR, 1);
		else
			gpio_set_level(DOWN_DIR, 0);
		direction = M_DIR_DOWN;
	}
	if (state == M_STOPED) { 
		if (ON_LEVEL) {
			gpio_set_level(DOWN_DIR, 0);
			gpio_set_level(UP_DIR, 0);
		} else {
			gpio_set_level(DOWN_DIR, 1);
			gpio_set_level(UP_DIR, 1);
		}
		blind_time.b_h.prot = DEAD_TIME;

		hall_ticks = 0;
		if (user_motor_var.max_r_step)
			user_motor_var.perc_roll = user_motor_var.current_step * 100 / user_motor_var.max_r_step;
		direction = M_STOPED;
	}

	return direction;
}

void sg_allert_position(uint8_t val)
{
  alert_position = true;
  ws_dev_client.recv.cmd_val = val;
  ws_dev_client.recv.cmd_len = 1;
  ws_dev_client.recv.cmd = S_IO_CONTROL;
}

void set_down_end_point(void){
	user_motor_var.current_step = 0;
	user_motor_var.perc_roll = 0;
	user_motor_var.set_step = 0;
	user_motor_var.set_t_step = 0;
}

void set_up_end_point(void){
	//user_motor_var.angle_t = MAX_DEF_T_STEPS;
	user_motor_var.current_step = user_motor_var.max_r_step;
	user_motor_var.perc_roll = 100;
	user_motor_var.set_step = user_motor_var.max_r_step;
	user_motor_var.set_t_step = MAX_DEF_T_STEPS;
}

void check_alarm(void)
{
	if ((Alarm == false) && (!blind_time.b_h.work))
		Alarm = true;
	else if ((Alarm == true) && (blind_time.b_h.work >= MIN_WORK_TIMEP))
		Alarm = false;
}

void sg_conf_save_position(void)
{
	int32_t val1 = 0;
	val1 = (user_motor_var.angle_t << 16);
	val1 |= user_motor_var.current_step;

	sg_conf_lock_open_readwrite();
	sg_conf_save_height_cycles_set(val1);
	sg_conf_commit_close_unlock();
}

void save_position_per_int(void)
{
	if (!(user_motor_var.current_step % ((MAX_DEF_T_STEPS + (2*MAX_DEF_T_STEPS/6)))/2))
	{
		if (!(user_motor_var.current_step % (MAX_DEF_T_STEPS + (2*MAX_DEF_T_STEPS/6)))){
			sg_conf_save_position();
		}
		position_point = 1;
	}
}

motor_movement_t angle_direction(uint32_t tilt)
{
	motor_movement_t rez = M_SUCCESS;
	uint8_t angle = tilt / DIV_ANGLE;

	if (motor_driver_state(M_DIR_GET) == M_STOPED) {
		if (angle > user_motor_var.set_t_step) {
			ESP_LOGI(__func__, "Titl direction - UP");
			if (motor_driver_state(M_DIR_GET) != M_DIR_UP)
				motor_driver_state(M_STOPED);
			rez = M_DIR_UP;
			user_motor_var.set_step += (angle
					- user_motor_var.set_t_step);
		} else if (angle < user_motor_var.set_t_step) {
			ESP_LOGI(__func__, "Titl direction - DOWN");
			if (motor_driver_state(M_DIR_GET) != M_DIR_DOWN)
				motor_driver_state(M_STOPED);
			rez = M_DIR_DOWN;
			user_motor_var.set_step -= (user_motor_var.set_t_step
					- angle);
		}
		user_motor_var.angle_t = angle;
		ESP_LOGI(__func__, "Current tilt = %d",
				user_motor_var.set_t_step);
	}
	return rez;
}

motor_movement_t roll_direction()
{
	motor_movement_t rez = M_SUCCESS;
	if (ws_dev_client.recv.cmd_val == 101){
		ESP_LOGI(__func__, "#Special cmd : Curent: Percent = %d,Step = %d",
						user_motor_var.perc_roll, user_motor_var.set_step);
			ESP_LOGI(__func__, "Direction - UP");
			if (motor_driver_state(M_DIR_GET) != M_DIR_UP)
				motor_driver_state(M_STOPED);
			rez = M_DIR_UP;
			user_motor_var.set_step = 100;
	}
	else if (ws_dev_client.recv.cmd_val == 102)
	{
		ESP_LOGI(__func__, "#Special cmd : CURR: Percent = %d,Step = %d",
										user_motor_var.perc_roll, user_motor_var.set_step);
		ESP_LOGI(__func__, "Direction - DOWN");
		if (motor_driver_state(M_DIR_GET) != M_DIR_DOWN)
			motor_driver_state(M_STOPED);
		rez = M_DIR_DOWN;
		user_motor_var.set_step = 0;
	}
	else {
		//ws_dev_client.recv.cmd_val &= 0x000000FF;
		ESP_LOGI(__func__, "Curent: Percent = %d,Step = %d",
				user_motor_var.perc_roll, user_motor_var.set_step);
		if (ws_dev_client.recv.cmd_val > user_motor_var.perc_roll) {
			ESP_LOGI(__func__, "Direction - UP");
			if (motor_driver_state(M_DIR_GET) != M_DIR_UP)
				motor_driver_state(M_STOPED);
			rez = M_DIR_UP;
			user_motor_var.set_step = ws_dev_client.recv.cmd_val;
		} else if (ws_dev_client.recv.cmd_val < user_motor_var.perc_roll) {
			ESP_LOGI(__func__, "Direction - DOWN");
			if (motor_driver_state(M_DIR_GET) != M_DIR_DOWN)
				motor_driver_state(M_STOPED);
			rez = M_DIR_DOWN;
			user_motor_var.set_step = ws_dev_client.recv.cmd_val;
		}
		else motor_driver_state(M_STOPED); // fix for arbitrary lifting of blinds
	}
return rez;
}

void HardmainTask(void) {
	static enum uint8_t {
		wait_movement = 0,
		research_movement,
		down,
		up,
		time_out,
		stop,
		//motor calibration cases
		init,
		down_init,
		time_out_init,
		up_init,
		saving_parameters,
		//do nothing without hall sensor signals
		no_hall_sens
	} State = wait_movement;
	static uint8_t reState = stop;
	static uint8_t move_k = 0;
	check_ota_start();
	check_alarm();
	get_power_per_int();
	switch (State) {
		case no_hall_sens:
		//ESP_LOGI(__func__, "Init No hall sens");
		  {
			static bool add_msg = 0;
			if (!add_msg)
			{
			  if (ws_dev_client.recv.cmd == S_IO_CONTROL) {
				  ws_dev_client.recv.cmd_val = 0;
				  ws_dev_client.recv.cmd_len = 0;
				  ws_dev_client.recv.cmd = JSON_EMPTY_CMD;
			  }
			  coralogix_add_msg(rtc_timestamp/1000,els_error,2,eli_timeout,0);
			  leds_command(status_cal_fail);
			  add_msg = 1;
			}
		  }
			break;

		case init:
			//ESP_LOGI(__func__, "Init direction - DOWN");

			if (gpio_get_level(BUTTON) != 0) leds_command(status_cal_stage);
			if (motor_driver_state(M_DIR_GET) != M_STOPED)
				motor_driver_state(M_STOPED);
			State = down_init;
			reState = time_out_init;
			user_motor_var.set_step = 0;
			user_motor_var.current_step = 10000;
			break;

		case down_init:
		    //ESP_LOGI(__func__, "Init DOWN point");
			if (blind_time.b_h.prot){
				break;}
			motor_driver_state(M_DIR_DOWN);
			blind_time.b_h.move = STEP_TIME_OUT * 3;
			State = reState;
			reState = up_init;
			break;

		case up_init:
		    //ESP_LOGI(__func__, "Init UP point");
			if (blind_time.b_h.prot){
				break;}
			motor_driver_state(M_DIR_UP);
			blind_time.b_h.move = STEP_TIME_OUT * 3;
			State = reState;
			reState = saving_parameters;
			break;

		case time_out_init:
		    //ESP_LOGI(__func__, "Init Time out");
			if (!blind_time.b_h.move && !hall_ticks) {
				if (reState == up_init) {
					//ESP_LOGI(__func__, "Hall tick fail - DOWN");
					motor_driver_state(M_STOPED);

					user_motor_var.set_step = 10000;
					user_motor_var.current_step = 0;
					State = reState;
					reState = time_out_init;
					break;
				} else {
					//ESP_LOGI(__func__, "Hall sensor no found");
					motor_driver_state(M_STOPED);
					State = no_hall_sens;
					break;
				}

			}
			if (!hall_ticks)
				//ESP_LOGI(__func__, "hall_ticks: %d", hall_ticks);
				//ESP_LOGI(__func__, "motor_feedback: %d", motor_feedback);
				break;
			if (!blind_time.b_h.move) {

				if (reState == up_init) {
					motor_driver_state(M_STOPED);

					user_motor_var.set_step = 10000;
					user_motor_var.current_step = 0;
					State = reState;
					reState = time_out_init;
					break;
				} else {

					user_motor_var.max_r_step = hall_ticks;
					motor_driver_state(M_STOPED);
					//ESP_LOGI(__func__, "Max = %d", user_motor_var.max_r_step);
					user_motor_var.set_step = user_motor_var.current_step =
							user_motor_var.max_r_step;
					blind_time.b_c.telemetry = 10000;
					State = reState;
					{
						user_motor_var.angle_t = user_motor_var.set_t_step =
								user_motor_var.max_t_step;
						sg_conf_save_position();
					}
					break;
				}
			}

			break;

		case saving_parameters:
		    //ESP_LOGI(__func__, "Init Save parameters");
			if (user_motor_var.max_r_step && !blind_time.b_c.telemetry) {
				sg_conf_lock_open_readwrite();
				sg_conf_user_height_cycles_set(user_motor_var.max_r_step);
				sg_conf_height_cycles_set(user_motor_var.max_r_step);
				sg_conf_commit_close_unlock();
				State = wait_movement;
				leds_command(status_cal_ok);
			}
			break;

		case wait_movement:
		    //ESP_LOGI(__func__, "Init Wait movement");
			if (!user_motor_var.max_r_step) {
				State = init;
				break;
			}
			if ((user_motor_var.set_t_step != user_motor_var.angle_t)
					&& (ws_dev_client.recv.cmd == JSON_EMPTY_CMD)
					&& (!blind_time.b_h.move)
					&& (user_state != JSON_STATE_MAX_EFFICIENCY)
					&& (user_motor_var.condition == false)) {
				ws_dev_client.recv.cmd = S_IO_CONTROL;
				ws_dev_client.recv.cmd_len = 2;
				ws_dev_client.recv.cmd_val = user_motor_var.angle_t * DIV_ANGLE;
				//ESP_LOGI(__func__, "Return angle");
			}

			if (ws_dev_client.recv.cmd == JSON_EMPTY_CMD)
				break;
			if (ws_dev_client.recv.cmd == S_IO_UPDATE)
				break;
			if (ws_dev_client.recv.cmd == S_IO_RESET)
				break;
			if (ws_dev_client.recv.cmd == S_IO_CONTROL) {
				coralogix_add_msg(rtc_timestamp/1000,els_info,1,eli_ctrl_cmd,ws_dev_client.recv.cmd);
				//ESP_LOGI(__func__, "Scene = %d",user_state);
				State = research_movement;
				reState = wait_movement;
				ws_dev_client.recv.cmd = JSON_EMPTY_CMD;
			}

			break;

		case research_movement:
		    //ESP_LOGI(__func__, "Init Reserch movement");
			State = wait_movement;
			bool tilt = 0;
			motor_movement_t direct;
			user_motor_var.perc_roll = user_motor_var.current_step * 100
					/ user_motor_var.max_r_step;

			if (user_motor_var.current_step <= (user_motor_var.max_t_step*2))
				tilt = 1;
			else if ((user_motor_var.current_step <= (user_motor_var.max_r_step - (user_motor_var.max_t_step * 2))))
				tilt = 1;
			else if (ws_dev_client.recv.cmd_len == 2){
				//user_motor_var.set_t_step = user_motor_var.angle_t;
				user_motor_var.condition = true;
				coralogix_add_msg(rtc_timestamp/1000,els_war,elpf_tilt_only,eli_titl,user_motor_var.angle_t * DIV_ANGLE);
			}

			if ((ws_dev_client.recv.cmd_len == 2)&& tilt){ //tilt only

				// ESP_LOGI(__func__, "Resived tilt(only) = %d",
				// 		ws_dev_client.recv.cmd_val);
				coralogix_add_msg(rtc_timestamp/1000,els_info,elpf_tilt_only,eli_tilt_only,ws_dev_client.recv.cmd_val);
				direct = angle_direction(ws_dev_client.recv.cmd_val);
				if (direct == M_DIR_UP)
				{
					State = up;
					reState = time_out;
				}
				else if (direct == M_DIR_DOWN)
				{
					State = down;
					reState = time_out;
				}
				move_k = 3;
				//ESP_LOGI(__func__, "Set tilt = %d, state %d", user_motor_var.angle_t,motor_driver_state(M_DIR_GET));
			} else if ((ws_dev_client.recv.cmd_len == 3)
					&& (user_motor_var.current_step
							<= (user_motor_var.max_r_step
									- (user_motor_var.max_t_step * 2)))) //tilt + roll
					{
				uint16_t angle = (ws_dev_client.recv.cmd_val >> 8);
				//ESP_LOGI(__func__, "Resived tilt(roll) = %d", angle);
				user_motor_var.angle_t = angle / DIV_ANGLE;
				coralogix_add_msg(rtc_timestamp/1000,els_info,elpf_tilt,eli_titl,angle);
			}

			if (ws_dev_client.recv.cmd_len != 2){ //roll
				ws_dev_client.recv.cmd_val &= 0x000000FF;
				coralogix_add_msg(rtc_timestamp/1000,els_info,elpf_roll,eli_roll,ws_dev_client.recv.cmd_val);
				direct = roll_direction() ;
				if (direct== M_DIR_UP)
				{
					State = up;
					reState = time_out;
				}
				else if (direct== M_DIR_DOWN)
				{
					State = down;
					reState = time_out;
				}
				move_k = 2;
				user_motor_var.condition = false;
				user_motor_var.set_step = user_motor_var.set_step
						* user_motor_var.max_r_step / 100;
				// ESP_LOGI(__func__, "SET: Percent = %d,Step = %d",
				// 		user_motor_var.perc_roll, user_motor_var.set_step);

			}

			ws_dev_client.recv.cmd_val = 0;
			ws_dev_client.recv.cmd_len = 0;
			ws_dev_client.recv.cmd = JSON_EMPTY_CMD;
			break;

		case down:
			CS_RESP = c_s_checking;
			blind_time.b_h.tilt = 0;
			if (blind_time.b_h.prot)
				break;
			motor_driver_state(M_DIR_DOWN);
			blind_time.b_h.move = STEP_TIME_OUT * move_k;
			if (reState == down)
				reState = stop;
			State = reState;
			coralogix_add_msg(rtc_timestamp/1000,els_info,elpf_down_dir,eli_down,user_motor_var.set_step);
			break;

		case up:
			CS_RESP = c_s_checking;
			blind_time.b_h.tilt = 0;
			if (blind_time.b_h.prot)
				break;
			motor_driver_state(M_DIR_UP);
			blind_time.b_h.move = STEP_TIME_OUT * move_k;
			if (reState == up)
				reState = stop;
			State = reState;
			coralogix_add_msg(rtc_timestamp/1000,els_info,elpf_up_dir,eli_up,user_motor_var.set_step);
			break;

		case stop:
		    //ESP_LOGI(__func__, "Stop");
			save_position_per_int();

			if (!blind_time.b_h.move) {
				//ESP_LOGI(__func__, "mode tiomeout in waiting stop state");
				motor_driver_state(M_STOPED);
				if (user_motor_var.set_step == 0) //down end point
						{
					set_down_end_point();
					//ESP_LOGI(__func__, "set_down_end_point");
					coralogix_add_msg(rtc_timestamp/1000,els_info,elpf_dep,eli_dep,0);
				} else if (user_motor_var.set_step == user_motor_var.max_r_step) //up end point
						{
					set_up_end_point();
					//ESP_LOGI(__func__, "set_up_end_point");
					coralogix_add_msg(rtc_timestamp/1000,els_info,elpf_uep,eli_uep,0);
				}
			}

			if (user_state == JSON_STATE_MAX_EFFICIENCY){

			}

			if ((motor_driver_state(M_DIR_GET) != M_STOPED)
					&& (ws_dev_client.recv.cmd == JSON_EMPTY_CMD))
				break;
			/*check tilt only*/
			if (ws_dev_client.recv.cmd == S_IO_CONTROL &&
						ws_dev_client.recv.cmd_len == 2
						&& (motor_driver_state(M_DIR_GET) != M_STOPED))
			break;
			// ESP_LOGI(__func__, "Ticks = %d,Set steps = %d,current_steps = %d,tilt_time = %d",
			// 		hall_ticks, user_motor_var.set_step,
			// 		user_motor_var.current_step,blind_time.b_h.tilt);

			if (ws_dev_client.recv.cmd == S_IO_CONTROL) {
				State = research_movement;
				reState = wait_movement;
				ws_dev_client.recv.cmd = JSON_EMPTY_CMD;
				break;
			} else if (ws_dev_client.recv.cmd != JSON_EMPTY_CMD)
				break;
			State = wait_movement;
			{
				sg_conf_save_position();
				position_point = 1;
				//ESP_LOGI(__func__, "Position saved");
				coralogix_add_msg(rtc_timestamp/1000,els_info,2,eli_check_stop,user_motor_var.set_step);
			}
			break;

		case time_out:
		    //ESP_LOGI(__func__, "Time out");
			if (motor_driver_state(M_DIR_GET) == M_STOPED){
				CS_RESP = c_s_success;
				control_point = 1;
				coralogix_add_msg(rtc_timestamp/1000,els_info,1,eli_check_stop,0);
				State = stop;
			}
			if (!blind_time.b_h.move) {
				uint8_t redirect = 102;
				motor_driver_state(M_STOPED);
				if (user_motor_var.set_step > user_motor_var.current_step)
					redirect = 102;
				else if (user_motor_var.set_step < user_motor_var.current_step)
					redirect = 101;
				user_motor_var.set_step = user_motor_var.current_step;
				user_motor_var.angle_t = user_motor_var.set_t_step;
				State = wait_movement;
				//ESP_LOGI(__func__, "Timout");
				CS_RESP = c_s_halt;
				control_point = 1;
				coralogix_add_msg(rtc_timestamp/1000,els_error,1,eli_timeout,0);
				if (!alert_position)
					sg_allert_position(redirect);
				else if (alert_position == true)
					alert_position = false;
			}
			if (!hall_ticks)
				break;
			CS_RESP = c_s_success;
			control_point = 1;
			coralogix_add_msg(rtc_timestamp/1000,els_info,1,eli_check_stop,0);
			State = stop;
			break;
	}
}