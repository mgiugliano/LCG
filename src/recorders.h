#ifndef RECORDERS_H
#define RECORDERS_H

#include <stdio.h>

#include <vector>
#include <map>
#include <queue>

#include <boost/thread.hpp>

#include "hdf5.h"

#include "utils.h"
#include "entity.h"

namespace dynclamp {

namespace recorders {

class Recorder : public Entity {
public:
        Recorder(uint id = GetId());
        virtual double output() const;
};

class ASCIIRecorder : public Recorder {
public:
        ASCIIRecorder(const char *filename = NULL, uint id = GetId());
        ~ASCIIRecorder();
        virtual bool initialise();
        virtual void step();

private:
        void openFile();
        void closeFile();

private:
        bool m_makeFilename;
        char m_filename[FILENAME_MAXLEN];
        FILE *m_fid;
        bool m_closeFile;
};


#define DATASET_NAME_LEN 128

#define H5_FILE_OPEN_ERROR      -1
#define H5_GROUPS_OPEN_ERROR    -2
#define H5_MISC_WRITE_ERROR     -3
#define H5_DATASET_ERROR        -4
#define H5_SHUFFLE_ERROR        -5
#define H5_DEFLATE_ERROR        -6

#define H5_NO_GZIP_COMPRESSION  -7
#define H5_NO_FILTER_INFO       -8
#define H5_NO_SHUFFLE_FILTER    -9

class H5Recorder : public Recorder {
public:
        H5Recorder(bool compress, const char *filename = NULL, uint id = GetId());
        ~H5Recorder();
        virtual bool initialise();
        virtual void step();

public:
        static const hsize_t rank;
        static const hsize_t unlimitedSize;
        static const hsize_t chunkSize;
        static const uint    numberOfChunks; 
        static const hsize_t bufferSize;
        static const double  fillValue;

protected:
        virtual void addPre(Entity *entity);

private:
        int  isCompressionAvailable() const;
        int  openFile();
        void closeFile();
        void startWriterThread();
        void stopWriterThread();
        void buffersWriter();
        bool writeMiscellanea();
        bool createGroups();
        int  checkCompression();
        void allocateForEntity(Entity *entity);
        bool writeArrayAttribute(hid_t dataset, const std::string& attrName,
                                 const double *data, const hsize_t *dims, int ndims);
        bool writeStringAttribute(hid_t dataset, const std::string& attrName, const std::string& attrValue);

private:
        // the handle of the file
        hid_t m_fid;
        // whether compression is turned on or off
        bool m_compress;
        // dataset creation property list
        hid_t m_datasetPropertiesList;
        // the data
        std::vector<double**> m_data;
        // the queue of the indices of the buffers to save
        std::deque<uint> m_dataQueue;
        // number of inputs
        uint m_numberOfInputs;

        // the name of the file
        char m_filename[FILENAME_MAXLEN];
        // tells whether the filename should be generated from the timestamp
        bool m_makeFilename;

        // position in the buffer
        uint m_bufferPosition;
        // the length of each buffer
        hsize_t *m_bufferLengths;
        // the number of buffers
        uint m_numberOfBuffers;
        // the thread that continuosly waits for data to save
        boost::thread m_writerThread;
        // the index of the buffer in which the main thread saves data
        uint m_bufferInUse;
        // multithreading stuff for controlling access to the queue m_dataQueue;
        boost::mutex m_mutex;
        boost::condition_variable m_cv;
        bool m_threadRun;

        // H5 stuff
        std::map<std::string,hid_t> m_groups;
        std::vector<hid_t> m_dataspaces;
        std::vector<hid_t> m_datasets;
        hsize_t m_offset;
        hsize_t m_datasetSize;

};

} // namespace recorders

} // namespace dynclamp

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

dynclamp::Entity* ASCIIRecorderFactory(dictionary& args);
dynclamp::Entity* H5RecorderFactory(dictionary& args);
	
#ifdef __cplusplus
}
#endif


#endif

