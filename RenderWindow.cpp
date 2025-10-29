#include "RenderWindow.h"
#include <QVulkanFunctions>
#include <QFile>
#include <fstream>
#include <vector>
#include <iostream>
#include <algorithm>

// Structure for point cloud data
struct TerrainPoint {
    float x, y, z;
    float r, g, b;
};

// Global variables for terrain data
static std::vector<TerrainPoint> terrainPoints;
static int pointCount = 0;

//Utility variable and function for alignment:
static const int UNIFORM_DATA_SIZE = 16 * sizeof(float);
static inline VkDeviceSize aligned(VkDeviceSize v, VkDeviceSize byteAlign)
{
    return (v + byteAlign - 1) & ~(byteAlign - 1);
}

// Function to load terrain.xyz
bool loadTerrainData(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        qDebug("Failed to open %s", filename.c_str());
        return false;
    }

    // Read first line (total point count)
    int expectedPoints;
    file >> expectedPoints;
    qDebug("Expected points: %d", expectedPoints);

    terrainPoints.clear();
    terrainPoints.reserve(expectedPoints);

    float minX = 1e10f, maxX = -1e10f;
    float minY = 1e10f, maxY = -1e10f;
    float minZ = 1e10f, maxZ = -1e10f;

    // Read all points
    float x, y, z;
    while (file >> x >> y >> z) {
        minX = std::min(minX, x);
        maxX = std::max(maxX, x);
        minY = std::min(minY, y);
        maxY = std::max(maxY, y);
        minZ = std::min(minZ, z);
        maxZ = std::max(maxZ, z);

        terrainPoints.push_back({x, y, z, 0, 0, 0});
    }

    pointCount = terrainPoints.size();
    qDebug("Loaded %d points", pointCount);
    qDebug("X range: [%.2f, %.2f]", minX, maxX);
    qDebug("Y range: [%.2f, %.2f]", minY, maxY);
    qDebug("Z range: [%.2f, %.2f]", minZ, maxZ);

    // Normalize coordinates to center around origin
    float centerX = (minX + maxX) / 2.0f;
    float centerY = (minY + maxY) / 2.0f;
    float centerZ = (minZ + maxZ) / 2.0f;

    float rangeX = maxX - minX;
    float rangeY = maxY - minY;
    float rangeZ = maxZ - minZ;
    float maxRange = std::max({rangeX, rangeY, rangeZ});
    float scale = 2.0f / maxRange; // Scale to fit in [-1, 1]

    // Store original Z values for coloring
    std::vector<float> originalZ;
    originalZ.reserve(terrainPoints.size());
    for (const auto& point : terrainPoints) {
        originalZ.push_back(point.z);
    }

    // Find min and max of original Z for color mapping
    float origMinZ = *std::min_element(originalZ.begin(), originalZ.end());
    float origMaxZ = *std::max_element(originalZ.begin(), originalZ.end());

    // Normalize and color points based on height
    for (size_t i = 0; i < terrainPoints.size(); i++) {
        auto& point = terrainPoints[i];

        // Normalize position
        point.x = (point.x - centerX) * scale;
        point.y = (point.y - centerY) * scale;
        point.z = (point.z - centerZ) * scale;

        // Color based on ORIGINAL height (before normalization)
        float normalizedHeight = (originalZ[i] - origMinZ) / (origMaxZ - origMinZ);
        normalizedHeight = std::max(0.0f, std::min(1.0f, normalizedHeight));

        // Better color gradient: Blue (low) -> Cyan -> Green -> Yellow -> Red (high)
        if (normalizedHeight < 0.25f) {
            // Blue to Cyan
            float t = normalizedHeight * 4.0f;
            point.r = 0.0f;
            point.g = t;
            point.b = 1.0f;
        } else if (normalizedHeight < 0.5f) {
            // Cyan to Green
            float t = (normalizedHeight - 0.25f) * 4.0f;
            point.r = 0.0f;
            point.g = 1.0f;
            point.b = 1.0f - t;
        } else if (normalizedHeight < 0.75f) {
            // Green to Yellow
            float t = (normalizedHeight - 0.5f) * 4.0f;
            point.r = t;
            point.g = 1.0f;
            point.b = 0.0f;
        } else {
            // Yellow to Red
            float t = (normalizedHeight - 0.75f) * 4.0f;
            point.r = 1.0f;
            point.g = 1.0f - t;
            point.b = 0.0f;
        }
    }

    qDebug("Terrain data loaded and normalized successfully");
    return true;
}

/*** RenderWindow class ***/

RenderWindow::RenderWindow(QVulkanWindow *w, bool msaa)
    : mWindow(w)
{
    if (msaa) {
        const QList<int> counts = w->supportedSampleCounts();
        qDebug() << "Supported sample counts:" << counts;
        for (int s = 16; s >= 4; s /= 2) {
            if (counts.contains(s)) {
                qDebug("Requesting sample count %d", s);
                mWindow->setSampleCount(s);
                break;
            }
        }
    }
}

void RenderWindow::initResources()
{
    qDebug("\n ***************************** initResources ******************************************* \n");

    // Load terrain data FIRST
    if (!loadTerrainData("terrain.xyz")) {
        qFatal("Failed to load terrain data!");
    }

    VkDevice logicalDevice = mWindow->device();
    mDeviceFunctions = mWindow->vulkanInstance()->deviceFunctions(logicalDevice);

    const int concurrentFrameCount = mWindow->concurrentFrameCount();
    const VkPhysicalDeviceLimits *pdevLimits = &mWindow->physicalDeviceProperties()->limits;
    const VkDeviceSize uniAlign = pdevLimits->minUniformBufferOffsetAlignment;
    qDebug("uniform buffer offset alignment is %u", (uint)uniAlign);

    VkBufferCreateInfo bufInfo;
    memset(&bufInfo, 0, sizeof(bufInfo));
    bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;

    // Calculate buffer size for terrain points + uniform buffers
    const VkDeviceSize vertexAllocSize = aligned(pointCount * sizeof(TerrainPoint), uniAlign);
    const VkDeviceSize uniformAllocSize = aligned(UNIFORM_DATA_SIZE, uniAlign);
    bufInfo.size = vertexAllocSize + concurrentFrameCount * uniformAllocSize;
    bufInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    VkResult err = mDeviceFunctions->vkCreateBuffer(logicalDevice, &bufInfo, nullptr, &mBuffer);
    if (err != VK_SUCCESS)
        qFatal("Failed to create buffer: %d", err);

    VkMemoryRequirements memReq;
    mDeviceFunctions->vkGetBufferMemoryRequirements(logicalDevice, mBuffer, &memReq);

    VkMemoryAllocateInfo memAllocInfo = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        nullptr,
        memReq.size,
        mWindow->hostVisibleMemoryIndex()
    };

    err = mDeviceFunctions->vkAllocateMemory(logicalDevice, &memAllocInfo, nullptr, &mBufferMemory);
    if (err != VK_SUCCESS)
        qFatal("Failed to allocate memory: %d", err);

    err = mDeviceFunctions->vkBindBufferMemory(logicalDevice, mBuffer, mBufferMemory, 0);
    if (err != VK_SUCCESS)
        qFatal("Failed to bind buffer memory: %d", err);

    quint8 *p;
    err = mDeviceFunctions->vkMapMemory(logicalDevice, mBufferMemory, 0, memReq.size, 0, reinterpret_cast<void **>(&p));
    if (err != VK_SUCCESS)
        qFatal("Failed to map memory: %d", err);

    // Copy terrain data to GPU
    memcpy(p, terrainPoints.data(), pointCount * sizeof(TerrainPoint));

    QMatrix4x4 ident;
    memset(mUniformBufferInfo, 0, sizeof(mUniformBufferInfo));
    for (int i = 0; i < concurrentFrameCount; ++i) {
        const VkDeviceSize offset = vertexAllocSize + i * uniformAllocSize;
        memcpy(p + offset, ident.constData(), 16 * sizeof(float));
        mUniformBufferInfo[i].buffer = mBuffer;
        mUniformBufferInfo[i].offset = offset;
        mUniformBufferInfo[i].range = uniformAllocSize;
    }
    mDeviceFunctions->vkUnmapMemory(logicalDevice, mBufferMemory);

    /********************************* Vertex layout: *********************************/
    VkVertexInputBindingDescription vertexBindingDesc = {
        0,
        sizeof(TerrainPoint), // 6 floats: X, Y, Z, R, G, B
        VK_VERTEX_INPUT_RATE_VERTEX
    };

    /********************************* Shader bindings: *********************************/
    VkVertexInputAttributeDescription vertexAttrDesc[] = {
        { // position
            0,
            0,
            VK_FORMAT_R32G32B32_SFLOAT,
            0
        },
        { // color
            1,
            0,
            VK_FORMAT_R32G32B32_SFLOAT,
            3 * sizeof(float)
        }
    };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.pNext = nullptr;
    vertexInputInfo.flags = 0;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &vertexBindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = 2;
    vertexInputInfo.pVertexAttributeDescriptions = vertexAttrDesc;

    // Set up descriptor set and its layout
    VkDescriptorPoolSize descPoolSizes = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uint32_t(concurrentFrameCount) };
    VkDescriptorPoolCreateInfo descPoolInfo;
    memset(&descPoolInfo, 0, sizeof(descPoolInfo));
    descPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descPoolInfo.maxSets = concurrentFrameCount;
    descPoolInfo.poolSizeCount = 1;
    descPoolInfo.pPoolSizes = &descPoolSizes;
    err = mDeviceFunctions->vkCreateDescriptorPool(logicalDevice, &descPoolInfo, nullptr, &mDescriptorPool);
    if (err != VK_SUCCESS)
        qFatal("Failed to create descriptor pool: %d", err);

    /********************************* Uniform (projection matrix) bindings: *********************************/
    VkDescriptorSetLayoutBinding layoutBinding = {
        0,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        1,
        VK_SHADER_STAGE_VERTEX_BIT,
        nullptr
    };
    VkDescriptorSetLayoutCreateInfo descLayoutInfo = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        nullptr,
        0,
        1,
        &layoutBinding
    };
    err = mDeviceFunctions->vkCreateDescriptorSetLayout(logicalDevice, &descLayoutInfo, nullptr, &mDescriptorSetLayout);
    if (err != VK_SUCCESS)
        qFatal("Failed to create descriptor set layout: %d", err);

    for (int i = 0; i < concurrentFrameCount; ++i) {
        VkDescriptorSetAllocateInfo descSetAllocInfo = {
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            nullptr,
            mDescriptorPool,
            1,
            &mDescriptorSetLayout
        };
        err = mDeviceFunctions->vkAllocateDescriptorSets(logicalDevice, &descSetAllocInfo, &mDescriptorSet[i]);
        if (err != VK_SUCCESS)
            qFatal("Failed to allocate descriptor set: %d", err);

        VkWriteDescriptorSet descWrite;
        memset(&descWrite, 0, sizeof(descWrite));
        descWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descWrite.dstSet = mDescriptorSet[i];
        descWrite.descriptorCount = 1;
        descWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descWrite.pBufferInfo = &mUniformBufferInfo[i];
        mDeviceFunctions->vkUpdateDescriptorSets(logicalDevice, 1, &descWrite, 0, nullptr);
    }

    // Pipeline cache
    VkPipelineCacheCreateInfo pipelineCacheInfo;
    memset(&pipelineCacheInfo, 0, sizeof(pipelineCacheInfo));
    pipelineCacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    err = mDeviceFunctions->vkCreatePipelineCache(logicalDevice, &pipelineCacheInfo, nullptr, &mPipelineCache);
    if (err != VK_SUCCESS)
        qFatal("Failed to create pipeline cache: %d", err);

    // Pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo;
    memset(&pipelineLayoutInfo, 0, sizeof(pipelineLayoutInfo));
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &mDescriptorSetLayout;
    err = mDeviceFunctions->vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &mPipelineLayout);
    if (err != VK_SUCCESS)
        qFatal("Failed to create pipeline layout: %d", err);

    /********************************* Create shaders *********************************/
    VkShaderModule vertShaderModule = createShader(QStringLiteral(":/color_vert.spv"));
    VkShaderModule fragShaderModule = createShader(QStringLiteral(":/color_frag.spv"));

    // Graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo;
    memset(&pipelineInfo, 0, sizeof(pipelineInfo));
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    VkPipelineShaderStageCreateInfo shaderStages[2] = {
        {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            nullptr,
            0,
            VK_SHADER_STAGE_VERTEX_BIT,
            vertShaderModule,
            "main",
            nullptr
        },
        {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            nullptr,
            0,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            fragShaderModule,
            "main",
            nullptr
        }
    };

    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;

    VkPipelineInputAssemblyStateCreateInfo ia;
    memset(&ia, 0, sizeof(ia));
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST; // Render as points
    pipelineInfo.pInputAssemblyState = &ia;

    VkPipelineViewportStateCreateInfo vp;
    memset(&vp, 0, sizeof(vp));
    vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.viewportCount = 1;
    vp.scissorCount = 1;
    pipelineInfo.pViewportState = &vp;

    VkPipelineRasterizationStateCreateInfo rs;
    memset(&rs, 0, sizeof(rs));
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_NONE;
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.lineWidth = 1.0f;
    pipelineInfo.pRasterizationState = &rs;

    VkPipelineMultisampleStateCreateInfo ms;
    memset(&ms, 0, sizeof(ms));
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = mWindow->sampleCountFlagBits();
    pipelineInfo.pMultisampleState = &ms;

    VkPipelineDepthStencilStateCreateInfo ds;
    memset(&ds, 0, sizeof(ds));
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable = VK_TRUE;
    ds.depthWriteEnable = VK_TRUE;
    ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    pipelineInfo.pDepthStencilState = &ds;

    VkPipelineColorBlendStateCreateInfo cb;
    memset(&cb, 0, sizeof(cb));
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    VkPipelineColorBlendAttachmentState att;
    memset(&att, 0, sizeof(att));
    att.colorWriteMask = 0xF;
    cb.attachmentCount = 1;
    cb.pAttachments = &att;
    pipelineInfo.pColorBlendState = &cb;

    VkDynamicState dynEnable[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dyn;
    memset(&dyn, 0, sizeof(dyn));
    dyn.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dyn.dynamicStateCount = sizeof(dynEnable) / sizeof(VkDynamicState);
    dyn.pDynamicStates = dynEnable;
    pipelineInfo.pDynamicState = &dyn;

    pipelineInfo.layout = mPipelineLayout;
    pipelineInfo.renderPass = mWindow->defaultRenderPass();

    err = mDeviceFunctions->vkCreateGraphicsPipelines(logicalDevice, mPipelineCache, 1, &pipelineInfo, nullptr, &mPipeline);
    if (err != VK_SUCCESS)
        qFatal("Failed to create graphics pipeline: %d", err);

    if (vertShaderModule)
        mDeviceFunctions->vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
    if (fragShaderModule)
        mDeviceFunctions->vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);

    qDebug("\n ***************************** initResources finished ******************************************* \n");

    getVulkanHWInfo();
}

void RenderWindow::initSwapChainResources()
{
    qDebug("\n ***************************** initSwapChainResources ******************************************* \n");

    mProjectionMatrix.setToIdentity();
    const QSize sz = mWindow->swapChainImageSize();

    // Better perspective for terrain viewing
    mProjectionMatrix.perspective(15.0f, sz.width() / (float) sz.height(), 0.1f, 100.0f);

    // Position camera for top-down view
    mProjectionMatrix.translate(0, 0, -2.5); // Pull camera back

    // Rotate to look down at terrain (like a map view)
    // Rotate -60 degrees around X axis to tilt view downward
    mProjectionMatrix.rotate(-30.0f, 1, 0, 0);
    // Rotate around Y axis to get side view
    //mProjectionMatrix.rotate(0.0f, 0, 1, 0);
    mProjectionMatrix.translate(0, 0.5f, 0); // shift scene upward for centering

    mProjectionMatrix.scale(1.0f, -1.0f, 1.0); // Flip Y for Vulkan
}


void RenderWindow::startNextFrame()
{
    VkDevice dev = mWindow->device();
    VkCommandBuffer cb = mWindow->currentCommandBuffer();
    const QSize sz = mWindow->swapChainImageSize();

    VkClearColorValue clearColor = {{0.0f, 0.5f, 0.6f, 1.0f }}; // Currently different, but almost black background (0.05, 0.05, 0.05, 1)

    VkClearDepthStencilValue clearDS = { 1, 0 };
    VkClearValue clearValues[3];
    memset(clearValues, 0, sizeof(clearValues));
    clearValues[0].color = clearValues[2].color = clearColor;
    clearValues[1].depthStencil = clearDS;

    VkRenderPassBeginInfo rpBeginInfo;
    memset(&rpBeginInfo, 0, sizeof(rpBeginInfo));
    rpBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBeginInfo.renderPass = mWindow->defaultRenderPass();
    rpBeginInfo.framebuffer = mWindow->currentFramebuffer();
    rpBeginInfo.renderArea.extent.width = sz.width();
    rpBeginInfo.renderArea.extent.height = sz.height();
    rpBeginInfo.clearValueCount = mWindow->sampleCountFlagBits() > VK_SAMPLE_COUNT_1_BIT ? 3 : 2;
    rpBeginInfo.pClearValues = clearValues;
    VkCommandBuffer cmdBuf = mWindow->currentCommandBuffer();
    mDeviceFunctions->vkCmdBeginRenderPass(cmdBuf, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    quint8* GPUmemPointer;
    VkResult err = mDeviceFunctions->vkMapMemory(dev, mBufferMemory, mUniformBufferInfo[mWindow->currentFrame()].offset,
                                                 UNIFORM_DATA_SIZE, 0, reinterpret_cast<void **>(&GPUmemPointer));
    if (err != VK_SUCCESS)
        qFatal("Failed to map memory: %d", err);

    // Apply slow rotation around Z axis (vertical rotation when viewed from top)
    QMatrix4x4 tempMatrix = mProjectionMatrix;
    //tempMatrix.rotate(mRotation, 0, 0, 1); // Rotate around Z axis for map-like rotation

    memcpy(GPUmemPointer, tempMatrix.constData(), 16 * sizeof(float));
    mDeviceFunctions->vkUnmapMemory(dev, mBufferMemory);

    //mRotation += 0.3f; // Slower rotation

    mDeviceFunctions->vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);
    mDeviceFunctions->vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 0, 1,
                                              &mDescriptorSet[mWindow->currentFrame()], 0, nullptr);
    VkDeviceSize vbOffset = 0;
    mDeviceFunctions->vkCmdBindVertexBuffers(cb, 0, 1, &mBuffer, &vbOffset);

    VkViewport viewport;
    viewport.x = viewport.y = 0;
    viewport.width = sz.width();
    viewport.height = sz.height();
    viewport.minDepth = 0;
    viewport.maxDepth = 1;
    mDeviceFunctions->vkCmdSetViewport(cb, 0, 1, &viewport);

    VkRect2D scissor;
    scissor.offset.x = scissor.offset.y = 0;
    scissor.extent.width = viewport.width;
    scissor.extent.height = viewport.height;
    mDeviceFunctions->vkCmdSetScissor(cb, 0, 1, &scissor);

    // Draw all points!
    mDeviceFunctions->vkCmdDraw(cb, pointCount, 1, 0, 0);

    mDeviceFunctions->vkCmdEndRenderPass(cmdBuf);

    mWindow->frameReady();
    mWindow->requestUpdate();
}

VkShaderModule RenderWindow::createShader(const QString &name)
{
    QFile file(name);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning("Failed to read shader %s", qPrintable(name));
        return VK_NULL_HANDLE;
    }
    QByteArray blob = file.readAll();
    file.close();

    VkShaderModuleCreateInfo shaderInfo;
    memset(&shaderInfo, 0, sizeof(shaderInfo));
    shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderInfo.codeSize = blob.size();
    shaderInfo.pCode = reinterpret_cast<const uint32_t *>(blob.constData());
    VkShaderModule shaderModule;
    VkResult err = mDeviceFunctions->vkCreateShaderModule(mWindow->device(), &shaderInfo, nullptr, &shaderModule);
    if (err != VK_SUCCESS) {
        qWarning("Failed to create shader module: %d", err);
        return VK_NULL_HANDLE;
    }

    return shaderModule;
}

void RenderWindow::getVulkanHWInfo()
{
    qDebug("\n ***************************** Vulkan Hardware Info ******************************************* \n");
    QVulkanInstance *inst = mWindow->vulkanInstance();
    mDeviceFunctions = inst->deviceFunctions(mWindow->device());

    QString info;
    info += QString::asprintf("Number of physical devices: %d\n", int(mWindow->availablePhysicalDevices().count()));

    QVulkanFunctions *f = inst->functions();
    VkPhysicalDeviceProperties props;
    f->vkGetPhysicalDeviceProperties(mWindow->physicalDevice(), &props);
    info += QString::asprintf("Active physical device name: '%s' version %d.%d.%d\nAPI version %d.%d.%d\n",
                              props.deviceName,
                              VK_VERSION_MAJOR(props.driverVersion), VK_VERSION_MINOR(props.driverVersion),
                              VK_VERSION_PATCH(props.driverVersion),
                              VK_VERSION_MAJOR(props.apiVersion), VK_VERSION_MINOR(props.apiVersion),
                              VK_VERSION_PATCH(props.apiVersion));

    info += QStringLiteral("Supported instance layers:\n");
    for (const QVulkanLayer &layer : inst->supportedLayers())
        info += QString::asprintf("    %s v%u\n", layer.name.constData(), layer.version);
    info += QStringLiteral("Enabled instance layers:\n");
    for (const QByteArray &layer : inst->layers())
        info += QString::asprintf("    %s\n", layer.constData());

    info += QStringLiteral("Supported instance extensions:\n");
    for (const QVulkanExtension &ext : inst->supportedExtensions())
        info += QString::asprintf("    %s v%u\n", ext.name.constData(), ext.version);
    info += QStringLiteral("Enabled instance extensions:\n");
    for (const QByteArray &ext : inst->extensions())
        info += QString::asprintf("    %s\n", ext.constData());

    info += QString::asprintf("Color format: %u\nDepth-stencil format: %u\n",
                              mWindow->colorFormat(), mWindow->depthStencilFormat());

    info += QStringLiteral("Supported sample counts:");
    const QList<int> sampleCounts = mWindow->supportedSampleCounts();
    for (int count : sampleCounts)
        info += QLatin1Char(' ') + QString::number(count);
    info += QLatin1Char('\n');

    qDebug(info.toUtf8().constData());
    qDebug("\n ***************************** Vulkan Hardware Info finished ******************************************* \n");
}

void RenderWindow::releaseSwapChainResources()
{
    qDebug("\n ***************************** releaseSwapChainResources ******************************************* \n");
}

void RenderWindow::releaseResources()
{
    qDebug("\n ***************************** releaseResources ******************************************* \n");

    VkDevice dev = mWindow->device();

    if (mPipeline) {
        mDeviceFunctions->vkDestroyPipeline(dev, mPipeline, nullptr);
        mPipeline = VK_NULL_HANDLE;
    }

    if (mPipelineLayout) {
        mDeviceFunctions->vkDestroyPipelineLayout(dev, mPipelineLayout, nullptr);
        mPipelineLayout = VK_NULL_HANDLE;
    }

    if (mPipelineCache) {
        mDeviceFunctions->vkDestroyPipelineCache(dev, mPipelineCache, nullptr);
        mPipelineCache = VK_NULL_HANDLE;
    }

    if (mDescriptorSetLayout) {
        mDeviceFunctions->vkDestroyDescriptorSetLayout(dev, mDescriptorSetLayout, nullptr);
        mDescriptorSetLayout = VK_NULL_HANDLE;
    }

    if (mDescriptorPool) {
        mDeviceFunctions->vkDestroyDescriptorPool(dev, mDescriptorPool, nullptr);
        mDescriptorPool = VK_NULL_HANDLE;
    }

    if (mBuffer) {
        mDeviceFunctions->vkDestroyBuffer(dev, mBuffer, nullptr);
        mBuffer = VK_NULL_HANDLE;
    }

    if (mBufferMemory) {
        mDeviceFunctions->vkFreeMemory(dev, mBufferMemory, nullptr);
        mBufferMemory = VK_NULL_HANDLE;
    }
}
