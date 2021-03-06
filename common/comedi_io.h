/*=========================================================================
 *
 *   Program:     lcg
 *   Filename:    comedi_io.h
 *
 *   Copyright (C) 2012,2013,2014 Daniele Linaro
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *   
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *   
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *=========================================================================*/

#ifndef COMEDI_IO_H
#define COMEDI_IO_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_LIBCOMEDI
#include <string>
#include <map>
#include <comedilib.h>
#include "types.h"
#include "entity.h"
#include "common.h"

/** Non-Referenced Single Ended */
#define NRSE AREF_COMMON

/** Ground-Referenced Single Ended */
#define GRSE AREF_GROUND

/**
 * We ask the card to operate at a frequency
 * of OVERSAMPLING_FACTOR times the sampling rate.
 */
#define OVERSAMPLING_FACTOR 1

namespace lcg {

/**
 * \brief Base class for analog I/O with Comedi.
 */
class ComediAnalogIO {
public:
        ComediAnalogIO(const char *deviceFile, uint subdevice,
                       uint *channels, uint nChannels,
                       uint range = PLUS_MINUS_TEN,
                       uint aref = GRSE);
        virtual ~ComediAnalogIO();

        virtual bool addChannel(uint channel);
        bool isChannelPresent(uint channel);

        const char* deviceFile() const;
        uint subdevice() const;
        const uint* channels() const;
        uint nChannels() const;
        uint range() const;
        uint reference() const;

protected:
        bool openDevice();
        bool closeDevice();

protected:
        char m_deviceFile[30];
        comedi_t *m_device;
        uint m_subdevice;
        uint *m_channels;
        uint m_nChannels;
        uint m_range;
        uint m_aref;
        lsampl_t *m_data;
};

/**
 * \brief Base class for analog I/O with software calibration.
 */
class ComediAnalogIOSoftCal : public ComediAnalogIO {
public:
        ComediAnalogIOSoftCal(const char *deviceFile, uint subdevice,
                              uint *channels, uint nChannels,
                              uint range = PLUS_MINUS_TEN,
                              uint aref = GRSE);
        ~ComediAnalogIOSoftCal();

protected:
        bool readCalibration();

protected:
        char *m_calibrationFile;
        comedi_calibration_t *m_calibration;
};

/**
 * \brief Class for analog input from a single channel with software calibration.
 */
class ComediAnalogInputSoftCal : public ComediAnalogIOSoftCal {
public:
        ComediAnalogInputSoftCal(const char *deviceFile, uint inputSubdevice,
                                 uint readChannel, double inputConversionFactor,
                                 uint range = PLUS_MINUS_TEN,
                                 uint aref = GRSE);
        ~ComediAnalogInputSoftCal();
        bool initialise();
        double inputConversionFactor() const;
        double read();
private:
        comedi_polynomial_t m_converter;
        double m_inputConversionFactor;
};

/**
 * \brief Class for analog output to a single channel with software calibration.
 */
class ComediAnalogOutputSoftCal : public ComediAnalogIOSoftCal {
public:
        ComediAnalogOutputSoftCal(const char *deviceFile, uint outputSubdevice,
                                  uint writeChannel, double outputConversionFactor,
                                  uint aref = GRSE, bool resetOutput = true);
        ~ComediAnalogOutputSoftCal();
        bool initialise();
        double outputConversionFactor() const;
        void write(double data);
private:
        comedi_polynomial_t m_converter;
#ifdef TRIM_ANALOG_OUTPUT
        comedi_range *m_dataRange;
#endif
        double m_outputConversionFactor;
        bool m_resetOutput;
};

/**
 * \brief Class for analog input from a single channel with hardware calibration.
 */
class ComediAnalogInputHardCal : public ComediAnalogIO {
public:
        ComediAnalogInputHardCal(const char *deviceFile, uint outputSubdevice,
                                 uint readChannel, double inputConversionFactor,
                                 uint range = PLUS_MINUS_TEN, uint aref = GRSE);
        bool initialise();
        double inputConversionFactor() const;
        double read();
private:
        comedi_range *m_dataRange;
        lsampl_t m_maxData;
        double m_inputConversionFactor;
};

/**
 * \brief Class for analog output to a single channel with hardware calibration.
 */
class ComediAnalogOutputHardCal : public ComediAnalogIO {
public:
        ComediAnalogOutputHardCal(const char *deviceFile, uint outputSubdevice,
                                  uint writeChannel, double outputConversionFactor,
                                  uint aref = GRSE, bool resetOutput = true);
        ~ComediAnalogOutputHardCal();
        bool initialise();
        double outputConversionFactor() const;
        void write(double data);
private:
        comedi_range *m_dataRange;
        lsampl_t m_maxData;
        double m_outputConversionFactor;
        bool m_resetOutput;
};


/**
 * \brief Class for digital input/output from a DIO.
 */
class ComediDigitalIO {
public:
        ComediDigitalIO(const char *deviceFile, uint subdevice,
                       uint *channels, uint nChannels);
        virtual ~ComediDigitalIO();

        virtual bool addChannel(uint channel);
        bool isChannelPresent(uint channel);

        const char* deviceFile() const;
        uint subdevice() const;
        const uint* channels() const;
        uint nChannels() const;

protected:
        bool openDevice();
        bool closeDevice();

protected:
        char m_deviceFile[30];
        comedi_t *m_device;
        uint m_subdevice;
        uint *m_channels;
        uint m_nChannels;
};


/**
 * \brief Class for digital output to a single channel.
 */
class ComediDigitalOutput : public ComediDigitalIO {
public:
        ComediDigitalOutput(const char *deviceFile, uint outputSubdevice,
	                    uint writeChannel);
        ~ComediDigitalOutput();
        bool initialise();
        void write(double data); // keeping the format of Analog IOs
};

class ComediDigitalInput : public ComediDigitalIO {
public:
        ComediDigitalInput(const char *deviceFile, uint outputSubdevice,
	                    uint readChannel);
        ~ComediDigitalInput();
        bool initialise();
        double read(); 
};

} // namespace lcg

#endif // HAVE_LIBCOMEDI

#endif // COMEDI_IO_H

