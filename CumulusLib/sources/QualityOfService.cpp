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
#include "math.h"

using namespace Poco;
using namespace std;

namespace Cumulus {

class Sample {
public:
	Sample(Timestamp& time,UInt32 received,UInt32 lost,UInt32 size,Int64 latencyGradient) : time(time),received(received),lost(lost),size(size),latencyGradient(latencyGradient) {}
	~Sample() {}
	const Timestamp time;
	const UInt32	received;
	const UInt32	lost;
	const UInt32	size;
	const Int64		latencyGradient;
};

QualityOfService::QualityOfService() : lostRate(0),byteRate(0),latency(0),congestionRate(0),_latency(0),_prevTime(0),droppedFrames(0),_num(0),_den(0),_size(0),_latencyGradient(0),_fullSample(false) {
}


QualityOfService::~QualityOfService() {
	reset();
}

void QualityOfService::add(UInt32 time,UInt32 received,UInt32 lost,UInt32 size,UInt32 ping) {

	Int64 latencyGradient = 0;

	if(!_samples.empty()) {
		if(time>=_prevTime) {
			UInt32 delta = time-_prevTime;
			UInt32 deltaReal =  UInt32(_reception.elapsed()/1000);
			latencyGradient = (Int64)deltaReal-delta;
			_latency += latencyGradient;
		} else
			WARN("QoS computing with a error time value (%u) inferiors than precedent time (%u)",time,_prevTime);
	} else
		_latency = ping/2;
	(UInt32&)latency = _latency<0 ? 0 : (UInt32)_latency;

	_prevTime=time;
	_num += lost;
	_den += (lost+received);
	_size += size;
	_latencyGradient += latencyGradient;
	
	list<Sample*>::iterator it=_samples.begin();
	while(it!=_samples.end()) {
		Sample& sample(**it);
		if(!sample.time.isElapsed(5000000)) { // 5 secondes
			_fullSample=true;
			break;
		}
		_den -= (sample.received+sample.lost);
		_num -= sample.lost;
		_size -= sample.size;
		_latencyGradient -= sample.latencyGradient;
		delete *it;
		_samples.erase(it++);
	}
	_reception.update();
	_samples.push_back(new Sample(_reception,received,lost,size,latencyGradient));
	
	UInt32 elapsed = _fullSample ? 5000 : UInt32((*_samples.begin())->time.elapsed()/1000);

	(double&)byteRate = 0;
	double congestion = 0;
	if(elapsed>0) {
		(double&)byteRate = (double)_size/elapsed*1000;
		congestion = (double)_latencyGradient/elapsed;
	}

	if(_den==0)
		ERROR("Lost rate computing with a impossible null number of fragments received")
	else {
		(double&)lostRate = (double)_num/_den;
		congestion += lostRate;
	}

	(double&)congestionRate = congestion>1 ? 1 : (congestion<-1 ? -1 : congestion);
}

void QualityOfService::reset() {
	(double&)lostRate = 0;
	(double&)byteRate = 0;
	(double&)congestionRate = 0;
	(UInt32&)latency = 0;
	(UInt32&)droppedFrames = 0;
	_fullSample=false;
	_latencyGradient=_latency=0;
	_size=_num=_den=_prevTime=0;
	list<Sample*>::iterator it;
	for(it=_samples.begin();it!=_samples.end();++it)
		delete (*it);
	_samples.clear();
}


} // namespace Cumulus
