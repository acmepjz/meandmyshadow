#ifndef TIMER_H
#define TIMER_H

class Timer
{
private:
	int ticks;

public:
	void start();

	int get_ticks();
};

#endif