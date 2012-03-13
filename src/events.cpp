#include "events.h"
#include "entity.h"
#include "thread_safe_queue.h"

namespace dynclamp {

ThreadSafeQueue<Event*> eventsQueue;

void EnqueueEvent(Event *event)
{
        eventsQueue.push_back(event);
        Logger(Debug, "Enqueued event sent from entity #%d.\n", event->sender()->id());
}

void ProcessEvents()
{
        Event *event;
        uint i, j, nEvents, nPost;
        nEvents = eventsQueue.size();
        Logger(All, "There are %d events in the queue.\n", nEvents);
        for (i=0; i<nEvents; i++) {
                event = eventsQueue.pop_front();
                const std::vector<Entity*>& post = event->sender()->post();
                nPost = post.size();
                for (j=0; j<nPost; j++)
                        post[j]->handleEvent(event);
                delete event;
                Logger(Debug, "Deleted event sent from entity #%d.\n", event->sender()->id());
        }
}

Event::Event(EventType type, const Entity *sender)
        : m_type(type), m_sender(sender), m_time(GetGlobalTime())
{}

EventType Event::type() const
{
        return m_type;
}

const Entity* Event::sender() const
{
        return m_sender;
}

double Event::time() const
{
        return m_time;
}

SpikeEvent::SpikeEvent(const Entity *sender)
        : Event(SPIKE, sender)
{}

} // namespace dynclamp

