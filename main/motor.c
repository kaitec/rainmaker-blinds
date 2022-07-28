#include <stdint.h>
#include <stdbool.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_system.h>
#include "hardware.h"
#include "flash.h"
#include "motor.h"

motor_t user_motor_var;
recivcmd_t reciv;
bool position_point; //set when need send notification to server? payload are current position
bool Alarm = false; //set when corrupt motor timing recommendations

uint32_t hall_ticks = 0; // feedback from hall sensor
uint32_t esp_logi_ticks = 0; //for debug

bool control_point = 0; // TODO ?
bool reset_point = 0;   // TODO ?

bool alert_position = 0;
//bool motor_feedback = !FB_IN_MOTION;

C_STATUS_CODE_t CS_RESP = c_s_success;

uint16_t feedback_timer_counter=0;
bool motor_feedback = !FB_IN_MOTION;

void set_blind(uint8_t len, uint8_t val)
{
		reciv.cmd=S_IO_CONTROL;
		reciv.cmd_len=len;
		reciv.cmd_val=val;
}

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
		if (ON_LEVEL){
			gpio_set_level(UP_DIR, 1);
		}
		else
			gpio_set_level(UP_DIR, 0);
		direction = M_DIR_UP;
	}
	if (state == M_DIR_DOWN) { 
		if (ON_LEVEL){
			gpio_set_level(DOWN_DIR, 1);
		}
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
		if (user_motor_var.max_r_step){
			user_motor_var.perc_roll = user_motor_var.current_step * 100 / user_motor_var.max_r_step;
		}
		direction = M_STOPED;
	}
	return direction;
}

void sg_allert_position(uint8_t val)
{
  alert_position = true;
  reciv.cmd_val = val;
  reciv.cmd_len = 1;
  reciv.cmd = S_IO_CONTROL;
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
	// if ((Alarm == false) && (!blind_time.b_h.work))
	// 	Alarm = true;
	// else if ((Alarm == true) && (blind_time.b_h.work >= MIN_WORK_TIMEP))
	// 	Alarm = false;
}

void sg_conf_save_position(void)
{
	// int32_t val1 = 0;
	// val1 = (user_motor_var.angle_t << 16);
	// val1 |= user_motor_var.current_step;

	// sg_conf_lock_open_readwrite();
	// sg_conf_save_height_cycles_set(val1);
	// sg_conf_commit_close_unlock();
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

void timer_function(void)
{
     if(feedback_timer_counter < PSEVDO_HALL) 
     {
       if(motor_feedback == FB_IN_MOTION) feedback_timer_counter++;
     }
     else
     {
       feedback_timer_counter=0;
       motor_HallFb_function();
     }
    /******************************************************************/
    if (blind_time.b_p.search_inetrval) blind_time.b_p.search_inetrval--;

    if (blind_time.b_p.obtain_ping) blind_time.b_p.obtain_ping--;

    if  (blind_time.b_h.move) blind_time.b_h.move--;
    if  (blind_time.b_h.prot) blind_time.b_h.prot--;

    if  (blind_time.b_h.rest && !hall_ticks)
    {
      blind_time.b_h.rest--;
      if  (!(blind_time.b_h.rest%15)) blind_time.b_h.work++;
    }
    if  ((blind_time.b_h.work) && hall_ticks)
    {
      blind_time.b_h.work--;
      blind_time.b_h.rest+=15;
    }
}

void motor_HallFb_function(void)
{
	blind_time.b_h.move = STEP_TIME_OUT;
	esp_logi_ticks++;
	if (user_motor_var.set_step != user_motor_var.current_step) {
		hall_ticks++;
		if (motor_driver_state(M_DIR_GET) == M_DIR_UP) {
			user_motor_var.current_step++;
			if (user_motor_var.set_t_step < user_motor_var.max_t_step)
				user_motor_var.set_t_step++;
		} else if (motor_driver_state(M_DIR_GET) == M_DIR_DOWN) {
			user_motor_var.current_step--;
			if (user_motor_var.set_t_step)
				user_motor_var.set_t_step--;
		}
	}
	if ((user_motor_var.set_step == user_motor_var.current_step)
			&& ((user_motor_var.set_step != 0)
					&& (user_motor_var.set_step != user_motor_var.max_r_step)))
		motor_driver_state(M_STOPED);
}

motor_movement_t angle_direction(uint32_t tilt)
{
	motor_movement_t rez = M_SUCCESS;
	uint8_t angle = tilt / DIV_ANGLE;

	if (motor_driver_state(M_DIR_GET) == M_STOPED) {
		if (angle > user_motor_var.set_t_step) {
			//ESP_LOGI(__func__, "Titl direction - UP");
			if (motor_driver_state(M_DIR_GET) != M_DIR_UP)
				motor_driver_state(M_STOPED);
			rez = M_DIR_UP;
			user_motor_var.set_step += (angle
					- user_motor_var.set_t_step);
		} else if (angle < user_motor_var.set_t_step) {
			//ESP_LOGI(__func__, "Titl direction - DOWN");
			if (motor_driver_state(M_DIR_GET) != M_DIR_DOWN)
				motor_driver_state(M_STOPED);
			rez = M_DIR_DOWN;
			user_motor_var.set_step -= (user_motor_var.set_t_step
					- angle);
		}
		user_motor_var.angle_t = angle;
		//ESP_LOGI(__func__, "Current tilt = %d",	user_motor_var.set_t_step);
	}
	return rez;
}

motor_movement_t roll_direction()
{
	motor_movement_t rez = M_SUCCESS;
	if (reciv.cmd_val == 101){
		//ESP_LOGI(__func__, "#Special cmd : Curent: Percent = %d,Step = %d",	user_motor_var.perc_roll, user_motor_var.set_step);
			//ESP_LOGI(__func__, "Direction - UP");
			if (motor_driver_state(M_DIR_GET) != M_DIR_UP)
				motor_driver_state(M_STOPED);
			rez = M_DIR_UP;
			user_motor_var.set_step = 100;
	}
	else if (reciv.cmd_val == 102)
	{
		//ESP_LOGI(__func__, "#Special cmd : CURR: Percent = %d,Step = %d",	user_motor_var.perc_roll, user_motor_var.set_step);
		//ESP_LOGI(__func__, "Direction - DOWN");
		if (motor_driver_state(M_DIR_GET) != M_DIR_DOWN)
			motor_driver_state(M_STOPED);
		rez = M_DIR_DOWN;
		user_motor_var.set_step = 0;
	}
	else {
		//reciv.cmd_val &= 0x000000FF;
		//ESP_LOGI(__func__, "Curent: Percent = %d,Step = %d",	user_motor_var.perc_roll, user_motor_var.set_step);
		if (reciv.cmd_val > user_motor_var.perc_roll) {
			//ESP_LOGI(__func__, "Direction - UP");
			if (motor_driver_state(M_DIR_GET) != M_DIR_UP)
				motor_driver_state(M_STOPED);
			rez = M_DIR_UP;
			user_motor_var.set_step = reciv.cmd_val;
		} else if (reciv.cmd_val < user_motor_var.perc_roll) {
			//ESP_LOGI(__func__, "Direction - DOWN");
			if (motor_driver_state(M_DIR_GET) != M_DIR_DOWN)
				motor_driver_state(M_STOPED);
			rez = M_DIR_DOWN;
			user_motor_var.set_step = reciv.cmd_val;
		}
		else motor_driver_state(M_STOPED); // fix for arbitrary lifting of blinds
	}
return rez;
}

void HardmainTask(void) {
	static enum uint8_t {
		wait_movement = 0,
		research_movement, /*1*/
		down,              /*2*/
		up,                /*3*/
		time_out,          /*4*/
		stop,              /*5*/
		init,              /*6*/
		down_init,         /*7*/
		time_out_init,     /*8*/
		up_init,           /*9*/
		saving_parameters, /*10*/
		no_hall_sens       /*11*/
	} State = wait_movement;
	static uint8_t reState = stop;
	static uint8_t move_k = 0;
	//static uint16_t cnt=0;
	check_alarm();

		 /******* For debug **********/ 
     // cnt++;
     // if(cnt>50){
     // 	 ESP_LOGI(__func__, "State = %d,	b_h.prot = %d,	b_h.move = %d", State, blind_time.b_h.prot, blind_time.b_h.move);
     // 	 cnt=0;}
	   /******** END ***************/

	switch (State) {
		case no_hall_sens:
		if(DEBUG==MOTOR) ESP_LOGI(__func__, "Init No hall sens");
		  {
			static bool add_msg = 0;
			if (!add_msg)
			{
			  if (reciv.cmd == S_IO_CONTROL) {
				  reciv.cmd_val = 0;
				  reciv.cmd_len = 0;
				  reciv.cmd = JSON_EMPTY_CMD;
			  }
			  add_msg = 1;
			}
		  }
			break;

		case init:
			if(DEBUG==MOTOR) ESP_LOGI(__func__, "Init direction - DOWN");

			if (motor_driver_state(M_DIR_GET) != M_STOPED)
				motor_driver_state(M_STOPED);
			State = down_init;
			reState = time_out_init;
			user_motor_var.set_step = 0;
			user_motor_var.current_step = 10000;
			break;

		case down_init:
		  if(DEBUG==MOTOR) ESP_LOGI(__func__, "Init DOWN point");
			if (blind_time.b_h.prot){
				break;}
			motor_driver_state(M_DIR_DOWN);
			blind_time.b_h.move = STEP_TIME_OUT * 3;
			State = reState;
			reState = up_init;
			break;

		case up_init:
		  if(DEBUG==MOTOR) ESP_LOGI(__func__, "Init UP point");
			if (blind_time.b_h.prot){
				break;}
			motor_driver_state(M_DIR_UP);
			blind_time.b_h.move = STEP_TIME_OUT * 3;
			State = reState;
			reState = saving_parameters;
			break;

		case time_out_init:
		  if(DEBUG==MOTOR) ESP_LOGI(__func__, "Init Time out");
			if (!blind_time.b_h.move && !hall_ticks) {
				if (reState == up_init) {
					if(DEBUG==MOTOR) ESP_LOGI(__func__, "Hall tick fail - DOWN");
					motor_driver_state(M_STOPED);

					user_motor_var.set_step = 10000;
					user_motor_var.current_step = 0;
					State = reState;
					reState = time_out_init;
					break;
				} else {
					if(DEBUG==MOTOR) ESP_LOGI(__func__, "Hall sensor no found");
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
					user_motor_var.set_step = user_motor_var.current_step =
							user_motor_var.max_r_step;
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
			if (user_motor_var.max_r_step) {
				// TODO add save parameter
				// sg_conf_lock_open_readwrite();
				// sg_conf_user_height_cycles_set(user_motor_var.max_r_step);
				// sg_conf_height_cycles_set(user_motor_var.max_r_step);
				// sg_conf_commit_close_unlock();
				State = wait_movement;
			}
			break;

		case wait_movement:
		  if(DEBUG==MOTOR) ESP_LOGI(__func__, "Init Wait movement");
			if (!user_motor_var.max_r_step) {
				State = init;
				break;
			}
			if ((user_motor_var.set_t_step != user_motor_var.angle_t)
					&& (reciv.cmd == JSON_EMPTY_CMD)
					&& (!blind_time.b_h.move)
					&& (user_motor_var.condition == false)) {
				reciv.cmd = S_IO_CONTROL;
				reciv.cmd_len = 2;
				reciv.cmd_val = user_motor_var.angle_t * DIV_ANGLE;
				//ESP_LOGI(__func__, "Return angle");
			}

			if (reciv.cmd == JSON_EMPTY_CMD)
				break;

			if (reciv.cmd == S_IO_CONTROL) {
				State = research_movement;
				reState = wait_movement;
				reciv.cmd = JSON_EMPTY_CMD;
			}

			break;

		case research_movement:
		  if(DEBUG==MOTOR) ESP_LOGI(__func__, "Init Reserch movement");
			State = wait_movement;
			bool tilt = 0;
			motor_movement_t direct;
			user_motor_var.perc_roll = user_motor_var.current_step * 100
					/ user_motor_var.max_r_step;

			if (user_motor_var.current_step <= (user_motor_var.max_t_step*2))
				tilt = 1;
			else if ((user_motor_var.current_step <= (user_motor_var.max_r_step - (user_motor_var.max_t_step * 2))))
				tilt = 1;
			else if (reciv.cmd_len == 2){
				//user_motor_var.set_t_step = user_motor_var.angle_t;
				user_motor_var.condition = true;
			}

			if ((reciv.cmd_len == 2)&& tilt){ //tilt only
				direct = angle_direction(reciv.cmd_val);
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
			} else if ((reciv.cmd_len == 3)
					&& (user_motor_var.current_step
							<= (user_motor_var.max_r_step
									- (user_motor_var.max_t_step * 2)))) //tilt + roll
					{
				uint16_t angle = (reciv.cmd_val >> 8);
				user_motor_var.angle_t = angle / DIV_ANGLE;
			}

			if (reciv.cmd_len != 2){ //roll
				reciv.cmd_val &= 0x000000FF;
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
			}

			reciv.cmd_val = 0;
			reciv.cmd_len = 0;
			reciv.cmd = JSON_EMPTY_CMD;
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
			break;

		case stop:
		  if(DEBUG==MOTOR) ESP_LOGI(__func__, "Stop");
			save_position_per_int();

			if (!blind_time.b_h.move) {
				motor_driver_state(M_STOPED);
				if (user_motor_var.set_step == 0) //down end point
						{
					set_down_end_point();
				} else if (user_motor_var.set_step == user_motor_var.max_r_step) //up end point
						{
					set_up_end_point();
				}
			}

			if ((motor_driver_state(M_DIR_GET) != M_STOPED)
					&& (reciv.cmd == JSON_EMPTY_CMD))
				break;
			/*check tilt only*/
			if (reciv.cmd == S_IO_CONTROL &&
						reciv.cmd_len == 2
						&& (motor_driver_state(M_DIR_GET) != M_STOPED))
			break;
			// ESP_LOGI(__func__, "Ticks = %d,Set steps = %d,current_steps = %d,tilt_time = %d",
			// 		hall_ticks, user_motor_var.set_step,
			// 		user_motor_var.current_step,blind_time.b_h.tilt);

			if (reciv.cmd == S_IO_CONTROL) {
				State = research_movement;
				reState = wait_movement;
				reciv.cmd = JSON_EMPTY_CMD;
				break;
			} else if (reciv.cmd != JSON_EMPTY_CMD)
				break;
			State = wait_movement;
			{
				sg_conf_save_position();
				position_point = 1;
			}
			break;

		case time_out:
		  if(DEBUG==MOTOR) ESP_LOGI(__func__, "Time out");
			if (motor_driver_state(M_DIR_GET) == M_STOPED){
				CS_RESP = c_s_success;
				control_point = 1;
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
				CS_RESP = c_s_halt;
				control_point = 1;
				if (!alert_position)
					sg_allert_position(redirect);
				else if (alert_position == true)
					alert_position = false;
			}
			if (!hall_ticks)
				break;
			CS_RESP = c_s_success;
			control_point = 1;
			State = stop;
			break;
	}
}

void load_position(void)
{
  // int32_t val1 = 0;
  // sg_conf_user_height_cycles_get(&val1);
  // ESP_LOGI(__func__, "user height: %d", val1);
  // user_motor_var.max_r_step = val1;
  // sg_conf_height_cycles_get(&val1);
  // ESP_LOGI(__func__, "height: %d", val1);
  // if(user_motor_var.max_r_step)
  // {
  //   sg_conf_save_height_cycles_get(&val1);
  //   user_motor_var.set_step = user_motor_var.current_step = val1&0x0000FFFF;
  //   user_motor_var.perc_roll = user_motor_var.current_step*100/user_motor_var.max_r_step;
  //   user_motor_var.angle_t = user_motor_var.set_t_step = val1>>16;
  //   ESP_LOGI(__func__, "save height: %d", val1);
  //   ESP_LOGI(__func__, "current height: %d", user_motor_var.current_step);
  //   ESP_LOGI(__func__, "current angle: %d", user_motor_var.set_t_step);
  // }
}

void reset_movement_variables(void)
{
	user_motor_var.max_t_step = MAX_DEF_T_STEPS;

	blind_time.b_h.rest = 0;
	blind_time.b_h.work = MAX_WORK_TIMEP;
	position_point = 0;

	control_point = 0;
  reset_point = 0;

	load_position();
}

void motor_init(void)
{
	reset_movement_variables();
}