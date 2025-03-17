
// setup GPIO board before running with base speed and acceleration
void setup(double speed, int acceleration);

// run main motors loop. Only will return when user closes
void receive_commands();

// cleanup GPIO and shutdown program
void cleanup();