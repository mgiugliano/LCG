#!/usr/bin/env python

import getopt
import glob
import lcg
import numpy as np
import os
import subprocess as sub
import sys

config_file = 'stimulus.xml'
max_steps = 50

def usage():
    print('This program applies the stimulation described by one or more stimulus files')
    print('to the specified output channel(s), while at the same time recording from the')
    print('corresponding input channels, located on the same subdevice.')
    print('')
    print('Usage: %s [option <value>]' % os.path.basename(sys.argv[0]))
    print('')
    print('where options are:')
    print('')
    print(' -h, --help            display this help message and exit')
    print(' -s, --stimulus        stimulus file(s) to use')
    print(' -d, --directory       directory where the stimulus files are located')
    print(' -l, --duration        duration of the recording (only if -s or -d are not specified)')
    print(' -n, --repetitions     number of repetitions (default 1)')
    print(' -i, --interval        interval between repetitions (default 0 s)')
    print(' -F, --sampling-rate   sampling frequency (default %s Hz)' % os.environ['SAMPLING_RATE'])
    print(' -D, --device          input device (default %s)' % os.environ['COMEDI_DEVICE'])
    print(' -S, --subdevice       input subdevice (default %s)' % os.environ['AI_SUBDEVICE'])
    print(' -I, --input-channels  input channels (default %s, but see note at the end for how to specify input channels)' % os.environ['AI_CHANNEL'])
    print(' -g, --input-gains     input conversion factors (comma separated values, default %s' % os.environ['AI_CONVERSION_FACTOR_CC'])
    print('                       (or %s if --vclamp is used) for all channels)' % os.environ['AI_CONVERSION_FACTOR_VC'])
    print(' -u, --input-units     input units (comma separated values, default %s (or %s if' % (os.environ['AI_UNITS_CC'],os.environ['AI_UNITS_VC']))
    print('                       --vclamp is used) for all channels)')
    print(' -O, --output-channels output channel(s) (default %s, but see note at the end for how to specify output channels)' % os.environ['AO_CHANNEL'])
    print(' -G, --output-gains    output conversion factors (comma separated values, default %s' % os.environ['AO_CONVERSION_FACTOR_CC'])
    print('                       (or %s if --vclamp is used) for all channels)' % os.environ['AO_CONVERSION_FACTOR_VC'])
    print(' -U, --output-units    output units (comma separated values, default %s (or %s if' % (os.environ['AO_UNITS_CC'],os.environ['AO_UNITS_VC']))
    print('                       --vclamp is used) for all channels)')
    print(' -V, --vclamp          use default conversion factor and units for voltage clamp')
    print(' -o, --offset          offset value, summed to the stimulation (in pA or mV, default 0)')
    print('')
    print('Input and output channels (-I and -O switches, respectively) can be specified in one of four ways:')
    print('')
    print('  1) single value, as in `-I 1\'')
    print('  2) a list of comma separated values, as in `-I 0,1,2,3\'')
    print('  3) a range of values, as in `-I 0-3\', which is equivalent to `-I 0,1,2,3\'')
    print('  4) the string ``none\'\', as in `-I none\', in which case no input or output channels are used')
    print('')

def get_stimulus_duration(stimfile):
    stim = np.loadtxt(stimfile)
    if len(stim.shape) == 1:
        return stim[0]
    return np.sum(stim[:,0])

def main():
    try:
        opts,args = getopt.getopt(sys.argv[1:], 'hs:d:l:n:i:F:D:S:I:g:u:O:G:U:Vo:',
                                  ['help','stimulus=','directory=','duration=','repetitions=','interval=','sampling-rate=',
                                   'device=','subdevice=',
                                   'input-channels=','input-gains=','input-units=',
                                   'output-channels=','output-gains=','output-units=',
                                   'vclamp','offset='])
    except getopt.GetoptError, err:
        print(str(err))
        usage()
        sys.exit(1)

    # default values
    stimfiles = None
    stimdir = None
    duration = None
    repetitions = 1
    interval = 0       # [s]
    
    samplingRate = float(os.environ['SAMPLING_RATE'])    # [Hz]

    device = os.environ['COMEDI_DEVICE']
    subdevice = os.environ['AI_SUBDEVICE']
    inputChannels = []
    inputGains = []
    inputUnits = []
    outputChannels = []
    outputGains = []
    outputUnits = []

    suffix = 'CC'

    offsets = []

    # parse arguments
    for o,a in opts:
        if o in ('-h','--help'):
            usage()
            sys.exit(0)
        elif o in ('-s','--stimulus'):
            stimfiles = []
            for f in a.split(','):
                stimfiles.append(f)
        elif o in ('-d','--directory'):
            stimdir = a
        elif o in ('-l','--duration'):
            duration_option = o
            duration = float(a)
        elif o in ('-n','--repetitions'):
            repetitions = int(a)
            if repetitions <= 0:
                print('The number of repetitions must be at least one.')
                sys.exit(1)
        elif o in ('-i','--interval'):
            interval = float(a)
            if interval < 0:
                print('The interval between repetitions must be non-negative.')
                sys.exit(1)
        elif o in ('-F','--sampling-rate'):
            samplingRate = float(a)
            if interval <= 0:
                print('The sampling rate must be positive.')
                sys.exit(1)
        elif o in ('-D','--device'):
            if not os.path.exists(a):
                print('Device \'%s\' does not exist.' % a)
                sys.exit(1)
            device = a
        elif o in ('-S','--subdevice'):
            subdevice = a
        elif o in ('-I','--input-channels'):
            if a.lower() == 'none':
                # no input channels
                inputChannels = None
            elif '-' in a:
                # a range of input channels
                inputChannels = range(int(a.split('-')[0]),int(a.split('-')[1])+1)
            elif ',' in a:
                # a list of input channels
                for chan in a.split(','):
                    inputChannels.append(int(chan))
            else:
                # just one input channel
                inputChannels.append(int(a))
        elif o in ('-g','--input-gains'):
            for gain in a.split(','):
                inputGains.append(float(gain))
        elif o in ('-u','--input-units'):
            for unit in a.split(','):
                inputUnits.append(unit)
        elif o in ('-O','--output-channels'):
            if a.lower() == 'none':
                # no output channels
                outputChannels = None
            elif '-' in a:
                # a range of output channels
                outputChannels = range(int(a.split('-')[0]),int(a.split('-')[1])+1)
            elif ',' in a:
                # a list of output channels
                for chan in a.split(','):
                    outputChannels.append(int(chan))
            else:
                # just one output channel
                outputChannels.append(int(a))
        elif o in ('-G','--output-gains'):
            for gain in a.split(','):
                outputGains.append(float(gain))
        elif o in ('-U','--output-units'):
            for unit in a.split(','):
                outputUnits.append(unit)
        elif o in ('-V','--vclamp'):
            suffix = 'VC'
        elif o in ('-o','--offset'):
            for offset in a.split(','):
                offsets.append(float(offset))

    if inputChannels is None and outputChannels is None:
        print('No input or output channels specified. I cowardly refuse to continue.')
        sys.exit(0)

    if not outputChannels is None and not duration is None and (not stimfiles is None or not stimdir is None):
        print('Warning: if at least one output channel is specified, the duration of the recording')
        print('is given by the duration of the stimulus. Ignoring your %s option.' % duration_option)

    if outputChannels is None and not offset is None:
        print('You specified an offset but no output channel(s). I don\'t know what to do.')
        sys.exit(0)

    # check the correctness of the arguments
    if not inputChannels is None:
        if len(inputChannels) == 0:
            inputChannels = [int(os.environ['AI_CHANNEL'])]
        if len(inputGains) == 0:
            inputGains = [float(os.environ['AI_CONVERSION_FACTOR_' + suffix]) for i in range(len(inputChannels))]
        if len(inputUnits) == 0:
            inputUnits = [os.environ['AI_UNITS_' + suffix] for i in range(len(inputChannels))]
        if len(inputChannels) != len(inputGains) or len(inputChannels) != len(inputUnits):
            print('The number of input channels, input gains and input units must be the same.')
            sys.exit(1)

    if not outputChannels is None:
        if len(outputChannels) == 0:
            outputChannels = [int(os.environ['AO_CHANNEL'])]
        if len(outputGains) == 0:
            outputGains = [float(os.environ['AO_CONVERSION_FACTOR_' + suffix]) for i in range(len(outputChannels))]
        if len(outputUnits) == 0:
            outputUnits = [os.environ['AO_UNITS_' + suffix] for i in range(len(outputChannels))]
        if len(offsets) == 0:
            offsets = [0. for i in range(len(outputChannels))]
        if len(outputChannels) != len(outputGains) or len(outputChannels) != len(outputUnits) or len(outputChannels) != len(offsets):
            print('The number of output channels, output gains, output units and offsets must be the same.')
            sys.exit(1)

        if stimfiles is None and stimdir is None:
            # no -s or -d option: check whether an offset and a duration were specified...
            if len(offset) > 0 and not duration is None:
                stimfiles = ['/tmp/tmp.stim']
                sub.call('lcg stimgen -o %s dc -d %g 0' % (stimfiles[0],duration), shell=True)
            else:
                # or whether a stimulus file was piped from lcg-stimgen
                stimulus = sys.stdin.read()
                if len(stimulus):
                    stimfiles = ['/tmp/tmp.stim']
                    with open(stimfiles[0],'w') as fid:
                        fid.write(stimulus)
                else:
                    print('You must specify one of -s or -d.')
                    sys.exit(0)

        if stimdir is not None:
            if stimfiles is not None:
                print('You cannot specify both -s and -D at the same time.')
                sys.exit(1)
            if stimdir[-1] == '/':
                stimdir = stimdir[:-1]
            if not os.path.isdir(stimdir):
                print('%s: no such directory.' % stimdir)
                sys.exit(1)
            stimfiles = glob.glob(stimdir + '/*.stim')
            if len(stimfiles) == 0:
                print('The directory %s contains no stimulus files.' % stimdir)
                sys.exit(0)
        else:
            for f in stimfiles:
                if not os.path.isfile(f):
                    print('%s: no such file.' % f)
                    sys.exit(1)

        # if there is one stimfile and many output channels, we output the same stimulus
        # to all of them
        if len(stimfiles) == 1 and len(outputChannels) > 1:
            stimfiles = [stimfiles[0] for i in range(len(outputChannels))]

        if len(outputChannels) != 1 and len(stimfiles) != len(outputChannels):
            print('There are %d output channels and %d stimulus files: I don\'t know what to do.' % (len(outputChannels),len(stimfiles)))
            sys.exit(1)

    if outputChannels is None or len(stimfiles) == len(outputChannels):
        total = repetitions
    else:
        total = repetitions * len(stimfiles)
    cnt = 1
    for i in range(repetitions):
        if outputChannels is None or len(stimfiles) == len(outputChannels):
            channels = []
            # input channels
            if not inputChannels is None:
                channels = [{'type':'input', 'channel':inputChannels[j], 'factor':inputGains[j],
                             'units':inputUnits[j]} for j in range(len(inputChannels))]
            if not outputChannels is None:
                # there are as many stimulus files as output channels
                duration = get_stimulus_duration(stimfiles[0])
                differentDurations = False
                for f in stimfiles:
                    d = get_stimulus_duration(f)
                    if d != duration:
                        differentDurations = True
                        if d > duration:
                            duration = d
                if i == 0 and differentDurations:
                    print('Warning: not all stimulus files have the same duration. Will use the longest, %g sec.' % duration)
                for j in range(len(outputChannels)):
                    channels.append({'type':'output', 'channel':outputChannels[j], 'factor':outputGains[j],
                                     'units':outputUnits[j], 'stimfile':stimfiles[j], 'offset':offsets[j]})
            lcg.writeIOConfigurationFile(config_file,samplingRate,duration,channels,False)
            sys.stdout.write('\rTrial %02d/%02d   [' % (cnt,total))
            percent = float(cnt)/total
            n_steps = int(round(percent*max_steps))
            for i in range(n_steps-1):
                sys.stdout.write('=')
            sys.stdout.write('>')
            for i in range(max_steps-n_steps):
                sys.stdout.write(' ')
            sys.stdout.write('] ')
            sys.stdout.flush()
            sub.call(lcg.common.prog_name + ' -V 3 -c ' + config_file, shell=True)
            if cnt < total:
                sub.call('sleep ' + str(interval), shell=True)
            else:
                sys.stdout.write('\n')
            cnt += 1
        else:
            # there is one output channel and many stimulus files
            for f in stimfiles:
                duration = np.sum(np.loadtxt(f)[:,0])
                channels = [{'type':'input', 'channel':inputChannels[j], 'factor':inputGains[j],
                             'units':inputUnits[j]} for j in range(len(inputChannels))]
                channels.append({'type':'output', 'channel':outputChannels[0], 'factor':outputGains[0],
                                 'units':outputUnits[0], 'stimfile':f, 'offset':offsets[0]})
                lcg.writeIOConfigurationFile(config_file,samplingRate,duration,channels,False)
                sys.stdout.write('\rTrial %02d/%02d   [' % (cnt,total))
                percent = float(cnt)/total
                n_steps = int(round(percent*max_steps))
                for i in range(n_steps-1):
                    sys.stdout.write('=')
                sys.stdout.write('>')
                for i in range(max_steps-n_steps):
                    sys.stdout.write(' ')
                sys.stdout.write('] ')
                sys.stdout.flush()
                sub.call(lcg.common.prog_name + ' -V 3 -c ' + config_file, shell=True)
                if cnt < total:
                    sub.call('sleep ' + str(interval), shell=True)
                else:
                    sys.stdout.write('\n')
                cnt += 1

if __name__ == '__main__':
    main()
