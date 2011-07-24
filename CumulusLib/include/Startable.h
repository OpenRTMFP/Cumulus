/* 
	Copyright 2010 OpenRTMFP
 
	This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License received along this program for more
	details (or else see http://www.gnu.org/licenses/).

	This file is a part of Cumulus.
*/

#pragma once

#include "Cumulus.h"
#include "Poco/Thread.h"

namespace Cumulus {

class CUMULUS_API Startable : private Poco::Runnable {

public:
	void				start();
	void				stop();

	void				setPriority(Poco::Thread::Priority priority);

	bool				running() const;		
	const std::string&	name() const;

protected:
	Startable(const std::string& name);
	virtual ~Startable();

	virtual void	run(const volatile bool& terminate) = 0;
	virtual bool	prerun(); // Retourne true si le process s'est terminé de lui meme (sans un appel à stop())

private:
	void			run();

	Poco::Thread			_thread;
	Poco::FastMutex			_mutex;
	volatile bool			_terminate;
	std::string				_name;
};

inline void Startable::setPriority(Poco::Thread::Priority priority) {
	_thread.setPriority(priority);
}

inline bool Startable::running() const {
	return _thread.isRunning();
}

inline const std::string& Startable::name() const {
	return _name;
}

} // namespace Cumulus
