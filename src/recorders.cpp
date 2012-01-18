#include "recorders.h"

dynclamp::Entity* ASCIIRecorderFactory(dictionary& args)
{
        uint id;
        double dt;
        std::string filename;
        dynclamp::GetIdAndDtFromDictionary(args, &id, &dt);
        if ( ! dynclamp::CheckAndExtractValue(args, "filename", filename))
                return NULL;
        return new dynclamp::recorders::ASCIIRecorder(filename.c_str(), id, dt);
}

dynclamp::Entity* H5RecorderFactory(dictionary& args)
{       
        uint id;
        double dt;
        std::string filename;
        bool compress;
        dynclamp::GetIdAndDtFromDictionary(args, &id, &dt);
        if ( ! dynclamp::CheckAndExtractValue(args, "filename", filename) ||
             ! dynclamp::CheckAndExtractBool(args, "compress", &compress))
                return NULL;
        return new dynclamp::recorders::H5Recorder(filename.c_str(), compress, id, dt);

}

namespace dynclamp {

namespace recorders {

Recorder::Recorder(uint id, double dt)
        : Entity(id, dt)
{}

double Recorder::output() const
{
        return 0.0;
}

ASCIIRecorder::ASCIIRecorder(const char *filename, uint id, double dt)
        : Recorder(id, dt)
{
        m_fid = fopen(filename, "w");
        if (m_fid == NULL) {
                char msg[100];
                sprintf(msg, "Unable to open %s.\n", filename); 
                throw msg;
        }
        m_closeFile = true;
}

ASCIIRecorder::ASCIIRecorder(FILE *fid, uint id, double dt)
        : Recorder(id, dt), m_fid(fid), m_closeFile(false)
{}

ASCIIRecorder::~ASCIIRecorder()
{
        if (m_closeFile)
                fclose(m_fid);
}

void ASCIIRecorder::step()
{
        fprintf(m_fid, "%14e", GetGlobalTime());
        uint i, n = m_inputs.size();
        for (i=0; i<n; i++)
                fprintf(m_fid, " %14e", m_inputs[i]);
        fprintf(m_fid, "\n");
}

const hsize_t H5Recorder::rank           = 1;
const hsize_t H5Recorder::maxSize         = H5S_UNLIMITED;
const hsize_t H5Recorder::chunkSize      = 100;
const uint    H5Recorder::numberOfChunks = 20;
const hsize_t H5Recorder::bufferSize     = 2000;
const double  H5Recorder::fillValue      = 0.0;

H5Recorder::H5Recorder(const char *filename, bool compress, uint id, double dt)
        : Recorder(id, dt),
          m_data(), m_numberOfInputs(0),
          m_bufferPosition(0), m_bufferLength(bufferSize), m_bufferInUse(0), m_bufferToSave(0),
          m_mutex(), m_cv(), m_dataReady(false), m_threadRun(true),
          m_dataspaces(), m_datasets(),
          m_offset(0), m_datasetSize(0)
{
        if (open(filename, compress) != 0)
                throw "Unable to open H5 file.";
        m_writerThread = boost::thread(&H5Recorder::buffersWriter, this); 
}

H5Recorder::~H5Recorder()
{
        Logger(Debug, "H5Recorder::~H5Recorder() >> Terminating writer thread.\n");
        {
                boost::unique_lock<boost::mutex> lock(m_mutex);
                m_bufferLength = m_bufferPosition;
                m_bufferToSave = m_bufferInUse;
                m_threadRun = false;
                m_dataReady = true;
                Logger(Debug, "H5Recorder::~H5Recorder() >> %d values left to save.\n", m_bufferLength);
        }
        m_cv.notify_all();
        m_writerThread.join();
        Logger(Debug, "H5Recorder::~H5Recorder() >> Writer thread has terminated.\n");
        close();
        for (uint i=0; i<m_numberOfInputs; i++) {
                delete m_data[i][0];
                delete m_data[i][1];
                delete m_data[i];
        }
}

void H5Recorder::step()
{
        if (m_bufferPosition == 0)
                Logger(Debug, "         H5Recorder::step() >> Started writing in buffer #%d.\n", m_bufferInUse);
        for (uint i=0; i<m_numberOfInputs; i++) {
                m_data[i][m_bufferInUse][m_bufferPosition] = m_inputs[i];
        }
        m_bufferPosition = (m_bufferPosition+1) % bufferSize;

        if (m_bufferPosition == 0) {
                Logger(Debug, "         H5Recorder::step() >> Finished writing in buffer #%d.\n", m_bufferInUse);
                {
                        boost::unique_lock<boost::mutex> lock(m_mutex);
                        m_bufferLength = bufferSize;
                        m_bufferToSave = m_bufferInUse;
                        m_bufferInUse = (m_bufferInUse + 1) % 2;
                        m_dataReady = true;
                }
                m_cv.notify_all();
        }
}

void H5Recorder::buffersWriter()
{
        while (m_threadRun) {
                boost::unique_lock<boost::mutex> lock(m_mutex);
                while (!m_dataReady)
                        m_cv.wait(lock);
                Logger(Debug, "H5Recorder::buffersWriter() >> Acquired lock for buffer #%d.\n", m_bufferToSave);
                Logger(Debug, "H5Recorder::buffersWriter() >> Starting writing data from buffer #%d.\n", m_bufferToSave);

                hid_t filespace;
                herr_t status;
                if (m_bufferLength == 0)
                        // no data to save...
                        continue;
                m_datasetSize += m_bufferLength;

                Logger(Debug, "Dataset size = %d. Offset = %d\n", m_datasetSize, m_offset);

                for (uint i=0; i<m_numberOfInputs; i++) {

                        // extend the dataset
                        status = H5Dset_extent(m_datasets[i], &m_datasetSize);
                        if (status < 0)
                                throw "Unable to extend dataset.";
                        else
                                Logger(Debug, "Extended dataset.\n");

                        // get the filespace
                        filespace = H5Dget_space(m_datasets[i]);
                        if (filespace < 0)
                                throw "Unable to get filespace.";
                        else
                                Logger(Debug, "Obtained filespace.\n");

                        // select an hyperslab
                        status = H5Sselect_hyperslab(filespace, H5S_SELECT_SET, &m_offset, NULL, &m_bufferLength, NULL);
                        if (status < 0) {
                                H5Sclose(filespace);
                                throw "Unable to select hyperslab.";
                        }
                        else {
                                Logger(Debug, "Selected hyperslab.\n");
                        }

                        // define memory space
                        m_dataspaces[i] = H5Screate_simple(rank, &m_bufferLength, NULL);
                        if (m_dataspaces[i] < 0) {
                                H5Sclose(filespace);
                                throw "Unable to define memory space.";
                        }
                        else {
                                Logger(Debug, "Memory space defined.\n");
                        }

                        // write data
                        status = H5Dwrite(m_datasets[i], H5T_IEEE_F64LE, m_dataspaces[i], filespace, H5P_DEFAULT, m_data[i][m_bufferToSave]);
                        if (status < 0) {
                                H5Sclose(filespace);
                                throw "Unable to write data.";
                        }
                        else {
                                Logger(Debug, "Written data.\n");
                        }
                }
                H5Sclose(filespace);
                m_offset = m_datasetSize;
                
                m_dataReady = false;
                Logger(Debug, "H5Recorder::buffersWriter() >> Finished writing data from buffer #%d.\n", m_bufferToSave);
        }
        Logger(Debug, "H5Recorder::buffersWriter() >> Writing thread has terminated.\n");
}

void H5Recorder::addPre(Entity *entity, double input)
{
        Entity::addPre(entity, input);

        Logger(Debug, "--- H5Recorder::addPre(Entity*, double) ---\n");

        hid_t cparms;

        m_numberOfInputs++;
        uint k = m_numberOfInputs-1;
        double **buffer = new double*[2];
        buffer[0] = new double[bufferSize];
        buffer[1] = new double[bufferSize];
        m_data.push_back(buffer);

        herr_t status;

        // the name of the dataset
        char datasetName[7];
        sprintf(datasetName, "CH%04d", m_numberOfInputs);

        // create the dataspace with unlimited dimensions
        m_dataspaces.push_back(H5Screate_simple(rank, &bufferSize, &maxSize));
        if (m_dataspaces[k] < 0)
                throw "Unable to create dataspace.";
        else
                Logger(Debug, "Dataspace created\n");

        // modify dataset creation properties, i.e. enable chunking.
        cparms = H5Pcreate(H5P_DATASET_CREATE);
        if (cparms < 0) {
                H5Sclose(m_dataspaces[k]);
                throw "Unable to create dataset properties.";
        }
        else {
                Logger(Debug, "Dataset properties created.\n");
        }

        status = H5Pset_chunk(cparms, rank, &chunkSize);
        if (status < 0) {
                H5Sclose(m_dataspaces[k]);
                H5Pclose(cparms);
                throw "Unable to set chunking.";
        }
        else {
                Logger(Debug, "Chunking set.\n");
        }

        status = H5Pset_fill_value(cparms, H5T_IEEE_F64LE, &fillValue);
        if (status < 0) {
                H5Sclose(m_dataspaces[k]);
                H5Pclose(cparms);
                throw "Unable to set fill value.";
        }
        else {
                Logger(Debug, "Fill value set.\n");
        }

        // create a new dataset within the file using cparms creation properties.
        m_datasets.push_back(H5Dcreate2(m_fid, datasetName,
                                        H5T_IEEE_F64LE, m_dataspaces[k],
                                        H5P_DEFAULT, cparms, H5P_DEFAULT));
        if (m_datasets[k] < 0) {
                H5Sclose(m_dataspaces[k]);
                H5Pclose(cparms);
                throw "Unable to create a dataset.";
        }
        else {
                Logger(Debug, "Dataset created.\n");
        }

        H5Pclose(cparms);
}

int H5Recorder::isCompressionAvailable() const
{
        htri_t avail;
        herr_t status;
        uint filterInfo;
        
        Logger(Debug, "Checking whether GZIP compression is available...");
        // check if gzip compression is available
        avail = H5Zfilter_avail (H5Z_FILTER_DEFLATE);
        if (!avail) {
                Logger(Debug, "\nGZIP compression is not available on this system.\n");
                return H5_NO_GZIP_COMPRESSION;
        }
        Logger(Debug, " ok.\nGetting filter info...");
        status = H5Zget_filter_info (H5Z_FILTER_DEFLATE, &filterInfo);
        if (!(filterInfo & H5Z_FILTER_CONFIG_ENCODE_ENABLED)) {
                Logger(Debug, "\nUnable to get filter info: disabling compression.\n");
                return H5_NO_FILTER_INFO;
        }
        Logger(Debug, " ok.\nChecking whether the shuffle filter is available...");
        // check for availability of the shuffle filter.
        avail = H5Zfilter_avail(H5Z_FILTER_SHUFFLE);
        if (!avail) {
                Logger(Debug, "\nThe shuffle filter is not available on this system.\n");
                return H5_NO_SHUFFLE_FILTER;
        }
        Logger(Debug, " ok.\nGetting filter info...");
        status = H5Zget_filter_info (H5Z_FILTER_SHUFFLE, &filterInfo);
        if ( !(filterInfo & H5Z_FILTER_CONFIG_ENCODE_ENABLED) ) {
                Logger(Debug, "Unable to get filter info: disabling compression.\n");
                return H5_NO_FILTER_INFO;
        }
        Logger(Debug, " ok.\nCompression is enabled.\n");
        return OK;
}

int H5Recorder::open(const char *filename, bool compress)
{
        Logger(Debug, "--- H5Recorder::open(const char*, bool) ---\n");
        Logger(Debug, "Opening file %s.\n", filename);

        if(!compress || isCompressionAvailable() != OK)
                m_compressed = false;
        else
                m_compressed = true;

        m_fid = H5Fcreate(filename, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
        if(m_fid < 0)
                return H5_CREATE_ERROR;
        if(m_compressed) {
                // Create the dataset creation property list and add the shuffle
                // filter and the gzip compression filter.
                // The order in which the filters are added here is significant -
                // we will see much greater results when the shuffle is applied
                // first.  The order in which the filters are added to the property
                // list is the order in which they will be invoked when writing
                // data.
                m_datasetPropertiesList = H5Pcreate(H5P_DATASET_CREATE);
                if(m_datasetPropertiesList < 0)
                        return H5_DATASET_ERROR;
                if(H5Pset_shuffle(m_datasetPropertiesList) < 0)
                        return H5_SHUFFLE_ERROR;
                if(H5Pset_deflate(m_datasetPropertiesList, 9) < 0)
                        return H5_DEFLATE_ERROR;
        }
        return OK;
}

void H5Recorder::close()
{
        Logger(Debug, "--- H5Recorder::close() ---\n");
        Logger(Debug, "Closing file.\n");
        if (m_fid != -1) {

                if(m_compressed)
                        H5Pclose(m_datasetPropertiesList);

                for (uint i=0; i<m_numberOfInputs; i++) {
                        H5Dclose(m_datasets[i]);
                        H5Sclose(m_dataspaces[i]);
                }

                H5Fclose(m_fid);
                m_fid = -1;
        }
}

} // namespace recorders

} // namespace dynclamp

