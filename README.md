# VulkanSharedTextures

**VulkanSharedTextures** is a graphics project designed to explore high-performance texture memory sharing between two separate applications using Vulkan, DMA-BUF, and `shm_open`.

## 📦 Project Description

This project explores **graphics memory sharing between applications using the Vulkan API**, comparing two inter-process communication (IPC) techniques:

## ✨ Features
- Modular Vulkan renderer
- Image loading via stb_image
- Shared memory APIs (POSIX + Linux DMA-BUF)
- Support for texture streaming and future video frame decoding
- Two separate Vulkan-based apps: Producer and Consumer

### ✅ Supported Modes

1. **DMA-BUF (Zero-copy GPU sharing)**
   - Uses Vulkan's `VK_KHR_external_memory_fd` extension.
   - Shares GPU texture memory directly between `vst_producer` and `vst_consumer`.
   - Requires a Unix domain socket for sending the DMA-BUF file descriptor.

2. **POSIX Shared Memory (`shm_open`)**
   - CPU-side texture sharing using `shm_open`, `ftruncate`, and `mmap`.
   - The consumer maps shared memory and uploads it to the GPU manually using a staging buffer.
   - Useful when DMA-BUF is unavailable or unsupported.

## 🎮 Applications

- `vst_producer`: Loads an image, uploads it to the GPU, and shares it via DMA-BUF or shm.
- `vst_consumer`: Connects to the producer and displays the shared image in a separate Vulkan window.

## 🚀 Build Instructions

```bash
git clone git@gitlab.com:<your-username>/<your-repo>.git
cd VulkanSharedTextures
mkdir build && cd build
cmake ..
make -j$(nproc)

./vst_producer ~/PATH/TO/IMAGE --mode=(dma|shm)
./vst_consumer
