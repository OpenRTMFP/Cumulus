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

#include "Streams.h"
#include "Logs.h"

using namespace std;
using namespace Poco;

namespace Cumulus {

Streams::Streams() : _nextId(0) {
	
}

Streams::~Streams() {
	// delete subscriptions
	SubscriptionIt it;
	for(it=_subscriptions.begin();it!=_subscriptions.end();++it)
		delete it->second;
}

Streams::SubscriptionIt Streams::subscriptionIt(const string& name) {
	// Return a suscription iterator and create the subscrition if it doesn't exist
	SubscriptionIt it = _subscriptions.find(name);
	if(it != _subscriptions.end())
		return it;
	return _subscriptions.insert(pair<string,Subscription*>(name,new Subscription())).first;
}

void Streams::cleanSubscription(SubscriptionIt& it) {
	// Delete susbscription is no more need
	if(it->second->count()==0 && it->second->idPublisher==0)
		_subscriptions.erase(it);
}

bool Streams::publish(UInt32 id,const string& name) {
	SubscriptionIt it = subscriptionIt(name);
	if(it->second->idPublisher!=0)
		return false; // has already a publisher
	((UInt8&)it->second->idPublisher) = id;
	return true;
}

void Streams::unpublish(UInt32 id,const string& name) {
	SubscriptionIt it = subscriptionIt(name);
	if(it->second->idPublisher!=id) {
		WARN("Unpublish '%s' operation with a '%d' id different than its publisher '%d' id",name.c_str(),id,it->second->idPublisher);
		return;
	}
	((UInt8&)it->second->idPublisher) = 0;
	cleanSubscription(it);
}

void Streams::subscribe(const string& name,Listener& listener) {
	subscriptionIt(name)->second->add(listener);
}

void Streams::unsubscribe(const string& name,Listener& listener) {
	SubscriptionIt it = subscriptionIt(name);
	it->second->remove(listener);
	cleanSubscription(it);
}

Subscription* Streams::subscription(UInt32 id) {
	SubscriptionIt it;
	for(it=_subscriptions.begin();it!=_subscriptions.end();++it) {
		if(it->second->idPublisher==id)
			return it->second;
	}
	return NULL;
}

UInt32 Streams::create() {
	while(!_streams.insert(++_nextId).second);
	return _nextId;
}

void Streams::destroy(UInt32 id) {
	_streams.erase(id);
	SubscriptionIt it;
	for(it=_subscriptions.begin();it!=_subscriptions.end();++it) {
		if(it->second->idPublisher==id) {
			((UInt8&)it->second->idPublisher) = 0;
			cleanSubscription(it);
		}
	}
	
}


} // namespace Cumulus
