
// setup GPIO board before running with base speed and acceleration
void setup(double speed, int acceleration, char *dir);

// run main motors loop. Only will return when user closes
void enable_motors();

// cleanup GPIO and shutdown program
void cleanup();