#include "connections.h"
#include "engine.h"
#include "common.h"

dynclamp::Entity* ConnectionFactory(string_dict& args)
{
        uint id;
        double delay;

        id = dynclamp::GetIdFromDictionary(args);
        if (! dynclamp::CheckAndExtractDouble(args, "delay", &delay)) {
                dynclamp::Logger(dynclamp::Critical, "Unable to build a Connection.\n");
                return NULL;
        }

        return new dynclamp::Connection(delay, id);
}

dynclamp::Entity* VariableDelayConnectionFactory(string_dict& args)
{
        return new dynclamp::VariableDelayConnection(dynclamp::GetIdFromDictionary(args));
}

namespace dynclamp {

bool CompareFirst(const std::pair<double,Event*>& p1, 
                  const std::pair<double,Event*>& p2)
{
        return p1.first < p2.first;
}

Connection::Connection(double delay, uint id)
        : Entity(id), m_events()
{
        m_parameters["delay"] = delay;
        setName("Connection");
}
        
Connection::~Connection()
{
        clearEventsList();
}

void Connection::setDelay(double delay)
{
        if (delay >= 0)
                m_parameters["delay"] = delay;
        else
                Logger(Important, "Tried to set a negative delay.\n");
}

void Connection::step()
{
        std::list< std::pair<double,Event*> >::iterator it;
        for (it=m_events.begin(); it!=m_events.end(); it++)
                it->first -= GetGlobalDt();
        int i;
        while (!m_events.empty() && m_events.front().first <= 0) {
                for (i=0; i<m_post.size(); i++)
                        m_post[i]->handleEvent(m_events.front().second);
                delete m_events.front().second;
                m_events.pop_front();
        }
}

double Connection::output()
{
        return 0.0;
}

bool Connection::initialise()
{
        clearEventsList();
        return true;
}

void Connection::terminate()
{
        clearEventsList();
}

void Connection::clearEventsList()
{
        if (!m_events.empty()) {
                std::list< std::pair<double,Event*> >::iterator it;
                for (it=m_events.begin(); it!=m_events.end(); it++)
                        delete it->second;
                m_events.clear();
        }
}

void Connection::handleEvent(const Event *event)
{
        m_events.push_back(std::make_pair(m_parameters["delay"]-GetGlobalDt(), new Event(*event)));
        m_events.sort(CompareFirst);
}

VariableDelayConnection::VariableDelayConnection(uint id)
        : Connection(0, id)
{
        setName("VariableDelayConnection");
}

void VariableDelayConnection::handleEvent(const Event *event)
{
        double delay;
        while ((delay = (*m_functor)()) == INFINITE)
                ;
        setDelay(delay);
        Connection::handleEvent(event);
}

void VariableDelayConnection::addPre(Entity *entity)
{
        Entity::addPre(entity);
        Functor *f = dynamic_cast<Functor*>(entity);
        if (f != NULL) {
                Logger(Debug, "A Functor (id #%d) was connected.\n", entity->id());
                m_functor = f;
        }
        else {
                Logger(Debug, "Entity #%d is not a functor.\n", entity->id());
        }
}

}//namespace dynclamp

