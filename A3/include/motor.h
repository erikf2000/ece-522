
// initialize connection to GPIO to enable motor control
void start_GPIO_connection();

// setup rover with inital speed and acceleration percentages
void setup_motor(int motor, double speed, int acceleration, char *dir);

// start given motor
void start_motor(int motor);

// stop given motor
void stop_motor(int motor);

// set given motor to percentage of max speed
void motor_speed(int motor, double speed);

// close down connection to GPIO and shutdown
void shutdown_GPIO_connection();
