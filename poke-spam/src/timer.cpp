#include "Timer.h"

Timer::Timer(const Timeout &timeout)
	: _timeout(timeout)
{
}

Timer::Timer(const Timer::Timeout &timeout,
	const Timer::Interval &interval,
	bool singleShot)
	: _isSingleShot(singleShot),
	_interval(interval),
	_timeout(timeout)
{
}

void Timer::start(bool multiThread)
{
	if (this->running() == true)
		return;

	_running = true;

	if (multiThread == true) {
		_thread = std::thread(
			&Timer::_temporize, this);
	}
	else {
		this->_temporize();
	}
}

void Timer::stop()
{
	_running = false;
	_thread.join();
}
