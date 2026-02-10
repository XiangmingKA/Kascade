#include "GltfPrimitive.h"
#include <stdexcept>

void GltfPrimitive::create(const Device& device,
                            VkCommandPool cmdPool,
                            VkQueue queue,
                            const std::vector<GltfVertex>& vertices,
                            const std::vector<uint32_t>& indices) {
    if (vertices.empty() || indices.empty()) {
        throw std::runtime_error("GltfPrimitive: Cannot create with empty vertex or index data");
    }

    vertexCount = static_cast<uint32_t>(vertices.size());
    indexCount = static_cast<uint32_t>(indices.size());

    // Create vertex buffer
    VkDeviceSize vertexBufferSize = sizeof(GltfVertex) * vertices.size();
    vertexBuffer.createFrom(device, cmdPool, queue, vertices.data(), vertexBufferSize);

    // Create index buffer
    indexBuffer.createFromVector(device, cmdPool, queue, indices);
}

void GltfPrimitive::destroy(const Device& device) {
    vertexBuffer.destroy(device);
    indexBuffer.destroy(device);

    // Destroy morph target buffers if any
    for (auto& morphBuffer : morphTargetBuffers) {
        morphBuffer.destroy(device);
    }
    morphTargetBuffers.clear();
    morphWeights.clear();

    vertexCount = 0;
    indexCount = 0;
}

void GltfPrimitive::draw(VkCommandBuffer cmd) const {
    if (!isValid()) {
        return;
    }

    // Bind vertex buffer
    VkBuffer vertexBuffers[] = { vertexBuffer.get() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);

    // Bind index buffer
    vkCmdBindIndexBuffer(cmd, indexBuffer.get(), 0, VK_INDEX_TYPE_UINT32);

    // Draw indexed
    vkCmdDrawIndexed(cmd, indexCount, 1, firstIndex, vertexOffset, 0);
}
