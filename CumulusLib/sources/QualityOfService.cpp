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

#include "QualityOfService.h"
#include "Logs.h"

using namespace Poco;
using namespace std;

namespace Cumulus {

class Sample {
public:
	Sample(UInt32 time,UInt32 received,UInt32 lost) : time(time),received(received),lost(lost) {}
	~Sample() {}
	const UInt32 time;
	const UInt32 received;
	const UInt32 lost;
};

QualityOfService::QualityOfService() : lostRate(0),latency(0),_prevTime(0),droppedFrames(0),_num(0),_den(0) {
}


QualityOfService::~QualityOfService() {
	reset();
}

void QualityOfService::add(UInt32 time,UInt32 received,UInt32 lost) {

	if(_prevTime>0) {
		if(time>=_prevTime) {
			UInt32 delta = time-_prevTime;
			UInt32 deltaReal =  UInt32(_reception.elapsed()/1000);
			Int64 result = latency+(Int64)deltaReal-delta;
			if(result<0)
				result=0;
			(UInt32&)latency = (UInt32)result;
			_reception.update();
		} else {
			ERROR("QoS computing with a error time value (%u) inferiors than precedent time (%u)",time,_prevTime);
			time = _prevTime;
		}
	}

	_prevTime=time;
	_num += lost;
	_den += (lost+received);
	
	Sample* pSample = new Sample(time,received,lost);
		
	list<Sample*>::iterator it=_samples.begin();
	
	UInt32 boundTime = time<10000 ? 0 : (time-10000);

	while(it!=_samples.end()) {
		Sample& sample(**it);
		if(sample.time>=boundTime) // 10 secondes
			break;
		_den -= (sample.received+sample.lost);
		_num -= sample.lost;
		delete *it;
		_samples.erase(it++);
	}
	//TRACE("_samples.size()=%u",_samples.size());
	
	if(_den==0)
		ERROR("Lost rate computing with a impossible null number of fragments received")
	else
		(double&)lostRate = (double)_num/_den;

	_samples.push_back(pSample);
}

void QualityOfService::reset() {
	(double&)lostRate = 0;
	(UInt32&)latency = 0;
	(UInt32&)droppedFrames = 0;
	_num=_den=_prevTime=0;
	list<Sample*>::iterator it;
	for(it=_samples.begin();it!=_samples.end();++it)
		delete (*it);
	_samples.clear();
}


} // namespace Cumulus
