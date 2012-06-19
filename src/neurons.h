#ifndef NEURONS_H
#define NEURONS_H

#include "dynamical_entity.h"
#include "types.h"

#ifdef HAVE_CONFIG_H
#include "config.h"

#ifdef HAVE_LIBCOMEDI
#include <comedilib.h>
#include "analog_io.h"
#include "aec.h"
#endif // HAVE_LIBCOMEDI
#endif // HAVE_CONFIG_H

#define VM       m_state[0]

#define LIF_C      m_parameters[0]
#define LIF_TAU    m_parameters[1]
#define LIF_TARP   m_parameters[2]
#define LIF_ER     m_parameters[3]
#define LIF_E0     m_parameters[4]
#define LIF_VTH    m_parameters[5]
#define LIF_IEXT   m_parameters[6]
#define LIF_LAMBDA m_parameters[7]
#define LIF_RL     m_parameters[8]
#define LIF_ARTIFICIAL_SPIKE

#define CBN_VM_PREV             m_state[1]
#define CBN_EL                  m_parameters[2]
#define CBN_IEXT                m_parameters[3]
#define CBN_SPIKE_THRESH        m_parameters[5]
#define CBN_GL                  m_parameters[6]
#define CBN_COEFF               m_parameters[7]

namespace dynclamp {

namespace neurons {

class Neuron : public DynamicalEntity {
public:
        Neuron(double Vm0, uint id = GetId());
        virtual bool initialise();
        double Vm() const;
        double Vm0() const;
        virtual double output() const;
protected:
        void emitSpike() const;
private:
        double m_Vm0;
};

class LIFNeuron : public Neuron {
public:

        /**
         * \param C membrane capacitance.
         * \param tau membrane time constant.
         * \param tarp absolute refractory period.
         * \param Er reset voltage.
         * \param E0 resting potential.
         * \param Vth threshold.
         * \param Iext externally applied current.
         */
        LIFNeuron(double C, double tau, double tarp,
                  double Er, double E0, double Vth, double Iext,
                  uint id = GetId());
        virtual bool initialise();

protected:
        virtual void evolve();

private:
        double m_tPrevSpike;
};

class ConductanceBasedNeuron : public Neuron {
public:
        /**
         * \param C membrane capacitance (uF/cm^2)
         * \param gl leak conductance (S/cm^2)
         * \param El leak reversal potential (mV)
         * \param Iext externally applied current (pA)
         * \param area surface of the membrane (um^2)
         * \param spikeThreshold threshold for emitting a spike (mV)
         * \param V0 initial value of the membrane potential (mV)
         */
        ConductanceBasedNeuron(double C, double gl, double El, double Iext,
                               double area, double spikeThreshold, double V0,
                               uint id = GetId());
protected:
        virtual void evolve();
};

#ifdef HAVE_LIBCOMEDI

#define RN_VM_PREV              m_state[1]
#define RN_SPIKE_THRESH         m_parameters[0]

class RealNeuron : public Neuron {
public:
        RealNeuron(double spikeThreshold, double V0,
                   const char *deviceFile,
                   uint inputSubdevice, uint outputSubdevice,
                   uint readChannel, uint writeChannel,
                   double inputConversionFactor, double outputConversionFactor,
                   uint inputRange, uint reference, uint voltageDelaySteps,
                   bool adaptiveThreshold = false, const char *kernelFile = NULL, uint id = GetId());

        RealNeuron(double spikeThreshold, double V0,
                   const char *deviceFile,
                   uint inputSubdevice, uint outputSubdevice,
                   uint readChannel, uint writeChannel,
                   double inputConversionFactor, double outputConversionFactor,
                   uint inputRange, uint reference, uint voltageDelaySteps,
                   const double *AECKernel, size_t kernelSize,
                   bool adaptiveThreshold = false, uint id = GetId());

        ~RealNeuron();

        virtual bool initialise();

        virtual bool hasMetadata(size_t *ndims) const;
        virtual const double* metadata(size_t *dims, char *label) const;

        uint voltageDelaySteps() const;

protected:
        virtual void evolve();

private:
        AEC m_aec;
        ComediAnalogInputSoftCal  m_input;
        ComediAnalogOutputSoftCal m_output;
        uint m_delaySteps;
        double *m_VrDelay;

        // adaptive threshold variables
        bool m_adaptiveThreshold;
        double m_Vmax, m_Vmin, m_Vth;
};
#endif // HAVE_LIBCOMEDI

} // namespace neurons

} // namespace dynclamp


/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

dynclamp::Entity* LIFNeuronFactory(dictionary& args);
dynclamp::Entity* ConductanceBasedNeuronFactory(dictionary& args);
#ifdef HAVE_LIBCOMEDI
dynclamp::Entity* RealNeuronFactory(dictionary& args);
#endif
	
#ifdef __cplusplus
}
#endif

#endif

