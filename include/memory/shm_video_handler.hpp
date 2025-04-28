#pragma once

#include <string>
#include <memory>
#include <atomic>
#include <mutex>
#include <vector>
#include <opencv2/core.hpp>

namespace vst
{
    namespace memory
    {

        /**
         * @brief Struct to hold frame metadata in shared memory
         */
        struct ShmVideoFrameHeader
        {
            uint32_t width;       // Frame width
            uint32_t height;      // Frame height
            uint32_t channels;    // Number of channels (3 for RGB, 4 for RGBA)
            uint32_t frameIndex;  // Current frame index
            uint32_t totalFrames; // Total number of frames (0 if unknown)
            double fps;           // Frames per second
            uint64_t timestamp;   // Timestamp in milliseconds
            bool isNewFrame;      // Flag to indicate a new frame is available
            bool isEndOfVideo;    // Flag to indicate end of video
        };

        /**
         * @brief Class for handling video sharing via shared memory
         */
        class ShmVideoHandler
        {
        public:
            /**
             * @brief Constructor
             */
            ShmVideoHandler();

            /**
             * @brief Destructor
             */
            ~ShmVideoHandler();

            /**
             * @brief Creates a shared memory segment for video streaming
             *
             * @param name Name of the shared memory segment
             * @param width Frame width
             * @param height Frame height
             * @param channels Number of channels
             * @return true if successful, false otherwise
             */
            bool createSharedMemory(const std::string &name, uint32_t width, uint32_t height, uint32_t channels = 4);

            /**
             * @brief Opens an existing shared memory segment for video streaming
             *
             * @param name Name of the shared memory segment
             * @return true if successful, false otherwise
             */
            bool openSharedMemory(const std::string &name);

            /**
             * @brief Writes a frame to shared memory
             *
             * @param frame OpenCV frame to write
             * @param frameIndex Current frame index
             * @param totalFrames Total number of frames (0 if unknown)
             * @param fps Frames per second
             * @param timestamp Timestamp in milliseconds
             * @return true if successful, false otherwise
             */
            bool writeFrame(const cv::Mat &frame, uint32_t frameIndex, uint32_t totalFrames, double fps, uint64_t timestamp);

            /**
             * @brief Reads a frame from shared memory
             *
             * @param frame OpenCV frame to read into
             * @param waitForNewFrame Whether to wait for a new frame
             * @return true if successful, false otherwise
             */
            bool readFrame(cv::Mat &frame, bool waitForNewFrame = true);

            /**
             * @brief Gets the current frame metadata
             *
             * @return Reference to the frame metadata
             */
            ShmVideoFrameHeader getFrameMetadata();

            /**
             * @brief Closes and cleans up shared memory
             */
            void closeSharedMemory();

            /**
             * @brief Checks if the shared memory is open
             *
             * @return true if open, false otherwise
             */
            bool isOpen() const;

            /**
             * @brief Signals the end of the video stream
             */
            void signalEndOfVideo();

        private:
            std::string m_shmName;
            int m_shmFd;
            size_t m_shmSize;
            void *m_shmPtr;
            ShmVideoFrameHeader *m_header;
            uint8_t *m_frameData;

            std::mutex m_mutex;
            std::atomic<bool> m_isOpen;

            /**
             * @brief Calculates the total size needed for shared memory
             *
             * @param width Frame width
             * @param height Frame height
             * @param channels Number of channels
             * @return Size in bytes
             */
            size_t calculateShmSize(uint32_t width, uint32_t height, uint32_t channels);
        };

    } // namespace memory
} // namespace vst