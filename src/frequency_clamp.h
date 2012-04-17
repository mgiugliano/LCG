#ifndef FREQUENCY_CLAMP_H
#define FREQUENCY_CLAMP_H

#include "generator.h"
#include "entity.h"
#include "utils.h"

#define FC_BASELINE     m_parameters[0]
#define FC_TAU          m_parameters[1]
#define FC_GP           m_parameters[2]
#define FC_GI           m_parameters[3]
#define FC_GD           m_parameters[4]
#define FC_F            m_parameters[5]

namespace dynclamp {

namespace generators {

class FrequencyClamp : public Generator {
public:
        FrequencyClamp(double baselineCurrent, double tau,
                       double gp, double gi, double gd,
                       uint id = GetId());
        FrequencyClamp(double frequency, double baselineCurrent, double tau,
                       double gp, double gi, double gd,
                       uint id = GetId());
        bool hasNext() const;
        virtual void initialise();
        void step();
        double output() const;
        void handleEvent(const Event *event);

protected:
        virtual void addPre(Entity *entity);

private:
        bool m_constantFrequency;
        Generator *m_generator;
        double m_estimatedFrequency, m_tPrevSpike;
        double m_errp, m_erri, m_errd, m_errorpPrev;
        double m_current;
};

} // namespace generators

}

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

dynclamp::Entity* FrequencyClampFactory(dictionary& args);
        
#ifdef __cplusplus
}
#endif

 // namespace dynclamp

#endif
