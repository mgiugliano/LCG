/*=========================================================================
 *
 *   Program:     lcg
 *   Filename:    aec.cpp
 *
 *   Copyright (C) 2012 Daniele Linaro
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

#include <stdio.h>
#include "utils.h"
#include "aec.h"

namespace lcg {

AEC::AEC(const char *kernelFile)
{
        if (kernelFile == NULL) {
                m_length = 1;
                m_kernel = new double[m_length];
                m_current = new double[m_length];
                m_kernel[0] = 0.0;
                m_current[0] = 0.0;
                m_withKernel = false;
        }
        else {
                FILE *fid;
                double tmp;
        
                fid = fopen(kernelFile, "r");
                if (fid == NULL) {
                        Logger(Critical, "%s: no such file.\n", kernelFile);
                        throw "Unable to open kernel file.";
                }
        
                m_length = 0;
                while (fscanf(fid, "%le\n", &tmp) != EOF)
                        m_length++;
                fclose(fid);
        
                fid = fopen(kernelFile, "r");
                m_kernel = new double[m_length];
                m_current = new double[m_length];
                for (int i=0; i<m_length; i++) {
                        fscanf(fid, "%le\n", &m_kernel[i]);
                        m_current[i] = 0.0;
                }
                fclose(fid);
                m_withKernel = true;
        }
        Logger(Debug, "The kernel has %d samples.\n", m_length);
}

AEC::AEC(const double *kernel, size_t kernelSize)
        : m_length(kernelSize)
{
        m_kernel = new double[m_length];
        m_current = new double[m_length];
        for (int i=0; i<m_length; i++) {
                m_kernel[i] = kernel[i];
                m_current[i] = 0.0;
        }
        m_withKernel = true;
}

AEC::~AEC()
{
        delete m_kernel;
        delete m_current;
}

bool AEC::initialise(double I, double V)
{
        for (int i=0; i<m_length; i++)
                m_current[i] = I*1e-12;
        m_pos = 0;
        m_buffer[0] = V;
        m_buffer[1] = V;
        if (I != 0)
                Logger(Important, "Initialised kernel with %lf (pA).\n", I);
        return true;
}

bool AEC::initialise(double *I, double V)
{
        if (! initialise(I[0], V))
                return false;
        for (int i=0; i<m_length; i++)
                m_current[i] = I[i]*1e-12;
        return true;
}

void AEC::pushBack(double I)
{
        m_current[m_pos] = I*1e-12;
        m_pos = (m_pos+1) % m_length;
}

double AEC::compensate(double V)
{
        if (!m_withKernel)
                return V;
        double toCompensate = m_buffer[0];
        m_buffer[0] = m_buffer[1];
        m_buffer[1] = V;
        return toCompensate - 1e3*convolve();
}

double AEC::convolve()
{
        int i, j;
        double U = 0.0;
        for (i=m_pos-1, j=0; i>=0; i--, j++)
                U += m_current[i] * m_kernel[j];
        for (i=m_length-1; j<m_length; i--, j++)
                U += m_current[i] * m_kernel[j];
        return U;
}

const double* AEC::kernel() const
{
        return m_kernel;
}

size_t AEC::kernelLength() const
{
        return m_length;
}

bool AEC::hasKernel() const
{
        return m_withKernel;
}

} // namespace lcg

