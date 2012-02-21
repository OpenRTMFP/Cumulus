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
	Sample(UInt32 time,UInt32 received,UInt32 lost,UInt32 size,UInt32 latency) : time(time),received(received),lost(lost),size(size),latency(latency) {}
	~Sample() {}
	const UInt32 time;
	const UInt32 received;
	const UInt32 lost;
	const UInt32 size;
	const UInt32 latency;
};

QualityOfService::QualityOfService() : lostRate(0),byteRate(0),latency(0),congestionRate(0),_latency(0),_prevTime(0),droppedFrames(0),_num(0),_den(0),_size(0) {
}


QualityOfService::~QualityOfService() {
	reset();
}

void QualityOfService::add(UInt32 time,UInt32 received,UInt32 lost,UInt32 size) {

	if(_prevTime>0) {
		if(time>=_prevTime) {
			UInt32 delta = time-_prevTime;
			UInt32 deltaReal =  UInt32(_reception.elapsed()/1000);
			_latency += (Int64)deltaReal-delta;
			(UInt32&)latency = _latency<0 ? 0 : (UInt32)_latency;
		} else
			WARN("QoS computing with a error time value (%u) inferiors than precedent time (%u)",time,_prevTime);
	}
	_reception.update();

	_prevTime=time;
	_num += lost;
	_den += (lost+received);
	_size += size;
	
	Sample* pSample = new Sample(time,received,lost,size,latency);
		
	list<Sample*>::iterator it=_samples.begin();
	
	UInt32 boundTime = time<=5000 ? 0 : (time-5000);

	while(it!=_samples.end()) {
		Sample& sample(**it);
		if(sample.time>=boundTime) // 5 secondes
			break;
		_den -= (sample.received+sample.lost);
		_num -= sample.lost;
		_size -= sample.size;
		delete *it;
		_samples.erase(it++);
	}
	_samples.push_back(pSample);

	//TRACE("_samples.size()=%u",_samples.size());

	double congestion = 0;
	if(boundTime==0)
		(double&)byteRate = (double)_size/time*1000;
	else {
		congestion = ((double)(*(--_samples.end()))->latency-(*_samples.begin())->latency) / 5000;
		(double&)byteRate = (double)_size/5;
	}
	if(congestion<0)
		congestion=0;

	if(_den==0)
		ERROR("Lost rate computing with a impossible null number of fragments received")
	else {
		(double&)lostRate = (double)_num/_den;
		congestion += lostRate;
	}

	(double&)congestionRate = congestion>congestionRate ? (congestion+congestionRate)/2.0 : congestion; // average with prec value
}

void QualityOfService::reset() {
	(double&)lostRate = 0;
	(double&)byteRate = 0;
	(double&)congestionRate = 0;
	(UInt32&)latency = 0;
	(UInt32&)droppedFrames = 0;
	_latency=0;
	_size=_num=_den=_prevTime=0;
	list<Sample*>::iterator it;
	for(it=_samples.begin();it!=_samples.end();++it)
		delete (*it);
	_samples.clear();
}


} // namespace Cumulus
