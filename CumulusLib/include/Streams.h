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
#include "Listener.h"
#include "Subscription.h"
#include <set>


namespace Cumulus {


class Streams {
public:
	Streams();
	virtual ~Streams();

	Poco::UInt32	create();
	void			destroy(Poco::UInt32 id);

	bool			publish(Poco::UInt32 id,const std::string& name);
	void			unpublish(Poco::UInt32 id,const std::string& name);
	void			subscribe(const std::string& name,Listener& listener);
	void			unsubscribe(const std::string& name,Listener& listener);

	Subscription*	subscription(Poco::UInt32 id);

private:
	typedef std::map<std::string,Subscription*>::iterator SubscriptionIt;

	SubscriptionIt  subscriptionIt(const std::string& name);
	void			cleanSubscription(SubscriptionIt& it);

	std::set<Poco::UInt32>				_streams;
	std::map<std::string,Subscription*>	_subscriptions;
	Poco::UInt32						_nextId;

};


} // namespace Cumulus
