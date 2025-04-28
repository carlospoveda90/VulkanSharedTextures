#include "memory/shm_video_handler.hpp"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <chrono>
#include <thread>
#include <opencv2/imgproc.hpp>
#include "utils/logger.hpp"

namespace vst
{
    namespace memory
    {

        ShmVideoHandler::ShmVideoHandler()
            : m_shmFd(-1),
              m_shmSize(0),
              m_shmPtr(nullptr),
              m_header(nullptr),
              m_frameData(nullptr),
              m_isOpen(false)
        {
        }

        ShmVideoHandler::~ShmVideoHandler()
        {
            closeSharedMemory();
        }

        bool ShmVideoHandler::createSharedMemory(const std::string &name, uint32_t width, uint32_t height, uint32_t channels)
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            // Close any existing shared memory
            closeSharedMemory();

            // Store the name (remove leading '/' if present)
            m_shmName = name;
            if (!m_shmName.empty() && m_shmName[0] == '/')
            {
                m_shmName = m_shmName.substr(1);
            }

            // Calculate the size needed
            m_shmSize = calculateShmSize(width, height, channels);

            // Create the shared memory object
            m_shmFd = shm_open(("/" + m_shmName).c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
            if (m_shmFd == -1)
            {
                LOG_ERR("Failed to create shared memory object: " + std::string(strerror(errno)));
                return false;
            }

            // Set the size of the shared memory object
            if (ftruncate(m_shmFd, m_shmSize) == -1)
            {
                LOG_ERR("Failed to set size of shared memory: " + std::string(strerror(errno)));
                close(m_shmFd);
                m_shmFd = -1;
                return false;
            }

            // Map the shared memory object
            m_shmPtr = mmap(NULL, m_shmSize, PROT_READ | PROT_WRITE, MAP_SHARED, m_shmFd, 0);
            if (m_shmPtr == MAP_FAILED)
            {
                LOG_ERR("Failed to map shared memory: " + std::string(strerror(errno)));
                close(m_shmFd);
                m_shmFd = -1;
                return false;
            }

            // Set up the header and frame data pointers
            m_header = static_cast<ShmVideoFrameHeader *>(m_shmPtr);
            m_frameData = static_cast<uint8_t *>(m_shmPtr) + sizeof(ShmVideoFrameHeader);

            // Initialize the header
            m_header->width = width;
            m_header->height = height;
            m_header->channels = channels;
            m_header->frameIndex = 0;
            m_header->totalFrames = 0;
            m_header->fps = 0.0;
            m_header->timestamp = 0;
            m_header->isNewFrame = false;
            m_header->isEndOfVideo = false;

            m_isOpen = true;

            LOG_INFO("Created shared memory for video: " + m_shmName +
                     " (size: " + std::to_string(m_shmSize) + " bytes, dimensions: " +
                     std::to_string(width) + "x" + std::to_string(height) + "x" + std::to_string(channels) + ")");

            return true;
        }

        bool ShmVideoHandler::openSharedMemory(const std::string &name)
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            // Close any existing shared memory
            closeSharedMemory();

            // Store the name (remove leading '/' if present)
            m_shmName = name;
            if (!m_shmName.empty() && m_shmName[0] == '/')
            {
                m_shmName = m_shmName.substr(1);
            }

            // Open the shared memory object
            m_shmFd = shm_open(("/" + m_shmName).c_str(), O_RDWR, S_IRUSR | S_IWUSR);
            if (m_shmFd == -1)
            {
                LOG_ERR("Failed to open shared memory object: " + std::string(strerror(errno)));
                return false;
            }

            // Get the size of the shared memory object
            struct stat sb;
            if (fstat(m_shmFd, &sb) == -1)
            {
                LOG_ERR("Failed to get size of shared memory: " + std::string(strerror(errno)));
                close(m_shmFd);
                m_shmFd = -1;
                return false;
            }
            m_shmSize = sb.st_size;

            // Map the shared memory object
            m_shmPtr = mmap(NULL, m_shmSize, PROT_READ | PROT_WRITE, MAP_SHARED, m_shmFd, 0);
            if (m_shmPtr == MAP_FAILED)
            {
                LOG_ERR("Failed to map shared memory: " + std::string(strerror(errno)));
                close(m_shmFd);
                m_shmFd = -1;
                return false;
            }

            // Set up the header and frame data pointers
            m_header = static_cast<ShmVideoFrameHeader *>(m_shmPtr);
            m_frameData = static_cast<uint8_t *>(m_shmPtr) + sizeof(ShmVideoFrameHeader);

            m_isOpen = true;

            LOG_INFO("Opened shared memory for video: " + m_shmName +
                     " (size: " + std::to_string(m_shmSize) + " bytes, dimensions: " +
                     std::to_string(m_header->width) + "x" + std::to_string(m_header->height) +
                     "x" + std::to_string(m_header->channels) + ")");

            return true;
        }

        bool ShmVideoHandler::writeFrame(const cv::Mat &frame, uint32_t frameIndex, uint32_t totalFrames, double fps, uint64_t timestamp)
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            if (!m_isOpen || !m_header || !m_frameData)
            {
                LOG_ERR("Shared memory is not open");
                return false;
            }

            // Check if the frame dimensions match the shared memory
            if (frame.cols != static_cast<int>(m_header->width) ||
                frame.rows != static_cast<int>(m_header->height))
            {
                LOG_ERR("Frame dimensions do not match shared memory");
                return false;
            }

            // Update the header
            m_header->frameIndex = frameIndex;
            m_header->totalFrames = totalFrames;
            m_header->fps = fps;
            m_header->timestamp = timestamp;

            // Convert the frame to the right format if needed
            cv::Mat convertedFrame;
            if (frame.channels() != static_cast<int>(m_header->channels))
            {
                if (m_header->channels == 3 && frame.channels() == 4)
                {
                    cv::cvtColor(frame, convertedFrame, cv::COLOR_RGBA2RGB);
                }
                else if (m_header->channels == 4 && frame.channels() == 3)
                {
                    cv::cvtColor(frame, convertedFrame, cv::COLOR_BGR2RGBA);
                }
                else
                {
                    LOG_ERR("Unsupported channel conversion");
                    return false;
                }
            }
            else
            {
                frame.copyTo(convertedFrame);
            }

            // Copy the frame data
            size_t frameDataSize = m_header->width * m_header->height * m_header->channels;
            if (convertedFrame.isContinuous())
            {
                std::memcpy(m_frameData, convertedFrame.data, frameDataSize);
            }
            else
            {
                // Copy row by row if the data is not continuous
                for (uint32_t y = 0; y < m_header->height; ++y)
                {
                    std::memcpy(
                        m_frameData + y * m_header->width * m_header->channels,
                        convertedFrame.ptr(y),
                        m_header->width * m_header->channels);
                }
            }

            // Set the new frame flag
            m_header->isNewFrame = true;

            return true;
        }

        bool ShmVideoHandler::readFrame(cv::Mat &frame, bool waitForNewFrame)
        {
            // Use a unique_lock instead of lock_guard so we can unlock it temporarily
            std::unique_lock<std::mutex> lock(m_mutex);

            if (!m_isOpen || !m_header || !m_frameData)
            {
                LOG_ERR("Shared memory is not open");
                return false;
            }

            // Check if we need to wait for a new frame
            if (waitForNewFrame)
            {
                // Poll for a new frame with a timeout
                const int maxAttempts = 100; // 5 seconds timeout (100 * 50ms)
                int attempts = 0;

                while (!m_header->isNewFrame && !m_header->isEndOfVideo && attempts < maxAttempts)
                {
                    // Release the lock while waiting to avoid deadlock
                    lock.unlock();
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    lock.lock();
                    attempts++;
                }

                if (attempts >= maxAttempts)
                {
                    LOG_ERR("Timeout waiting for new frame");
                    return false;
                }

                if (m_header->isEndOfVideo)
                {
                    LOG_INFO("End of video reached");
                    return false;
                }
            }

            // Check if there's a new frame
            if (!m_header->isNewFrame && waitForNewFrame)
            {
                LOG_ERR("No new frame available");
                return false;
            }

            // Determine the OpenCV matrix type based on the number of channels
            int type;
            switch (m_header->channels)
            {
            case 1:
                type = CV_8UC1;
                break;
            case 3:
                type = CV_8UC3;
                break;
            case 4:
                type = CV_8UC4;
                break;
            default:
                LOG_ERR("Unsupported number of channels: " + std::to_string(m_header->channels));
                return false;
            }

            // Get dimensions and size before releasing lock
            int width = m_header->width;
            int height = m_header->height;
            size_t frameDataSize = width * height * m_header->channels;

            // Create a temporary buffer to hold the frame data
            std::vector<uint8_t> tempBuffer(frameDataSize);

            // Copy the frame data to our temporary buffer
            std::memcpy(tempBuffer.data(), m_frameData, frameDataSize);

            // Reset the new frame flag if we're reading it
            if (waitForNewFrame)
            {
                m_header->isNewFrame = false;
            }

            // We can release the lock now since we've copied all the data we need
            lock.unlock();

            // Create or resize the output matrix
            if (frame.empty() ||
                frame.cols != width ||
                frame.rows != height ||
                frame.type() != type)
            {
                frame = cv::Mat(height, width, type);
            }

            // Copy from our temporary buffer to the frame
            if (frame.isContinuous())
            {
                std::memcpy(frame.data, tempBuffer.data(), frameDataSize);
            }
            else
            {
                // Copy row by row if the data is not continuous
                int rowSize = width * m_header->channels;
                for (int y = 0; y < height; ++y)
                {
                    std::memcpy(
                        frame.ptr(y),
                        tempBuffer.data() + y * rowSize,
                        rowSize);
                }
            }

            return true;
        }

        // Replace the existing getFrameMetadata function with this safer version
        ShmVideoFrameHeader ShmVideoHandler::getFrameMetadata()
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            if (!m_isOpen || !m_header)
            {
                // Return an empty header with zero values if not open
                ShmVideoFrameHeader emptyHeader{};
                return emptyHeader;
            }

            // Return a copy of the header structure
            return *m_header;
        }

        void ShmVideoHandler::closeSharedMemory()
        {
            // Try to lock without blocking to prevent deadlock
            std::unique_lock<std::mutex> lock(m_mutex, std::defer_lock);
            if (!lock.try_lock())
            {
                // If we can't get the lock immediately, log a warning and proceed anyway
                LOG_ERR("Could not acquire lock for closeSharedMemory, proceeding without lock");
            }

            if (m_isOpen)
            {
                if (m_shmPtr != nullptr && m_shmPtr != MAP_FAILED)
                {
                    munmap(m_shmPtr, m_shmSize);
                    m_shmPtr = nullptr;
                }

                if (m_shmFd != -1)
                {
                    close(m_shmFd);
                    m_shmFd = -1;
                }

                m_header = nullptr;
                m_frameData = nullptr;
                m_shmSize = 0;
                m_isOpen = false;

                LOG_INFO("Closed shared memory for video: " + m_shmName);
            }
        }

        bool ShmVideoHandler::isOpen() const
        {
            return m_isOpen;
        }

        void ShmVideoHandler::signalEndOfVideo()
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            if (m_isOpen && m_header)
            {
                m_header->isEndOfVideo = true;
            }
        }

        size_t ShmVideoHandler::calculateShmSize(uint32_t width, uint32_t height, uint32_t channels)
        {
            // Header size + frame data size
            return sizeof(ShmVideoFrameHeader) + (width * height * channels);
        }

    } // namespace memory
} // namespace vst