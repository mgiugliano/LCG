/*=========================================================================
 *
 *   Program:     lcg
 *   Filename:    analogy_io.h
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

#ifndef ANALOGY_IO_H
#define ANALOGY_IO_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string>
#include <map>
#include <analogy/analogy.h>
#include "types.h"
#include "entity.h"
#include "common.h"

namespace lcg {

/**
 * \brief Base class for analog I/O with Analogy.
 */
class AnalogyAnalogIO {
public:
        AnalogyAnalogIO(const char *deviceFile, uint subdevice,
                        uint *channels, uint nChannels,
                        uint range = PLUS_MINUS_TEN,
                        uint aref = GRSE);
        virtual ~AnalogyAnalogIO();

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
        a4l_desc_t m_dsc;
        uint m_subdevice;
        uint *m_channels;
        uint m_nChannels;
        uint m_range;
        uint m_aref;
        double *m_data;
};

/**
 * \brief Class for analog input from a single channel with hardware calibration.
 */
/*
class AnalogyAnalogInputHardCal : public AnalogyAnalogIO {
public:
        AnalogyAnalogInputHardCal(const char *deviceFile, uint outputSubdevice,
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
*/

/**
 * \brief Class for analog output to a single channel with hardware calibration.
 */
/*
class AnalogyAnalogOutputHardCal : public AnalogyAnalogIO {
public:
        AnalogyAnalogOutputHardCal(const char *deviceFile, uint outputSubdevice,
                                  uint writeChannel, double outputConversionFactor,
                                  uint aref = GRSE, bool resetOutput = true);
        ~AnalogyAnalogOutputHardCal();
        bool initialise();
        double outputConversionFactor() const;
        void write(double data);
private:
        comedi_range *m_dataRange;
        lsampl_t m_maxData;
        double m_outputConversionFactor;
        bool m_resetOutput;
};
*/

/**
 * \brief Base class for analog I/O with software calibration.
 */
class AnalogyAnalogIOSoftCal : public AnalogyAnalogIO {
public:
        AnalogyAnalogIOSoftCal(const char *deviceFile, uint subdevice,
                               uint *channels, uint nChannels,
                               uint range = PLUS_MINUS_TEN,
                               uint aref = GRSE);
        ~AnalogyAnalogIOSoftCal();

protected:
        bool readCalibration();

protected:
        char *m_calibrationFile;
};

/**
 * \brief Class for analog input from a single channel with software calibration.
 */
class AnalogyAnalogInputSoftCal : public AnalogyAnalogIOSoftCal {
public:
        AnalogyAnalogInputSoftCal(const char *deviceFile, uint inputSubdevice,
                                  uint readChannel, double inputConversionFactor,
                                  uint range = PLUS_MINUS_TEN,
                                  uint aref = GRSE);
        ~AnalogyAnalogInputSoftCal();
        bool initialise();
        double inputConversionFactor() const;
        double read();
private:
        double m_inputConversionFactor;
};

/**
 * \brief Class for analog output to a single channel with software calibration.
 */
class AnalogyAnalogOutputSoftCal : public AnalogyAnalogIOSoftCal {
public:
        AnalogyAnalogOutputSoftCal(const char *deviceFile, uint outputSubdevice,
                                  uint writeChannel, double outputConversionFactor,
                                  uint aref = GRSE, bool resetOutput = true);
        ~AnalogyAnalogOutputSoftCal();
        bool initialise();
        double outputConversionFactor() const;
        void write(double data);
private:
        double m_outputConversionFactor;
        bool m_resetOutput;
};

} // namespace lcg

#endif // ANALOGY_IO_H

