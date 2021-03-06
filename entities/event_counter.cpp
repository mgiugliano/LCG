#include <strings.h>
#include "event_counter.h"

lcg::Entity* EventCounterFactory(string_dict& args)
{
        uint id, maxCount;
        bool autoReset;
        std::string eventToSendStr, eventToCountStr;
        lcg::EventType eventToSend, eventToCount;

        id = lcg::GetIdFromDictionary(args);
        if (! lcg::CheckAndExtractUnsignedInteger(args, "maxCount", &maxCount)) {
                lcg::Logger(lcg::Critical, "EventCounter(%d): Unable to build. Need to specify maxCount.\n", id);
                return NULL;
        }

        if (! lcg::CheckAndExtractBool(args, "autoReset", &autoReset)) {
                autoReset = true;
        }
        if (lcg::CheckAndExtractValue(args, "eventToCount", eventToCountStr)) {
                int i=0;
                while(i < NUMBER_OF_EVENT_TYPES && strcasecmp(eventToCountStr.c_str(), lcg::eventTypeNames[i].c_str()))
                    i++;
                if (i == NUMBER_OF_EVENT_TYPES) {
                    lcg::Logger(lcg::Critical, "EventCounter(%d): Unknown eventToCount type [%s].\n", id, eventToCountStr.c_str());
                    return NULL;
                }
                eventToCount = static_cast<lcg::EventType>(i);
				if (eventToCount == lcg::RESET) {
                    lcg::Logger(lcg::Debug, "EventCounter(%d): Counting RESET events.\n", id);
					autoReset = false;
				}
			}
        else {
                eventToCount = lcg::SPIKE;
        }
        if (lcg::CheckAndExtractValue(args, "eventToSend", eventToSendStr)) {
                int i=0;
                while(i < NUMBER_OF_EVENT_TYPES && strcasecmp(eventToSendStr.c_str(), lcg::eventTypeNames[i].c_str()))
                    i++;
                if (i == NUMBER_OF_EVENT_TYPES) {
                    lcg::Logger(lcg::Critical, "EventCounter(%d): Unknown eventToSend type [%s].\n", id, eventToSendStr.c_str());
                    return NULL;
                }
                eventToSend = static_cast<lcg::EventType>(i);
        }
        else {
                eventToSend = lcg::TRIGGER;
        }
        return new lcg::EventCounter(maxCount, autoReset, eventToCount, eventToSend, id);
}

namespace lcg {

EventCounter::EventCounter(uint maxCount, bool autoReset, EventType eventToCount, EventType eventToSend, uint id)
        : Entity(id), m_maxCount(maxCount), m_autoReset(autoReset), m_eventToCount(eventToCount), m_eventToSend(eventToSend)
{
        m_parameters["maxCount"] = maxCount;
        setName("EventCounter");
}

uint EventCounter::maxCount() const
{
        return m_maxCount;
}

bool EventCounter::autoReset() const
{
        return m_autoReset;
}

EventType EventCounter::eventToCount() const
{
        return m_eventToCount;
}

EventType EventCounter::eventToSend() const
{
        return m_eventToSend;
}

uint EventCounter::count() const
{
        return m_count;
}

void EventCounter::setMaxCount(uint maxCount)
{
        m_maxCount = maxCount;
}

void EventCounter::setAutoReset(bool autoReset)
{
        m_autoReset = autoReset;
}

void EventCounter::setEventToCount(EventType eventToCount)
{
        m_eventToCount = eventToCount;
}

void EventCounter::setEventToSend(EventType eventToSend)
{
        m_eventToSend = eventToSend;
}

void EventCounter::handleEvent(const Event *event)
{	
	//Logger(Debug, "EventCounter(%d): Received %d , comparing with %d.\n", id(), event->type(), m_eventToCount);
	if(event->type() == m_eventToCount) {
	        m_count++;
	        if (m_count == m_maxCount) {
	                Logger(Debug, "EventCounter(%d): Received %d events at t = %g sec.\n", id(), m_maxCount, GetGlobalTime());
	        	dispatch(); 
	        	if (m_autoReset) {
	        	        reset();
                        }
	        }
	}
	else {
                if(event->type() == RESET) {
		        reset();
                }
        }
}

void EventCounter::step()
{}

double EventCounter::output()
{
        return 0.0;
}

bool EventCounter::hasOutput() const
{
        return false;
}

bool EventCounter::initialise()
{
        m_count = 0;
        return true;
}

void EventCounter::reset()
{
        m_count = 0;
}

void EventCounter::dispatch()
{
        switch (m_eventToSend) {
                case TRIGGER: 
                        emitEvent(new TriggerEvent(this));
                        break;
                case SPIKE:
                        emitEvent(new SpikeEvent(this));
                        break;
                case RESET:
                        emitEvent(new ResetEvent(this));
                        break;
                case TOGGLE:
                        emitEvent(new ToggleEvent(this));
                        break;
		case STOPRUN:
                        emitEvent(new StopRunEvent(this));
                        Logger(Important, "Simulation terminated by EventCounter(%d). Counted %d events.\n", id(), m_count);
			break;
                default:
                        Logger(Important, "EventCounter(%d): Can't send event.\n", id());
                        break;
        }
}

} // namespace lcg

